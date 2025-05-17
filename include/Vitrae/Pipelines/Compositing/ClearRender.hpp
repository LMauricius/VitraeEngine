#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Clears the frame with the default clear colors for each render component
 *
 */
class ComposeClearRender : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> outputTokenNames;
    };
};

struct ComposeClearRenderKeeperSeed
{
    using Asset = ComposeClearRender;
    std::variant<ComposeClearRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeClearRenderKeeper = dynasma::AbstractKeeper<ComposeClearRenderKeeperSeed>;
} // namespace Vitrae