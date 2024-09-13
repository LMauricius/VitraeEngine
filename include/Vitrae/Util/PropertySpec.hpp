#pragma once

#include "Vitrae/Util/Variant.hpp"
#include "Vitrae/Types/Typedefs.hpp"

namespace Vitrae {
    
struct PropertySpec
{
    String name;
    const TypeInfo &typeInfo;
};

}