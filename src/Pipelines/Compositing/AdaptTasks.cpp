#include "Vitrae/Pipelines/Compositing/AdaptTasks.hpp"

#include "MMeter.h"

#include <ranges>

namespace Vitrae {

ComposeAdaptTasks::ComposeAdaptTasks(const SetupParams &params)
    : ComposeTask({}, {}), m_friendlyName(params.friendlyName) {

    for (const auto &[extNameId, intSpec] :
         params.extProperty2InternalInputSpecs) {
        m_ext2InternalInputs.emplace(extNameId, intSpec.name);
        m_internalInputSpecs.emplace_back(intSpec);
    }
    for (const auto &[intSpec, extSpec] :
         params.internalProperty2ExtOutputSpecs) {
        m_outputSpecs.emplace(extSpec.name, extSpec);
        m_internalOutputSpecs.emplace_back(intSpec);
        m_internal2ExtOutputs.emplace(intSpec.name, extSpec.name);
    }
}

const StableMap<StringId, PropertySpec> &
ComposeAdaptTasks::getInputSpecs(const RenderSetupContext &args) const {
    return m_compiledSubPipelines[{args.p_defaultVertexMethod,
                                   args.p_defaultFragmentMethod,
                                   args.p_defaultComputeMethod}]
        .inputSpecs;
}

void ComposeAdaptTasks::run(RenderRunContext args) const {
    MMETER_SCOPE_PROFILER("ComposeAdaptTasks::run");

    RenderSetupContext setupArgs = {
        args.renderer, args.p_composeMethod, args.p_defaultVertexMethod,
        args.p_defaultFragmentMethod, args.p_defaultComputeMethod};

    auto it = m_compiledSubPipelines.find({args.p_defaultVertexMethod,
                                           args.p_defaultFragmentMethod,
                                           args.p_defaultComputeMethod});
    if (it == m_compiledSubPipelines.end()) {
        m_compiledSubPipelines.emplace(
            {args.p_defaultVertexMethod, args.p_defaultFragmentMethod,
             args.p_defaultComputeMethod},
            args.p_composeMethod, m_internalInputSpecs, m_internalOutputSpecs,
            setupArgs);

        throw ComposeTaskRequirementsChangedException();
    } else {

        auto &pipeline = (*it).second;

        try {
            // execute the pipeline
            {
                MMETER_SCOPE_PROFILER("Pipeline execution");

                for (auto &pipeitem : pipeline.items) {
                    pipeitem.p_task->run(args);
                }
            }
        } catch (ComposeTaskRequirementsChangedException) {
            // Rebuild and notify parent runner to rebuild
            pipeline = Pipeline<ComposeTask>(args.p_composeMethod,
                                             m_internalInputSpecs,
                                             m_internalOutputSpecs, setupArgs);

            throw ComposeTaskRequirementsChangedException();
        }
    }
}

void ComposeAdaptTasks::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
    const ScopedDict &properties, const RenderSetupContext &context) const {

    auto &pipeline = m_compiledSubPipelines[{context.p_defaultVertexMethod,
                                             context.p_defaultFragmentMethod,
                                             context.p_defaultComputeMethod}];

    for (auto &pipeitem : std::ranges::reverse_view{pipeline.items}) {
        pipeitem.p_task->prepareRequiredLocalAssets(frameStores, textures,
                                                    properties, context);
    }
}

StringView ComposeAdaptTasks::getFriendlyName() const { return m_friendlyName; }

} // namespace Vitrae