#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Setup/ImageFiltering.hpp"

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

    struct TextureStats
    {
        glm::vec4 averageColor;
    };

    struct FileLoadParams
    {
        ComponentRoot &root;
        std::filesystem::path filepath;
        TextureFilteringParams filtering;
    };
    struct EmptyParams
    {
        ComponentRoot &root;
        glm::uvec2 size;
        BufferFormat format = BufferFormat::RGB_STANDARD;
        TextureFilteringParams filtering;
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