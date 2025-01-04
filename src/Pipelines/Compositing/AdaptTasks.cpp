#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"

#include "MMeter.h"

#include <fstream>
#include <ranges>

namespace Vitrae {

ComposeAdaptTasks::ComposeAdaptTasks(const SetupParams &params) : m_params(params) {}

std::size_t ComposeAdaptTasks::memory_cost() const
{
    return sizeof(ComposeAdaptTasks);
}

const PropertyList &ComposeAdaptTasks::getInputSpecs(const PropertyAliases &externalAliases) const
{
    return getAdaptorPerAliases(externalAliases, m_params.root.getComponent<MethodCollection>())
        .inputSpecs;
}

const PropertyList &ComposeAdaptTasks::getOutputSpecs() const
{
    return m_params.desiredOutputs;
}

const PropertyList &ComposeAdaptTasks::getFilterSpecs(const PropertyAliases &externalAliases) const
{
    return getAdaptorPerAliases(externalAliases, m_params.root.getComponent<MethodCollection>())
        .filterSpecs;
}

const PropertyList &ComposeAdaptTasks::getConsumingSpecs(
    const PropertyAliases &externalAliases) const
{
    return getAdaptorPerAliases(externalAliases, m_params.root.getComponent<MethodCollection>())
        .consumeSpecs;
}

void ComposeAdaptTasks::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                         const PropertyAliases &aliases) const
{
    auto &specs = getAdaptorPerAliases(aliases, m_params.root.getComponent<MethodCollection>());

    for (const PropertyList *p_specs :
         {&specs.inputSpecs, &m_params.desiredOutputs, &specs.filterSpecs, &specs.consumeSpecs}) {
        for (const PropertySpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeAdaptTasks::extractSubTasks(std::set<const Task *> &taskSet,
                                        const PropertyAliases &aliases) const
{
    taskSet.insert(this);
}

void ComposeAdaptTasks::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER("ComposeAdaptTasks::run");

    const AdaptorPerAliases &adaptor =
        getAdaptorPerAliases(ctx.aliases, m_params.root.getComponent<MethodCollection>());

    ScopedDict encapsulatedScope(&ctx.properties.getUnaliasedScope());

    // Note: we will use only the pipeline's usedSelection in the subpipeline,
    // because anything else is unused and potential performance hog
    ArgumentScope encapsulatedArgumentScope(&encapsulatedScope, &adaptor.pipeline.usedSelection);

    // construct the encapsulated context
    RenderComposeContext subCtx{
        .properties = encapsulatedArgumentScope,
        .root = ctx.root,
        .aliases = adaptor.pipeline.usedSelection,
    };

    // execute the pipeline
    try {
        {
            MMETER_SCOPE_PROFILER("Pipeline execution");

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
        ctx.properties.set(entry.second, encapsulatedScope.move(entry.first));
    }
}

void ComposeAdaptTasks::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER("ComposeAdaptTasks::run");

    const AdaptorPerAliases &adaptor =
        getAdaptorPerAliases(ctx.aliases, ctx.root.getComponent<MethodCollection>());

    ScopedDict encapsulatedScope(&ctx.properties.getUnaliasedScope());

    // Note: we will use only the pipeline's usedSelection in the subpipeline,
    // because anything else is unused and potential performance hog
    ArgumentScope encapsulatedArgumentScope(&encapsulatedScope, &adaptor.pipeline.usedSelection);

    // construct the encapsulated context
    RenderComposeContext subCtx{
        .properties = encapsulatedArgumentScope,
        .root = ctx.root,
        .aliases = adaptor.pipeline.usedSelection,
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

    // map from internal scope to external scope
    // since we are running in reverse, map input specs of the pipeline
    for (auto p_specs : {&adaptor.pipeline.inputSpecs, &adaptor.pipeline.filterSpecs}) {
        for (auto specNameId : p_specs->getSpecNameIds()) {
            ctx.properties.set(m_params.adaptorAliases.choiceFor(specNameId),
                               encapsulatedArgumentScope.move(specNameId));
        }
    }
}

StringView ComposeAdaptTasks::getFriendlyName() const
{
    return m_params.friendlyName;
}

const ComposeAdaptTasks::AdaptorPerAliases &ComposeAdaptTasks::getAdaptorPerAliases(
    const PropertyAliases &externalAliases, const MethodCollection &methodCollection) const
{
    auto it = m_adaptorPerSelectionHash.find(externalAliases.hash());
    if (it == m_adaptorPerSelectionHash.end()) {
        it = m_adaptorPerSelectionHash
                 .emplace(externalAliases.hash(), m_params.adaptorAliases, m_params.desiredOutputs,
                          externalAliases, methodCollection, m_params.friendlyName)
                 .first;

        return (*it).second;
    }
    return (*it).second;
}

void ComposeAdaptTasks::forgetAdaptorPerAliases(const PropertyAliases &externalAliases) const
{
    m_adaptorPerSelectionHash.erase(externalAliases.hash());
}

ComposeAdaptTasks::AdaptorPerAliases::AdaptorPerAliases(const PropertyAliases &adaptorAliases,
                                                        const PropertyList &desiredOutputs,
                                                        const PropertyAliases &externalAliases,
                                                        const MethodCollection &methodCollection,
                                                        StringView friendlyName)
{
    const PropertyAliases *aliasArray[] = {&adaptorAliases, &externalAliases};
    PropertyAliases subAliases(aliasArray);

    pipeline =
        Pipeline<ComposeTask>(methodCollection.getComposeMethod(), desiredOutputs, subAliases);

    String filePrefix =
        std::string("shaderdebug/") + "adaptor_" + String(friendlyName) + getPipelineId(pipeline);
    {
        std::ofstream file;
        String filename = filePrefix + ".dot";
        file.open(filename);
        exportPipeline(pipeline, file);
        file.close();
    }

    using ListConvPair = std::pair<const PropertyList *, PropertyList *>;

    for (auto [p_specs, p_targetSpecs] : {ListConvPair{&pipeline.inputSpecs, &inputSpecs},
                                          ListConvPair{&pipeline.filterSpecs, &filterSpecs},
                                          ListConvPair{&pipeline.consumingSpecs, &consumeSpecs}}) {
        for (auto &spec : p_specs->getSpecList()) {
            p_targetSpecs->insert_back(PropertySpec{
                .name = adaptorAliases.choiceStringFor(spec.name),
                .typeInfo = spec.typeInfo,
            });
        }
    }

    for (auto [desiredId, desiredSpec] : desiredOutputs.getMappedSpecs()) {
        auto outerChoice = adaptorAliases.choiceFor(desiredId);

        bool found = false;

        // check if it's in the outputs
        for (auto [nameId, spec] : pipeline.outputSpecs.getMappedSpecs()) {
            auto innerChoice = adaptorAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (spec.typeInfo != Variant::getTypeInfo<void>()) {
                    finishingMapping.emplace(innerChoice, desiredId);
                }
                found = true;
                break;
            }
        }

        // check if it's in the filters
        for (auto [nameId, spec] : pipeline.filterSpecs.getMappedSpecs()) {
            auto innerChoice = adaptorAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (desiredId != innerChoice) {
                    if (spec.typeInfo != Variant::getTypeInfo<void>()) {
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
            auto innerChoice = adaptorAliases.choiceFor(nameId);
            if (outerChoice == innerChoice) {
                if (desiredId != innerChoice) {
                    if (spec.typeInfo != Variant::getTypeInfo<void>()) {
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
            throw std::runtime_error("Desired output spec not found");
        }
    }
}

} // namespace Vitrae