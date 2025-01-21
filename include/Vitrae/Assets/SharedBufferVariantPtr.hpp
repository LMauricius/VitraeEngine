#pragma once

#include "Vitrae/Assets/SharedBuffer.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <cstddef>

namespace Vitrae
{

/**
 * A SharedBufferVariantPtr is used to access a shared buffer, with a type-checked underlying type.
 */
class SharedBufferVariantPtr
{
  public:
    /**
     * @return The offset (in bytes) of the first FAM element
     * @param headerTypeinfo The TypeInfo for the header
     * @param elementTypeinfo The TypeInfo for the FAM elements
     */
    static std::ptrdiff_t getFirstElementOffset(const TypeInfo &headerTypeinfo,
                                                const TypeInfo &elementTypeinfo);

    /**
     * @return Minimum buffer size for the given number of FAM elements
     * @param headerTypeinfo The TypeInfo for the header
     * @param elementTypeinfo The TypeInfo for the FAM elements
     * @param numElements The number of FAM elements
     */
    static std::size_t calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                             const TypeInfo &elementTypeinfo,
                                             std::size_t numElements);

    /**
     * @return Minimum buffer size with no elements
     * @param headerTypeinfo The TypeInfo for the header
     * @param elementTypeinfo The TypeInfo for the FAM elements
     */
    static std::size_t calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                             const TypeInfo &elementTypeinfo);

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with enough size for the HeaderT
     */
    static SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                             const TypeInfo &elementTypeinfo,
                                             BufferUsageHints usage, StringView friendlyName = "");

    /**
     * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
     * with the specified number of elements
     */
    static SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                             const TypeInfo &elementTypeinfo,
                                             BufferUsageHints usage, std::size_t numElements,
                                             StringView friendlyName = "");

    /**
     * Constructor for the buffer along with explicit header and element types
     */
    SharedBufferVariantPtr(dynasma::FirmPtr<RawSharedBuffer> buffer, const TypeInfo &headerTypeinfo,
                           const TypeInfo &elementTypeinfo);

    /// Default constructor, sets type infos to void
    SharedBufferVariantPtr();

    SharedBufferVariantPtr(const SharedBufferVariantPtr &) = default;
    SharedBufferVariantPtr(SharedBufferVariantPtr &&) = default;
    ~SharedBufferVariantPtr() = default;
    SharedBufferVariantPtr &operator=(const SharedBufferVariantPtr &) = default;
    SharedBufferVariantPtr &operator=(SharedBufferVariantPtr &&) = default;

    // Comparison (inline)

    inline bool operator==(const SharedBufferVariantPtr &o) const
    {
        return mp_buffer == o.mp_buffer && *mp_headerTypeinfo == *o.mp_headerTypeinfo &&
               *mp_elementTypeinfo == *o.mp_elementTypeinfo;
    }

    inline std::weak_ordering operator<=>(const SharedBufferVariantPtr &o) const
    {
        if (auto c = mp_buffer <=> o.mp_buffer; c != std::strong_ordering::equivalent) {
            return c;
        } else if (auto c = *mp_headerTypeinfo <=> *o.mp_headerTypeinfo;
                   c != std::weak_ordering::equivalent) {
            return c;
        } else {
            return *mp_elementTypeinfo <=> *o.mp_elementTypeinfo;
        }
    }

    inline const TypeInfo &getHeaderTypeinfo() const { return *mp_headerTypeinfo; }
    inline const TypeInfo &getElementTypeinfo() const { return *mp_elementTypeinfo; }

    /**
     * @returns the number of FAM elements in the underlying RawSharedBuffer
     */
    std::size_t numElements() const;

    /**
     * @returns The header of the buffer
     */
    template <typename HeaderT> const HeaderT &getHeader() const
    {
        if (TYPE_INFO<HeaderT> != *mp_headerTypeinfo) {
            throw std::bad_cast("Header type mismatch: desired type " +
                                String(TYPE_INFO<HeaderT>.getShortTypeName()) +
                                " from buffer with header " +
                                String(mp_headerTypeinfo->getShortTypeName()));
        }
        return *reinterpret_cast<const HeaderT *>(mp_buffer->data());
    }
    template <typename HeaderT> HeaderT &getHeader()
    {
        if (TYPE_INFO<HeaderT> != *mp_headerTypeinfo) {
            throw std::bad_cast("Header type mismatch: desired type " +
                                String(TYPE_INFO<HeaderT>.getShortTypeName()) +
                                " from buffer with header " +
                                String(mp_headerTypeinfo->getShortTypeName()));
        }
        return *reinterpret_cast<HeaderT *>((*mp_buffer)[{0, sizeof(HeaderT)}].data());
    }

    /**
     * @returns The FAM element at the given index
     */
    template <typename ElementT> const ElementT &getElement(std::size_t index) const
    {
        if (TYPE_INFO<ElementT> != *mp_elementTypeinfo) {
            throw std::bad_cast("Element type mismatch: desired type " +
                                String(TYPE_INFO<ElementT>.getShortTypeName()) +
                                " from buffer with elements " +
                                String(mp_elementTypeinfo->getShortTypeName()));
        }
        return *reinterpret_cast<const ElementT *>(
            mp_buffer->data() + getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
            sizeof(ElementT) * index);
    }
    template <typename ElementT> ElementT &getElement(std::size_t index)
    {
        if (TYPE_INFO<ElementT> != *mp_elementTypeinfo) {
            throw std::bad_cast("Element type mismatch: desired type " +
                                String(TYPE_INFO<ElementT>.getShortTypeName()) +
                                " from buffer with elements " +
                                String(mp_elementTypeinfo->getShortTypeName()));
        }
        return *reinterpret_cast<ElementT *>(
            (*mp_buffer)[{getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
                              sizeof(ElementT) * index,
                          getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
                              sizeof(ElementT) * index + sizeof(ElementT)}]
                .data());
    }

    /**
     * @returns A span of all FAM elements
     */
    template <typename ElementT> std::span<ElementT> getElements()
    {
        if (TYPE_INFO<ElementT> != *mp_elementTypeinfo) {
            throw std::bad_cast("Element type mismatch: desired type " +
                                String(TYPE_INFO<ElementT>.getShortTypeName()) +
                                " from buffer with elements " +
                                String(mp_elementTypeinfo->getShortTypeName()));
        }
        return std::span<ElementT>(
            reinterpret_cast<ElementT *>(
                mp_buffer->data() + getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)),
            numElements());
    }
    template <typename ElementT> std::span<const ElementT> getElements() const
    {
        if (TYPE_INFO<ElementT> != *mp_elementTypeinfo) {
            throw std::bad_cast("Element type mismatch: desired type " +
                                String(TYPE_INFO<ElementT>.getShortTypeName()) +
                                " from buffer with elements " +
                                String(mp_elementTypeinfo->getShortTypeName()));
        }
        return std::span<const ElementT>(
            reinterpret_cast<const ElementT *>(
                dynasma::const_pointer_cast<const RawSharedBuffer>(mp_buffer)->data() +
                getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)),
            numElements());
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() const { return mp_buffer; }

    // Modification

    /**
     * Resizes the underlying RawSharedBuffer to contain the given number of FAM elements
     */
    void resizeElements(std::size_t numElements);

    /**
     * @returns the number of bytes in the underlying RawSharedBuffer
     */
    inline std::size_t byteSize() const { return mp_buffer->size(); }

  protected:
    dynasma::FirmPtr<RawSharedBuffer> mp_buffer;

    const TypeInfo *mp_headerTypeinfo;
    const TypeInfo *mp_elementTypeinfo;
};
} // namespace Vitrae