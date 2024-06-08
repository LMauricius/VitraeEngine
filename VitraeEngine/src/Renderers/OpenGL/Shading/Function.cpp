#include "Vitrae/Renderers/OpenGL/Shading/Function.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include <fstream>

namespace Vitrae
{

OpenGLShaderFunction::OpenGLShaderFunction(const SetupParams &params)
    : ShaderFunction(params), m_functionName(params.functionName)
{
    for (const auto &spec : params.inputSpecs) {
        m_inputOrder.emplace_back(spec.name);
    }
    for (const auto &spec : params.outputSpecs) {
        m_outputOrder.emplace_back(spec.name);
    }

    std::ifstream stream(params.filepath);
    std::ostringstream sstr;
    sstr << stream.rdbuf();
    m_fileSnippet = sstr.str();
}

std::size_t OpenGLShaderFunction::memory_cost() const
{
    /// TODO: Calculate the cost of the function
    return 1;
}

void OpenGLShaderFunction::outputDeclarationCode(BuildContext args) const
{
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);

    args.output << "void " << m_functionName << "(";
    bool hadFirstArg = false;
    for (const auto &nameId : m_inputOrder) {
        const PropertySpec &spec = m_inputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec = renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (hadFirstArg) {
            args.output << ", ";
        }
        args.output << glTypeSpec.glTypeName << " i_" << spec.name;
        hadFirstArg = true;
    }
    hadFirstArg = false;
    for (const auto &nameId : m_outputOrder) {
        const PropertySpec &spec = m_outputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec = renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (hadFirstArg) {
            args.output << ", ";
        }
        args.output << "out " << glTypeSpec.glTypeName << " o_" << spec.name;
        hadFirstArg = true;
    }
    args.output << ");\n";
}

void OpenGLShaderFunction::outputDefinitionCode(BuildContext args) const
{
    args.output << m_fileSnippet;
}

void OpenGLShaderFunction::outputUsageCode(
    BuildContext args, const std::map<StringId, String> &inputParamsToSharedVariables,
    const std::map<StringId, String> &outputParamsToSharedVariables) const
{
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);

    args.output << m_functionName << "(";
    bool hadFirstArg = false;
    for (const auto &nameId : m_inputOrder) {
        if (hadFirstArg) {
            args.output << ", ";
        }
        args.output << inputParamsToSharedVariables.at(nameId);
        hadFirstArg = true;
    }
    hadFirstArg = false;
    for (const auto &nameId : m_outputOrder) {
        if (hadFirstArg) {
            args.output << ", ";
        }
        args.output << inputParamsToSharedVariables.at(nameId);
        hadFirstArg = true;
    }
    args.output << ");\n";
}

void OpenGLShaderFunction::hookSetupFunctions(SetupContext args) const {}

} // namespace Vitrae