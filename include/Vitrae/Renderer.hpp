#pragma once

#include "Vitrae/Data/Typedefs.hpp"

#include "dynasma/managers/abstract.hpp"

namespace Vitrae
{
class ComponentRoot;

class Renderer
{
  public:
    virtual ~Renderer() = default;

    virtual void mainThreadSetup(ComponentRoot &root) = 0;
    virtual void mainThreadFree() = 0;
    virtual void mainThreadUpdate() = 0;

    virtual void anyThreadEnable() = 0;
    virtual void anyThreadDisable() = 0;

    virtual void specifyVertexBuffer(const ParamSpec &newElSpec) = 0;

    /**
     * Specifies a texture type that will be used.
     * In shaders, sample_<colorName> can be used to retrieve the fragment's color in this texture,
     * if tex_<colorName> and coord_<colorName> are set,
     * or if sample_<colorName> is aliased to another value.
     * @param colorName The name of the color the texture represents
     * @note Automatically called when Material::setTexture is called
     */
    virtual void specifyTextureSampler(StringView colorName) = 0;
};

} // namespace Vitrae