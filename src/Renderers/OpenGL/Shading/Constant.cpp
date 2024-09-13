#include "Vitrae/Renderers/OpenGL/Shading/Constant.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

namespace Vitrae
{
OpenGLShaderConstant::OpenGLShaderConstant(const SetupParams &params) : ShaderConstant(params) {}

std::size_t OpenGLShaderConstant::memory_cost() const
{
    /// TODO: Calculate the cost of the constant
    return 1;
}

void OpenGLShaderConstant::extractUsedTypes(std::set<const TypeInfo *> &typeSet) const
{
    typeSet.insert(&m_outputSpec.typeInfo);
}
void OpenGLShaderConstant::extractSubTasks(std::set<const Task *> &taskSet) const
{
    taskSet.insert(this);
}
void OpenGLShaderConstant::outputDeclarationCode(BuildContext args) const
{
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);
    const GLTypeSpec &glTypeSpec = renderer.getTypeConversion(m_outputSpec.typeInfo).glTypeSpec;

    args.output << "const " << glTypeSpec.glMutableTypeName << " C_" << m_outputSpec.name << " = "
                << m_value.toString() << ";\n";
}

void OpenGLShaderConstant::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderConstant::outputUsageCode(
    BuildContext args, const StableMap<StringId, String> &inputParamsToSharedVariables,
    const StableMap<StringId, String> &outputParamsToSharedVariables) const
{
    args.output << outputParamsToSharedVariables.at(m_outputNameId) << " = C_" << m_outputSpec.name
                << ";\n";
}

} // namespace Vitrae