#pragma once

#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

#include "glm/glm.hpp"

namespace Vitrae
{

class Renderer;

class ShaderTask : public Task
{
  public:
    using InputSpecsDeducingContext = std::monostate;

    struct BuildContext
    {
        std::stringstream &output;
        Renderer &renderer;
    };

    struct RunContext
    {
        const ScopedDict &properties;
        Renderer &renderer;
    };

    struct SetupContext
    {
        std::vector<std::function<void(RunContext)>> setupFunctions;
        Renderer &renderer;
    };

    using Task::Task;

    inline const StableMap<StringId, PropertySpec> &getInputSpecs(InputSpecsDeducingContext) const
    {
        return m_inputSpecs;
    }
    inline const StableMap<StringId, PropertySpec> &getOutputSpecs() const { return m_outputSpecs; }

    virtual void outputDeclarationCode(BuildContext args) const = 0;
    virtual void outputDefinitionCode(BuildContext args) const = 0;
    virtual void outputUsageCode(
        BuildContext args, const StableMap<StringId, String> &inputParamsToSharedVariables,
        const StableMap<StringId, String> &outputParamsToSharedVariables) const = 0;
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
static constexpr const TypeInfo &INPUT_VIEW = Variant::getTypeInfo<glm::mat4>();
static constexpr const TypeInfo &INPUT_PROJECTION = Variant::getTypeInfo<glm::mat4>();
static constexpr const TypeInfo &INPUT_MODEL = Variant::getTypeInfo<glm::mat4>();

static constexpr const TypeInfo &FRAGMENT_OUTPUT = Variant::getTypeInfo<glm::vec4>();
static constexpr const TypeInfo &VERTEX_OUTPUT = Variant::getTypeInfo<glm::vec4>();
}

} // namespace Vitrae