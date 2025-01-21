#pragma once

#include "Vitrae/Assets/BufferUtil/LayoutInfo.hpp"
#include "Vitrae/Assets/BufferUtil/Ptr.hpp"
#include "Vitrae/Assets/SharedBuffer.hpp"

#include <tuple>

namespace Vitrae
{

/**
 * A SharedSubBufferPtr is used to access a shared buffer, with a safer underlying type.
 * @tparam ElementT the type stored in the array after the header. Must not be void.
 * @note It can be cast from SharedSubBufferVariantPtr
 */
template <class TElementT> class SharedSubBufferPtr
{
  public:
    using ElementT = TElementT;
    static_assert(!std::is_same_v<ElementT, void>, "ElementT must not be void");

    /**
     * Constructs a SharedSubBufferPtr from a RawSharedBuffer FirmPtr
     */
    SharedSubBufferPtr(dynasma::FirmPtr<RawSharedBuffer> p_buffer, std::size_t bytesOffset,
                       std::size_t bytesStride, std::size_t numElements)
        : mp_buffer(p_buffer), m_bytesOffset(bytesOffset), m_bytesStride(bytesStride),
          m_numElements(numElements)
    {}

    /**
     * Constructs a SharedSubBufferPtr from a SharedBufferPtr
     */
    template <typename HeaderT>
    SharedSubBufferPtr(SharedBufferPtr<HeaderT, ElementT> p_buffer)
        : mp_buffer(p_buffer),
          m_bytesOffset(BufferLayoutInfo::getFirstElementOffset<HeaderT, ElementT>()),
          m_bytesStride(sizeof(ElementT)), m_numElements(p_buffer->numElements())
    {}

    /**
     * @brief Default constructs a null pointer
     * @note Using any methods that count or access elements is UB,
     * until reassigning this to a properly constructed SharedSubBufferPtr
     */
    SharedSubBufferPtr() = default;

    /**
     * @brief Moves the pointer. Nullifies the other
     */
    SharedSubBufferPtr(SharedSubBufferPtr &&other) = default;

    /**
     * @brief copies the pointer.
     */
    SharedSubBufferPtr(const SharedSubBufferPtr &other) = default;

    /**
     * @brief Move assigns the pointer. Nullifies the other
     */
    SharedSubBufferPtr &operator=(SharedSubBufferPtr &&other) = default;

    /**
     * @brief Copy assigns the pointer.
     */
    SharedSubBufferPtr &operator=(const SharedSubBufferPtr &other) = default;

    /**
     * @returns the number of FAM elements in the underlying RawSharedBuffer subset
     */
    inline std::size_t numElements() const { return m_numElements; }

    /**
     * @returns The offset of the subbuffer from the buffer start
     */
    inline std::size_t getBytesOffset() const { return m_bytesOffset; }

    /**
     * @returns The stride (in bytes) of the elements
     */
    inline std::size_t getBytesStride() const { return m_bytesStride; }

    /**
     * @returns The FAM element at the given index
     */
    const ElementT &getElement(std::size_t index) const
    {
        return *reinterpret_cast<const ElementT *>(mp_buffer->data() + m_bytesOffset +
                                                   m_bytesStride * index);
    }
    ElementT &getElement(std::size_t index)
    {
        return *reinterpret_cast<ElementT *>(
            (*mp_buffer)[{m_bytesOffset + m_bytesStride * index,
                          m_bytesOffset + m_bytesStride * index + sizeof(ElementT)}]
                .data());
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() const { return mp_buffer; }

  protected:
    dynasma::FirmPtr<RawSharedBuffer> mp_buffer;

    std::size_t m_bytesOffset;
    std::size_t m_bytesStride;
    std::size_t m_numElements;
};

// Buffer constructions

/**
 * Constructs a new RawSharedBuffer allocated from the
 * Keeper in the root with the specified number of elements
 * @tparam ElementTs The types of the sub-elements that will be interleaved
 * @param root
 * @param outPtrs SharedSubBufferVariantPtr objects, that will be assigned to interleaved subbuffers
 * @param usage Usage hints that will be passed to the RawSharedBuffer constructor
 * @param numElements The number of elements in the interleaved buffer, per each subbuffer
 * @warning The number of outPtrs must be equal to the number of elementTypeinfos!
 */
template <typename... ElementTs>
std::tuple<SharedSubBufferPtr<ElementTs>...> makeBufferInterleaved(ComponentRoot &root,
                                                                   BufferUsageHints usage,
                                                                   std::size_t numElements,
                                                                   StringView friendlyName = "")
{
    std::array<std::size_t, sizeof...(ElementTs)> offsets;

    // Calculate the total size per element
    std::size_t aggregateSize = 0;
    std::size_t maxAlignment = 0;
    std::size_t i = 0;
    (
        [&]() {
            aggregateSize =
                (aggregateSize + alignof(ElementTs) - 1) / alignof(ElementTs) * alignof(ElementTs);

            offsets[i] = aggregateSize;

            aggregateSize += sizeof(ElementTs);

            if (alignof(ElementTs) > maxAlignment)
                maxAlignment = alignof(ElementTs);
            ++i;
        },
        ...);
    aggregateSize = (aggregateSize + maxAlignment - 1) / maxAlignment * maxAlignment;

    // make buffer
    auto p_buffer = root.getComponent<RawSharedBufferKeeper>().new_asset(
        RawSharedBufferKeeperSeed{.kernel = RawSharedBuffer::SetupParams{
                                      .renderer = root.getComponent<Renderer>(),
                                      .root = root,
                                      .usage = usage,
                                      .size = aggregateSize * numElements,
                                      .friendlyName = String(friendlyName),
                                  }});

    // make subbuffers
    i = 0;
    std::tuple<SharedSubBufferPtr<ElementTs>...> ret = {[&]() {
        return SharedSubBufferPtr<ElementTs>(p_buffer, offsets[i++], aggregateSize, numElements);
    }()...};

    return ret;
}

} // namespace Vitrae