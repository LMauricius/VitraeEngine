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

    StringView getFriendlyName() const override;

  protected:
    ParamList m_outputSpecs;
    Variant m_value;
    String m_friendlyName;
};

} // namespace Vitrae