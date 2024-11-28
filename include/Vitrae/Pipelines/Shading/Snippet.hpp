#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class ShaderSnippet : public ShaderTask {
  public:
    struct FileLoadParams
    {
        std::vector<PropertySpec> inputSpecs;
        std::vector<PropertySpec> outputSpecs;
        std::filesystem::path filepath;
        String functionName;
    };
    struct StringParams
    {
        std::vector<PropertySpec> inputSpecs;
        std::vector<PropertySpec> outputSpecs;
        String snippet;
        String functionName;
    };

    inline ShaderSnippet(const FileLoadParams &params)
        : ShaderTask(params.inputSpecs, params.outputSpecs) {}

    inline ShaderSnippet(const StringParams &params)
        : ShaderTask(params.inputSpecs, params.outputSpecs) {}
};

struct ShaderSnippetKeeperSeed {
    using Asset = ShaderSnippet;
    std::variant<ShaderSnippet::FileLoadParams, ShaderSnippet::StringParams>
        kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderSnippetKeeper = dynasma::AbstractKeeper<ShaderSnippetKeeperSeed>;

} // namespace Vitrae