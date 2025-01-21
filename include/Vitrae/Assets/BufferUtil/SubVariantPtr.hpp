#pragma once

#include "Vitrae/Assets/BufferUtil/LayoutInfo.hpp"
#include "Vitrae/Assets/SharedBuffer.hpp"
#include "Vitrae/Containers/StridedSpan.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <cstddef>

namespace Vitrae
{
class SharedBufferVariantPtr;
template <class THeaderT, class TElementT> class SharedBufferPtr;
template <class TElementT> class SharedSubBufferPtr;

/**
 * A SharedSubBufferVariantPtr is used to access a shared buffer, with a type-checked underlying
 * type. It has a defined header (TYPE_INFO<void> if unused), and a FAM array of elements
 * (TYPE_INFO<void> if unused).
 * @note It can be returned along with a constructed buffer by makeBuffer(...) functions
 */
class SharedSubBufferVariantPtr
{
  public:
    /**
     * Constructor for the buffer along with explicit element type
     */
    SharedSubBufferVariantPtr(dynasma::FirmPtr<RawSharedBuffer> buffer,
                              const TypeInfo &elementTypeinfo, std::size_t bytesOffset,
                              std::size_t bytesStride, std::size_t numElements);
    /**
     * Constructor for the buffer along with explicit element type
     */
    SharedSubBufferVariantPtr(const SharedBufferVariantPtr &p_buffer);

    /**
     * Constructor from fixed type buffer ptr
     */
    template <class HeaderT, class ElementT>
    SharedSubBufferVariantPtr(SharedBufferPtr<HeaderT, ElementT> p_buffer)
        : SharedSubBufferVariantPtr(p_buffer.getRawBuffer(), TYPE_INFO<ElementT>,
                                    BufferLayoutInfo::getFirstElementOffset<HeaderT, ElementT>(),
                                    sizeof(ElementT), p_buffer.numElements())
    {}

    /**
     * Constructor from fixed type buffer ptr
     */
    template <class ElementT>
    SharedSubBufferVariantPtr(SharedSubBufferPtr<ElementT> p_buffer)
        : SharedSubBufferVariantPtr(p_buffer.getRawBuffer(), TYPE_INFO<ElementT>,
                                    p_buffer.getBytesOffset(), p_buffer.getBytesStride(),
                                    p_buffer.numElements())
    {}

    /// Default constructor, sets type infos to void
    SharedSubBufferVariantPtr();

    SharedSubBufferVariantPtr(const SharedSubBufferVariantPtr &) = default;
    SharedSubBufferVariantPtr(SharedSubBufferVariantPtr &&) = default;
    ~SharedSubBufferVariantPtr() = default;
    SharedSubBufferVariantPtr &operator=(const SharedSubBufferVariantPtr &) = default;
    SharedSubBufferVariantPtr &operator=(SharedSubBufferVariantPtr &&) = default;

    // Comparison (inline)

    inline bool operator==(const SharedSubBufferVariantPtr &o) const
    {
        return mp_buffer == o.mp_buffer && *mp_elementTypeinfo == *o.mp_elementTypeinfo &&
               m_bytesOffset == o.m_bytesOffset;
    }

    inline std::weak_ordering operator<=>(const SharedSubBufferVariantPtr &o) const
    {
        if (auto c = mp_buffer <=> o.mp_buffer; c != std::strong_ordering::equivalent) {
            return c;
        } else if (auto c = *mp_elementTypeinfo <=> *o.mp_elementTypeinfo;
                   c != std::weak_ordering::equivalent) {
            return c;
        } else {
            return m_bytesOffset <=> o.m_bytesOffset;
        }
    }

    inline const TypeInfo &getElementTypeinfo() const { return *mp_elementTypeinfo; }

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
    template <typename ElementT> const ElementT &getElement(std::size_t index) const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return *reinterpret_cast<const ElementT *>(mp_buffer->data() + m_bytesOffset +
                                                   m_bytesStride * index);
    }
    template <typename ElementT> ElementT &getMutableElement(std::size_t index) const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return *reinterpret_cast<ElementT *>(
            (*mp_buffer)[{m_bytesOffset + m_bytesStride * index,
                          m_bytesOffset + m_bytesStride * index + sizeof(ElementT)}]
                .data());
    }

    /**
     * @returns A StridedSpan of all elements
     */
    template <typename ElementT> StridedSpan<const ElementT> getElements() const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return StridedSpan<const ElementT>(
            reinterpret_cast<const ElementT *>(mp_buffer->data() + m_bytesOffset), m_numElements,
            m_bytesStride);
    }
    template <typename ElementT> StridedSpan<ElementT> getMutableElements() const
    {
        throwIfElementMismatch(TYPE_INFO<ElementT>);
        return StridedSpan<ElementT>(
            reinterpret_cast<ElementT *>(
                (*mp_buffer)[{m_bytesOffset, m_bytesOffset + m_bytesStride * m_numElements}]
                    .data()),
            m_numElements, m_bytesStride);
    }

    /**
     * @returns the underlying RawSharedBuffer, type agnostic
     */
    dynasma::FirmPtr<RawSharedBuffer> getRawBuffer() const { return mp_buffer; }

  protected:
    dynasma::FirmPtr<RawSharedBuffer> mp_buffer;

    const TypeInfo *mp_elementTypeinfo;

    std::size_t m_bytesOffset;
    std::size_t m_bytesStride;
    std::size_t m_numElements;

    void throwIfElementMismatch(const TypeInfo &elementTypeinfo) const;
};

// Buffer constructions

/**
 * Constructs a new RawSharedBuffer allocated from the
 * Keeper in the root with the specified number of elements
 * @param root
 * @param elementTypeinfos The types of the sub-elements that will be interleaved
 * @param outPtrs SharedSubBufferVariantPtr objects, that will be assigned to interleaved subbuffers
 * @param usage Usage hints that will be passed to the RawSharedBuffer constructor
 * @param numElements The number of elements in the interleaved buffer, per each subbuffer
 * @warning The number of outPtrs must be equal to the number of elementTypeinfos!
 */
void makeBufferInterleaved(ComponentRoot &root, std::span<const TypeInfo *> elementTypeInfoPtrs,
                           std::span<SharedSubBufferVariantPtr> outPtrs, BufferUsageHints usage,
                           std::size_t numElements, StringView friendlyName = "");

} // namespace Vitrae