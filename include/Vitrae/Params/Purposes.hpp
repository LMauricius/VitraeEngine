#pragma once

namespace Vitrae
{

/**
 * Namespace containing commonly used names of purposes for the Model forms
 */
namespace Purposes
{
/**
 * Presenting the model to a human observer, with the most detail.
 * This form is often the basis for generating all other forms.
 * Best used for textured shading with lights.
 */
inline constexpr const char visual[] = "visual";

/**
 * Rendering the model's shape, with no detail except for the shape's intrinsic parts.
 * A silhouette's has the same contours as the 'visual' forms', but they are filled differently.
 * The vertices with the same positions are merged, and their normals averaged.
 * Best suited for shadow rendering, depth pre-pass, simple collision and some physics calculations.
 */
inline constexpr const char silhouetting[] = "silhouetting";
} // namespace Purposes

} // namespace Vitrae