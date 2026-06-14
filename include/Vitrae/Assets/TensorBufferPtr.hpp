#pragma once

#include "Vitrae/Data/PixelTyping.hpp"
#include "Vitrae/Data/Sides.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Dynamic/TypeMeta/Tensor.hpp"
#include "dynasma/indirect.hpp"
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
 * @tparam TElementType The element type of the buffer
 */
template <typename TElementType> class TensorBufferBaseTyped : public TensorBufferBase
{
  public:
    static_assert(Tensor<TElementType>,
                  "ElementType must be a Tensor type according to TYPE_META<ElementType>");

    /// The element type
    using ElementType = TElementType;

    /**
     * @return The element type info
     */
    const TypeInfo &getElementType() const override { return TYPE_INFO<ElementType>; }

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
    virtual CompatiblePixelFormat<TYPE_META<ElementType>.CORE_VECTOR_KIND> getPixelFormat()
        const = 0;
};

/**
 * A 1D TensorBuffer
 */
template <typename TElementType> class TensorBuffer1D : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline unsigned int getSize() const { return m_size; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, 1, 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 1; }

  protected:
    unsigned int m_size;
};

/**
 * A 2D TensorBuffer
 */
template <typename TElementType> class TensorBuffer2D : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec2 getSize() const { return m_size; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 2; }

  protected:
    glm::uvec2 m_size;
};

/**
 * A 3D TensorBuffer
 */
template <typename TElementType> class TensorBuffer3D : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec3 getSize() const { return m_size; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }

  protected:
    glm::uvec3 m_size;
};

/**
 * A 3D collection of 6 2D TensorBuffers
 */
template <typename TElementType>
class TensorBufferCubemap : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec2 getSize() const { return {m_size, m_size}; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, m_size, 6, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }

    /**
     * @returns pointer to a TensorBuffer2D face of this cubemap
     */
    virtual dynasma::IndirectPtr<TensorBuffer2D<TElementType>> getFace(Side side) = 0;
    virtual dynasma::IndirectPtr<const TensorBuffer2D<TElementType>> getFace(Side side) const = 0;

  protected:
    unsigned int m_size;
};

/**
 * A list of 1D TensorBuffers of shared size
 */
template <typename TElementType>
class TensorBuffer1DLayered : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec2 getSize() const { return {m_size}; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, 1, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 2; }

    /**
     * @returns pointer to a 1D TensorBuffer layer of this multi-layer
     */
    virtual dynasma::IndirectPtr<TensorBuffer1D<TElementType>> getLayer(std::size_t y) = 0;
    virtual dynasma::IndirectPtr<const TensorBuffer1D<TElementType>> getLayer(
        std::size_t y) const = 0;

  protected:
    glm::uvec2 m_size;
};

/**
 * A list of 2D TensorBuffers of shared size
 */
template <typename TElementType>
class TensorBuffer2DLayered : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec3 getSize() const { return {m_size}; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size, 1}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 3; }

    /**
     * @returns pointer to a 2D TensorBuffer layer of this multi-layer
     */
    virtual dynasma::IndirectPtr<TensorBuffer2D<TElementType>> getLayer(std::size_t z) = 0;
    virtual dynasma::IndirectPtr<const TensorBuffer2D<TElementType>> getLayer(
        std::size_t z) const = 0;

  protected:
    glm::uvec3 m_size;
};

/**
 * A list of Cubemap TensorBuffers of shared size
 */
template <typename TElementType>
class TensorBufferCubemapLayered : public TensorBufferBaseTyped<TElementType>
{
  public:
    /**
     * @return The size of the buffer
     */
    inline glm::uvec4 getSize() const { return {m_size.x, m_size.x, 6, m_size.y}; }

    /**
     * @return The size of the buffer, in 4 dimensions
     * @note In unused dimensions the size is 1
     */
    glm::uvec4 getNDSize() const override { return {m_size.x, m_size.x, 6, m_size.y}; }

    /**
     * @return The number of buffer dimensions
     */
    std::size_t getNumDimensions() const override { return 4; }

    /**
     * @returns pointer to a Cubemap TensorBuffer layer of this multi-layer
     */
    virtual dynasma::IndirectPtr<TensorBufferCubemap<TElementType>> getLayer(std::size_t w) = 0;
    virtual dynasma::IndirectPtr<const TensorBufferCubemap<TElementType>> getLayer(
        std::size_t w) const = 0;

  protected:
    glm::uvec2 m_size;
};

} // namespace Vitrae