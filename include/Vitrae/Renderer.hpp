#pragma once

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Types/Typedefs.hpp"

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
};

} // namespace Vitrae