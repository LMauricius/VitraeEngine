#pragma once

#include "Vitrae/Assets/Scene.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Setup/Rasterizing.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>

namespace Vitrae
{

class ComposeSceneRender : public ComposeTask
{
  public:
    using FilterFunc = std::function<bool(const MeshProp &prop)>;
    using SortFunc = std::function<bool(const MeshProp &l, const MeshProp &r)>;
    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> inputTokenNames;
        std::vector<String> outputTokenNames;
        RasterizingSetupParams rasterizing;
        struct
        {
            ParamList inputSpecs;
            ParamList filterSpecs;
            ParamList consumingSpecs;
            std::function<std::pair<FilterFunc, SortFunc>(const Scene &scene,
                                                          const RenderComposeContext &ctx)>
                generateFilterAndSort;
        } ordering;
    };
};

struct ComposeSceneRenderKeeperSeed
{
    using Asset = ComposeSceneRender;
    std::variant<ComposeSceneRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeSceneRenderKeeper = dynasma::AbstractKeeper<ComposeSceneRenderKeeperSeed>;
} // namespace Vitrae