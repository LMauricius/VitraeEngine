#pragma once

#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/Monostates.hpp"
#include "Vitrae/Data/SamplingMode.hpp"
#include "Vitrae/Setup/TextureFiltering.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Specifies how the texture/image will be used in a shading task.
 * By default, Textures can only be sampled directly according to their internal filtering
 * configuration and Images cannot be accessed at all.
 * This ParamAttribute allows you to configure filtering specifically for this parameter,
 * enable special sampling operations (e.g. hardware accelerated shadow sampling using parametrized
 * sampling), and to read/write to image buffers directly.
 *
 *@note To use parametrized sampling BufferType of the texture must be BufferType::DEPTH.
 */
struct ShadingImageUse
{
    /// The sampling mode, or UNUSED if you don't want to sample this texture
    std::variant<UnusedT, SamplingMode> sampling = UNUSED;

    /// Custom filtering for this sampler
    TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;

    /// How we interpret the image buffer when reading from it, or UNUSED if we don't read from it
    std::variant<UnusedT, AnyBufferStorage> bufferReadFormat = UNUSED;

    /// How we interpret the image buffer when writing to it, or UNUSED if we don't write to it
    std::variant<UnusedT, AnyBufferStorage> bufferWriteFormat = UNUSED;
};

} // namespace Vitrae