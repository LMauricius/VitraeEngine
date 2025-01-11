#include "Vitrae/Renderers/OpenGL/Shading/FunctionCall.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <fstream>

namespace Vitrae {

OpenGLShaderFunctionCall::OpenGLShaderFunctionCall(const StringParams &params)
    : m_inputSpecs(params.inputSpecs), m_filterSpecs(params.filterSpecs),
      m_outputSpecs(params.outputSpecs), m_consumingSpecs(params.consumingSpecs),
      m_functionName(params.functionName)
{}

std::size_t OpenGLShaderFunctionCall::memory_cost() const
{
    /// TODO: Calculate the cost of the function
    return 1;
}

const ParamList &OpenGLShaderFunctionCall::getInputSpecs(const ParamAliases &) const
{
    return m_inputSpecs;
}

const ParamList &OpenGLShaderFunctionCall::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &OpenGLShaderFunctionCall::getFilterSpecs(const ParamAliases &) const
{
    return m_filterSpecs;
}

const ParamList &OpenGLShaderFunctionCall::getConsumingSpecs(const ParamAliases &) const
{
    return m_consumingSpecs;
}

void OpenGLShaderFunctionCall::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                                const ParamAliases &aliases) const
{
    for (auto p_specs : {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs, &m_consumingSpecs}) {
        for (auto &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLShaderFunctionCall::extractSubTasks(std::set<const Task *> &taskSet,
                                               const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLShaderFunctionCall::outputDeclarationCode(BuildContext args) const {}

void OpenGLShaderFunctionCall::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderFunctionCall::outputUsageCode(BuildContext args) const
{
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);

    args.output << m_functionName << "(";

    bool hadFirstArg = false;
    for (auto p_specs : {&m_consumingSpecs, &m_inputSpecs, &m_filterSpecs, &m_outputSpecs}) {
        for (auto &spec : p_specs->getSpecList()) {
            const GLTypeSpec &glTypeSpec = renderer.getTypeConversion(spec.typeInfo).glTypeSpec;

            if (glTypeSpec.valueTypeName.size() > 0 || glTypeSpec.opaqueTypeName.size() > 0) {
                if (hadFirstArg) {
                    args.output << ", ";
                }
                args.output << args.aliases.choiceStringFor(spec.name);
                hadFirstArg = true;
            }
        }
    }

    args.output << ");";
}

StringView OpenGLShaderFunctionCall::getFriendlyName() const {
    return m_functionName;
}

} // namespace Vitrae
