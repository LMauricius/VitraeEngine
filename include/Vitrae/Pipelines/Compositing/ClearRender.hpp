#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

class ComposeClearRender : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        glm::vec4 backgroundColor;
        String displayInputPropertyName;
        String displayOutputPropertyName;
    };

    using ComposeTask::ComposeTask;
};

struct ComposeClearRenderKeeperSeed
{
    using Asset = ComposeClearRender;
    std::variant<ComposeClearRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeClearRenderKeeper = dynasma::AbstractKeeper<ComposeClearRenderKeeperSeed>;
} // namespace Vitrae