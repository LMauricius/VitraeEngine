#pragma once

#include "Vitrae/Assets/SharedBuffer.hpp"

namespace Vitrae
{

/**
 * A SharedBufferPtr is used to access a shared buffer, with a safer underlying type.
 * @tparam HeaderT the type stored at the start of the buffer. Use void if there is no header.
 * @tparam ElementT the type stored in the array after the header. Use void if there is no FAM
 */
template <class THeaderT, class TElementT> class SharedBufferPtr
{
  public:
    using HeaderT = THeaderT;
    using ElementT = TElementT;

    static constexpr bool HAS_HEADER = !std::is_same_v<HeaderT, void>;
    static constexpr bool HAS_FAM_ELEMENTS = !std::is_same_v<ElementT, void>;

    /**
     * @return The offset (in bytes) of the first FAM element
     */
    template <typename ElementT2 = ElementT>
    static constexpr std::ptrdiff_t getFirstElementOffset()
        requires HAS_FAM_ELEMENTS
    {
        if constexpr (HAS_HEADER)
            return ((sizeof(HeaderT) - 1) / alignof(ElementT2) + 1) * alignof(ElementT2);
        else
            return 0;
    }

    /**
     * @return Minimum buffer size for the given number of FAM elements
     * @param numElements The number of FAM elements
     */
    template <typename ElementT2 = ElementT>
    static constexpr std::size_t calcMinimumBufferSize(std::size_t numElements)
        requires HAS_FAM_ELEMENTS
    {
        if constexpr (HAS_HEADER)
            return getFirstElementOffset() + sizeof(ElementT2) * numElements;
        else
            return sizeof(ElementT2) * numElements;
    }

    /**
     * @return Minimum buffer size with no elements
     */
    static constexpr std::size_t calcMinimumBufferSize()
    {
        if constexpr (HAS_FAM_ELEMENTS)
            return getFirstElementOffset();
        else if constexpr (HAS_HEADER)
            return sizeof(HeaderT);
        return 0;
    }

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with enough size for the HeaderT
     */
    static SharedBufferPtr makeBuffer(ComponentRoot &root, BufferUsageHints usage,
                                      StringView friendlyName = "")

    {
        return SharedBufferPtr(root.getComponent<RawSharedBufferKeeper>().new_asset(
            RawSharedBufferKeeperSeed{.kernel = RawSharedBuffer::SetupParams{
                                          .renderer = root.getComponent<Renderer>(),
                                          .root = root,
                                          .usage = usage,
                                          .size = calcMinimumBufferSize(),
                                          .friendlyName = String(friendlyName),
                                      }}));
    }

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with the specified number of elements
     */
    static SharedBufferPtr makeBuffer(ComponentRoot &root, BufferUsageHints usage,
                                      std::size_t numElements, StringView friendlyName = "")
        requires HAS_FAM_ELEMENTS
    {
        return SharedBufferPtr(root.getComponent<RawSharedBufferKeeper>().new_asset(
            RawSharedBufferKeeperSeed{.kernel = RawSharedBuffer::SetupParams{
                                          .renderer = root.getComponent<Renderer>(),
                                          .root = root,
                                          .usage = usage,
                                          .size = calcMinimumBufferSize(numElements),
                                          .friendlyName = String(friendlyName),
                                      }}));
    }

    /**
     * Constructs a SharedBufferPtr from a RawSharedBuffer FirmPtr
     */
    SharedBufferPtr(dynasma::FirmPtr<RawSharedBuffer> p_buffer) : mp_buffer(p_buffer) {}

    /**
     * @brief Default constructs a null pointer
     * @note Using any methods that count or access elements is UB,
     * until reassigning this to a properly constructed SharedBufferPtr
     */
    SharedBufferPtr() = default;

    /**
     * @brief Moves the pointer. Nullifies the other
     */
    SharedBufferPtr(SharedBufferPtr &&other) = default;

    /**
     * @brief copies the pointer.
     */
    SharedBufferPtr(const SharedBufferPtr &other) = default;

    /**
     * @brief Move assigns the pointer. Nullifies the other
     */
    SharedBufferPtr &operator=(SharedBufferPtr &&other) = default;

    /**
     * @brief Copy assigns the pointer.
     */
    SharedBufferPtr &operator=(const SharedBufferPtr &other) = default;

    /**
     * @brief Assigns the pointer to the raw buffer.
     * @note Make sure the buffer is compatible with the expected data types
     */
    SharedBufferPtr &operator=(dynasma::FirmPtr<RawSharedBuffer> p_buffer)
    {
        mp_buffer = p_buffer;
        return *this;
    }

    /**
     * Resizes the underlying RawSharedBuffer to contain the given number of FAM elements
     */
    template <typename ElementT2 = ElementT>
    void resizeElements(std::size_t numElements)
        requires HAS_FAM_ELEMENTS
    {
        mp_buffer->resize(calcMinimumBufferSize(numElements));
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
        return (mp_buffer->size() - getFirstElementOffset()) / sizeof(ElementT2);
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
    HeaderT2 &getHeader()
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
        return *reinterpret_cast<const ElementT2 *>(mp_buffer->data() + getFirstElementOffset() +
                                                    sizeof(ElementT2) * index);
    }
    template <typename ElementT2 = ElementT>
    ElementT2 &getElement(std::size_t index)
        requires HAS_FAM_ELEMENTS
    {
        return *reinterpret_cast<ElementT2 *>(
            (*mp_buffer)[{getFirstElementOffset() + sizeof(ElementT2) * index,
                          getFirstElementOffset() + sizeof(ElementT2) * index + sizeof(ElementT2)}]
                .data());
    }
    template <typename ElementT2 = ElementT>
    std::span<ElementT2> getElements()
        requires HAS_FAM_ELEMENTS
    {
        return std::span<ElementT2>(
            reinterpret_cast<ElementT2 *>(mp_buffer->data() + getFirstElementOffset()),
            numElements());
    }
    template <typename ElementT2 = ElementT>
    std::span<const ElementT2> getElements() const
        requires HAS_FAM_ELEMENTS
    {
        return std::span<const ElementT2>(
            reinterpret_cast<const ElementT2 *>(
                dynasma::const_pointer_cast<const RawSharedBuffer>(mp_buffer)->data() +
                getFirstElementOffset()),
            numElements());
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() { return mp_buffer; }
    dynasma::FirmPtr<const RawSharedBuffer> getRawBuffer() const { return mp_buffer; }

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
} // namespace Vitrae