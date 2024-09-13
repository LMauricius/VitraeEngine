#pragma once

#include "assimp/types.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <cstdlib>

namespace Vitrae
{

template <class VecT> struct VectorTypeInfo;

template <> struct VectorTypeInfo<float>
{
    using value_type = float;
    static constexpr std::size_t NumComponents = 1;
};

template <> struct VectorTypeInfo<aiVector2D>
{
    using value_type = ai_real;
    static constexpr std::size_t NumComponents = 2;
};
template <> struct VectorTypeInfo<aiVector3D>
{
    using value_type = ai_real;
    static constexpr std::size_t NumComponents = 3;
};
template <> struct VectorTypeInfo<aiColor3D>
{
    using value_type = ai_real;
    static constexpr std::size_t NumComponents = 3;
};
template <> struct VectorTypeInfo<aiColor4D>
{
    using value_type = ai_real;
    static constexpr std::size_t NumComponents = 4;
};

template <> struct VectorTypeInfo<glm::vec1>
{
    using value_type = float;
    static constexpr std::size_t NumComponents = 1;
};
template <> struct VectorTypeInfo<glm::vec2>
{
    using value_type = float;
    static constexpr std::size_t NumComponents = 2;
};
template <> struct VectorTypeInfo<glm::vec3>
{
    using value_type = float;
    static constexpr std::size_t NumComponents = 3;
};
template <> struct VectorTypeInfo<glm::vec4>
{
    using value_type = float;
    static constexpr std::size_t NumComponents = 4;
};
} // namespace Vitrae