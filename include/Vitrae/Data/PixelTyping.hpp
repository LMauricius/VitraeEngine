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
enum class PixelType {
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

// used for specializing enums and types tied to a specific PixelType
template <PixelType PIXEL_TYPE> struct PixelTypeSpecialization;

/**
 * Storage format specification of image (and other) buffers.
 * This value specifies how the bits in memory represent data in our buffers
 * Each PixelType has its own format constants.
 *
 * While PixelType elements are channels of vector values, PixelFormat elements are slots of
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
 * @note Alternatively you can use constants from enum classes named as PixelFormat_TYPE,
 * where TYPE is the PixelType that would be passed as a template parameter
 */
template <PixelType BT> using PixelFormat = typename PixelTypeSpecialization<BT>::PixelFormat;

/**
 * A variant of all PixelFormat formats that can store data for the specified PixelType.
 * Generally, you can use any format with the same channel type but a different vector size.
 * @note If not using the native format (i.e. PixelFormat<BT> with BT being the PixelType),
 * you should use the texture channel swizzling to specify how native type channels are
 * calculated from the stored data.
 */
template <PixelType BT>
using CompatiblePixelFormat = typename PixelTypeSpecialization<BT>::CompatiblePixelFormat;

/**
 * The native (CPU) channel type of the specified PixelType.
 * @note For PixelType::DEPTH_AND_STENCIL, this is a pair of native types
 */
template <PixelType BT>
using PixelChannelType = typename PixelTypeSpecialization<BT>::PixelChannelType;

/**
 * The number of channels in the specified PixelType.
 */
template <PixelType BT>
constexpr std::size_t CHANNEL_COUNT = PixelTypeSpecialization<BT>::CHANNEL_COUNT;

/**
 * The native (CPU) vector/scalar type of a single entry of the specified PixelType.
 */
template <PixelType BT>
using PixelValueType = std::conditional_t<(CHANNEL_COUNT<BT> == 1), PixelChannelType<BT>,
                                          glm::vec<CHANNEL_COUNT<BT>, PixelChannelType<BT>>>;

enum class PixelFormat_REAL_SCALAR {
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

enum class PixelFormat_REAL_VEC2 {
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

enum class PixelFormat_REAL_VEC3 {
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

enum class PixelFormat_REAL_VEC4 {
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

enum class PixelFormat_WHOLE_SCALAR {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class PixelFormat_WHOLE_VEC2 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class PixelFormat_WHOLE_VEC3 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class PixelFormat_WHOLE_VEC4 {
    GENERIC,
    INT8,
    INT16,
    INT32,
};

enum class PixelFormat_COUNT_SCALAR {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class PixelFormat_COUNT_VEC2 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class PixelFormat_COUNT_VEC3 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,
};

enum class PixelFormat_COUNT_VEC4 {
    GENERIC,
    UNSIGNED8,
    UNSIGNED16,
    UNSIGNED32,

    UNSIGNED_10_10_10_2,
};

enum class PixelFormat_DEPTH {
    GENERIC,
    NORM16,
    NORM24,
    NORM32,
    FLOAT32,
};

enum class PixelFormat_STENCIL {
    GENERIC,
    BOOL,
    UNSIGNED4,
    UNSIGNED8,
    UNSIGNED16,
};

enum class PixelFormat_DEPTH_AND_STENCIL {
    GENERIC,
    NORM24_UNSIGNED8,
    FLOAT32_UNSIGNED8,
};

using AnyPixelFormat =
    std::variant<PixelFormat_REAL_SCALAR, PixelFormat_REAL_VEC2, PixelFormat_REAL_VEC3,
                 PixelFormat_REAL_VEC4, PixelFormat_WHOLE_SCALAR, PixelFormat_WHOLE_VEC2,
                 PixelFormat_WHOLE_VEC3, PixelFormat_WHOLE_VEC4, PixelFormat_COUNT_SCALAR,
                 PixelFormat_COUNT_VEC2, PixelFormat_COUNT_VEC3, PixelFormat_COUNT_VEC4,
                 PixelFormat_DEPTH, PixelFormat_STENCIL, PixelFormat_DEPTH_AND_STENCIL>;

// ==== Helpers for handling all these types =======================================================

/**
 * @returns The number of channels for the specified PixelType
 * @param pixelType The PixelType
 */
std::size_t getChannelCount(PixelType pixelType);

/**
 * Calls the templated visitor on all PixelTypes
 * @param visitor its operator() has to accept a PixelType as its template parameter
 * @param args The arguments to pass to the visitor
 * @note You can use a template lambda for this
 * @example @code
 *  forPixelTypes(
 *      []<PixelType BT>(std::string_view str) {
 *          std::print("{}{}\n", str, BT)
 *      },
 *      "BufTp: "
 *  );
 * @endcode
 */
template <class VisitorT, typename... ArgTs>
constexpr void forPixelTypes(VisitorT &&visitor, ArgTs &&...args);

/**
 * A variant that contains OptionT<PIXEL_TYPE> for all PixelTypes
 * @param OptionT a template class that is parametrized by a PixelType
 */
template <template <auto> class OptionT>
using VariantForPixelTypes = std::variant<
    OptionT<PixelType::REAL_SCALAR>, OptionT<PixelType::REAL_VEC2>, OptionT<PixelType::REAL_VEC3>,
    OptionT<PixelType::REAL_VEC4>, OptionT<PixelType::WHOLE_SCALAR>, OptionT<PixelType::WHOLE_VEC2>,
    OptionT<PixelType::WHOLE_VEC3>, OptionT<PixelType::WHOLE_VEC4>,
    OptionT<PixelType::COUNT_SCALAR>, OptionT<PixelType::COUNT_VEC2>,
    OptionT<PixelType::COUNT_VEC3>, OptionT<PixelType::COUNT_VEC4>, OptionT<PixelType::DEPTH>,
    OptionT<PixelType::STENCIL>, OptionT<PixelType::DEPTH_AND_STENCIL>>;

// ==== Specialization wrappers per PixelType =====================================================

template <> struct PixelTypeSpecialization<PixelType::REAL_SCALAR>
{
    using PixelFormat = PixelFormat_REAL_SCALAR;
    using CompatiblePixelFormat = std::variant<PixelFormat_REAL_SCALAR, PixelFormat_REAL_VEC2,
                                               PixelFormat_REAL_VEC3, PixelFormat_REAL_VEC4>;
    using PixelChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct PixelTypeSpecialization<PixelType::REAL_VEC2>
{
    using PixelFormat = PixelFormat_REAL_VEC2;
    using CompatiblePixelFormat = std::variant<PixelFormat_REAL_SCALAR, PixelFormat_REAL_VEC2,
                                               PixelFormat_REAL_VEC3, PixelFormat_REAL_VEC4>;
    using PixelChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct PixelTypeSpecialization<PixelType::REAL_VEC3>
{
    using PixelFormat = PixelFormat_REAL_VEC3;
    using CompatiblePixelFormat = std::variant<PixelFormat_REAL_SCALAR, PixelFormat_REAL_VEC2,
                                               PixelFormat_REAL_VEC3, PixelFormat_REAL_VEC4>;
    using PixelChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct PixelTypeSpecialization<PixelType::REAL_VEC4>
{
    using PixelFormat = PixelFormat_REAL_VEC4;
    using CompatiblePixelFormat = std::variant<PixelFormat_REAL_SCALAR, PixelFormat_REAL_VEC2,
                                               PixelFormat_REAL_VEC3, PixelFormat_REAL_VEC4>;
    using PixelChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct PixelTypeSpecialization<PixelType::WHOLE_SCALAR>
{
    using PixelFormat = PixelFormat_WHOLE_SCALAR;
    using CompatiblePixelFormat = std::variant<PixelFormat_WHOLE_SCALAR, PixelFormat_WHOLE_VEC2,
                                               PixelFormat_WHOLE_VEC3, PixelFormat_WHOLE_VEC4>;
    using PixelChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct PixelTypeSpecialization<PixelType::WHOLE_VEC2>
{
    using PixelFormat = PixelFormat_WHOLE_VEC2;
    using CompatiblePixelFormat = std::variant<PixelFormat_WHOLE_SCALAR, PixelFormat_WHOLE_VEC2,
                                               PixelFormat_WHOLE_VEC3, PixelFormat_WHOLE_VEC4>;
    using PixelChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct PixelTypeSpecialization<PixelType::WHOLE_VEC3>
{
    using PixelFormat = PixelFormat_WHOLE_VEC3;
    using CompatiblePixelFormat = std::variant<PixelFormat_WHOLE_SCALAR, PixelFormat_WHOLE_VEC2,
                                               PixelFormat_WHOLE_VEC3, PixelFormat_WHOLE_VEC4>;
    using PixelChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct PixelTypeSpecialization<PixelType::WHOLE_VEC4>
{
    using PixelFormat = PixelFormat_WHOLE_VEC4;
    using CompatiblePixelFormat = std::variant<PixelFormat_WHOLE_SCALAR, PixelFormat_WHOLE_VEC2,
                                               PixelFormat_WHOLE_VEC3, PixelFormat_WHOLE_VEC4>;
    using PixelChannelType = int;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct PixelTypeSpecialization<PixelType::COUNT_SCALAR>
{
    using PixelFormat = PixelFormat_COUNT_SCALAR;
    using CompatiblePixelFormat = std::variant<PixelFormat_COUNT_SCALAR, PixelFormat_COUNT_VEC2,
                                               PixelFormat_COUNT_VEC3, PixelFormat_COUNT_VEC4>;
    using PixelChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct PixelTypeSpecialization<PixelType::COUNT_VEC2>
{
    using PixelFormat = PixelFormat_COUNT_VEC2;
    using CompatiblePixelFormat = std::variant<PixelFormat_COUNT_SCALAR, PixelFormat_COUNT_VEC2,
                                               PixelFormat_COUNT_VEC3, PixelFormat_COUNT_VEC4>;
    using PixelChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 2;
};

template <> struct PixelTypeSpecialization<PixelType::COUNT_VEC3>
{
    using PixelFormat = PixelFormat_COUNT_VEC3;
    using CompatiblePixelFormat = std::variant<PixelFormat_COUNT_SCALAR, PixelFormat_COUNT_VEC2,
                                               PixelFormat_COUNT_VEC3, PixelFormat_COUNT_VEC4>;
    using PixelChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 3;
};

template <> struct PixelTypeSpecialization<PixelType::COUNT_VEC4>
{
    using PixelFormat = PixelFormat_COUNT_VEC4;
    using CompatiblePixelFormat = std::variant<PixelFormat_COUNT_SCALAR, PixelFormat_COUNT_VEC2,
                                               PixelFormat_COUNT_VEC3, PixelFormat_COUNT_VEC4>;
    using PixelChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 4;
};

template <> struct PixelTypeSpecialization<PixelType::DEPTH>
{
    using PixelFormat = PixelFormat_DEPTH;
    using CompatiblePixelFormat = std::variant<PixelFormat_DEPTH>;
    using PixelChannelType = float;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct PixelTypeSpecialization<PixelType::DEPTH_AND_STENCIL>
{
    using PixelFormat = PixelFormat_DEPTH_AND_STENCIL;
    using CompatiblePixelFormat = std::variant<PixelFormat_DEPTH_AND_STENCIL>;
    using PixelChannelType = std::pair<float, unsigned int>;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

template <> struct PixelTypeSpecialization<PixelType::STENCIL>
{
    using PixelFormat = PixelFormat_STENCIL;
    using CompatiblePixelFormat = std::variant<PixelFormat_STENCIL>;
    using PixelChannelType = unsigned int;
    constexpr static std::size_t CHANNEL_COUNT = 1;
};

// ==== Helper implementation ======================================================================

inline std::size_t getChannelCount(PixelType pixelType)
{

    switch (pixelType) {
    case PixelType::REAL_SCALAR:
    case PixelType::WHOLE_SCALAR:
    case PixelType::COUNT_SCALAR:
    case PixelType::DEPTH:
    case PixelType::STENCIL:
    case PixelType::DEPTH_AND_STENCIL:
        return 1;
    case PixelType::REAL_VEC2:
    case PixelType::WHOLE_VEC2:
    case PixelType::COUNT_VEC2:
        return 2;
    case PixelType::REAL_VEC3:
    case PixelType::WHOLE_VEC3:
    case PixelType::COUNT_VEC3:
        return 3;
    case PixelType::REAL_VEC4:
    case PixelType::WHOLE_VEC4:
    case PixelType::COUNT_VEC4:
        return 4;
    }
}

template <class VisitorT, typename... ArgTs>
constexpr void forPixelTypes(VisitorT &&visitor, ArgTs &&...args)
{
    visitor.template operator()<PixelType::REAL_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::REAL_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::REAL_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::REAL_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::WHOLE_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::WHOLE_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::WHOLE_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::WHOLE_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::COUNT_SCALAR>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::COUNT_VEC2>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::COUNT_VEC3>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::COUNT_VEC4>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::DEPTH>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::STENCIL>(std::forward<ArgTs>(args)...);
    visitor.template operator()<PixelType::DEPTH_AND_STENCIL>(std::forward<ArgTs>(args)...);
}

} // namespace Vitrae