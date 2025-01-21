#include "Vitrae/Assets/SharedBufferVariantPtr.hpp"

namespace Vitrae
{

SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                  const TypeInfo &elementTypeinfo, BufferUsageHints usage,
                                  StringView friendlyName)

{
    return SharedBufferVariantPtr(
        root.getComponent<RawSharedBufferKeeper>().new_asset(RawSharedBufferKeeperSeed{
            .kernel =
                RawSharedBuffer::SetupParams{
                    .renderer = root.getComponent<Renderer>(),
                    .root = root,
                    .usage = usage,
                    .size =
                        BufferLayoutInfo::calcMinimumBufferSize(headerTypeinfo, elementTypeinfo),
                    .friendlyName = String(friendlyName),
                }}),
        headerTypeinfo, elementTypeinfo);
}

SharedBufferVariantPtr makeBuffer(ComponentRoot &root, const TypeInfo &headerTypeinfo,
                                  const TypeInfo &elementTypeinfo, BufferUsageHints usage,
                                  std::size_t numElements, StringView friendlyName)
{
    if (elementTypeinfo == TYPE_INFO<void>)
        throw std::runtime_error(
            "void element type not supported by SharedBufferVariantPtr with elements");

    return SharedBufferVariantPtr(
        root.getComponent<RawSharedBufferKeeper>().new_asset(
            RawSharedBufferKeeperSeed{.kernel =
                                          RawSharedBuffer::SetupParams{
                                              .renderer = root.getComponent<Renderer>(),
                                              .root = root,
                                              .usage = usage,
                                              .size = BufferLayoutInfo::calcMinimumBufferSize(
                                                  headerTypeinfo, elementTypeinfo, numElements),
                                              .friendlyName = String(friendlyName),
                                          }}),
        headerTypeinfo, elementTypeinfo);
}

SharedBufferVariantPtr::SharedBufferVariantPtr(dynasma::FirmPtr<RawSharedBuffer> buffer,
                                               const TypeInfo &headerTypeinfo,
                                               const TypeInfo &elementTypeinfo)
    : mp_buffer(buffer), mp_headerTypeinfo(&headerTypeinfo), mp_elementTypeinfo(&elementTypeinfo)
{}

SharedBufferVariantPtr::SharedBufferVariantPtr()
    : mp_headerTypeinfo(&TYPE_INFO<void>), mp_elementTypeinfo(&TYPE_INFO<void>)
{}

void SharedBufferVariantPtr::resizeElements(std::size_t numElements)
{
    if (*mp_elementTypeinfo == TYPE_INFO<void>) {
        throw std::runtime_error(
            "void element type resizing not supported by SharedBufferVariantPtr");
    }
    mp_buffer->resize(BufferLayoutInfo::calcMinimumBufferSize(*mp_headerTypeinfo,
                                                              *mp_elementTypeinfo, numElements));
}

void SharedBufferVariantPtr::throwIfHeaderMismatch(const TypeInfo &headerTypeinfo) const
{
    if (headerTypeinfo != *mp_headerTypeinfo) {
        throw std::runtime_error(
            "Header type mismatch: desired type " + String(headerTypeinfo.getShortTypeName()) +
            " from buffer with header " + String(mp_headerTypeinfo->getShortTypeName()));
    }
}

void SharedBufferVariantPtr::throwIfElementMismatch(const TypeInfo &elementTypeinfo) const
{
    if (elementTypeinfo != *mp_elementTypeinfo) {
        throw std::runtime_error(
            "Element type mismatch: desired type " + String(elementTypeinfo.getShortTypeName()) +
            " from buffer with elements " + String(mp_elementTypeinfo->getShortTypeName()));
    }
}

std::size_t SharedBufferVariantPtr::numElements() const
{
    if (*mp_elementTypeinfo == TYPE_INFO<void>) {
        throw std::runtime_error(
            "void element type resizing not supported by SharedBufferVariantPtr");
    }
    return (mp_buffer->size() -
            BufferLayoutInfo::getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)) /
           mp_elementTypeinfo->size;
}

} // namespace Vitrae