#pragma once

#include <cstdlib>

namespace Vitrae
{
inline std::size_t getUniqueID()
{
    static std::size_t ctr = 0;
    ctr++;
    return ctr;
}

template <class T> std::size_t getClassID()
{
    static std::size_t id = getUniqueID();
    return id;
}
} // namespace Vitrae