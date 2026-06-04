#pragma once

#include "glm/glm.hpp"

namespace Vitrae
{

/**
 * @defgroup VectorPromotion Vector component access with zero-padding
 *
 * These helpers safely read a specific component of a GLM vector of any length,
 * returning `0` when the vector is too short to have that component. They are
 * designed for use in generic code that must handle vectors of mixed
 * dimensionality uniformly (e.g. promoting a vec2 to a vec4).
 */

/// Returns the X component of @p v (always available for vectors).
template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorXor0(const glm::vec<L, T, Q> &v)
{
    return v.x;
}

/// Returns the Y component of @p v, or `0` if L <= 1.
template <glm::length_t L, typename T, glm::qualifier Q>
T getVectorYor0(const glm::vec<L, T, Q> &v);

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorYor0(const glm::vec<L, T, Q> &v)
    requires(L > 1)
{
    return v.y;
}

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorYor0(const glm::vec<L, T, Q> &v)
    requires(L <= 1)
{
    return T{0};
}

/// Returns the Z component of @p v, or `0` if L <= 2.
template <glm::length_t L, typename T, glm::qualifier Q>
T getVectorZor0(const glm::vec<L, T, Q> &v);

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorZor0(const glm::vec<L, T, Q> &v)
    requires(L > 2)
{
    return v.z;
}

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorZor0(const glm::vec<L, T, Q> &v)
    requires(L <= 2)
{
    return T{0};
}

/// Returns the W component of @p v, or `0` if L <= 3.
template <glm::length_t L, typename T, glm::qualifier Q>
T getVectorWor0(const glm::vec<L, T, Q> &v);

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorWor0(const glm::vec<L, T, Q> &v)
    requires(L > 3)
{
    return v.w;
}

template <glm::length_t L, typename T, glm::qualifier Q>
inline T getVectorWor0(const glm::vec<L, T, Q> &v)
    requires(L <= 3)
{
    return T{0};
}

/** @} */ // end of VectorPromotion group

/**
 * @brief Promotes any GLM vector to a `vec4` by zero-padding missing components.
 *
 * @code
 * glm::vec2 v2{1.0f, 2.0f};
 * auto v4 = promotePadVector(v2); // => vec4{1.0, 2.0, 0.0, 0.0}
 * @endcode
 *
 * @tparam L  Source vector length (1–4).
 * @tparam T  Component scalar type.
 * @tparam Q  GLM qualifier (default, mediump, etc.).
 * @param  v  Source vector.
 * @return    A `vec4<T, Q>` with the source components followed by zeros.
 */
template <glm::length_t L, typename T, glm::qualifier Q>
inline glm::vec<4, T, Q> promotePadVector(const glm::vec<L, T, Q> &v)
{
    return {getVectorXor0(v), getVectorYor0(v), getVectorZor0(v), getVectorWor0(v)};
}

} // namespace Vitrae