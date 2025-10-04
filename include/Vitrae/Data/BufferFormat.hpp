#pragma once

namespace Vitrae
{

/**
 * Native value type specification of image (and other) buffers.
 * These are generally taken very strictly, as they dictate not only the format of the buffer,
 * but also the types of value retrieved after reading the buffer,
 * the operations that can be performed on the buffer,
 * and where it can be used.
 *
 * Constants are named like this: <Element type>_<Vector size>
 * The element type is the type of one component of the stored value.
 * The vector size is the number of components of the stored value.
 *
 * Element types:
 *  - FLOAT - A floating point value
 *  - INT - A signed integer value
 *  - UNSIGNED_INT - An unsigned integer value
 *
 * Vector sizes:
 *  - SCALAR - the value is the element, so not a vector at all
 *  - VEC2 - 2 components stored
 *  - VEC3 - 3 components stored
 *  - VEC4 - 4 components stored
 *
 * For convenience, the following aliases are defined:
 *  - COLOR_OPAQUE - A 3 component float vector holding an RGB color
 *  - COLOR_TRANSPARENT - A 4 component float vector holding an RGBA color
 *
 * There are some special types that generally require special handling:
 *  - DEPTH -  Distance of rendered geometry (a special floating point value)
 *  - STENCIL - A stencil value (a special integer value used for many effects)
 */
enum class BufferType {
    FLOAT_SCALAR,
    FLOAT_VEC2,
    FLOAT_VEC3,
    FLOAT_VEC4,
    INT_SCALAR,
    INT_VEC2,
    INT_VEC3,
    INT_VEC4,
    UNSIGNED_SCALAR,
    UNSIGNED_VEC2,
    UNSIGNED_VEC3,
    UNSIGNED_VEC4,
    DEPTH,
    STENCIL,
    DEPTH_AND_STENCIL,
    COLOR_OPAQUE = FLOAT_VEC3,
    COLOR_TRANSPARENT = FLOAT_VEC4,
};

// used for specializing enums and types tied to a specific BufferType
template <BufferType BUFFER_TYPE> struct BufferTypeSpecialization;

/**
 * Storage format specification enum of image (and other) buffers.
 * Each BufferType has its own format constants.
 *
 * The constants are named in the format of <Base type><Bit count>, with some exceptions.
 * The base type is the format of each element.
 * The bit count is the number of bits per element. Some storage types use a different bit coult per
 * each component of the vector, in which case the bit counts are given in order.
 *
 * Base types:
 *  - FLOAT - A floating point value, with the range depending on the bit count
 *  - INT - A signed integer value, with the range depending on the bit count
 *  - UNSIGNED_INT - An unsigned integer value, with the range depending on the bit count
 *  - UNORM - A value with the range [0.0, 1.0], stored as an unsigned integer but read as a float
 *  - SNORM - A value with the range [-1.0, 1.0], stored as a signed integer but read as a float
 *
 * Special types:
 *  - GENERIC / GENERIC_FULL - A large ranged type, with the format left to the implementation
 *  - GENERIC_UNORM - A type with the range [0.0, 1.0], with the format left to the implementation
 *  - LINEAR_COLOR - A type with the range [0.0, 1.0], fitting for the linear color space
 *  - SRGB_COLOR - A type with the range [0.0, 1.0], fitting for the sRGB color space
 *  - SRGB_ALPHA_COLOR - sRGB color, with the alpha in linear color space
 *  - UFLOAT_<*> - A floating point value without the sign bit
 *  - <*>_EXP<Bits> - A vector of floating point values with a shared exponent of specified bits
 *
 * @note Alternatively you can use constants from enum classes named as BufferStorage_TYPE,
 * where TYPE is the BufferType that would be passed as a template parameter
 */
template <BufferType BT> using BufferStorage = typename BufferTypeSpecialization<BT>::BufferStorage;

enum class BufferStorage_FLOAT_SCALAR {
    GENERIC_FULL,
    GENERIC_UNORM,
    LINEAR_COLOR,
    FLOAT16,
    FLOAT32,
    UNORM8,
    UNORM16,
    SNORM8,
    SNORM16,
};

enum class BufferStorage_FLOAT_VEC2 {
    GENERIC_FULL,
    GENERIC_UNORM,
    LINEAR_COLOR,
    FLOAT16,
    FLOAT32,
    UNORM8,
    UNORM16,
    SNORM8,
    SNORM16,
};

enum class BufferStorage_FLOAT_VEC3 {
    GENERIC_FULL,
    GENERIC_UNORM,
    LINEAR_COLOR,
    SRGB_COLOR,
    FLOAT16,
    FLOAT32,
    UNORM4,
    UNORM5,
    UNORM8,
    UNORM10,
    UNORM12,
    UNORM16,
    SNORM8,
    SNORM16,

    UNORM_3_3_2,
    UFLOAT_11_11_10,
    FLOAT9_EXP5,
    UNORM_5_6_5,
};

enum class BufferStorage_FLOAT_VEC4 {
    GENERIC_FULL,
    GENERIC_UNORM,
    LINEAR_COLOR,
    SRGB_ALPHA_COLOR,
    FLOAT16,
    FLOAT32,
    UNORM2,
    UNORM4,
    UNORM8,
    UNORM12,
    UNORM16,
    SNORM8,
    SNORM16,

    UNORM_5_5_5_1,
    UNORM_10_10_10_2,
};

enum class BufferStorage_INT_SCALAR {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferStorage_INT_VEC2 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferStorage_INT_VEC3 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferStorage_INT_VEC4 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class BufferStorage_UNSIGNED_SCALAR {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferStorage_UNSIGNED_VEC2 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferStorage_UNSIGNED_VEC3 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class BufferStorage_UNSIGNED_VEC4 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,

    UNSIGNED_10_10_10_2,
};

enum class BufferStorage_DEPTH {
    GENERIC,
    NORM16,
    NORM24,
    NORM32,
    FLOAT32,
};

enum class BufferStorage_STENCIL {
    GENERIC,
    BOOL,
    UNSIGNED4,
    UNSIGNED8,
    UNSIGNED16,
};

enum class BufferStorage_DEPTH_AND_STENCIL {
    GENERIC,
    NORM24_AND_UNSIGNED8,
    FLOAT32_AND_UNSIGNED8,
};

// ---- Specialization wrappers per BufferType ----

template <> struct BufferTypeSpecialization<BufferType::FLOAT_SCALAR>
{
    using BufferStorage = BufferStorage_FLOAT_SCALAR;
};

template <> struct BufferTypeSpecialization<BufferType::FLOAT_VEC2>
{
    using BufferStorage = BufferStorage_FLOAT_VEC2;
};

template <> struct BufferTypeSpecialization<BufferType::FLOAT_VEC3>
{
    using BufferStorage = BufferStorage_FLOAT_VEC3;
};

template <> struct BufferTypeSpecialization<BufferType::FLOAT_VEC4>
{
    using BufferStorage = BufferStorage_FLOAT_VEC4;
};

template <> struct BufferTypeSpecialization<BufferType::INT_SCALAR>
{
    using BufferStorage = BufferStorage_INT_SCALAR;
};

template <> struct BufferTypeSpecialization<BufferType::INT_VEC2>
{
    using BufferStorage = BufferStorage_INT_VEC2;
};

template <> struct BufferTypeSpecialization<BufferType::INT_VEC3>
{
    using BufferStorage = BufferStorage_INT_VEC3;
};

template <> struct BufferTypeSpecialization<BufferType::INT_VEC4>
{
    using BufferStorage = BufferStorage_INT_VEC4;
};

template <> struct BufferTypeSpecialization<BufferType::UNSIGNED_SCALAR>
{
    using BufferStorage = BufferStorage_UNSIGNED_SCALAR;
};

template <> struct BufferTypeSpecialization<BufferType::UNSIGNED_VEC2>
{
    using BufferStorage = BufferStorage_UNSIGNED_VEC2;
};

template <> struct BufferTypeSpecialization<BufferType::UNSIGNED_VEC3>
{
    using BufferStorage = BufferStorage_UNSIGNED_VEC3;
};

template <> struct BufferTypeSpecialization<BufferType::UNSIGNED_VEC4>
{
    using BufferStorage = BufferStorage_UNSIGNED_VEC4;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH>
{
    using BufferStorage = BufferStorage_DEPTH;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH_AND_STENCIL>
{
    using BufferStorage = BufferStorage_DEPTH_AND_STENCIL;
};

template <> struct BufferTypeSpecialization<BufferType::STENCIL>
{
    using BufferStorage = BufferStorage_STENCIL;
};

} // namespace Vitrae