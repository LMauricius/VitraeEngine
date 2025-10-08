#pragma once

#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/Typedefs.hpp"

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
};
template <BufferType BUFFER_TYPE, typename SizeT> struct EmptyParams
{
    ComponentRoot &root;
    SizeT size;
    BufferFormat<BUFFER_TYPE> storageFormat;
    String friendlyName = "";
};
template <BufferType BUFFER_TYPE> struct PureColorParams
{
    ComponentRoot &root;
    glm::vec4 color;
    BufferFormat<BUFFER_TYPE> storageFormat;
};

} // namespace ImageCommon

} // namespace Vitrae