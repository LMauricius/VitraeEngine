#pragma once

#include "Vitrae/Pipelines/Shading/Constant.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class OpenGLShaderConstant : public ShaderConstant
{
  public:
    OpenGLShaderConstant(const SetupParams &params);
    ~OpenGLShaderConstant() = default;

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

    StringView getFriendlyName() const override;

  protected:
    PropertyList m_outputSpecs;
    Variant m_value;
    String m_friendlyName;
};

} // namespace Vitrae