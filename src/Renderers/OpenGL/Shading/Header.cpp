#include "Vitrae/Renderers/OpenGL/Shading/Header.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <fstream>

namespace Vitrae
{

OpenGLShaderHeader::OpenGLShaderHeader(const FileLoadParams &params)
    : m_params{.inputSpecs = params.inputSpecs,
               .outputSpecs = params.outputSpecs,
               .filterSpecs = params.filterSpecs,
               .consumingSpecs = params.consumingSpecs,
               .friendlyName = params.friendlyName}
{
    std::ifstream stream(params.filepath);
    std::ostringstream sstr;
    sstr << stream.rdbuf();
    m_params.snippet = clearIndents(sstr.str());
}

OpenGLShaderHeader::OpenGLShaderHeader(const StringParams &params) : m_params(params) {}

std::size_t OpenGLShaderHeader::memory_cost() const
{
    /// TODO: Calculate the cost of the function
    return 1;
}

const ParamList &OpenGLShaderHeader::getInputSpecs(const ParamAliases &) const
{
    return m_params.inputSpecs;
}

const ParamList &OpenGLShaderHeader::getOutputSpecs() const
{
    return m_params.outputSpecs;
}

const ParamList &OpenGLShaderHeader::getFilterSpecs(const ParamAliases &) const
{
    return m_params.filterSpecs;
}

const ParamList &OpenGLShaderHeader::getConsumingSpecs(const ParamAliases &) const
{
    return m_params.consumingSpecs;
}

void OpenGLShaderHeader::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                          const ParamAliases &aliases) const
{
    for (const ParamList *p_specs : {&m_params.inputSpecs, &m_params.outputSpecs,
                                     &m_params.filterSpecs, &m_params.consumingSpecs}) {
        for (const auto &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLShaderHeader::extractSubTasks(std::set<const Task *> &taskSet,
                                         const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLShaderHeader::outputDeclarationCode(BuildContext args) const
{
    for (auto p_specs : {
             &m_params.inputSpecs,
             &m_params.outputSpecs,
             &m_params.filterSpecs,
             &m_params.consumingSpecs,
         }) {
        for (auto &spec : p_specs->getSpecList()) {
            if (spec.name != args.aliases.choiceStringFor(spec.name)) {
                args.output << "#define " << spec.name << " "
                            << args.aliases.choiceStringFor(spec.name) << "\n";
            }
        }
    }
    args.output << m_params.snippet;

    for (auto p_specs : {
             &m_params.inputSpecs,
             &m_params.outputSpecs,
             &m_params.filterSpecs,
             &m_params.consumingSpecs,
         }) {
        for (auto &spec : p_specs->getSpecList()) {
            if (spec.name != args.aliases.choiceStringFor(spec.name)) {
                args.output << "#undef " << spec.name << "\n";
            }
        }
    }
}

void OpenGLShaderHeader::outputDefinitionCode(BuildContext args) const {}

void OpenGLShaderHeader::outputUsageCode(BuildContext args) const {}

StringView OpenGLShaderHeader::getFriendlyName() const
{
    return m_params.friendlyName;
}

} // namespace Vitrae
