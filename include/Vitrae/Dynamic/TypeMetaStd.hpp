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
struct TypeMeta<glm::mat<C, R, T, Q>> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return C * R; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<T>; };
};

/**
 * VectorTypeInfo for glm vector types
 */
template <glm::length_t L, typename T, glm::qualifier Q>
struct TypeMeta<glm::vec<L, T, Q>> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return L; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<T>; };
};

// =========================== For assimp types ====================================================

template <> struct TypeMeta<aiVector2D> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return 2; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<ai_real>; };
};
template <> struct TypeMeta<aiVector3D> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return 3; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<ai_real>; };
};
template <> struct TypeMeta<aiColor3D> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return 3; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<ai_real>; };
};
template <> struct TypeMeta<aiColor4D> : public PolymorphicTypeMeta, public VectorMeta
{
    constexpr std::size_t getNumComponents() const override { return 4; }
    constexpr const TypeInfo &getComponentTypeInfo() const override { return TYPE_INFO<ai_real>; };
};

} // namespace Vitrae