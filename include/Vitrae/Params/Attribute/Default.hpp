#pragma once

#include "Vitrae/Dynamic/Variant.hpp"

namespace Vitrae
{
/**
 * Specifies that the parameter has a default value
 */
struct DefaultValue
{
    Variant value;
};

} // namespace Vitrae