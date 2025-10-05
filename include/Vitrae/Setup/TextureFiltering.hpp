#pragma once

#include "Vitrae/Data/Monostates.hpp"

#include "glm/glm.hpp"

#include <variant>

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
    std::variant<InheritedT, WrappingType> horWrap;
    std::variant<InheritedT, WrappingType> verWrap;
    std::variant<InheritedT, FilterType> minFilter;
    std::variant<InheritedT, FilterType> magFilter;
    std::variant<InheritedT, bool> useMipMaps;
    std::variant<InheritedT, UnusedT, FilterType> mipmapFilter;
    std::variant<InheritedT, UnusedT, glm::vec4> borderColor;
};

namespace FilteringCommon
{

constexpr TextureFilteringParams INHERIT_ALL{
    INHERITED, INHERITED, INHERITED, INHERITED, INHERITED, INHERITED, INHERITED,
};

constexpr TextureFilteringParams NEAREST_TILED{
    WrappingType::REPEAT,
    WrappingType::REPEAT,
    FilterType::NEAREST,
    FilterType::NEAREST,
    true,
    FilterType::NEAREST,
    UNUSED,
};

constexpr TextureFilteringParams BILINEAR_TILED{
    WrappingType::REPEAT,
    WrappingType::REPEAT,
    FilterType::LINEAR,
    FilterType::LINEAR,
    true,
    FilterType::NEAREST,
    UNUSED,
};

constexpr TextureFilteringParams TRILINEAR_TILED{
    WrappingType::REPEAT,
    WrappingType::REPEAT,
    FilterType::LINEAR,
    FilterType::LINEAR,
    true,
    FilterType::LINEAR,
    UNUSED,
};

} // namespace FilteringCommon

} // namespace Vitrae