#pragma once

#include "Vitrae/Data/Typedefs.hpp"

#include "dynasma/core_concepts.hpp"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"

#include "glm/glm.hpp"

#include <filesystem>
#include <optional>

namespace Vitrae
{
class ComponentRoot;

/**
 * A Texture is a single image-like resource
 */
class Texture : public dynasma::PolymorphicBase
{
  public:
    enum class ChannelType {
        GRAYSCALE,
        GRAYSCALE_ALPHA,
        RGB,
        RGBA,
        DEPTH,
        SCALAR_SNORM8,
        VEC2_SNORM8,
        VEC3_SNORM8,
        VEC4_SNORM8,
        SCALAR_UNORM8,
        VEC2_UNORM8,
        VEC3_UNORM8,
        VEC4_UNORM8,
        SCALAR_FLOAT16,
        VEC2_FLOAT16,
        VEC3_FLOAT16,
        VEC4_FLOAT16,
        SCALAR_FLOAT32,
        VEC2_FLOAT32,
        VEC3_FLOAT32,
        VEC4_FLOAT32,
    };
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

    struct TextureStats
    {
        glm::vec4 averageColor;
    };

    struct FileLoadParams
    {
        ComponentRoot &root;
        std::filesystem::path filepath;
        WrappingType horWrap = WrappingType::REPEAT;
        WrappingType verWrap = WrappingType::REPEAT;
        FilterType minFilter = FilterType::LINEAR;
        FilterType magFilter = FilterType::LINEAR;
        bool useMipMaps = true;
    };
    struct EmptyParams
    {
        ComponentRoot &root;
        glm::uvec2 size;
        ChannelType channelType = ChannelType::RGB;
        WrappingType horWrap = WrappingType::REPEAT;
        WrappingType verWrap = WrappingType::REPEAT;
        FilterType minFilter = FilterType::LINEAR;
        FilterType magFilter = FilterType::LINEAR;
        bool useMipMaps = true;
        glm::vec4 borderColor = {0.0f, 0.0f, 0.0f, 0.0f};
        String friendlyName = "";
    };
    struct PureColorParams
    {
        ComponentRoot &root;
        glm::vec4 color;
    };

    virtual ~Texture() = default;

    virtual std::size_t memory_cost() const = 0;

    inline glm::vec2 getSize() const { return glm::vec2(mWidth, mHeight); }
    inline const std::optional<TextureStats> &getStats() const { return m_stats; }

  protected:
    int mWidth, mHeight;
    std::optional<TextureStats> m_stats;
};

struct TextureSeed
{
    using Asset = Texture;

    inline std::size_t load_cost() const { return 1; }

    std::variant<Texture::FileLoadParams, Texture::EmptyParams, Texture::PureColorParams> kernel;
};

using TextureManager = dynasma::AbstractManager<TextureSeed>;
// using TextureKeeper = dynasma::AbstractKeeper<ImmediateTextureSeed>;
} // namespace Vitrae