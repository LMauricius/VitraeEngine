#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/ComponentRoot.hpp"

#include "MMeter.h"

#include <ranges>

namespace Vitrae {

ComposeAdaptTasks::ComposeAdaptTasks(const SetupParams &params) : m_params(params) {}

const PropertyList &ComposeAdaptTasks::getInputSpecs(const PropertyAliases &externalAliases) const
{
    return getAdaptorPerAliases(externalAliases, m_params.root.getComponent<MethodCollection>())
        .inputSpecs;
}

const PropertyList &ComposeAdaptTasks::getOutputSpecs(const PropertyAliases &externalAliases) const
{
    return getAdaptorPerAliases(externalAliases, m_params.root.getComponent<MethodCollection>())
        .outputSpecs;
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
                          externalAliases, methodCollection)
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
                                                        const MethodCollection &methodCollection)
{
    const PropertyAliases *aliasArray[] = {&adaptorAliases, &externalAliases};
    PropertyAliases subAliases(aliasArray);

    pipeline =
        Pipeline<ComposeTask>(methodCollection.getComposeMethod(), desiredOutputs, subAliases);

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

    std::unordered_map<StringId, StringId> choosen2desired;
    for (auto desiredId : desiredOutputs.getSpecNameIds()) {
        auto choice = adaptorAliases.choiceFor(desiredId);
        choosen2desired.emplace(choice, desiredId);
    }

    for (const auto &specs : {pipeline.outputSpecs, pipeline.filterSpecs}) {
        for (auto id : specs.getSpecNameIds()) {
            for (auto desiredId : desiredOutputs.getSpecNameIds()) {

                auto outerChoice = adaptorAliases.choiceFor(desiredId);
                auto innerChoice = adaptorAliases.choiceFor(id);

                if (outerChoice == innerChoice) {
                    if (innerChoice != desiredId) {
                        finishingMapping.emplace(innerChoice, desiredId);
                    }
                    break;
                }
            }
        }
    }
}

} // namespace Vitrae