#pragma once

#include "Vitrae/Dynamic/TypeInfo.hpp"

#include <vector>

namespace Vitrae
{

struct DependentMeta
{
    std::vector<std::reference_wrapper<const TypeInfo>> dependencyTypes;
};

} // namespace Vitrae