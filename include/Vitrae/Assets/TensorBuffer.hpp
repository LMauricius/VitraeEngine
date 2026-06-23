#pragma once

#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Data/Sides.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Dynamic/TypeMeta/Tensor.hpp"
#include "dynasma/pin.hpp"
#include "glm/glm.hpp"

namespace Vitrae
{
class ComponentRoot;

/**
 * A tensor buffer is a multi-dimensional buffer that has tensors as its elements.
 * Unlike SharedBuffers which map data directly, it gives more control over data formatting and
 * supports 1-3 dimensions and a number of layers as an additional dimension. While SharedBuffers
 * support any kind of structures as data, TensorBuffers have a limitation that their data need to
 * be tensor-like structures serialized using vectors as their basic building blocks.
 *
 * TensorBuffers are how texture data is stored, but can be used by themselves for more complex
 * tensors.
 * @see SharedBuffer
 */
class TensorBufferBase : public dynasma::PolymorphicBase
{
  public:
    virtual ~TensorBufferBase() = default;

    virtual std::size_t memory_cost() const = 0;

    /**
     * @return The element type info
     */
    virtual const TypeInfo &getElementType() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    virtual glm::uvec4 getNDSize() const = 0;

    /**
     * @return The number of buffer dimensions
     */
    virtual std::size_t getNumDimensions() const = 0;

    /**
     * @return The format of the buffer, among all PixelFormats
     */
    virtual AnyPixelFormat getAnyPixelFormat() const = 0;
};

/**
 * Base for any TensorBufferBase type that has this Tensor type
 * @tparam TPIXEL_TYPE The element type of the buffer
 */
template <PixelType TPIXEL_TYPE> class TensorBufferBaseTyped : public TensorBufferBase
{
  public:
    /// The element type
    constexpr static auto PIXEL_TYPE = TPIXEL_TYPE;

    /**
     * @return The element type info
     */
    const TypeInfo &getElementType() const override
    {
        return TYPE_INFO<PixelValueType<PIXEL_TYPE>>;
    }

    /**
     * @return The format of the buffer, among all PixelFormats
     */
    AnyPixelFormat getAnyPixelFormat() const override
    {
        // Always convert from getPixelFormat()
        return std::visit([](auto compatible_format) { return AnyPixelFormat{compatible_format}; },
                          getPixelFormat());
    }

    /**
     * @return The format of the buffer among those compatible with the element type
     */
    virtual CompatiblePixelFormat<PIXEL_TYPE> getPixelFormat() const = 0;
};

/**
 * A 1D TensorBuffer
 */
template <PixelType TPIXEL_TYPE> class TensorBuffer1D : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual unsigned int getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1, 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 1; }
};

/**
 * A 2D TensorBuffer
 */
template <PixelType TPIXEL_TYPE> class TensorBuffer2D : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec2 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 2; }
};

/**
 * A 3D TensorBuffer
 */
template <PixelType TPIXEL_TYPE> class TensorBuffer3D : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec3 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }
};

/**
 * A 3D collection of 6 2D TensorBuffers
 */
template <PixelType TPIXEL_TYPE>
class TensorBufferCubemap : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec3 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }

    /**
     * @returns pointer to a TensorBuffer2D face of this cubemap
     */
    virtual dynasma::PinPtr<TensorBuffer2D<TPIXEL_TYPE>> getFace(Side side) = 0;
    virtual dynasma::PinPtr<const TensorBuffer2D<TPIXEL_TYPE>> getFace(Side side) const = 0;
};

/**
 * A list of 1D TensorBuffers of shared size
 */
template <PixelType TPIXEL_TYPE>
class TensorBuffer1DLayered : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec2 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 2; }

    /**
     * @returns pointer to a 1D TensorBuffer layer of this multi-layer
     */
    virtual dynasma::PinPtr<TensorBuffer1D<TPIXEL_TYPE>> getLayer(std::size_t y) = 0;
    virtual dynasma::PinPtr<const TensorBuffer1D<TPIXEL_TYPE>> getLayer(std::size_t y) const = 0;
};

/**
 * A list of 2D TensorBuffers of shared size
 */
template <PixelType TPIXEL_TYPE>
class TensorBuffer2DLayered : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec3 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {getSize(), 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }

    /**
     * @returns pointer to a 2D TensorBuffer layer of this multi-layer
     */
    virtual dynasma::PinPtr<TensorBuffer2D<TPIXEL_TYPE>> getLayer(std::size_t z) = 0;
    virtual dynasma::PinPtr<const TensorBuffer2D<TPIXEL_TYPE>> getLayer(std::size_t z) const = 0;
};

/**
 * A list of Cubemap TensorBuffers of shared size
 */
template <PixelType TPIXEL_TYPE>
class TensorBufferCubemapLayered : public TensorBufferBaseTyped<TPIXEL_TYPE>
{
  public:
    /**
     * @return The size of the buffer
     */
    virtual glm::uvec4 getSize() const = 0;

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return getSize(); }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 4; }

    /**
     * @returns pointer to a Cubemap TensorBuffer layer of this multi-layer
     */
    virtual dynasma::PinPtr<TensorBufferCubemap<TPIXEL_TYPE>> getLayer(std::size_t w) = 0;
    virtual dynasma::PinPtr<const TensorBufferCubemap<TPIXEL_TYPE>> getLayer(
        std::size_t w) const = 0;
};

} // namespace Vitrae