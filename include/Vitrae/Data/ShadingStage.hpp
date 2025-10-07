#pragma once

namespace Vitrae
{

/**
 * Controls where in the shader pipeline we expect the value to be calculated.
 * As shading has many stages, some values are calculated exactly where needed,
 * while others are calculated earlier and then interpolated.
 * Earlier calculation generally brings more performance, but less smooth shading, so use with care.
 * @note The engine may choose to use a different stage than the one specified, for example when the
 * value depends on values from a later stage or when calculating it in the earlier stage makes no
 * difference.
 *
 * - SHAPE - Calculated per shape, so it will be uniform across the rendered shape.
 * - VERTEX_PERSPECTIVE - Calculated per vertex and interpolated in a perspective-correct fashion
 * - VERTEX_DISPLAY - Calculated per vertex and interpolated linearly on the display
 * - SURFACE - Calculated per each pixel of the surface
 */
enum class ShadingStage {
    SHAPE,
    VERTEX_PERSPECTIVE,
    VERTEX_DISPLAY,
    SURFACE,
};
} // namespace Vitrae