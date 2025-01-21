#include "Vitrae/Assets/SharedBufferVariantPtr.hpp"

namespace Vitrae
{

std::ptrdiff_t SharedBufferVariantPtr::getFirstElementOffset(const TypeInfo &headerTypeinfo,
                                                             const TypeInfo &elementTypeinfo)
{
    if (headerTypeinfo != TYPE_INFO<void>)
        return ((headerTypeinfo.size - 1) / elementTypeinfo.alignment + 1) *
               elementTypeinfo.alignment;
    else
        return 0;
}

std::size_t SharedBufferVariantPtr::calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                                          const TypeInfo &elementTypeinfo,
                                                          std::size_t numElements)
{
    if (headerTypeinfo != TYPE_INFO<void>)
        return getFirstElementOffset(headerTypeinfo, elementTypeinfo) +
               elementTypeinfo.size * numElements;
    else
        return elementTypeinfo.size * numElements;
}

std::size_t SharedBufferVariantPtr::calcMinimumBufferSize(const TypeInfo &headerTypeinfo,
                                                          const TypeInfo &elementTypeinfo)
{
    if (elementTypeinfo != TYPE_INFO<void>)
        return getFirstElementOffset(headerTypeinfo, elementTypeinfo);
    else
        return headerTypeinfo.size;
}

SharedBufferVariantPtr SharedBufferVariantPtr::makeBuffer(ComponentRoot &root,
                                                          const TypeInfo &headerTypeinfo,
                                                          const TypeInfo &elementTypeinfo,
                                                          BufferUsageHints usage,
                                                          StringView friendlyName)

{
    return SharedBufferVariantPtr(
        root.getComponent<RawSharedBufferKeeper>().new_asset(RawSharedBufferKeeperSeed{
            .kernel =
                RawSharedBuffer::SetupParams{
                    .renderer = root.getComponent<Renderer>(),
                    .root = root,
                    .usage = usage,
                    .size = calcMinimumBufferSize(headerTypeinfo, elementTypeinfo),
                    .friendlyName = String(friendlyName),
                }}),
        headerTypeinfo, elementTypeinfo);
}

SharedBufferVariantPtr SharedBufferVariantPtr::makeBuffer(
    ComponentRoot &root, const TypeInfo &headerTypeinfo, const TypeInfo &elementTypeinfo,
    BufferUsageHints usage, std::size_t numElements, StringView friendlyName)
{
    if (elementTypeinfo == TYPE_INFO<void>)
        throw std::runtime_error(
            "void element type not supported by SharedBufferVariantPtr with elements");

    return SharedBufferVariantPtr(
        root.getComponent<RawSharedBufferKeeper>().new_asset(RawSharedBufferKeeperSeed{
            .kernel =
                RawSharedBuffer::SetupParams{
                    .renderer = root.getComponent<Renderer>(),
                    .root = root,
                    .usage = usage,
                    .size = calcMinimumBufferSize(headerTypeinfo, elementTypeinfo, numElements),
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
    mp_buffer->resize(calcMinimumBufferSize(*mp_headerTypeinfo, *mp_elementTypeinfo, numElements));
}

std::size_t SharedBufferVariantPtr::numElements() const
{
    if (*mp_elementTypeinfo == TYPE_INFO<void>) {
        throw std::runtime_error(
            "void element type resizing not supported by SharedBufferVariantPtr");
    }
    return (mp_buffer->size() - getFirstElementOffset(*mp_headerTypeinfo, *mp_elementTypeinfo)) /
           mp_elementTypeinfo->size;
}

} // namespace Vitrae