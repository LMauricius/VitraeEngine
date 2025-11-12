#pragma once

namespace Vitrae
{

/**
 * A compile-time, inline global variable equal to VAL.
 * @note Useful for references to global constant versions of literal values
 */
template <auto VAL> constexpr auto GLOBAL_CONST = VAL;

} // namespace Vitrae