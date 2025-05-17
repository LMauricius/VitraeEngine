#pragma once

#include "Vitrae/Params/ParamSpec.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Identifies standard output properties of render passes.
 */
enum class FixedRenderComponent {
    Depth,
};

/**
 * Identifies output properties of render passes.
 * For generic/color-based properties PropertySpecs are used as multiples of them are supported.
 * For depth property, FixedRenderComponent::Depth is used
 */
using RenderComponent = std::variant<ParamSpec, FixedRenderComponent>;

} // namespace Vitrae