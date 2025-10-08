#pragma once

#include "Vitrae/Assets/Image.hpp"
#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/Sides.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Setup/ImageLoad.hpp"

#include "dynasma/managers/abstract.hpp"

#include "dynasma/pointer.hpp"
#include "glm/glm.hpp"

#include <optional>

namespace Vitrae
{
class ComponentRoot;

/**
 * An image is a 1-3 dimensional array of pixels
 */
class TextureBase : public dynasma::PolymorphicBase
{
  public:
    struct TextureStats
    {
        glm::vec4 averageColor;
    };

    virtual ~TextureBase() = default;

    virtual std::size_t memory_cost() const = 0;

    virtual glm::uvec4 getNDSize() const = 0;
    virtual std::size_t getNumDimensions() const = 0;
    const std::optional<TextureStats> &getStats() const { return m_stats; }
    virtual BufferType getBufferType() const = 0;
    virtual AnyBufferFormat getAnyBufferFormat() const = 0;

    void setProperty(StringId key, const Variant &value);
    void setProperty(StringId key, Variant &&value);
    const StableMap<StringId, Variant> &getProperties() const { return m_properties; }

  protected:
    std::optional<TextureStats> m_stats;
    StableMap<StringId, Variant> m_properties;
};

/**
 * A seed for any image type
 * @tparam TextureT the image type
 * @example @code TextureSeed<Texture2D<BufferType::COLOR_TRANSPARENT>> @endcode
 */
template <class TextureT> struct TextureSeed
{
    using Asset = TextureT;

    std::size_t load_cost() const { return 1; }

    std::variant<typename TextureT::FileLoadParams, typename TextureT::EmptyParams,
                 typename TextureT::PureColorParams>
        kernel;
};

/**
 * A manager for any image type
 * @tparam TextureT the image type
 * @example @code TextureManager<Texture2D<BufferType::COLOR_TRANSPARENT>> @endcode
 */
template <class TextureT> using TextureManager = dynasma::AbstractManager<TextureSeed<TextureT>>;

/**
 * A 1D image
 */
template <BufferType BUFFER_TYPE> class Texture1D : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, unsigned int>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    unsigned int getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1, 1}; }
    std::size_t getNumDimensions() const override { return 1; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image1D<BUFFER_TYPE>> getImage() const = 0;

  protected:
    unsigned int m_size;
};

/**
 * A 2D image
 */
template <BufferType BUFFER_TYPE> class Texture2D : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image2D<BUFFER_TYPE>> getImage() const = 0;

  protected:
    glm::uvec2 m_size;
};

/**
 * A 3D image
 */
template <BufferType BUFFER_TYPE> class Texture3D : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec3 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1}; }
    std::size_t getNumDimensions() const override { return 3; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image2D<BUFFER_TYPE>> getImage(unsigned int z) const = 0;

  protected:
    glm::uvec3 m_size;
};

/**
 * A 3D collection of 6 2D images used for cubemapping
 */
template <BufferType BUFFER_TYPE> class TextureCubemap : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 6, 1}; }
    std::size_t getNumDimensions() const override { return 3; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image2D<BUFFER_TYPE>> getImage(Side side) const = 0;

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 1D images, each a layer of 1 asset
 */
template <BufferType BUFFER_TYPE> class Texture1DArray : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image1D<BUFFER_TYPE>> getImage(unsigned int index) const = 0;

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 2D images
 */
template <BufferType BUFFER_TYPE> class Texture2DArray : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 3; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image2D<BUFFER_TYPE>> getImage(unsigned int index) const = 0;

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of cubemap images
 */
template <BufferType BUFFER_TYPE> class TextureCubemapArray : public TextureBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    glm::uvec3 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size.x, m_size.y, m_size.z, 1}; }
    std::size_t getNumDimensions() const override { return 4; }
    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    virtual dynasma::LazyPtr<Image2D<BUFFER_TYPE>> getImage(Side side,
                                                            unsigned int index) const = 0;

  protected:
    glm::uvec3 m_size;
};

} // namespace Vitrae