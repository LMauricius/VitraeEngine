#pragma once

#include "Vitrae/Pipelines/Shading/FunctionCall.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae {

class OpenGLShaderFunctionCall : public ShaderFunctionCall
{
    PropertyList m_inputSpecs, m_outputSpecs, m_filterSpecs, m_consumingSpecs;
    String m_functionName;

  public:
    OpenGLShaderFunctionCall(const StringParams &params);
    ~OpenGLShaderFunctionCall() = default;

    std::size_t memory_cost() const override;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs(const PropertyAliases &) const override;
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