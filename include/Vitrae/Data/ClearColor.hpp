#pragma once

#include "glm/glm.hpp"

#include <variant>

namespace Vitrae {

/**
 * Special colors for clearing
 */
enum class FixedClearColor {
    Garbage, // Undefined color, data left from previous usages of the buffer
    Default, // 1.0 for depth buffers, black transparent for color buffers
};

/**
 * Color/value used when clearing render buffers
 */
using ClearColor = std::variant<FixedClearColor, glm::vec4>;

}