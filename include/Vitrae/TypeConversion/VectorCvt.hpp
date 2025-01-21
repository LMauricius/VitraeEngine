#pragma once

#include "assimp/types.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <cstdlib>

namespace Vitrae
{

/**
 * A class containing static info about vector types.
 * For unknown and non-vector types, the component type is specified as void, and count as 0
 */
template <class VecT> struct VectorTypeInfo
{
    using value_type = void;
    static constexpr std::size_t NumComponents = 0;
};

// =========================== For glm types =======================================================

/**
 * VectorTypeInfo for glm vector types
 */
template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct VectorTypeInfo<glm::mat<C, R, T, Q>>
{
    using value_type = T;
    static constexpr std::size_t NumComponents = C * R;
};

/**
 * VectorTypeInfo for glm matrix types
 */
template <glm::length_t L, typename T, glm::qualifier Q> struct VectorTypeInfo<glm::vec<L, T, Q>>
{
    using value_type = T;
    static constexpr std::size_t NumComponents = L;
};

// =========================== For assimp types ====================================================

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
} // namespace Vitrae