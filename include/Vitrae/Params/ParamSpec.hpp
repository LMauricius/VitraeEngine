#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Dynamic/Variant.hpp"

namespace Vitrae
{

struct ParamSpec
{
    String name;
    const TypeInfo &typeInfo;
    Variant defaultValue = EMPTY_VALUE;
};

} // namespace Vitrae