#pragma once

#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Setup/Swizzle.hpp"
#include "Vitrae/Setup/TextureFiltering.hpp"

#include "glm/glm.hpp"

#include <filesystem>

namespace Vitrae
{
class ComponentRoot;

// Classes that are common to image types
namespace ImageCommon
{
struct FileLoadParams
{
    ComponentRoot &root;
    std::filesystem::path filepath;
    TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
};
template <PixelType PIXEL_TYPE, typename SizeT> struct EmptyParams
{
    ComponentRoot &root;
    SizeT size;
    PixelFormat<PIXEL_TYPE> storageFormat;
    SwizzleSpec<PIXEL_TYPE> swizzle = CommonSwizzleSpecs<PIXEL_TYPE>::NATURAL;
    TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
    String friendlyName = "";
};

template <PixelType PIXEL_TYPE> struct PureColorParams
{
    ComponentRoot &root;
    BufferValueType<PIXEL_TYPE> color;
    PixelFormat<PIXEL_TYPE> storageFormat;
};

} // namespace ImageCommon

} // namespace Vitrae