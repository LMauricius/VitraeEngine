#pragma once

#include "Vitrae/Pipelines/Shading/Snippet.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <filesystem>
#include <variant>

namespace Vitrae
{

class OpenGLShaderFunction : public ShaderFunction
{
    std::vector<StringId> m_inputOrder;
    std::vector<StringId> m_outputOrder;
    String m_functionName;
    String m_fileSnippet;

  public:
    OpenGLShaderFunction(const FileLoadParams &params);
    OpenGLShaderFunction(const StringParams &params);
    ~OpenGLShaderFunction() = default;

    std::size_t memory_cost() const override;
    void extractUsedTypes(std::set<const TypeInfo *> &typeSet) const override;
    void extractSubTasks(std::set<const Task *> &taskSet) const override;

    void outputDeclarationCode(BuildContext args) const override;
    void outputDefinitionCode(BuildContext args) const override;
    void outputUsageCode(
        BuildContext args, const StableMap<StringId, String> &inputParamsToSharedVariables,
        const StableMap<StringId, String> &outputParamsToSharedVariables) const override;

    virtual StringView getFriendlyName() const override;
};

} // namespace Vitrae