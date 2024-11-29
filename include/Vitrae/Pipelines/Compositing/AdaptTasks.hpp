#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "Vitrae/Pipelines/Pipeline.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <map>
#include <vector>

namespace Vitrae {

class ComposeAdaptTasks : public ComposeTask {
    String m_friendlyName;

    mutable StableMap<std::tuple<dynasma::LazyPtr<Method<ShaderTask>>,
                                 dynasma::LazyPtr<Method<ShaderTask>>,
                                 dynasma::LazyPtr<Method<ShaderTask>>>,
                      Pipeline<ComposeTask>>
        m_compiledSubPipelines;

    StableMap<StringId, StringId> m_ext2InternalInputs;
    StableMap<StringId, StringId> m_internal2ExtOutputs;
    std::vector<PropertySpec> m_internalInputSpecs;
    std::vector<PropertySpec> m_internalOutputSpecs;

  public:
    struct SetupParams {
        StableMap<StringId, PropertySpec> extProperty2InternalInputSpecs;
        StableMap<PropertySpec, PropertySpec> internalProperty2ExtOutputSpecs;
        String friendlyName;
    };

    ComposeAdaptTasks(const SetupParams &params);
    ~ComposeAdaptTasks() = default;

    const StableMap<StringId, PropertySpec> &
    getInputSpecs(const RenderSetupContext &args) const override;

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(
        StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
        StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
        const ScopedDict &properties,
        const RenderSetupContext &context) const override;

    StringView getFriendlyName() const override;
};

struct ComposeAdaptTasksKeeperSeed {
    using Asset = ComposeAdaptTasks;
    std::variant<ComposeAdaptTasks::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeAdaptTasksKeeper =
    dynasma::AbstractKeeper<ComposeAdaptTasksKeeperSeed>;
} // namespace Vitrae