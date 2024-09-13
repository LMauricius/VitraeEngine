#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class ComposeFunction : public ComposeTask
{
    std::function<void(const RenderRunContext &)> mp_function;
    String m_friendlyName;

  public:
    struct SetupParams
    {
        std::vector<PropertySpec> inputSpecs;
        std::vector<PropertySpec> outputSpecs;
        std::function<void(const RenderRunContext &)> p_function;
        String friendlyName;
    };

    ComposeFunction(const SetupParams &params);
    ~ComposeFunction() = default;

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
                                    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
                                    const ScopedDict &properties) const override;

    StringView getFriendlyName() const override;
};

struct ComposeFunctionKeeperSeed
{
    using Asset = ComposeFunction;
    std::variant<ComposeFunction::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFunctionKeeper = dynasma::AbstractKeeper<ComposeFunctionKeeperSeed>;
} // namespace Vitrae