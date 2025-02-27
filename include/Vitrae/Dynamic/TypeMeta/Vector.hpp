#pragma once

#include <cstdint>

namespace Vitrae
{

class TypeInfo;

struct VectorMeta
{
    const TypeInfo &componentTypeInfo;
    std::size_t numComponents;
};

} // namespace Vitrae