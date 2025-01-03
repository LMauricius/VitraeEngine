#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Clears the frame with the background color
 *
 */
class ComposeClearRender : public ComposeTask
{
  public:
    struct SetupParams
    {
        ComponentRoot &root;
        glm::vec4 backgroundColor;
        std::vector<String> outputTokenNames;
    };

    /**
     * The spec for frame_store filter property
     */
    static const PropertySpec FRAME_STORE_SPEC;

    /**
     * The constant filter specs of ComposeClearRender
     */
    static const PropertyList FILTER_SPECS;
};

struct ComposeClearRenderKeeperSeed
{
    using Asset = ComposeClearRender;
    std::variant<ComposeClearRender::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeClearRenderKeeper = dynasma::AbstractKeeper<ComposeClearRenderKeeperSeed>;
} // namespace Vitrae