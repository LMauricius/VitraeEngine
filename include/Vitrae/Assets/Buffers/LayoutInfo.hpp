#pragma once

#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <concepts>

namespace Vitrae
{

namespace BufferLayoutInfo
{
/**
 * @return The offset (in bytes) of the first FAM element
 * @param headerTypeinfo The TypeInfo for the header
 * @param elementTypeinfo The TypeInfo for the FAM elements
 */
std::ptrdiff_t getFirstElementOffset(const TypeInfo &headerTypeinfo,
                                     const TypeInfo &elementTypeinfo);

template <typename HeaderT, typename ElementT>
constexpr std::ptrdiff_t getFirstElementOffset()
    requires(!std::same_as<ElementT, void>)
{
    if constexpr (!std::same_as<HeaderT, void>)
        return ((sizeof(HeaderT) - 1) / alignof(ElementT) + 1) * alignof(ElementT);
    else
        return 0;
}

/**
 * @return Minimum buffer size for the given number of FAM elements
 * @param headerTypeinfo The TypeInfo for the header
 * @param elementTypeinfo The TypeInfo for the FAM elements
 * @param numElements The number of FAM elements
 */
std::size_t calcMinimumBufferSize(const TypeInfo &headerTypeinfo, const TypeInfo &elementTypeinfo,
                                  std::size_t numElements);

template <typename HeaderT, typename ElementT>
constexpr std::size_t calcMinimumBufferSize(std::size_t numElements)
    requires(!std::same_as<ElementT, void>)
{
    if constexpr (!std::same_as<HeaderT, void>)
        return BufferLayoutInfo::getFirstElementOffset<HeaderT, ElementT>() +
               sizeof(ElementT) * numElements;
    else
        return sizeof(ElementT) * numElements;
}

/**
 * @return Minimum buffer size with no elements
 * @param headerTypeinfo The TypeInfo for the header
 * @param elementTypeinfo The TypeInfo for the FAM elements
 */
std::size_t calcMinimumBufferSize(const TypeInfo &headerTypeinfo, const TypeInfo &elementTypeinfo);

template <typename HeaderT, typename ElementT> constexpr std::size_t calcMinimumBufferSize()
{
    if constexpr (!std::same_as<ElementT, void>)
        return BufferLayoutInfo::getFirstElementOffset<HeaderT, ElementT>();
    else if constexpr (!std::same_as<HeaderT, void>)
        return sizeof(HeaderT);
    return 0;
}

} // namespace BufferLayoutInfo

} // namespace Vitrae