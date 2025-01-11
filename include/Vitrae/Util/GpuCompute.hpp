#pragma once

#include "Vitrae/Params/ArgumentGetter.hpp"

namespace Vitrae
{

inline constexpr std::size_t GROUP_SIZE_AUTO = 0;

struct GpuComputeSetupParams
{
    ArgumentGetter<std::uint32_t> invocationCountX;
    ArgumentGetter<std::uint32_t> invocationCountY = 1;
    ArgumentGetter<std::uint32_t> invocationCountZ = 1;
    ArgumentGetter<std::uint32_t> groupSizeX = GROUP_SIZE_AUTO;
    ArgumentGetter<std::uint32_t> groupSizeY = GROUP_SIZE_AUTO;
    ArgumentGetter<std::uint32_t> groupSizeZ = GROUP_SIZE_AUTO;
    bool allowOutOfBoundsCompute = false;
};

} // namespace Vitrae