#pragma once

#include "Vitrae/Data/PixelTyping.hpp"

#include <glm/glm.hpp>

namespace Vitrae
{

enum class ChannelSource {
    SLOT_0,
    SLOT_1,
    SLOT_2,
    SLOT_3,
    CONSTANT_0,
    CONSTANT_1,
};

template <PixelType PIXEL_TYPE>
using SwizzleSpec = glm::vec<PixelTypeSpecialization<PIXEL_TYPE>::CHANNEL_COUNT, ChannelSource>;

template <PixelType PIXEL_TYPE> struct CommonSwizzleSpecs
{
    constexpr static SwizzleSpec<PIXEL_TYPE> NATURAL = glm::vec<4, ChannelSource>{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::SLOT_3,
    }; // this will be truncated as needed
};

template <> struct CommonSwizzleSpecs<PixelType::COLOR>
{
    constexpr static SwizzleSpec<PixelType::COLOR> NATURAL{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
    };

    constexpr static SwizzleSpec<PixelType::COLOR> GRAYSCALE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
    };
};

template <> struct CommonSwizzleSpecs<PixelType::TRANSPARENT>
{
    constexpr static SwizzleSpec<PixelType::TRANSPARENT> NATURAL{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::SLOT_3,
    };

    constexpr static SwizzleSpec<PixelType::TRANSPARENT> GRAYSCALE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
    };

    constexpr static SwizzleSpec<PixelType::TRANSPARENT> OPAQUE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::CONSTANT_1,
    };
};
} // namespace Vitrae