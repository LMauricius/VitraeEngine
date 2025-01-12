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

namespace StandardShaderPropertyNames
{
static constexpr const char INPUT_VIEW[] = "mat_view";
static constexpr const char INPUT_PROJECTION[] = "mat_proj";
static constexpr const char INPUT_MODEL[] = "mat_model";

static constexpr const char FRAGMENT_OUTPUT[] = "shade";
static constexpr const char COMPUTE_OUTPUT[] = "data_computed";
} // namespace StandardShaderOutputNames

namespace StandardShaderPropertyTypes
{
static constexpr const TypeInfo &INPUT_VIEW = TYPE_INFO<glm::mat4>;
static constexpr const TypeInfo &INPUT_PROJECTION = TYPE_INFO<glm::mat4>;
static constexpr const TypeInfo &INPUT_MODEL = TYPE_INFO<glm::mat4>;

static constexpr const TypeInfo &FRAGMENT_OUTPUT = TYPE_INFO<glm::vec4>;
static constexpr const TypeInfo &VERTEX_OUTPUT = TYPE_INFO<glm::vec4>;
}

} // namespace Vitrae