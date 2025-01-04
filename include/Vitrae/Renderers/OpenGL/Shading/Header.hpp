#pragma once

#include "Vitrae/Pipelines/Shading/Header.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class OpenGLShaderHeader : public ShaderHeader
{
    String m_friendlyName;
    String m_fileSnippet;
    PropertyList m_inputSpecs, m_outputSpecs;

  public:
    OpenGLShaderHeader(const FileLoadParams &params);
    OpenGLShaderHeader(const StringParams &params);
    ~OpenGLShaderHeader() = default;

    std::size_t memory_cost() const override;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs() const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const PropertyAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const PropertyAliases &aliases) const override;

    void outputDeclarationCode(BuildContext args) const override;
    void outputDefinitionCode(BuildContext args) const override;
    void outputUsageCode(BuildContext args) const override;

    virtual StringView getFriendlyName() const override;
};

} // namespace Vitrae