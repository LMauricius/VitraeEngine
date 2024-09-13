#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class ShaderFunction : public ShaderTask
{
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

    inline ShaderFunction(const FileLoadParams &params)
        : ShaderTask(params.inputSpecs, params.outputSpecs)
    {}

    inline ShaderFunction(const StringParams &params)
        : ShaderTask(params.inputSpecs, params.outputSpecs)
    {}
};

struct ShaderFunctionKeeperSeed
{
    using Asset = ShaderFunction;
    std::variant<ShaderFunction::FileLoadParams, ShaderFunction::StringParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderFunctionKeeper = dynasma::AbstractKeeper<ShaderFunctionKeeperSeed>;

} // namespace Vitrae