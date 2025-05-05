#include "Vitrae/Pipelines/Compositing/CacheTasks.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"

#include "MMeter.h"

#include <fstream>
#include <ranges>

namespace Vitrae
{

ComposeCacheTasks::ComposeCacheTasks(const CacheParams &params) : m_params(params) {}

std::size_t ComposeCacheTasks::memory_cost() const
{
    return sizeof(ComposeCacheTasks);
}

const ParamList &ComposeCacheTasks::getInputSpecs(const ParamAliases &externalAliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(externalAliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        return (*it).second->inputSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

const ParamList &ComposeCacheTasks::getOutputSpecs() const
{
    return m_params.desiredOutputs;
}

const ParamList &ComposeCacheTasks::getFilterSpecs(const ParamAliases &externalAliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(externalAliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        return (*it).second->filterSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

const ParamList &ComposeCacheTasks::getConsumingSpecs(const ParamAliases &externalAliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(externalAliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        return (*it).second->consumeSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

void ComposeCacheTasks::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                         const ParamAliases &aliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(aliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        const auto &specs = *(*it).second;

        for (const ParamList *p_specs : {&specs.inputSpecs, &m_params.desiredOutputs,
                                         &specs.filterSpecs, &specs.consumeSpecs}) {
            for (const ParamSpec &spec : p_specs->getSpecList()) {
                typeSet.insert(&spec.typeInfo);
            }
        }
    }
}

void ComposeCacheTasks::extractSubTasks(std::set<const Task *> &taskSet,
                                        const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

const Pipeline<ComposeTask> &ComposeCacheTasks::getContainedPipeline(
    const ParamAliases &aliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(aliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        return (*it).second->pipeline;
    }

    throw std::runtime_error{"Adaptor not found"};
}

ParamAliases ComposeCacheTasks::constructContainedPipelineAliases(const ParamAliases &aliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(aliases.hash());
        it != m_adaptorPerSelectionHash.end()) {
        return ParamAliases({{&m_params.adaptorAliases, &aliases}});
    }

    throw std::runtime_error{"Adaptor not found"};
}

void ComposeCacheTasks::rebuildContainedPipeline(const ParamAliases &aliases) const
{
    if (auto it = m_adaptorPerSelectionHash.find(aliases.hash());
        it != m_adaptorPerSelectionHash.end()) {

        std::unique_ptr<AdaptorPerAliases> p_adaptor = std::move((*it).second);
        m_adaptorPerSelectionHash.erase(it);
        ParamAliases subAliases({{&m_params.adaptorAliases, &aliases}});

        for (auto p_pipeitem : p_adaptor->pipeline.items) {
            if (auto p_container = dynamic_cast<const PipelineContainer *>(&*p_pipeitem);
                p_container) {
                p_container->rebuildContainedPipeline(subAliases);
            }
        }

        m_adaptorPerSelectionHash.emplace(
            aliases.hash(),
            new AdaptorPerAliases(m_params.adaptorAliases, m_params.desiredOutputs, aliases,
                                  m_params.root.getComponent<MethodCollection>(),
                                  m_params.friendlyName));
    }
}

void ComposeCacheTasks::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER(m_params.friendlyName.c_str());

    auto &myMemory = ctx.pipelineMemory.next<MyMemory>();

    // check if any output is missing
    if (myMemory.cachedProperties.size() == 0) {
        const AdaptorPerAliases &adaptor = *myMemory.adaptor;

        // VariantScope encapsulatedScope(&ctx.properties.getUnaliasedScope());

        // Note: we will use only the pipeline's usedSelection in the subpipeline,
        // because anything else is unused and potential performance hog
        ArgumentScope encapsulatedArgumentScope(&ctx.properties.getUnaliasedScope(),
                                                &adaptor.pipeline.usedSelection);

        // construct the encapsulated context
        const ParamAliases *aliasArray[] = {&m_params.adaptorAliases, &ctx.aliases};
        ParamAliases subAliases(aliasArray);
        RenderComposeContext subCtx{
            .properties = encapsulatedArgumentScope,
            .root = ctx.root,
            .aliases = subAliases,
            .pipelineMemory = myMemory.subPipelineMemory,
        };

        // map from external scope to internal scope
        for (const auto &entry : adaptor.finishingMapping) {
            if (ctx.properties.has(entry.second)) {
                encapsulatedArgumentScope.set(entry.first, ctx.properties.get(entry.second));
            }
        }

        // execute the pipeline
        try {
            {
                MMETER_SCOPE_PROFILER("Pipeline execution");

                myMemory.subPipelineMemory.restart();

                for (auto &pipeitem : adaptor.pipeline.items) {
                    pipeitem->run(subCtx);
                }
            }
        }
        catch (const ComposeTaskRequirementsChangedException &) {
            // Prepare for rebuild and notify parent runner to rebuild
            forgetAdaptorPerAliases(ctx.aliases);
            throw;
        }

        // map from internal scope to external scope
        for (const auto &entry : adaptor.finishingMapping) {
            myMemory.cachedProperties.emplace(entry.second,
                                              encapsulatedArgumentScope.move(entry.first));
        }
    }

    for (auto propertyCache : myMemory.cachedProperties) {
        ctx.properties.set(propertyCache.first, propertyCache.second);
    }
}

void ComposeCacheTasks::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER("ComposeCacheTasks::run");

    auto &myMemory = ctx.pipelineMemory.createNext<MyMemory>();

    auto it = m_adaptorPerSelectionHash.find(ctx.aliases.hash());
    if (it == m_adaptorPerSelectionHash.end()) {
        it = m_adaptorPerSelectionHash
                 .emplace(ctx.aliases.hash(),
                          new AdaptorPerAliases(
                              m_params.adaptorAliases, m_params.desiredOutputs, ctx.aliases,
                              ctx.root.getComponent<MethodCollection>(), m_params.friendlyName))
                 .first;
        throw ComposeTaskRequirementsChangedException();
    }

    const AdaptorPerAliases &adaptor = *(*it).second;
    myMemory.adaptor = &adaptor;

    // VariantScope encapsulatedScope(&ctx.properties.getUnaliasedScope());

    // Note: we will use only the pipeline's usedSelection in the subpipeline,
    // because anything else is unused and potential performance hog
    ArgumentScope encapsulatedArgumentScope(&ctx.properties.getUnaliasedScope(),
                                            &adaptor.pipeline.usedSelection);

    // construct the encapsulated context
    const ParamAliases *aliasArray[] = {&m_params.adaptorAliases, &ctx.aliases};
    ParamAliases subAliases(aliasArray);
    RenderComposeContext subCtx{
        .properties = encapsulatedArgumentScope,
        .root = ctx.root,
        .aliases = subAliases,
        .pipelineMemory = myMemory.subPipelineMemory,
    };

    // execute the pipeline
    try {
        {
            MMETER_SCOPE_PROFILER("Pipeline execution");

            for (auto &pipeitem : std::ranges::reverse_view{adaptor.pipeline.items}) {
                pipeitem->prepareRequiredLocalAssets(subCtx);
            }
        }
    }
    catch (const ComposeTaskRequirementsChangedException &) {
        // Prepare for rebuild and notify parent runner to rebuild
        forgetAdaptorPerAliases(ctx.aliases);
        throw;
    }

    // map from external scope to internal scope
    for (const auto &entry : adaptor.finishingMapping) {
        if (ctx.properties.has(entry.second)) {
            encapsulatedArgumentScope.set(entry.first, ctx.properties.move(entry.second));
        }
    }

    // map from internal scope to external scope
    // since we are running in reverse, map input specs of the pipeline
    /*for (auto p_specs : {&adaptor.pipeline.inputSpecs, &adaptor.pipeline.filterSpecs,
                         &adaptor.pipeline.pipethroughSpecs}) {
        for (const auto &[specNameId, spec] : p_specs->getMappedSpecs()) {
            if (spec.typeInfo != TYPE_INFO<void> &&
                encapsulatedArgumentScope.has(specNameId)) {
                ctx.properties.set(subAliases.choiceFor(specNameId),
                                   encapsulatedArgumentScope.move(specNameId));
            }
        }
    }*/

    // map from internal scope to external scope
    for (const auto &entry : adaptor.finishingMapping) {
        if (encapsulatedArgumentScope.has(entry.first)) {
            ctx.properties.set(entry.second, encapsulatedArgumentScope.move(entry.first));
        }
    }
}

StringView ComposeCacheTasks::getFriendlyName() const
{
    return m_params.friendlyName;
}

const ComposeCacheTasks::AdaptorPerAliases &ComposeCacheTasks::getAdaptorPerAliases(
    const ParamAliases &externalAliases, const MethodCollection &methodCollection) const
{
    auto it = m_adaptorPerSelectionHash.find(externalAliases.hash());
    if (it == m_adaptorPerSelectionHash.end()) {
        it = m_adaptorPerSelectionHash
                 .emplace(externalAliases.hash(),
                          new AdaptorPerAliases(m_params.adaptorAliases, m_params.desiredOutputs,
                                                externalAliases, methodCollection,
                                                m_params.friendlyName))
                 .first;

        return *(*it).second;
    }
    return *(*it).second;
}

void ComposeCacheTasks::forgetAdaptorPerAliases(const ParamAliases &externalAliases) const
{
    m_adaptorPerSelectionHash.erase(externalAliases.hash());
}

ComposeCacheTasks::AdaptorPerAliases::AdaptorPerAliases(const ParamAliases &adaptorAliases,
                                                        const ParamList &desiredOutputs,
                                                        const ParamAliases &externalAliases,
                                                        const MethodCollection &methodCollection,
                                                        StringView friendlyName)
{
    ParamAliases subAliases({{&adaptorAliases, &externalAliases}});

    // All desired outputs must be aliased
    for (auto desiredSpec : desiredOutputs.getSpecList()) {
        auto alias = subAliases.choiceFor(desiredSpec.name);
        if (alias == desiredSpec.name) {
            throw std::runtime_error("desired output '" + desiredSpec.name + "' must be aliased");
        }
    }

    // Find out where we need to stop the pipeline dependency generation
    // We only want to add tasks that are influenced by this adaptor's aliases,
    // So the tasks that could be added to the external pipeline will be added there
    // This is IMPORTANT to prevent double generation and execution of the same tasks
    std::unordered_map<StringId, StringId> totalUsedAliases;
    subAliases.extractAliasNameIds(totalUsedAliases);

    std::unordered_set<StringId> parameterProviderIds;
    parameterProviderIds.reserve(totalUsedAliases.size());
    for (auto [target, choice] : totalUsedAliases) {
        parameterProviderIds.insert(target);
    }
    for (auto desiredNameId : desiredOutputs.getSpecNameIds()) {
        parameterProviderIds.erase(desiredNameId);
    }

    // We use usedAliasChoices as parameters to the pipeline, so it doesn't generate tasks that
    // don't depend on our aliases
    std::vector<StringId> parameterProviderIdsVec(parameterProviderIds.begin(),
                                                  parameterProviderIds.end());
    pipeline =
        Pipeline<ComposeTask>(methodCollection.getComposeMethod(), parameterProviderIdsVec,
                              PipelineParametrizationPolicy::ParametrizedOrDirectDependencies,
                              desiredOutputs, subAliases);

    String filePrefix = std::string("shaderdebug/") + "adaptor_" + String(friendlyName) +
                        getPipelineId(pipeline, subAliases);
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(pipeline, subAliases, file);
        file.close();
    }

    using ListConvPair = std::pair<const ParamList *, ParamList *>;

    for (auto [p_specs, p_targetSpecs] : {ListConvPair{&pipeline.inputSpecs, &inputSpecs},
                                          ListConvPair{&pipeline.filterSpecs, &filterSpecs},
                                          ListConvPair{&pipeline.consumingSpecs, &consumeSpecs}}) {
        for (auto &spec : p_specs->getSpecList()) {
            p_targetSpecs->insert_back(ParamSpec{
                .name = subAliases.choiceStringFor(spec.name),
                .typeInfo = spec.typeInfo,
                .defaultValue = spec.defaultValue,
            });
        }
    }

    for (auto [desiredId, desiredSpec] : desiredOutputs.getMappedSpecs()) {
        auto outerChoice = subAliases.choiceFor(desiredId);

        bool found = false;

        // check if it's in the outputs
        for (auto [nameId, spec] : pipeline.outputSpecs.getMappedSpecs()) {
            auto innerChoice = subAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (spec.typeInfo != TYPE_INFO<void>) {
                    finishingMapping.emplace(innerChoice, desiredId);
                }
                found = true;
                break;
            }
        }

        // check if it's in the filters
        for (auto [nameId, spec] : pipeline.filterSpecs.getMappedSpecs()) {
            auto innerChoice = subAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (desiredId != innerChoice) {
                    if (spec.typeInfo != TYPE_INFO<void>) {
                        finishingMapping.emplace(innerChoice, desiredId);
                    }
                } else {
                    throw std::runtime_error("Desired output spec is a true filter property");
                }
                found = true;
                break;
            }
        }

        // check if it's in the pipethroughs
        for (auto [nameId, spec] : pipeline.pipethroughSpecs.getMappedSpecs()) {
            auto innerChoice = subAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (desiredId != innerChoice) {
                    if (spec.typeInfo != TYPE_INFO<void>) {
                        finishingMapping.emplace(innerChoice, desiredId);
                    }
                } else {
                    throw std::runtime_error("Desired output spec is a pipethrough property");
                }
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Desired output spec '" + desiredSpec.name + "' not found");
        }
    }
}

} // namespace Vitrae