#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class ShaderSnippet : public ShaderTask {
  public:
    struct StringParams
    {
        PropertyList inputSpecs;
        PropertyList outputSpecs;
        PropertyList filterSpecs;
        PropertyList consumingSpecs;
        String snippet;
    };
};

struct ShaderSnippetKeeperSeed {
    using Asset = ShaderSnippet;
    std::variant<ShaderSnippet::StringParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderSnippetKeeper = dynasma::AbstractKeeper<ShaderSnippetKeeperSeed>;

} // namespace Vitrae