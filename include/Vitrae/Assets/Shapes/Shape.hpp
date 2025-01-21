#pragma once

#include "Vitrae/Data/BoundingBox.hpp"
#include "Vitrae/Data/StringId.hpp"

#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

namespace Vitrae
{
class ComponentRoot;
class Material;
class Renderer;
class ParamList;

/**
 * A shape is a renderable object
 */
class Shape : public dynasma::PolymorphicBase
{
  public:
    virtual ~Shape() = default;

    virtual std::size_t memory_cost() const = 0;

    virtual BoundingBox getBoundingBox() const = 0;

    /**
     * Ensures (preferably) that this shape has the specified components ready
     * @param components List of params, such as 'position', 'normal' etc.
     * @note For mesh-like shapes, it is possible to use the MeshGeneratorCollection to generate the
     * missing components
     */
    virtual void prepareComponents(const ParamList &components) = 0;

    /**
     * Ensures that this shape is loaded to the GPU
     */
    virtual void loadToGPU(Renderer &rend) = 0;

    /**
     * Renders this shape to the FrameStore
     * The current FrameStore needs to be enabled,
     * and data such as shaders, textures, properties, etc. need to be set up beforehand
     */
    virtual void rasterize() const = 0;
};

} // namespace Vitrae