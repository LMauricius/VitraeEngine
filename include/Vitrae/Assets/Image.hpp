#pragma once

#include "Vitrae/Data/BufferFormat.hpp"
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
    virtual ~ImageBase() = default;

    virtual std::size_t memory_cost() const = 0;

    virtual glm::uvec2 getNDSize() const = 0;
    virtual std::size_t getNumDimensions() const = 0;

    virtual AnyBufferFormat getAnyBufferFormat() const = 0;
};

/**
 * A 1D image
 */
template <BufferType BUFFER_TYPE> class Image1D : public ImageBase
{
  public:
    inline unsigned int getSize() const { return m_size; }
    glm::uvec2 getNDSize() const override { return glm::uvec2{m_size, 1}; }
    std::size_t getNumDimensions() const override { return 1; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

  protected:
    unsigned int m_size;
};

/**
 * A 2D image
 */
template <BufferType BUFFER_TYPE> class Image2D : public ImageBase
{
  public:
    inline glm::uvec2 getSize() const { return m_size; }
    glm::uvec2 getNDSize() const override { return m_size; }
    std::size_t getNumDimensions() const override { return 2; }

    virtual CompatibleBufferFormat<BUFFER_TYPE> getBufferFormat() const = 0;

  protected:
    glm::uvec2 m_size;
};

} // namespace Vitrae