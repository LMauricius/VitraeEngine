#pragma once

namespace Vitrae
{

/**
 * Specifies how the data is interpolated between points
 * @note In surface shading specifies how the data from vertices will be interpolated on pixels
 * between them
 */
enum class DataInterpolationType {
    Shader,
    Task,
    Polygon,
    ViewPoint,
    SpacePoint,
};

} // namespace Vitrae