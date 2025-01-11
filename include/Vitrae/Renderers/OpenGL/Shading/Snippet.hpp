#pragma once

#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class OpenGLShaderSnippet : public ShaderSnippet
{
    ParamList m_inputSpecs, m_outputSpecs, m_filterSpecs, m_consumingSpecs;
    String m_friendlyName;
    String m_snippet;

  public:
    OpenGLShaderSnippet(const StringParams &params);
    ~OpenGLShaderSnippet() = default;

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &aliases) const override;

    void outputDeclarationCode(BuildContext args) const override;
    void outputDefinitionCode(BuildContext args) const override;
    void outputUsageCode(BuildContext args) const override;

    virtual StringView getFriendlyName() const override;
};

} // namespace Vitrae