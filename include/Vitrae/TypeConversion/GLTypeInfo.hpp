#pragma once

#include "glad/glad.h"
#include "glm/glm.hpp"
#include <cstdlib>

namespace Vitrae
{

template <class T> struct GLTypeInfo;

template <> struct GLTypeInfo<std::int8_t>
{
    static constexpr GLenum GlTypeId = GL_BYTE;
    static constexpr StringView GLSLTypeName = "int";
};
template <> struct GLTypeInfo<std::uint8_t>
{
    static constexpr GLenum GlTypeId = GL_UNSIGNED_BYTE;
    static constexpr StringView GLSLTypeName = "uint";
};
template <> struct GLTypeInfo<std::int16_t>
{
    static constexpr GLenum GlTypeId = GL_SHORT;
    static constexpr StringView GLSLTypeName = "int";
};
template <> struct GLTypeInfo<std::uint16_t>
{
    static constexpr GLenum GlTypeId = GL_UNSIGNED_SHORT;
    static constexpr StringView GLSLTypeName = "uint";
};
template <> struct GLTypeInfo<std::int32_t>
{
    static constexpr GLenum GlTypeId = GL_INT;
    static constexpr StringView GLSLTypeName = "int";
};
template <> struct GLTypeInfo<std::uint32_t>
{
    static constexpr GLenum GlTypeId = GL_UNSIGNED_INT;
    static constexpr StringView GLSLTypeName = "uint";
};

template <> struct GLTypeInfo<float>
{
    static constexpr GLenum GlTypeId = GL_FLOAT;
    static constexpr StringView GLSLTypeName = "float";
};
template <> struct GLTypeInfo<double>
{
    static constexpr GLenum GlTypeId = GL_DOUBLE;
    static constexpr StringView GLSLTypeName = "double";
};

template <> struct GLTypeInfo<glm::vec2>
{
    static constexpr StringView GLSLTypeName = "vec2";
};
template <> struct GLTypeInfo<glm::vec3>
{
    static constexpr StringView GLSLTypeName = "vec3";
};
template <> struct GLTypeInfo<glm::vec4>
{
    static constexpr StringView GLSLTypeName = "vec4";
};

template <> struct GLTypeInfo<glm::mat2>
{
    static constexpr StringView GLSLTypeName = "mat2";
};
template <> struct GLTypeInfo<glm::mat3>
{
    static constexpr StringView GLSLTypeName = "mat3";
};
template <> struct GLTypeInfo<glm::mat4>
{
    static constexpr StringView GLSLTypeName = "mat4";
};
} // namespace Vitrae