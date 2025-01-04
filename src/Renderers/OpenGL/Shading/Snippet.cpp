#include "Vitrae/Renderers/OpenGL/Shading/Snippet.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/ConstantDefs.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <fstream>

namespace Vitrae {

OpenGLShaderSnippet::OpenGLShaderSnippet(const StringParams &params)
    : m_inputSpecs(params.inputSpecs), m_outputSpecs(params.outputSpecs),
      m_filterSpecs(params.filterSpecs), m_consumingSpecs(params.consumingSpecs),
      m_snippet(clearIndents(params.snippet))
{
    m_friendlyName = String(GLSL_SHADER_INTERNAL_FUNCTION_PREFIX) + "calc";

    for (const auto &spec : params.outputSpecs.getSpecList()) {
        m_friendlyName += "_" + spec.name;
    }
}

std::size_t OpenGLShaderSnippet::memory_cost() const
{
    /// TODO: Calculate the cost of the function
    return 1;
}

const PropertyList &OpenGLShaderSnippet::getInputSpecs(const PropertyAliases &) const
{
    return m_inputSpecs;
}

const PropertyList &OpenGLShaderSnippet::getOutputSpecs() const
{
    return m_outputSpecs;
}

const PropertyList &OpenGLShaderSnippet::getFilterSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &OpenGLShaderSnippet::getConsumingSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void OpenGLShaderSnippet::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                           const PropertyAliases &aliases) const
{
    for (auto p_specs : {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs, &m_consumingSpecs}) {
        for (auto spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLShaderSnippet::extractSubTasks(std::set<const Task *> &taskSet,
                                          const PropertyAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLShaderSnippet::outputDeclarationCode(BuildContext args) const {}

void OpenGLShaderSnippet::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderSnippet::outputUsageCode(BuildContext args) const
{
    args.output << " {\n";
    args.output << m_snippet;
    args.output << "}\n";
}

StringView OpenGLShaderSnippet::getFriendlyName() const {
    return m_friendlyName;
}

} // namespace Vitrae
