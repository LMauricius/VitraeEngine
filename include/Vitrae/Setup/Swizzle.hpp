#pragma once

#include "Vitrae/Data/BufferFormat.hpp"

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

template <BufferType BUFFER_TYPE>
using SwizzleSpec = glm::vec<BufferTypeSpecialization<BUFFER_TYPE>::CHANNEL_COUNT, ChannelSource>;

template <BufferType BUFFER_TYPE> struct CommonSwizzleSpecs
{
    constexpr static SwizzleSpec<BUFFER_TYPE> NATURAL = glm::vec<4, ChannelSource>{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::SLOT_3,
    }; // this will be truncated as needed
};

template <> struct CommonSwizzleSpecs<BufferType::COLOR>
{
    constexpr static SwizzleSpec<BufferType::COLOR> NATURAL{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
    };

    constexpr static SwizzleSpec<BufferType::COLOR> GRAYSCALE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
    };
};

template <> struct CommonSwizzleSpecs<BufferType::TRANSPARENT>
{
    constexpr static SwizzleSpec<BufferType::TRANSPARENT> NATURAL{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::SLOT_3,
    };

    constexpr static SwizzleSpec<BufferType::TRANSPARENT> GRAYSCALE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
    };

    constexpr static SwizzleSpec<BufferType::TRANSPARENT> OPAQUE{
        ChannelSource::SLOT_0,
        ChannelSource::SLOT_1,
        ChannelSource::SLOT_2,
        ChannelSource::CONSTANT_1,
    };
};
} // namespace Vitrae