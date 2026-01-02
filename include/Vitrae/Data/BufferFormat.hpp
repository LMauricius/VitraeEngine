#pragma once

#include <utility>
#include <variant>

#include <glm/glm.hpp>

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
 * Constants are named like this: <Channel type>_<Vector size>
 * The element type is the type of one channel of the stored value.
 * The vector size is the number of channels of the stored value.
 *
 * Channel types:
 *  - REAL - A real value, with a number of decimal places
 *  - WHOLE - A signed integer (whole) value
 *  - COUNT - An unsigned integer (counting) value
 *
 * Vector sizes:
 *  - SCALAR - the value has a single channel, so not a vector at all
 *  - VEC2 - 2 channels stored in the value
 *  - VEC3 - 3 channels stored in the value
 *  - VEC4 - 4 channels stored in the value
 *
 * For convenience, the following aliases are defined:
 *  - COLOR - A 3 channel real number vector holding an RGB color
 *  - TRANSPARENT - A 4 channel real number vector holding an RGBA color
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
 * This value specifies how the bits in memory represent data in our buffers
 * Each BufferType has its own format constants.
 *
 * While BufferType elements are channels of vector values, BufferFormat elements are slots of
 * entry values. This naming scheme is used because the order of channels doesn't have to be the
 * same as the order of stored entry slots (see SwizzleSpec).
 *
 * The constants are named in the format of <Base type><Bit count>, with some exceptions.
 * The base type is the format of each element.
 * The bit count is the number of bits per element. Some storage types use a different bit count per
 * each slot of the entry, in which case the bit counts are given in order.
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
 * Generally, you can use any format with the same channel type but a different vector size.
 * @note If not using the native format (i.e. BufferFormat<BT> with BT being the BufferType),
 * you should use the texture channel swizzling to specify how native type channels are
 * calculated from the stored data.
 */
template <BufferType BT>
using CompatibleBufferFormat = typename BufferTypeSpecialization<BT>::CompatibleBufferFormat;

/**
 * The native (CPU) channel type of the specified BufferType.
 * @note For BufferType::DEPTH_AND_STENCIL, this is a pair of native types
 */
template <BufferType BT>
using BufferChannelType = typename BufferTypeSpecialization<BT>::BufferChannelType;

/**
 * The number of channels in the specified BufferType.
 */
template <BufferType BT>
constexpr std::size_t CHANNEL_COUNT = BufferTypeSpecialization<BT>::CHANNEL_COUNT;

/**
 * The native (CPU) vector/scalar type of a single entry of the specified BufferType.
 */
template <BufferType BT>
using BufferValueType = std::conditional_t<(CHANNEL_COUNT<BT> == 1), BufferChannelType<BT>,
                                           glm::vec<CHANNEL_COUNT<BT>, BufferChannelType<BT>>>;

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

// ==== Helpers for handling all these types =======================================================

/**
 * @returns The number of channels for the specified BufferType
 * @param bufferType The BufferType
 */
std::size_t getChannelCount(BufferType bufferType);

/**
 * Calls the templated visitor on all BufferTypes
 * @param visitor its operator() has to accept a BufferType as its template parameter
 * @param args The arguments to pass to the visitor
 * @note You can use a template lambda for this
 * @example @code
 *  forBufferTypes(
 *      []<BufferType BT>(std::string_view str) {
 *          std::print("{}{}\n", str, BT)
 *      },
 *      "BufTp: "
 *  );
 * @endcode
 */
template <class VisitorT, typename... ArgTs>
constexpr void forBufferTypes(VisitorT &&visitor, ArgTs &&...args);

/**
 * A variant that contains OptionT<BUFFER_TYPE> for all BufferTypes
 * @param OptionT a template class that is parametrized by a BufferType
 */
template <template <auto> class OptionT>
using VariantForBufferTypes =
    std::variant<OptionT<BufferType::REAL_SCALAR>, OptionT<BufferType::REAL_VEC2>,
                 OptionT<BufferType::REAL_VEC3>, OptionT<BufferType::REAL_VEC4>,
                 OptionT<BufferType::WHOLE_SCALAR>, OptionT<BufferType::WHOLE_VEC2>,
                 OptionT<BufferType::WHOLE_VEC3>, OptionT<BufferType::WHOLE_VEC4>,
                 OptionT<BufferType::COUNT_SCALAR>, OptionT<BufferType::COUNT_VEC2>,
                 OptionT<BufferType::COUNT_VEC3>, OptionT<BufferType::COUNT_VEC4>,
                 OptionT<BufferType::DEPTH>, OptionT<BufferType::STENCIL>,
                 OptionT<BufferType::DEPTH_AND_STENCIL>>;

// ==== Specialization wrappers per BufferType =====================================================

template <> struct BufferTypeSpecialization<BufferType::REAL_SCALAR>
{
    using BufferFormat = BufferFormat_REAL_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
    using BufferChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC2>
{
    using BufferFormat = BufferFormat_REAL_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
    using BufferChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC3>
{
    using BufferFormat = BufferFormat_REAL_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
    using BufferChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct BufferTypeSpecialization<BufferType::REAL_VEC4>
{
    using BufferFormat = BufferFormat_REAL_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_REAL_SCALAR, BufferFormat_REAL_VEC2,
                                                BufferFormat_REAL_VEC3, BufferFormat_REAL_VEC4>;
    using BufferChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_SCALAR>
{
    using BufferFormat = BufferFormat_WHOLE_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
    using BufferChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC2>
{
    using BufferFormat = BufferFormat_WHOLE_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
    using BufferChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC3>
{
    using BufferFormat = BufferFormat_WHOLE_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
    using BufferChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct BufferTypeSpecialization<BufferType::WHOLE_VEC4>
{
    using BufferFormat = BufferFormat_WHOLE_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_WHOLE_SCALAR, BufferFormat_WHOLE_VEC2,
                                                BufferFormat_WHOLE_VEC3, BufferFormat_WHOLE_VEC4>;
    using BufferChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_SCALAR>
{
    using BufferFormat = BufferFormat_COUNT_SCALAR;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
    using BufferChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC2>
{
    using BufferFormat = BufferFormat_COUNT_VEC2;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
    using BufferChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC3>
{
    using BufferFormat = BufferFormat_COUNT_VEC3;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
    using BufferChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct BufferTypeSpecialization<BufferType::COUNT_VEC4>
{
    using BufferFormat = BufferFormat_COUNT_VEC4;
    using CompatibleBufferFormat = std::variant<BufferFormat_COUNT_SCALAR, BufferFormat_COUNT_VEC2,
                                                BufferFormat_COUNT_VEC3, BufferFormat_COUNT_VEC4>;
    using BufferChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH>
{
    using BufferFormat = BufferFormat_DEPTH;
    using CompatibleBufferFormat = std::variant<BufferFormat_DEPTH>;
    using BufferChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct BufferTypeSpecialization<BufferType::DEPTH_AND_STENCIL>
{
    using BufferFormat = BufferFormat_DEPTH_AND_STENCIL;
    using CompatibleBufferFormat = std::variant<BufferFormat_DEPTH_AND_STENCIL>;
    using BufferChannelType = std::pair<float, unsigned int>;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct BufferTypeSpecialization<BufferType::STENCIL>
{
    using BufferFormat = BufferFormat_STENCIL;
    using CompatibleBufferFormat = std::variant<BufferFormat_STENCIL>;
    using BufferChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

// ==== Helper implementation ======================================================================

inline std::size_t getChannelCount(BufferType bufferType)
{

    switch (bufferType) {
    case BufferType::REAL_SCALAR:
    case BufferType::WHOLE_SCALAR:
    case BufferType::COUNT_SCALAR:
    case BufferType::DEPTH:
    case BufferType::STENCIL:
    case BufferType::DEPTH_AND_STENCIL:
        return 1;
    case BufferType::REAL_VEC2:
    case BufferType::WHOLE_VEC2:
    case BufferType::COUNT_VEC2:
        return 2;
    case BufferType::REAL_VEC3:
    case BufferType::WHOLE_VEC3:
    case BufferType::COUNT_VEC3:
        return 3;
    case BufferType::REAL_VEC4:
    case BufferType::WHOLE_VEC4:
    case BufferType::COUNT_VEC4:
        return 4;
    }
}

template <class VisitorT, typename... ArgTs>
constexpr void forBufferTypes(VisitorT &&visitor, ArgTs &&...args)
{
    visitor.template operator()<BufferType::REAL_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::REAL_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::REAL_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::REAL_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::WHOLE_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::WHOLE_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::WHOLE_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::WHOLE_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::COUNT_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::COUNT_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::COUNT_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::COUNT_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::DEPTH>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::STENCIL>(std::forward<ArgTs>(args)...);
    visitor.template operator()<BufferType::DEPTH_AND_STENCIL>(std::forward<ArgTs>(args)...);
}

} // namespace Vitrae