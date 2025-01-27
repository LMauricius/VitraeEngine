#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Setup/Rasterizing.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>

namespace Vitrae
{
class Model;

class ComposeDataRender : public ComposeTask
{
  public:
    using RenderCallback = std::function<void(const glm::mat4 &transform)>;
    using DataGeneratorFunction =
        std::function<void(const RenderComposeContext &context, RenderCallback callback)>;

    struct SetupParams
    {
        ComponentRoot &root;
        ParamList inputSpecs;
        ParamList filterSpecs;
        ParamList consumingSpecs;
        ParamList outputSpecs;
        dynasma::LazyPtr<Model> p_dataPointModel;
        DataGeneratorFunction dataGenerator;
        RasterizingSetupParams rasterizing;
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