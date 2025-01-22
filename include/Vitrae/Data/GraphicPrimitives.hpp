#pragma once

#include "glm/glm.hpp"

namespace Vitrae
{

struct Triangle
{
    unsigned int ind[3];
};

enum class FrontSideOrientation {
    /**
     * The vertices are ordered counter-clockwise around a polygon when viewed from the front
     */
    CounterClockwise,
    /**
     * The vertices are ordered clockwise around a polygon when viewed from the front
     */
    Clockwise,
    /**
     * Both sides are front sides
     */
    Both,
};
} // namespace Vitrae