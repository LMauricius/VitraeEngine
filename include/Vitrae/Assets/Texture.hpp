#pragma once

#include "Vitrae/Assets/TensorBuffer.hpp"
#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/Sides.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
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
 * Base for any TextureBase type that has this BufferType
 * @tparam BUFFER_TYPE The buffer type this texture uses
 */
template <BufferType TBUFFER_TYPE> class TextureBaseTyped : public TextureBase
{
  public:
    constexpr static BufferType BUFFER_TYPE = TBUFFER_TYPE;

    BufferType getBufferType() const override { return BUFFER_TYPE; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

    // Always convert from getBufferFormat()
    AnyBufferFormat getAnyBufferFormat() const override
    {
        return std::visit([](auto compatible_format) { return AnyBufferFormat{compatible_format}; },
                          getBufferFormat());
    }
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
 * A 1D image of any type
 */
class Texture1DBase : public TextureBase
{
  public:
    unsigned int getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1, 1}; }
    std::size_t getNumDimensions() const override { return 1; }

  protected:
    unsigned int m_size;
};

/**
 * A 1D image of a concrete type
 */
template <BufferType TBUFFER_TYPE>
class Texture1D : public Texture1DBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, unsigned int>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBuffer1D<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBuffer1D<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;

  protected:
    unsigned int m_size;
};

/**
 * A 2D image of any type
 */
class Texture2DBase : public TextureBase
{
  public:
    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A 2D image of a concrete type
 */
template <BufferType TBUFFER_TYPE>
class Texture2D : public Texture2DBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBuffer2D<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBuffer2D<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;
};

/**
 * A 3D image of any type
 */
class Texture3DBase : public TextureBase
{
  public:
    glm::uvec3 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec3 m_size;
};

/**
 * A 3D image of a concrete type
 */
template <BufferType TBUFFER_TYPE>
class Texture3D : public Texture3DBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBuffer3D<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBuffer3D<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;
};

/**
 * A 3D collection of 6 2D images used for cubemapping of any type
 */
class TextureCubemapBase : public TextureBase
{
  public:
    glm::uvec2 getSize() const { return {m_size, m_size}; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, m_size, 6, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    unsigned int m_size;
};

/**
 * A 3D collection of 6 2D images used for cubemapping of a concrete type
 */
template <BufferType TBUFFER_TYPE>
class TextureCubemap : public TextureCubemapBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to a Texture2D face of this cubemap
     */
    virtual dynasma::SharedPtr<Texture2D<TBUFFER_TYPE>> getFace(Side side) = 0;
    virtual dynasma::SharedPtr<const Texture2D<TBUFFER_TYPE>> getFace(Side side) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBufferCubemap<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBufferCubemap<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;
};

/**
 * A list of 1D images, each a layer of 1 asset of any type
 */
class Texture1DArrayBase : public TextureBase
{
  public:
    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 2; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 1D images, each a layer of 1 asset of a concrete type
 */
template <BufferType TBUFFER_TYPE>
class Texture1DArray : public Texture1DArrayBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to a Texture1D layer
     */
    virtual dynasma::SharedPtr<Texture1D<TBUFFER_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::SharedPtr<const Texture1D<TBUFFER_TYPE>> getLayer(std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBuffer1DArray<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBuffer1DArray<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;
};

/**
 * A list of 2D images
 */
class Texture2DArrayBase : public TextureBase
{
  public:
    glm::uvec2 getSize() const { return m_size; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size, 1, 1}; }
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 2D images
 */
template <BufferType TBUFFER_TYPE>
class Texture2DArray : public Texture2DArrayBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec2>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to a Texture2D layer
     */
    virtual dynasma::SharedPtr<Texture2D<TBUFFER_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::SharedPtr<const Texture2D<TBUFFER_TYPE>> getLayer(std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBuffer2DArray<BufferValueType<TBUFFER_TYPE>>> getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBuffer2DArray<BufferValueType<TBUFFER_TYPE>>> getBuffer()
        const = 0;
};

/**
 * A list of cubemap images
 */
class TextureCubemapArrayBase : public TextureBase
{
  public:
    glm::uvec3 getSize() const { return {m_size.x, m_size.x, m_size.y}; }
    glm::uvec4 getNDSize() const override { return glm::uvec4{m_size.x, m_size.x, 6, m_size.y}; }
    std::size_t getNumDimensions() const override { return 4; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of cubemap images
 */
template <BufferType TBUFFER_TYPE>
class TextureCubemapArray : public TextureCubemapArrayBase, public TextureBaseTyped<TBUFFER_TYPE>
{
  public:
    using FileLoadParams = ImageCommon::FileLoadParams;
    using EmptyParams = ImageCommon::EmptyParams<TBUFFER_TYPE, glm::uvec3>;
    using PureColorParams = ImageCommon::PureColorParams<TBUFFER_TYPE>;

    /**
     * @returns pointer to a TextureCubemap layer
     */
    virtual dynasma::SharedPtr<TextureCubemap<TBUFFER_TYPE>> getLayer(std::size_t layer) = 0;
    virtual dynasma::SharedPtr<const TextureCubemap<TBUFFER_TYPE>> getLayer(
        std::size_t layer) const = 0;

    /**
     * @returns pointer to the buffer
     */
    virtual dynasma::SharedPtr<TensorBufferCubemapArray<BufferValueType<TBUFFER_TYPE>>>
    getBuffer() = 0;
    virtual dynasma::SharedPtr<const TensorBufferCubemapArray<BufferValueType<TBUFFER_TYPE>>>
    getBuffer() const = 0;
};

// ==== Helpers for handling all these types =======================================================

/**
 * Calls the templated visitor on all Texture_<BufferType> templates
 * @param visitor its operator() has to accept a Texture_<BufferType> template as its parameter
 * @param args The arguments to pass to the visitor
 * @note You can use a template template parametrized lambda for this
 * @example @code
 *  forTextureTemplates(
 *      []<template<BufferType> class Texture>(std::string_view str) {
 *          std::print("{}{}\n", str, TYPE_INFO<Texture<BufferType::REAL_SCALAR>>.getShortName())
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
    visitor.template operator()<Texture1DArray>(std::forward<ArgTs>(args)...);
    visitor.template operator()<Texture2DArray>(std::forward<ArgTs>(args)...);
    visitor.template operator()<TextureCubemapArray>(std::forward<ArgTs>(args)...);
}

template <class VisitorT, typename... ArgTs>
constexpr void forTextureTypes(VisitorT &&visitor, ArgTs &&...args)
{
    forTextureTemplates([&]<template <BufferType> class Texture> {
        forBufferTypes([&]<BufferType BT> {
            visitor.template operator()<Texture<BT>>(std::forward<ArgTs>(args)...);
        });
    });
}

} // namespace Vitrae