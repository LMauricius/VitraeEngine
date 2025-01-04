#include "Vitrae/Renderers/OpenGL/Shading/Header.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <fstream>

namespace Vitrae
{

OpenGLShaderHeader::OpenGLShaderHeader(const FileLoadParams &params)
{
    for (auto &name : params.inputTokenNames) {
        m_inputSpecs.insert_back({.name = name, .typeInfo = Variant::getTypeInfo<void>()});
    }
    for (auto &name : params.outputTokenNames) {
        m_outputSpecs.insert_back({.name = name, .typeInfo = Variant::getTypeInfo<void>()});
    }

    std::ifstream stream(params.filepath);
    std::ostringstream sstr;
    sstr << stream.rdbuf();
    m_fileSnippet = clearIndents(sstr.str());
    m_friendlyName = params.friendlyName;
}

OpenGLShaderHeader::OpenGLShaderHeader(const StringParams &params)
{
    for (auto &name : params.inputTokenNames) {
        m_inputSpecs.insert_back({.name = name, .typeInfo = Variant::getTypeInfo<void>()});
    }
    for (auto &name : params.outputTokenNames) {
        m_outputSpecs.insert_back({.name = name, .typeInfo = Variant::getTypeInfo<void>()});
    }

    m_fileSnippet = clearIndents(params.snippet);
    m_friendlyName = params.friendlyName;
}

std::size_t OpenGLShaderHeader::memory_cost() const
{
    /// TODO: Calculate the cost of the function
    return 1;
}

const PropertyList &OpenGLShaderHeader::getInputSpecs(const PropertyAliases &) const
{
    return m_inputSpecs;
}

const PropertyList &OpenGLShaderHeader::getOutputSpecs() const
{
    return m_outputSpecs;
}

const PropertyList &OpenGLShaderHeader::getFilterSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &OpenGLShaderHeader::getConsumingSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void OpenGLShaderHeader::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                          const PropertyAliases &aliases) const
{
    typeSet.insert(&Variant::getTypeInfo<void>());
}

void OpenGLShaderHeader::extractSubTasks(std::set<const Task *> &taskSet,
                                         const PropertyAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLShaderHeader::outputDeclarationCode(BuildContext args) const
{
    args.output << m_fileSnippet;
}

void OpenGLShaderHeader::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderHeader::outputUsageCode(BuildContext args) const {}

StringView OpenGLShaderHeader::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae
