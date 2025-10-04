#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Setup/ImageLoad.hpp"

#include "dynasma/managers/abstract.hpp"

#include "glm/glm.hpp"

#include <optional>

namespace Vitrae
{
class ComponentRoot;

/**
 * An image is a 1-3 dimensional array of pixels
 */
class ImageBase : public dynasma::PolymorphicBase
{
  public:
    struct ImageStats
    {
        glm::vec4 averageColor;
    };

    virtual ~ImageBase() = default;

    virtual std::size_t memory_cost() const = 0;

    virtual glm::uvec4 getNDSize() const = 0;
    virtual std::size_t getNumDimensions() const = 0;
    inline const std::optional<ImageStats> &getStats() const { return m_stats; }

    void setProperty(StringId key, const Variant &value);
    void setProperty(StringId key, Variant &&value);
    inline const StableMap<StringId, Variant> &getProperties() const { return m_properties; }

  protected:
    std::optional<ImageStats> m_stats;
    StableMap<StringId, Variant> m_properties;
};

/**
 * A seed for any image type
 * @tparam ImageT the image type
 * @example @code ImageSeed<Image2D<BufferType::COLOR_TRANSPARENT>> @endcode
 */
template <class ImageT> struct ImageSeed
{
    using Asset = ImageT;

    inline std::size_t load_cost() const { return 1; }

    std::variant<typename ImageT::FileLoadParams, typename ImageT::EmptyParams,
                 typename ImageT::PureColorParams>
        kernel;
};

/**
 * A manager for any image type
 * @tparam ImageT the image type
 * @example @code ImageManager<Image2D<BufferType::COLOR_TRANSPARENT>> @endcode
 */
template <class ImageT> using ImageManager = dynasma::AbstractManager<ImageSeed<ImageT>>;

/**
 * A 1D image
 */
template <BufferType BUFFER_TYPE> class Image1D : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, unsigned int>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline unsigned int getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1, 1}; }
    std::size_t getNumDimensions() const override { return 1; }

  protected:
    unsigned int m_size;
};

/**
 * A 2D image
 */
template <BufferType BUFFER_TYPE> class Image2D : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A 3D image
 */
template <BufferType BUFFER_TYPE> class Image3D : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec3 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec3 m_size;
};

/**
 * A 3D collection of 6 2D images used for cubemapping
 */
template <BufferType BUFFER_TYPE> class ImageCubemap : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 6, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 1D images, each a layer of 1 asset
 */
template <BufferType BUFFER_TYPE> class Image1DArray : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 2D images
 */
template <BufferType BUFFER_TYPE> class Image2DArray : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of cubemap images
 */
template <BufferType BUFFER_TYPE> class ImageCubemapArray : public ImageBase
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<BUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<BUFFER_TYPE>;

    inline glm::uvec3 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size.x, m_size.y, 6, m_size.z}; }
    std::size_t getNumDimensions() const override { return 4; }

  protected:
    glm::uvec3 m_size;
};

} // namespace Vitrae