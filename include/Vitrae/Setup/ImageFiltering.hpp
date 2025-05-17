#pragma once

#include "glm/glm.hpp"

namespace Vitrae
{

enum class WrappingType {
    BORDER_COLOR,
    CLAMP,
    REPEAT,
    MIRROR
};
enum class FilterType {
    NEAREST,
    LINEAR
};

struct TextureFilteringParams
{
    WrappingType horWrap = WrappingType::REPEAT;
    WrappingType verWrap = WrappingType::REPEAT;
    FilterType minFilter = FilterType::LINEAR;
    FilterType magFilter = FilterType::LINEAR;
    bool useMipMaps = true;
    glm::vec4 borderColor = {0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace Vitrae