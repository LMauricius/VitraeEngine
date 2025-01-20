#pragma once

namespace Vitrae
{

/**
 * The method we use to select the desired LoD
 */
enum class LoDSelectionMethod {
    /**
     * Always choose the lowest available LoD
     * @note If the model has low enough LoDs, this causes terrible visual quality
     */
    Minimum,
    /**
     * Always choose the highest available LoD
     * @note Not reccomended except for debugging,
     * as this can cause a performance hit without improving the visual quality
     */
    Maximum,
    /**
     * Choose the highest quality LoD that's not too detailed according to the threshold
     * @note Ideal if we want to preserve performance while approaching the best visual quality.
     * If the model has enough different levels of detail, this should be indistinguishable from
     * FirstAbovehreshold, without going over the threshold
     */
    FirstBelowThreshold,
    /**
     * Choose the lowest quality LoD that's too detailed according to the threshold
     * @note Ideal if we want the highest visual quality without losing too much performance.
     * Using quality above this usually doesn't bring visual improvement
     * if the threshold is setup for the screen's real detail granularity
     */
    FirstAboveThreshold,
};

/**
 * The settings that describe a threshold quality that is too detailed for our needs.
 */
struct LoDThresholdParams
{
    /**
     * The minimal desired size of a shape's element.
     * For meshes, the element is an edge of a polygon.
     * If the unit is pixels, it is the minimum pixel size the elements are ideally displayed with.
     * @note Since modern GPUs shade pixels in goups of 2x2, this should be at least 2.
     * Pixel-sized triangles will cause *quad overdraw*, and cause most fragments to be discarded.
     * @note You can use a higher than needed value to hopefully increase performance
     * by reducing the number of rendered elements,
     * but this will reduce the quality of the rendered shape.
     * @note A value of 1 pixel should cause the shapes to use the highest visible quality,
     * but this will likely cause a performance hit due to quad overdraw.
     */
    float minElementSize;
};

/**
 * The settings for controlling how the LoD is selected
 */
struct LoDSelectionParams
{
    /**
     * The desired method
     */
    LoDSelectionMethod method;

    /**
     * The quality threshold that describes which quality is too detailed for our needs
     */
    LoDThresholdParams threshold;
};

/**
 * The context settings for the LoD selection.
 * This is usually calculated per each model for which we choose the LoD
 */
struct LoDContext
{
    /**
     * The scaling of the closest point of the shape to the view
     * It could be calculated by using the distance from the closest point of a shape to the camera,
     * and calculating the size of a unit-sized object at that distance in the current display.
     * The size could be expressed in pixels of the current viwport,
     * but the same unit should be used across these settings.
     */
    float closestPointScaling;
};

/**
 * The LoD measure of a sape.
 * This is an abstract class and allows differend methods of measuring how detailed a shape is,
 * affecting how we choose the desired LoD.
 */
struct LoDMeasure
{
    /**
     * @returns Whether this LoD is too detailed for the desired level of detail considering the ctx
     */
    virtual bool isTooDetailed(const LoDContext &ctx, const LoDThresholdParams &threshold) = 0;
};

/**
 * A measure of quality by the smallest element of a shape (a polygon edge for example),
 * in the shape's space
 */
struct SmallestElementSizeMeasure : public LoDMeasure
{
    float smallestElementSize;

    inline SmallestElementSizeMeasure(float smallestElementSize)
        : smallestElementSize(smallestElementSize)
    {}

    bool isTooDetailed(const LoDContext &ctx, const LoDThresholdParams &threshold) override;
};

} // namespace Vitrae