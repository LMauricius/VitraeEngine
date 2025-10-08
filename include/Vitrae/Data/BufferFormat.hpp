#pragma once

#include <variant>

namespace Vitrae
{

/**
 * Native value type specification of image (and other) buffers.
 * This specifies what the values of the buffer represent.
 * It is generally taken very strictly, as they dictate not only the format of the buffer,
 * but also the types of value retrieved after reading the buffer,
 * the operations that can be performed on the buffer,
 * and where it can be used.
 *
 * Constants are named like this: <Component type>_<Vector size>
 * The element type is the type of one item of the stored value.
 * The vector size is the number of items of the stored value.
 *
 * Component types:
 *  - REAL - A real value, with a number of decimal places
 *  - WHOLE - A signed integer (whole) value
 *  - COUNT - An unsigned integer (counting) value
 *
 * Vector sizes:
 *  - SCALAR - the value has a single component, so not a vector at all
 *  - VEC2 - 2 components stored in the value
 *  - VEC3 - 3 components stored in the value
 *  - VEC4 - 4 components stored in the value
 *
 * For convenience, the following aliases are defined:
 *  - COLOR - A 3 component real number vector holding an RGB color
 *  - TRANSPARENT - A 4 component real number vector holding an RGBA color
 *
 * There are some special types that generally require special handling:
 *  - DEPTH -  Distance of rendered geometry (a special real number value)
 *  - STENCIL - A stencil value (a special integer value used for many effects)
 *  - DEPTH_AND_STENCIL - A pair of depth and stencil values in one buffer
 */
enum class BufferType {
    REAL_SCALAR,
    REAL_VEC2,
    REAL_VEC3,
    REAL_VEC4,
    WHOLE_SCALAR,
    WHOLE_VEC2,
    WHOLE_VEC3,
    WHOLE_VEC4,
    COUNT_SCALAR,
    COUNT_VEC2,
    COUNT_VEC3,
    COUNT_VEC4,
    DEPTH,
    STENCIL,
    DEPTH_AND_STENCIL,
    COLOR = REAL_VEC3,
    TRANSPARENT = REAL_VEC4,
};

// used for specializing enums and types tied to a specific BufferType
template <BufferType BUFFER_TYPE> struct BufferTypeSpecialization;

/**
 * Storage format specification of image (and other) buffers.
 * This value specified how the bits in memory represent data in our buffers
 * Each BufferType has its own format constants.
 *
 * The constants are named in the format of <Base type><Bit count>, with some exceptions.
 * The base type is the format of each element.
 * The bit count is the number of bits per element. Some storage types use a different bit coult per
 * each component of the vector, in which case the bit counts are given in order.
 *
 * Base types:
 *  - FLOAT - Floating point value, with the range and precision depending on the bit count
 *  - UFLOAT - Unsigned floating point value (stored without the sign bit)
 *  - INT - Signed integer value, with the range depending on the bit count
 *  - UNSIGNED - Unsigned integer value, with the range depending on the bit count
 *  - NORM - Unsigned normalized float value in the range [0.0, 1.0], stored as an unsigned integer
 *  - SNORM - Signed normalized float value in the range [-1.0, 1.0], stored as a signed integer
 *
 * Special types:
 *  - GENERIC / GENERIC_FULL - Large ranged value, with the format left to the implementation
 *  - GENERIC_NORM - Value in the range [0.0, 1.0], with the format left to the implementation
 *  - BOOL - Either 1 or 0, i.e. unsigned integer with 1 bit
 *  - LINEAR_COLOR - Value in the range [0.0, 1.0], fitting for the linear color space
 *  - SRGB_COLOR - Value in the range [0.0, 1.0], fitting for the sRGB color space
 *  - SRGB_ALPHA_COLOR - sRGB color, with the alpha in linear color space
 *  - <*>_EXP<Bits> - Vector of floating point values with a shared exponent of specified bits
 *
 * @note Alternatively you can use constants from enum classes named as BufferFormat_TYPE,
 * where TYPE is the BufferType that would be passed as a template parameter
 */
template <BufferType BT> using BufferFormat = typename BufferTypeSpecialization<BT>::BufferFormat;

/**
 * A variant of all BufferFormat formats that can store data for the specified BufferType.
 * Generally, you can use any format with the same component type but a different vector size.
 * @note If not using the native format (i.e. BufferFormat<BT> with BT being the BufferType),
 * you should use the texture component swizzling to specify how native type components are
 * calculated from the stored data.
 */
template <BufferType BT>
using CompatibleBufferFormat = typename BufferTypeSpecialization<BT>::CompatibleBufferFormat;

enum class BufferFormat_REAL_SCALAR {
    GENERIC_FULL,
    GENERIC_NORM,
    LINEAR_COLOR,
    FLOAT16,
    FLOAT32,
    NORM8,
    NORM16,
    SNORM8,
    SNORM16,
};

enum class BufferFormat_REAL_VEC2 {
    GENERIC_FULL,
    GENERIC_NORM,
    LINEAR_COLOR,
    FLOAT16,
    FLOAT32,
    NORM8,
    NORM16,
    SNORM8,
    SNORM16,
};

enum class BufferFormat_REAL_VEC3 {
    GENERIC_FULL,
    GENERIC_NORM,
    LINEAR_COLOR,
    SRGB_COLOR,
    FLOAT16,
    FLOAT32,
    NORM4,
    NORM5,
    NORM8,
    NORM10,
    NORM12,
    NORM16,
    SNORM8,
    SNORM16,

    NORM_3_3_2,
    UFLOAT_11_11_10,
    UFLOAT9_EXP5,
    NORM_5_6_5,
};

enum class BufferFormat_REAL_VEC4 {
    GENERIC_FULL,
    GENERIC_NORM,
    LINEAR_COLOR,
    SRGB_ALPHA_COLOR,
    FLOAT16,
    FLOAT32,
    NORM2,
    NORM4,
    NORM8,
    NORM12,
    NORM16,
    SNORM8,
    SNORM16,

    NORM_5_5_5_1,
    NORM_10_10_10_2,
};

enum class BufferFormat_WHOLE_SCALAR {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferFormat_WHOLE_VEC2 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferFormat_WHOLE_VEC3 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferFormat_WHOLE_VEC4 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferFormat_COUNT_SCALAR {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferFormat_COUNT_VEC2 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferFormat_COUNT_VEC3 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferFormat_COUNT_VEC4 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,

    UNSIGNED_10_10_10_2,
};

enum class BufferFormat_DEPTH {
    GENERIC,
    NORM16,
    NORM24,
    NORM32,
    FLOAT32,
};

enum class BufferFormat_STENCIL {
    GENERIC,
    BOOL,
    UNSIGNED4,
    UNSIGNED8,
    UNSIGNED16,
};

enum class BufferFormat_DEPTH_AND_STENCIL {
    GENERIC,
    NORM24_UNSIGNED8,
    FLOAT32_UNSIGNED8,
};

using AnyBufferFormat =
    std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2, BufferFormat_REAL_VEC3,
                 BufferFormat_REAL_VEC4, BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                 BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4, BufferFormat_COUNT_SCALAR,
                 BufferFormat_COUNT_VEC2, BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4,
                 BufferFormat_DEPTH, BufferFormat_STENCIL, BufferFormat_DEPTH_AND_STENCIL>;

// ---- Specialization wrappers per BufferType ----

template <> struct BufferTypeSpecialization<BufferType::REAL_SCALAR>
{
    using BufferFormat = BufferFormat_REAL_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC2>
{
    using BufferFormat = BufferFormat_REAL_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC3>
{
    using BufferFormat = BufferFormat_REAL_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC4>
{
    using BufferFormat = BufferFormat_REAL_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_SCALAR>
{
    using BufferFormat = BufferFormat_WHOLE_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC2>
{
    using BufferFormat = BufferFormat_WHOLE_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC3>
{
    using BufferFormat = BufferFormat_WHOLE_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC4>
{
    using BufferFormat = BufferFormat_WHOLE_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_SCALAR>
{
    using BufferFormat = BufferFormat_COUNT_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC2>
{
    using BufferFormat = BufferFormat_COUNT_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC3>
{
    using BufferFormat = BufferFormat_COUNT_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC4>
{
    using BufferFormat = BufferFormat_COUNT_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH>
{
    using BufferFormat = BufferFormat_DEPTH;
    using CompatibleBufferFormat = std::variant<BufferFormat_DEPTH>;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH_AND_STENCIL>
{
    using BufferFormat = BufferFormat_DEPTH_AND_STENCIL;
    using CompatibleBufferFormat = std::variant<BufferFormat_DEPTH_AND_STENCIL>;
};

template <> struct BufferTypeSpecialization<BufferType::STENCIL>
{
    using BufferFormat = BufferFormat_STENCIL;
    using CompatibleBufferFormat = std::variant<BufferFormat_STENCIL>;
};

} // namespace Vitrae