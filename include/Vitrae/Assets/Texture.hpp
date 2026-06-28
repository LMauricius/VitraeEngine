#pragma once

#include "Vitrae/Assets/PixelImage.hpp"
#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Data/Sides.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Setup/ImageLoad.hpp"

#include "dynasma/managers/abstract.hpp"

#include "dynasma/pointer.hpp"
#include <glm/glm.hpp>

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
    virtual PixelType getPixelType() const = 0;
    virtual AnyPixelFormat getAnyPixelFormat() const = 0;
    virtual dynasma::PinPtr<PixelImageBase> getBufferBase() = 0;
    virtual dynasma::PinPtr<const PixelImageBase> getBufferBase() const = 0;

    void setProperty(StringId key, const Variant &value);
    void setProperty(StringId key, Variant &&value);
    const VariantScope &getProperties() const { return m_properties; }

  protected:
    std::optional<TextureStats> m_stats;
    VariantScope m_properties;
};

/**
 * Base for any TextureBase type that has this PixelType
 * @tparam PIXEL_TYPE The buffer type this texture uses
 */
template <PixelType TPIXEL_TYPE> class TextureBaseTyped : public virtual TextureBase
{
  public:
    constexpr static PixelType PIXEL_TYPE = TPIXEL_TYPE;

    PixelType getPixelType() const override { return PIXEL_TYPE; }

    virtual CompatiblePixelFormat<PIXEL_TYPE> getPixelFormat() const = 0;

    // Always convert from getPixelFormat()
    AnyPixelFormat getAnyPixelFormat() const override
    {
        return std::visit([](auto compatible_format) { return AnyPixelFormat{compatible_format}; },
                          getPixelFormat());
    }
};

/**
 * A seed for any image type
 * @tparam TextureT the image type
 * @example @code TextureSeed<Texture2D<PixelType::COLOR_TRANSPARENT>> @endcode
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
 * @example @code TextureManager<Texture2D<PixelType::COLOR_TRANSPARENT>> @endcode
 */
template <class TextureT> using TextureManager = dynasma::AbstractManager<TextureSeed<TextureT>>;

/**
 * A 1D image of any type
 */
class Texture1DBase : public virtual TextureBase
{
  public:
    virtual unsigned int getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1, 1, 1}; }
    std::size_t getNumDimensions() const override { return 1; }
};

/**
 * A 1D image of a concrete type
 */
template <PixelType TPIXEL_TYPE>
class Texture1D : public Texture1DBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, unsigned int>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImage1D<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImage1D<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A 2D image of any type
 */
class Texture2DBase : public virtual TextureBase
{
  public:
    virtual glm::uvec2 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }
};

/**
 * A 2D image of a concrete type
 */
template <PixelType TPIXEL_TYPE>
class Texture2D : public Texture2DBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImage2D<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImage2D<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A 3D image of any type
 */
class Texture3DBase : public virtual TextureBase
{
  public:
    virtual glm::uvec3 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1}; }
    std::size_t getNumDimensions() const override { return 3; }
};

/**
 * A 3D image of a concrete type
 */
template <PixelType TPIXEL_TYPE>
class Texture3D : public Texture3DBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImage3D<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImage3D<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A 3D collection of 6 2D images used for cubemapping of any type
 */
class TextureCubemapBase : public virtual TextureBase
{
  public:
    virtual glm::uvec3 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1}; }
    std::size_t getNumDimensions() const override { return 3; }
};

/**
 * A 3D collection of 6 2D images used for cubemapping of a concrete type
 */
template <PixelType TPIXEL_TYPE>
class TextureCubemap : public TextureCubemapBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, unsigned int>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to a Texture2D face of this cubemap
     */
    virtual dynasma::PinPtr<Texture2D<TPIXEL_TYPE>> getFace(Side side) = 0;
    virtual dynasma::PinPtr<const Texture2D<TPIXEL_TYPE>> getFace(Side side) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImageCubemap<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImageCubemap<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A list of 1D images, each a layer of 1 asset of any type
 */
class Texture1DLayeredBase : public virtual TextureBase
{
  public:
    virtual glm::uvec2 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }
};

/**
 * A list of 1D images, each a layer of 1 asset of a concrete type
 */
template <PixelType TPIXEL_TYPE>
class Texture1DLayered : public Texture1DLayeredBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to a Texture1D layer
     */
    virtual dynasma::PinPtr<Texture1D<TPIXEL_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::PinPtr<const Texture1D<TPIXEL_TYPE>> getLayer(std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImage1DLayered<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImage1DLayered<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A list of 2D images
 */
class Texture2DLayeredBase : public virtual TextureBase
{
  public:
    virtual glm::uvec3 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return glm::uvec4{getSize(), 1}; }
    std::size_t getNumDimensions() const override { return 3; }
};

/**
 * A list of 2D images
 */
template <PixelType TPIXEL_TYPE>
class Texture2DLayered : public Texture2DLayeredBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to a Texture2D layer
     */
    virtual dynasma::PinPtr<Texture2D<TPIXEL_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::PinPtr<const Texture2D<TPIXEL_TYPE>> getLayer(std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImage2DLayered<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImage2DLayered<TPIXEL_TYPE>> getBuffer() const = 0;
};

/**
 * A list of cubemap images
 */
class TextureCubemapLayeredBase : public virtual TextureBase
{
  public:
    virtual glm::uvec4 getSize() const = 0;
    glm::uvec4 getNDSize() const override { return getSize(); }
    std::size_t getNumDimensions() const override { return 4; }
};

/**
 * A list of cubemap images
 */
template <PixelType TPIXEL_TYPE>
class TextureCubemapLayered : public TextureCubemapLayeredBase, public TextureBaseTyped<TPIXEL_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TPIXEL_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TPIXEL_TYPE>;

    dynasma::PinPtr<PixelImageBase> getBufferBase() override { return getBuffer(); }
    dynasma::PinPtr<const PixelImageBase> getBufferBase() const override { return getBuffer(); }

    /**
     * @returns pointer to a TextureCubemap layer
     */
    virtual dynasma::PinPtr<TextureCubemap<TPIXEL_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::PinPtr<const TextureCubemap<TPIXEL_TYPE>> getLayer(
        std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::PinPtr<PixelImageCubemapLayered<TPIXEL_TYPE>> getBuffer() = 0;
    virtual dynasma::PinPtr<const PixelImageCubemapLayered<TPIXEL_TYPE>> getBuffer() const = 0;
};

// ==== Helpers for handling all these types =======================================================

/**
 * Calls the templated visitor on all Texture_<PixelType> templates
 * @param visitor its operator() has to accept a Texture_<PixelType> template as its parameter
 * @param args The arguments to pass to the visitor
 * @note You can use a template template parametrized lambda for this
 * @example @code
 *  forTextureTemplates(
 *      []<template<PixelType> class Texture>(std::string_view str) {
 *          std::print("{}{}\n", str, TYPE_INFO<Texture<PixelType::REAL_SCALAR>>.getShortName())
 *      },
 *      "TexTp: "
 *  );
 * @endcode
 */
template <class VisitorT, typename... ArgTs>
constexpr void forTextureTemplates(VisitorT &&visitor, ArgTs &&...args);

/**
 * @note You can use a template lambda for this
 * @example @code
 *  forTextureTypes(
 *      []<class Texture>(std::string_view str) {
 *          std::print("{}{}\n", str, TYPE_INFO<Texture>.getShortName())
 *      },
 *      "TexTp: "
 *  );
 * @endcode
 */
template <class VisitorT, typename... ArgTs>
constexpr void forTextureTypes(VisitorT &&visitor, ArgTs &&...args);

// ==== Helper implementation ======================================================================

template <class VisitorT, typename... ArgTs>
constexpr void forTextureTemplates(VisitorT &&visitor, ArgTs &&...args)
{
    visitor.template operator()<Texture1D>(std::forward<ArgTs>(args)...);
    visitor.template operator()<Texture2D>(std::forward<ArgTs>(args)...);
    visitor.template operator()<Texture3D>(std::forward<ArgTs>(args)...);
    visitor.template operator()<TextureCubemap>(std::forward<ArgTs>(args)...);
    visitor.template operator()<Texture1DLayered>(std::forward<ArgTs>(args)...);
    visitor.template operator()<Texture2DLayered>(std::forward<ArgTs>(args)...);
    visitor.template operator()<TextureCubemapLayered>(std::forward<ArgTs>(args)...);
}

template <class VisitorT, typename... ArgTs>
constexpr void forTextureTypes(VisitorT &&visitor, ArgTs &&...args)
{
    forTextureTemplates([&]<template <PixelType> class Texture> {
        forPixelTypes([&]<PixelType BT> {
            visitor.template operator()<Texture<BT>>(std::forward<ArgTs>(args)...);
        });
    });
}

} // namespace Vitrae