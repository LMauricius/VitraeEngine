#pragma once

#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Dynamic/TypeMeta.hpp"

#include "Vitrae/Dynamic/TypeMeta.hpp"
#include "glm/glm.hpp"
#include <concepts>
#include <cstdint>

namespace Vitrae
{

class TypeInfo;

/**
 * Tensors are usually considered a more general version of vectors.
 * Here, a tensor type refers to a structure that can be represented as a fixed-length array of
 * vectors or scalars (called core vector type).
 *
 * Besides the requirement that tensors must consist of one vector type as its building block,
 * these objects can have any structure or member layout.
 * (as long as those members are tensors with the same core vector type)
 *
 * @note Except for built-in vectors and matrices, TensorMeta's usefulness is usually limited
 * without StructMeta also defined for this type.
 *
 * @see TensorBuffer
 */
struct TensorMeta
{
    PixelType CORE_VECTOR_KIND;
    std::size_t NUM_CORE_VECTORS;
};

/**
 * Any type whose TYPE_META is a TensorMeta
 */
template <class T>
concept Tensor = std::derived_from<decltype(TYPE_META<T>), TensorMeta>;

} // namespace Vitrae