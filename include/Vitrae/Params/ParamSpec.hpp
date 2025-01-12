#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

namespace Vitrae
{

struct ParamSpec
{
    String name;
    const TypeInfo &typeInfo;
};

} // namespace Vitrae