#pragma once

#include <cstdint>
#include <span>

namespace Vitrae
{

template <std::size_t N = std::dynamic_extent>
inline std::size_t combinedHashes(std::span<const std::size_t, N> hashes)
{
    std::size_t seed = hashes[0];

    for (std::size_t hash : hashes.subspan(1)) {
        seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    return seed;
}

} // namespace Vitrae