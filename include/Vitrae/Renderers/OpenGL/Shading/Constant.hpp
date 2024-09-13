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
    void extractUsedTypes(std::set<const TypeInfo *> &typeSet) const override;
    void extractSubTasks(std::set<const Task *> &taskSet) const override;

    void outputDeclarationCode(BuildContext args) const override;
    void outputDefinitionCode(BuildContext args) const override;
    void outputUsageCode(
        BuildContext args, const StableMap<StringId, String> &inputParamsToSharedVariables,
        const StableMap<StringId, String> &outputParamsToSharedVariables) const override;
};

} // namespace Vitrae