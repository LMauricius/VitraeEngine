#pragma once

#include "dynasma/pointer.hpp"

#include <cstdint>

namespace Vitrae
{

class Variant;
class RawSharedBuffer;
class TypeInfo;

struct BufferPtrMeta
{
    dynasma::FirmPtr<RawSharedBuffer> (*getRawBuffer)(const Variant &variant);

    std::size_t (*getNumElements)(const Variant &variant);
    std::size_t (*getBytesOffset)(const Variant &variant);
    std::size_t (*getBytesStride)(const Variant &variant);

    const TypeInfo &headerTypeInfo;
    const TypeInfo &elementTypeInfo;
};

} // namespace Vitrae