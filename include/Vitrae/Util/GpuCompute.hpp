#pragma once

#include "Vitrae/Params/PropertyGetter.hpp"

namespace Vitrae
{

inline constexpr std::size_t GROUP_SIZE_AUTO = 0;

struct GpuComputeSetupParams
{
    PropertyGetter<std::uint32_t> invocationCountX;
    PropertyGetter<std::uint32_t> invocationCountY = 1;
    PropertyGetter<std::uint32_t> invocationCountZ = 1;
    PropertyGetter<std::uint32_t> groupSizeX = GROUP_SIZE_AUTO;
    PropertyGetter<std::uint32_t> groupSizeY = GROUP_SIZE_AUTO;
    PropertyGetter<std::uint32_t> groupSizeZ = GROUP_SIZE_AUTO;
    bool allowOutOfBoundsCompute = false;
};

} // namespace Vitrae