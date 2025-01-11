#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/Variant.hpp"

namespace Vitrae
{

struct ParamSpec
{
    String name;
    const TypeInfo &typeInfo;
};

} // namespace Vitrae