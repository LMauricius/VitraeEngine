#pragma once

#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Pipelines/Task.hpp"

#include "glm/glm.hpp"

namespace Vitrae
{

class ComponentRoot;
class Renderer;

class ShaderTask : public Task
{
  public:
    struct BuildContext
    {
        std::stringstream &output;
        ComponentRoot &root;
        Renderer &renderer;
        ParamAliases &aliases;
    };

    virtual void outputDeclarationCode(BuildContext args) const = 0;
    virtual void outputDefinitionCode(BuildContext args) const = 0;
    virtual void outputUsageCode(BuildContext args) const = 0;
};

} // namespace Vitrae