#pragma once

#include <cstdint>

namespace Vitrae
{

class TypeInfo;

struct VectorMeta
{
    virtual std::size_t getNumComponents() const = 0;
    virtual const TypeInfo &getComponentTypeInfo() const = 0;
};

} // namespace Vitrae