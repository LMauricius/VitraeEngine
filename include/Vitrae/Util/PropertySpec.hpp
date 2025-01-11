#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Util/Variant.hpp"

namespace Vitrae {
    
struct PropertySpec
{
    String name;
    const TypeInfo &typeInfo;
};

}