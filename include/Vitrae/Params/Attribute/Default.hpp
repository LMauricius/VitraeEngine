#pragma once

#include <any>

namespace Vitrae
{
/**
 * Specifies that the parameter has a default value
 */
struct DefaultValue
{
    std::any value;
};

} // namespace Vitrae