#pragma once

#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

class ShaderConstant : public ShaderTask
{
  public:
    struct SetupParams
    {
        ParamSpec outputSpec;
        Variant value;
    };
};

struct ShaderConstantKeeperSeed
{
    using Asset = ShaderConstant;
    std::variant<ShaderConstant::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ShaderConstantKeeper = dynasma::AbstractKeeper<ShaderConstantKeeperSeed>;

} // namespace Vitrae