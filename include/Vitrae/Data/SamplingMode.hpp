#pragma once

namespace Vitrae
{

/**
 * The way texture sampling is performed
 * - DIRECT - The texture will will be sampled at the sample position, with configured filtering
 * - PARAMETRIZED - The texture will be sampled at the sample position with the comparison operation
 *      performed between the texture data and the added parameter
 */
enum class SamplingMode {
    DIRECT,
    PARAMETRIZED,
};

} // namespace Vitrae