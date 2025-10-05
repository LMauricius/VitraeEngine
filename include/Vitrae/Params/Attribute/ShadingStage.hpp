#pragma once

#include "Vitrae/Data/ShadingStage.hpp"

namespace Vitrae
{

/**
 * Specifies what stage this parameter should be calculated in
 */
struct InherentShadingStage
{
    ShadingStage stage;
};

} // namespace Vitrae