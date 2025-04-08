#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Setup/Rasterizing.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>

namespace Vitrae
{
class Model;

class ComposeIndexRender : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        String sizeParamName;
        std::vector<String> inputTokenNames;
        std::vector<String> outputTokenNames;
        dynasma::LazyPtr<Model> p_dataPointModel;
        RasterizingSetupParams rasterizing;
    };
};

struct ComposeIndexRenderKeeperSeed
{
    using Asset = ComposeIndexRender;
    std::variant<ComposeIndexRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeIndexRenderKeeper = dynasma::AbstractKeeper<ComposeIndexRenderKeeperSeed>;
} // namespace Vitrae