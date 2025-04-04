#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae {

class ShaderFunctionCall : public ShaderTask {
  public:
    struct StringParams
    {
        ParamList consumingSpecs;
        ParamList inputSpecs;
        ParamList filterSpecs;
        ParamList outputSpecs;
        String functionName;
    };
};

struct ShaderFunctionCallKeeperSeed {
    using Asset = ShaderFunctionCall;
    std::variant<ShaderFunctionCall::StringParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderFunctionCallKeeper =
    dynasma::AbstractKeeper<ShaderFunctionCallKeeperSeed>;

} // namespace Vitrae