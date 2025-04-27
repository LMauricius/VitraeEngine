#pragma once

#include <cstdint>

namespace Vitrae
{

struct STD140LayoutMeta
{
    std::size_t std140Size;
    std::size_t std140Alignment;
};

} // namespace Vitrae