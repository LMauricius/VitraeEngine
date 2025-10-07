#pragma once

#include <variant>

namespace Vitrae
{
struct InheritedT : std::monostate
{};

inline InheritedT INHERITED;

struct UnusedT : std::monostate
{};

inline UnusedT UNUSED;

} // namespace Vitrae