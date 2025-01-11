#include "Vitrae/Renderers/OpenGL/Shading/Constant.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

namespace Vitrae
{
OpenGLShaderConstant::OpenGLShaderConstant(const SetupParams &params)
    : m_outputSpecs({params.outputSpec}), m_value(params.value),
      m_friendlyName(params.outputSpec.name + "\n" + params.value.toString())
{}

std::size_t OpenGLShaderConstant::memory_cost() const
{
    /// TODO: Calculate the cost of the constant
    return 1;
}

const ParamList &OpenGLShaderConstant::getInputSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &OpenGLShaderConstant::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &OpenGLShaderConstant::getFilterSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &OpenGLShaderConstant::getConsumingSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void OpenGLShaderConstant::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                            const ParamAliases &aliases) const
{
    // We have only one output spec
    const ParamSpec &outputSpec = m_outputSpecs.getSpecList().front();

    typeSet.insert(&outputSpec.typeInfo);
}
void OpenGLShaderConstant::extractSubTasks(std::set<const Task *> &taskSet,
                                           const ParamAliases &aliases) const
{
    taskSet.insert(this);
}
void OpenGLShaderConstant::outputDeclarationCode(BuildContext args) const {}

void OpenGLShaderConstant::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderConstant::outputUsageCode(BuildContext args) const
{
    // We have only one output spec
    const ParamSpec &outputSpec = m_outputSpecs.getSpecList().front();

    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);
    const GLTypeSpec &glTypeSpec = renderer.getTypeConversion(outputSpec.typeInfo).glTypeSpec;

    args.output << "const " << glTypeSpec.valueTypeName << " " << outputSpec.name << " = "
                << m_value.toString() << ";\n";
}

StringView OpenGLShaderConstant::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae