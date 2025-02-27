#pragma once

#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Dynamic/TypeMeta.hpp"
#include "Vitrae/Dynamic/TypeMeta/Vector.hpp"

#include "assimp/types.h"
#include "glm/glm.hpp"

namespace Vitrae
{

// =========================== For glm types =======================================================

/**
 * VectorTypeInfo for glm matrix types
 */
template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<glm::mat<C, R, T, Q>> = {{
    .componentTypeInfo = TYPE_INFO<T>,
    .numComponents = C * R,
}};

/**
 * VectorTypeInfo for glm vector types
 */
template <glm::length_t L, typename T, glm::qualifier Q>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<glm::vec<L, T, Q>> = {{
    .componentTypeInfo = TYPE_INFO<T>,
    .numComponents = L,
}};

// =========================== For assimp types ====================================================

template <>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<aiVector2D> = {{
    .componentTypeInfo = TYPE_INFO<ai_real>,
    .numComponents = 2,
}};
template <>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<aiVector3D> = {{
    .componentTypeInfo = TYPE_INFO<ai_real>,
    .numComponents = 3,
}};
template <>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<aiColor3D> = {{
    .componentTypeInfo = TYPE_INFO<ai_real>,
    .numComponents = 3,
}};
template <>
inline const CompoundTypeMeta<VectorMeta> TYPE_META<aiColor4D> = {{
    .componentTypeInfo = TYPE_INFO<ai_real>,
    .numComponents = 4,
}};

} // namespace Vitrae