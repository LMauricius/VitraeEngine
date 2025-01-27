#pragma once

#include "Vitrae/Assets/BufferUtil/LayoutInfo.hpp"
#include "Vitrae/Assets/BufferUtil/VariantPtr.hpp"
#include "Vitrae/Assets/SharedBuffer.hpp"

namespace Vitrae
{
class SharedBufferVariantPtr;

/**
 * A SharedBufferPtr is used to access a shared buffer, with a safer underlying type.
 * @tparam HeaderT the type stored at the start of the buffer. Use void if there is no header.
 * @tparam ElementT the type stored in the array after the header. Use void if there is no FAM
 * @note It can be returned along with a constructed buffer by makeBuffer<...>(...) functions
 */
template <class THeaderT, class TElementT> class SharedBufferPtr
{
  public:
    using HeaderT = THeaderT;
    using ElementT = TElementT;

    static constexpr bool HAS_HEADER = !std::is_same_v<HeaderT, void>;
    static constexpr bool HAS_FAM_ELEMENTS = !std::is_same_v<ElementT, void>;

    /**
     * Constructs a SharedBufferPtr from a RawSharedBuffer FirmPtr
     * @note Make sure the buffer is compatible with the expected data types
     */
    SharedBufferPtr(dynasma::FirmPtr<RawSharedBuffer> p_buffer) : mp_buffer(p_buffer) {}

    /**
     * Constructs a SharedBufferPtr from a SharedBufferVariantPtr FirmPtr
     */
    SharedBufferPtr(SharedBufferVariantPtr p_buffer) : mp_buffer(p_buffer.getRawBuffer())
    {
        if (p_buffer.getHeaderTypeInfo() != TYPE_INFO<HeaderT>) {
            throw std::invalid_argument(
                "SharedBufferPtr: Header type mismatch by assigning variant's " +
                String(p_buffer.getHeaderTypeInfo().getShortTypeName()));
        }
        if (p_buffer.getElementTypeInfo() != TYPE_INFO<ElementT>) {
            throw std::invalid_argument(
                "SharedBufferPtr: Element type mismatch by assigning variant's " +
                String(p_buffer.getElementTypeInfo().getShortTypeName()));
        }
    }

    /**
     * @brief Default constructs a null pointer
     * @note Using any methods that count or access elements is UB,
     * until reassigning this to a properly constructed SharedBufferPtr
     */
    SharedBufferPtr() = default;
    SharedBufferPtr(SharedBufferPtr &&other) = default;
    SharedBufferPtr(const SharedBufferPtr &other) = default;
    SharedBufferPtr &operator=(SharedBufferPtr &&other) = default;
    SharedBufferPtr &operator=(const SharedBufferPtr &other) = default;

    bool operator==(const SharedBufferPtr &other) const = default;
    auto operator<=>(const SharedBufferPtr &other) const = default;

    /**
     * Resizes the underlying RawSharedBuffer to contain the given number of FAM elements
     */
    template <typename ElementT2 = ElementT>
    void resizeElements(std::size_t numElements)
        requires HAS_FAM_ELEMENTS
    {
        mp_buffer->resize(
            BufferLayoutInfo::calcMinimumBufferSize<THeaderT, TElementT>(numElements));
    }

    /**
     * @returns the number of bytes in the underlying RawSharedBuffer
     */
    std::size_t byteSize() const { return mp_buffer->size(); }

    /**
     * @returns the number of FAM elements in the underlying RawSharedBuffer
     */
    template <typename ElementT2 = ElementT>
    std::size_t numElements() const
        requires HAS_FAM_ELEMENTS
    {
        return (mp_buffer->size() -
                BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>()) /
               sizeof(ElementT2);
    }

    /**
     * @returns The header of the buffer
     */
    template <typename HeaderT2 = HeaderT>
    const HeaderT2 &getHeader() const
        requires HAS_HEADER
    {
        return *reinterpret_cast<const HeaderT2 *>(mp_buffer->data());
    }
    template <typename HeaderT2 = HeaderT>
    HeaderT2 &getMutableHeader() const
        requires HAS_HEADER
    {
        return *reinterpret_cast<HeaderT2 *>((*mp_buffer)[{0, sizeof(HeaderT2)}].data());
    }

    /**
     * @returns The FAM element at the given index
     */
    template <typename ElementT2 = ElementT>
    const ElementT2 &getElement(std::size_t index) const
        requires HAS_FAM_ELEMENTS
    {
        return *reinterpret_cast<const ElementT2 *>(
            mp_buffer->data() + BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>() +
            sizeof(ElementT2) * index);
    }
    template <typename ElementT2 = ElementT>
    ElementT2 &getMutableElement(std::size_t index) const
        requires HAS_FAM_ELEMENTS
    {
        return *reinterpret_cast<ElementT2 *>(
            (*mp_buffer)[{BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>() +
                              sizeof(ElementT2) * index,
                          BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>() +
                              sizeof(ElementT2) * index + sizeof(ElementT2)}]
                .data());
    }
    /**
     * @returns A span of all FAM elements
     */
    template <typename ElementT2 = ElementT>
    std::span<const ElementT2> getElements() const
        requires HAS_FAM_ELEMENTS
    {
        return std::span<const ElementT2>(
            reinterpret_cast<const ElementT2 *>(
                mp_buffer->data() + BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>()),
            numElements());
    }
    template <typename ElementT2 = ElementT>
    std::span<ElementT2> getMutableElements() const
        requires HAS_FAM_ELEMENTS
    {
        return std::span<ElementT2>(
            reinterpret_cast<ElementT2 *>(
                mp_buffer->mutableData() +
                BufferLayoutInfo::getFirstElementOffset<THeaderT, TElementT>()),
            numElements());
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() const { return mp_buffer; }

  protected:
    dynasma::FirmPtr<RawSharedBuffer> mp_buffer;
};

/**
 * An instance of SharedBufferPtr<>
 */
template <class T>
concept SharedBufferPtrInst = requires(T t) {
    { SharedBufferPtr{t} } -> std::same_as<T>;
};

/**
 * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
 * with enough size for the HeaderT
 */
template <class THeaderT, class TElementT>
SharedBufferPtr<THeaderT, TElementT> makeBuffer(ComponentRoot &root, BufferUsageHints usage,
                                                StringView friendlyName = "")
{
    return SharedBufferPtr<THeaderT, TElementT>(
        root.getComponent<RawSharedBufferKeeper>().new_asset(RawSharedBufferKeeperSeed{
            .kernel = RawSharedBuffer::SetupParams{
                .renderer = root.getComponent<Renderer>(),
                .root = root,
                .usage = usage,
                .size = BufferLayoutInfo::calcMinimumBufferSize<THeaderT, TElementT>(),
                .friendlyName = String(friendlyName),
            }}));
}

/**
 * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
 * with the specified number of elements
 */
template <class THeaderT, class TElementT>
SharedBufferPtr<THeaderT, TElementT> makeBuffer(ComponentRoot &root, BufferUsageHints usage,
                                                std::size_t numElements,
                                                StringView friendlyName = "")
    requires(!std::same_as<TElementT, void>)
{
    return SharedBufferPtr<THeaderT, TElementT>(
        root.getComponent<RawSharedBufferKeeper>().new_asset(RawSharedBufferKeeperSeed{
            .kernel = RawSharedBuffer::SetupParams{
                .renderer = root.getComponent<Renderer>(),
                .root = root,
                .usage = usage,
                .size = BufferLayoutInfo::calcMinimumBufferSize<THeaderT, TElementT>(numElements),
                .friendlyName = String(friendlyName),
            }}));
}
} // namespace Vitrae