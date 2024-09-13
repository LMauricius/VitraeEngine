#pragma once

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Util/Rasterizing.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>

namespace Vitrae
{

class ComposeDataRender : public ComposeTask
{
  public:
    using RenderCallback = std::function<void(const glm::mat4 &transform)>;
    using DataGeneratorFunction =
        std::function<void(const RenderRunContext &context, RenderCallback callback)>;

    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<PropertySpec> inputSpecs;
        String displayInputPropertyName;
        String displayOutputPropertyName;
        dynasma::LazyPtr<Mesh> p_dataPointVisual;
        DataGeneratorFunction dataGenerator;
        String vertexPositionOutputPropertyName;
        CullingMode cullingMode = CullingMode::Backface;
        RasterizingMode rasterizingMode = RasterizingMode::DerivationalFillCenters;
        bool smoothFilling : 1 = false;
        bool smoothTracing : 1 = false;
        bool smoothDotting : 1 = false;
    };

    using ComposeTask::ComposeTask;
};

struct ComposeDataRenderKeeperSeed
{
    using Asset = ComposeDataRender;
    std::variant<ComposeDataRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeDataRenderKeeper = dynasma::AbstractKeeper<ComposeDataRenderKeeperSeed>;
} // namespace Vitrae