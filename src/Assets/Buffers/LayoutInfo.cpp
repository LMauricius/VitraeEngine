#include "Vitrae/Assets/Buffers/LayoutInfo.hpp"

namespace Vitrae
{

std::ptrdiff_t BufferLayoutInfo::getFirstElementOffset(const TypeInfo &headerTypeinfo,
                                                       const TypeInfo &elementTypeinfo)
{
    if (headerTypeinfo != TYPE_INFO<void>)
        return ((headerTypeinfo.size - 1) / elementTypeinfo.alignment + 1) *
               elementTypeinfo.alignment;
    else
        return 0;
}

std::size_t BufferLayoutInfo::calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                                    const TypeInfo &elementTypeinfo,
                                                    std::size_t numElements)
{
    if (headerTypeinfo != TYPE_INFO<void>)
        return getFirstElementOffset(headerTypeinfo, elementTypeinfo) +
               elementTypeinfo.size * numElements;
    else
        return elementTypeinfo.size * numElements;
}

std::size_t BufferLayoutInfo::calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                                    const TypeInfo &elementTypeinfo)
{
    if (elementTypeinfo != TYPE_INFO<void>)
        return getFirstElementOffset(headerTypeinfo, elementTypeinfo);
    else
        return headerTypeinfo.size;
}
} // namespace Vitrae