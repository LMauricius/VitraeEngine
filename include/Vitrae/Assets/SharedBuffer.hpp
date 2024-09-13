#pragma once

#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Util/NonCopyable.hpp"

#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Vitrae
{
class Renderer;

using BufferUsageHints = std::uint8_t;
struct BufferUsageHint
{
    static constexpr BufferUsageHints HOST_INIT = 1 << 0;
    static constexpr BufferUsageHints HOST_WRITE = 1 << 1;
    static constexpr BufferUsageHints HOST_READ = 1 << 2;
    static constexpr BufferUsageHints GPU_DRAW = 1 << 3;
    static constexpr BufferUsageHints GPU_COMPUTE = 1 << 4;
};

/**
 * A shared buffer is a data storage object that can easily be shared between CPU and GPU.
 * The raw shared buffer can be used to store any type of data.
 */
class RawSharedBuffer : public dynasma::PolymorphicBase
{
  public:
    struct SetupParams
    {
        Renderer &renderer;
        ComponentRoot &root;
        BufferUsageHints usage = BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW;
        std::size_t size = 0;
        String friendlyName = "";
    };

    virtual ~RawSharedBuffer() = default;

    virtual void synchronize() = 0;
    virtual bool isSynchronized() const = 0;
    virtual std::size_t memory_cost() const = 0;

    void resize(std::size_t size);

    inline std::size_t size() const { return m_size; }
    const Byte *data() const;
    Byte *data();

    std::span<const Byte> operator[](std::pair<std::size_t, std::size_t> slice) const;
    std::span<Byte> operator[](std::pair<std::size_t, std::size_t> slice);

  protected:
    /// @brief The current specified size of the buffer
    std::size_t m_size;
    /// @brief A pointer to the underlying buffer. nullptr if it needs to be requested
    mutable Byte *m_bufferPtr;
    /// @brief  The range of the buffer that needs to be synchronized
    mutable std::pair<std::size_t, std::size_t> m_dirtySpan;

    RawSharedBuffer();

    /**
     * @brief This function is called only while m_bufferPtr is nullptr, and has to set m_bufferPtr
     * to valid m_size long span of memory
     */
    virtual void requestBufferPtr() const = 0;
    /**
     * @brief This function is called before m_size gets changed, and ensures m_bufferPtr is
     * either nullptr or points to a valid m_size long span of memory
     */
    virtual void requestResizeBuffer(std::size_t size) const = 0;
};

struct RawSharedBufferKeeperSeed
{
    using Asset = RawSharedBuffer;
    std::variant<RawSharedBuffer::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using RawSharedBufferKeeper = dynasma::AbstractKeeper<RawSharedBufferKeeperSeed>;

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

    // Location of the FAM adjusted for alignment
    template <typename ElementT2 = ElementT>
    static constexpr std::ptrdiff_t getFirstElementOffset()
        requires HAS_FAM_ELEMENTS
    {
        if constexpr (HAS_HEADER)
            return ((sizeof(HeaderT) - 1) / alignof(ElementT2) + 1) * alignof(ElementT2);
        else
            return 0;
    }

    template <typename ElementT2 = ElementT>
    static constexpr std::size_t calcMinimumBufferSize(std::size_t numElements)
        requires HAS_FAM_ELEMENTS
    {
        if constexpr (HAS_HEADER)
            return getFirstElementOffset() + sizeof(ElementT2) * numElements;
        else
            return sizeof(ElementT2) * numElements;
    }
    static constexpr std::size_t calcMinimumBufferSize()
    {
        if constexpr (HAS_HEADER)
            return sizeof(HeaderT);
        else
            return 0;
    }

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with enough size for the HeaderT
     */
    SharedBufferPtr(ComponentRoot &root, BufferUsageHints usage, StringView friendlyName = "")
        : m_buffer(root.getComponent<RawSharedBufferKeeper>().new_asset(
              RawSharedBufferKeeperSeed{.kernel = RawSharedBuffer::SetupParams{
                                            .renderer = root.getComponent<Renderer>(),
                                            .root = root,
                                            .usage = usage,
                                            .size = calcMinimumBufferSize(),
                                            .friendlyName = String(friendlyName),
                                        }}))
    {}

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with the specified number of elements
     */
    SharedBufferPtr(ComponentRoot &root, BufferUsageHints usage, std::size_t numElements,
                    StringView friendlyName = "")
        requires HAS_FAM_ELEMENTS
        : m_buffer(root.getComponent<RawSharedBufferKeeper>().new_asset(
              RawSharedBufferKeeperSeed{.kernel = RawSharedBuffer::SetupParams{
                                            .renderer = root.getComponent<Renderer>(),
                                            .root = root,
                                            .usage = usage,
                                            .size = calcMinimumBufferSize(numElements),
                                            .friendlyName = String(friendlyName),
                                        }}))
    {}

    /**
     * Constructs a SharedBufferPtr from a RawSharedBuffer FirmPtr
     */
    SharedBufferPtr(dynasma::FirmPtr<RawSharedBuffer> p_buffer) : m_buffer(p_buffer) {}

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
        m_buffer = p_buffer;
        return *this;
    }

    /**
     * Resizes the underlying RawSharedBuffer to contain the given number of FAM elements
     */
    template <typename ElementT2 = ElementT>
    void resizeElements(std::size_t numElements)
        requires HAS_FAM_ELEMENTS
    {
        m_buffer->resize(calcMinimumBufferSize(numElements));
    }

    /**
     * @returns the number of bytes in the underlying RawSharedBuffer
     */
    std::size_t byteSize() const { return m_buffer->size(); }

    /**
     * @returns the number of FAM elements in the underlying RawSharedBuffer
     */
    template <typename ElementT2 = ElementT>
    std::size_t numElements() const
        requires HAS_FAM_ELEMENTS
    {
        return (m_buffer->size() - getFirstElementOffset()) / sizeof(ElementT2);
    }

    /**
     * @returns The header of the buffer
     */
    template <typename HeaderT2 = HeaderT>
    const HeaderT2 &getHeader() const
        requires HAS_HEADER
    {
        return *reinterpret_cast<const HeaderT2 *>(m_buffer->data());
    }
    template <typename HeaderT2 = HeaderT>
    HeaderT2 &getHeader()
        requires HAS_HEADER
    {
        return *reinterpret_cast<HeaderT2 *>((*m_buffer)[{0, sizeof(HeaderT2)}].data());
    }

    /**
     * @returns The FAM element at the given index
     */
    template <typename ElementT2 = ElementT>
    const ElementT2 &getElement(std::size_t index) const
        requires HAS_FAM_ELEMENTS
    {
        return *reinterpret_cast<const ElementT2 *>(m_buffer->data() + getFirstElementOffset() +
                                                    sizeof(ElementT2) * index);
    }
    template <typename ElementT2 = ElementT>
    ElementT2 &getElement(std::size_t index)
        requires HAS_FAM_ELEMENTS
    {
        return *reinterpret_cast<ElementT2 *>(
            (*m_buffer)[{getFirstElementOffset() + sizeof(ElementT2) * index,
                         getFirstElementOffset() + sizeof(ElementT2) * index + sizeof(ElementT2)}]
                .data());
    }
    template <typename ElementT2 = ElementT>
    std::span<ElementT2> getElements()
        requires HAS_FAM_ELEMENTS
    {
        return std::span<ElementT2>(
            reinterpret_cast<ElementT2 *>(m_buffer->data() + getFirstElementOffset()),
            numElements());
    }
    template <typename ElementT2 = ElementT>
    std::span<const ElementT2> getElements() const
        requires HAS_FAM_ELEMENTS
    {
        return std::span<const ElementT2>(
            reinterpret_cast<const ElementT2 *>(
                dynasma::const_pointer_cast<const RawSharedBuffer>(m_buffer)->data() +
                getFirstElementOffset()),
            numElements());
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() { return m_buffer; }
    dynasma::FirmPtr<const RawSharedBuffer> getRawBuffer() const { return m_buffer; }

  protected:
    dynasma::FirmPtr<RawSharedBuffer> m_buffer;
};

template <class T>
concept SharedBufferPtrInst = requires(T t) {
    { SharedBufferPtr{t} } -> std::same_as<T>;
};

} // namespace Vitrae