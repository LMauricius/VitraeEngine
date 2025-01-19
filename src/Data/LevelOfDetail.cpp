#include "Vitrae/Data/LevelOfDetail.hpp"

namespace Vitrae
{

bool SmallestElementSizeMeasure::isTooDetailed(const LoDContext &ctx,
                                               const LoDThresholdParams &threshold)
{
    return smallestElementSize * ctx.closestPointScaling < threshold.minElementSize;
}

} // namespace Vitrae