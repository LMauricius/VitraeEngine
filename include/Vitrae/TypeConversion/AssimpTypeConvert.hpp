#pragma once

#include "assimp/types.h"
#include "glm/glm.hpp"

namespace Vitrae
{

template <class T> struct aiTypeCvt
{
    using glmType = T;

    static glmType toGlmVal(T val) { return val; }
};

template <> struct aiTypeCvt<aiVector2D>
{
    using glmType = glm::vec2;

    static glmType toGlmVal(aiVector2D val) { return glm::vec2(val.x, val.y); }
};

template <> struct aiTypeCvt<aiVector3D>
{
    using glmType = glm::vec3;

    static glmType toGlmVal(aiVector3D val) { return glm::vec3(val.x, val.y, val.z); }
};

template <> struct aiTypeCvt<aiColor3D>
{
    using glmType = glm::vec3;

    static glmType toGlmVal(aiColor3D val) { return glm::vec3(val.r, val.g, val.b); }
};

template <> struct aiTypeCvt<aiColor4D>
{
    using glmType = glm::vec4;

    static glmType toGlmVal(aiColor4D val) { return glm::vec4(val.r, val.g, val.b, val.a); }
};
} // namespace Vitrae