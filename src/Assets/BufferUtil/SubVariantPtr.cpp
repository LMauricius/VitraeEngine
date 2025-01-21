#include "Vitrae/Assets/BufferUtil/SubVariantPtr.hpp"
#include "Vitrae/Assets/BufferUtil/VariantPtr.hpp"

namespace Vitrae
{

SharedSubBufferVariantPtr::SharedSubBufferVariantPtr(dynasma::FirmPtr<RawSharedBuffer> buffer,
                                                     const TypeInfo &elementTypeinfo,
                                                     std::size_t bytesOffset,
                                                     std::size_t bytesStride,
                                                     std::size_t numElements)
    : mp_buffer(buffer), mp_elementTypeinfo(&elementTypeinfo), m_bytesOffset(bytesOffset),
      m_numElements(numElements), m_bytesStride(bytesStride)
{}

SharedSubBufferVariantPtr::SharedSubBufferVariantPtr(const SharedBufferVariantPtr &p_buffer)
    : mp_buffer(p_buffer.getRawBuffer()), mp_elementTypeinfo(&p_buffer.getElementTypeInfo()),
      m_bytesOffset(0), m_numElements(p_buffer.numElements()),
      m_bytesStride(p_buffer.getElementTypeInfo().size)
{}

SharedSubBufferVariantPtr::SharedSubBufferVariantPtr() : mp_elementTypeinfo(&TYPE_INFO<void>) {}

void SharedSubBufferVariantPtr::throwIfElementMismatch(const TypeInfo &elementTypeinfo) const
{
    if (elementTypeinfo != *mp_elementTypeinfo) {
        throw std::runtime_error(
            "Element type mismatch: desired type " + String(elementTypeinfo.getShortTypeName()) +
            " from buffer with elements " + String(mp_elementTypeinfo->getShortTypeName()));
    }
}

void makeBufferInterleaved(ComponentRoot &root, std::span<const TypeInfo *> elementTypeInfoPtrs,
                           std::span<SharedSubBufferVariantPtr> outPtrs, BufferUsageHints usage,
                           std::size_t numElements, StringView friendlyName)
{
    if (outPtrs.size() != elementTypeInfoPtrs.size())
        throw std::runtime_error(
            "Number of outPtrs must be equal to the number of elementTypeinfos!");

    std::vector<std::size_t> offsets;
    offsets.reserve(elementTypeInfoPtrs.size());

    // Calculate the total size per element
    std::size_t aggregateSize = 0;
    std::size_t maxAlignment = 0;
    for (auto p_typeInfo : elementTypeInfoPtrs) {
        aggregateSize = (aggregateSize + p_typeInfo->alignment - 1) / p_typeInfo->alignment *
                        p_typeInfo->alignment;

        offsets.push_back(aggregateSize);

        aggregateSize += p_typeInfo->size;

        if (p_typeInfo->alignment > maxAlignment)
            maxAlignment = p_typeInfo->alignment;
    }
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
    for (int i = 0; i < elementTypeInfoPtrs.size(); i++) {
        outPtrs[i] = SharedSubBufferVariantPtr(p_buffer, *elementTypeInfoPtrs[i], offsets[i],
                                               aggregateSize, numElements);
    }
}

} // namespace Vitrae