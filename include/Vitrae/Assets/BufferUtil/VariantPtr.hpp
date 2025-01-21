#pragma once

#include "Vitrae/Assets/BufferUtil/LayoutInfo.hpp"
#include "Vitrae/Assets/SharedBuffer.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <cstddef>

namespace Vitrae
{

/**
 * A SharedBufferVariantPtr is used to access a shared buffer, with a type-checked underlying type.
 * It has a defined header (TYPE_INFO<void> if unused),
 * and a FAM array of elements (TYPE_INFO<void> if unused).
 * @note It can be returned along with a constructed buffer by makeBuffer(...) functions
 */
class SharedBufferVariantPtr
{
  public:
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
    inline const TypeInfo &getElementTypeInfo() const { return *mp_elementTypeinfo; }

    /**
     * @returns the number of FAM elements in the underlying RawSharedBuffer
     */
    std::size_t numElements() const;

    /**
     * @returns The header of the buffer
     */
    template <typename HeaderT> const HeaderT &getHeader() const
    {
        throwIfHeaderMismatch(TYPE_INFO<HeaderT>);
        return *reinterpret_cast<const HeaderT *>(mp_buffer->data());
    }
    template <typename HeaderT> HeaderT &getHeader()
    {
        throwIfHeaderMismatch(TYPE_INFO<HeaderT>);
        return *reinterpret_cast<HeaderT *>((*mp_buffer)[{0, sizeof(HeaderT)}].data());
    }

    /**
     * @returns The FAM element at the given index
     */
    template <typename ElementT> const ElementT &getElement(std::size_t index) const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return *reinterpret_cast<const ElementT *>(
            mp_buffer->data() +
            BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
            sizeof(ElementT) * index);
    }
    template <typename ElementT> ElementT &getElement(std::size_t index)
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return *reinterpret_cast<ElementT *>(
            (*mp_buffer)
                [{BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
                      sizeof(ElementT) * index,
                  BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo) +
                      sizeof(ElementT) * index + sizeof(ElementT)}]
                    .data());
    }

    /**
     * @returns A span of all FAM elements
     */
    template <typename ElementT> std::span<ElementT> getElements()
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return std::span<ElementT>(
            reinterpret_cast<ElementT *>(
                mp_buffer->data() +
                BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)),
            numElements());
    }
    template <typename ElementT> std::span<const ElementT> getElements() const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return std::span<const ElementT>(
            reinterpret_cast<const ElementT *>(
                dynasma::const_pointer_cast<const RawSharedBuffer>(mp_buffer)->data() +
                BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)),
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

    void throwIfHeaderMismatch(const TypeInfo &headerTypeinfo) const;
    void throwIfElementMismatch(const TypeInfo &elementTypeinfo) const;
};

// Buffer constructions

/**
 * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
 * with enough size for the HeaderT
 */
SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                  const TypeInfo &elementTypeinfo, BufferUsageHints usage,
                                  StringView friendlyName = "");

/**
 * Constructs a SharedBufferPtr with a new RawSharedBuffer allocated from the Keeper in the root
 * with the specified number of elements
 */
SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                  const TypeInfo &elementTypeinfo, BufferUsageHints usage,
                                  std::size_t numElements, StringView friendlyName = "");

} // namespace Vitrae