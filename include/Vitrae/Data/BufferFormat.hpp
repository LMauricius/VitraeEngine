#pragma once

namespace Vitrae
{

/**
 * Format of image (and other) buffer components in memory.
 * Constants are named like this: <Component type>_<Storage type>
 * Component type is the way we interpret the data along with some special features
 * Storage type is the way we store its components in memory, along with their most natural type
 * 
 * Component types:
 *  - GRAYSCALE - 1 component stored; interpreted as a 4 channel vector color with red, green and
 * blue of equal value and alpha of 1.0
 *  - GRAYSCALE_ALPHA - 2 components stored; interpreted as a 4 channel vector color with red, green
 * and blue of equal value and a separate alpha value
 *  - RGB - 3 components stored; interpreted as a 4 channel vector color with alpha of 1.0
 *  - RGBA - 4 components color; all separate channels
 *  - DEPTH - 1 component value; interpreted as the distance of rendered geometry in the range of
 * [0.0, 1.0]
 *  - SCALAR - 1 component scalar value, float or integer
 *  - VEC2 - 2 component vector value, float or integer components
 *  - VEC3 - 3 component vector value, float or integer components
 *  - VEC4 - 4 component vector value, float or integer components
 * 
 * Storage types:
 *  - STANDARD - the usual or most natural storage format for the component type with good
 * compromises. For color channels that gives real values in the range [0.0, 1.0]
 * - SNORM8 - 8-bit real value with a range of [-1.0, 1.0]
 * - UNORM8 - 8-bit real value with a range of [0.0, 1.0]
 * - FLOAT16 - 16-bit floating point value
 * - FLOAT32 - 32-bit floating point value
 */
enum class BufferFormat {
    GRAYSCALE_STANDARD,
    GRAYSCALE_ALPHA_STANDARD,
    RGB_STANDARD,
    RGBA_STANDARD,
    DEPTH_STANDARD,
    SCALAR_SNORM8,
    VEC2_SNORM8,
    VEC3_SNORM8,
    VEC4_SNORM8,
    SCALAR_UNORM8,
    VEC2_UNORM8,
    VEC3_UNORM8,
    VEC4_UNORM8,
    SCALAR_FLOAT16,
    VEC2_FLOAT16,
    VEC3_FLOAT16,
    VEC4_FLOAT16,
    SCALAR_FLOAT32,
    VEC2_FLOAT32,
    VEC3_FLOAT32,
    VEC4_FLOAT32,
};

} // namespace Vitrae