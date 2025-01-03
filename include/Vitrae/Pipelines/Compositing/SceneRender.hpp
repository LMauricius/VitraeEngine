#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Util/Rasterizing.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>

namespace Vitrae
{

class ComposeSceneRender : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> inputTokenNames;
        std::vector<String> outputTokenNames;
        String vertexPositionOutputPropertyName;
        CullingMode cullingMode = CullingMode::Backface;
        RasterizingMode rasterizingMode = RasterizingMode::DerivationalFillCenters;
        bool smoothFilling : 1 = false;
        bool smoothTracing : 1 = false;
        bool smoothDotting : 1 = false;
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