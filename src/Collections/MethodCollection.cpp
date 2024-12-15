#include "Vitrae/Collections/MethodCollection.hpp"

namespace Vitrae
{

MethodCollection::MethodCollection()
    : mp_vertexMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      mp_fragmentMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      mp_computeMethod(dynasma::makeStandalone<Method<ShaderTask>>()),
      mp_composeMethod(dynasma::makeStandalone<Method<ComposeTask>>())
{}

void MethodCollection::registerShaderTask(dynasma::FirmPtr<ShaderTask> p_task,
                                          ShaderStageFlags supportedStages)
{
    if (supportedStages & ShaderStageFlag::Vertex) {
        m_vertexTasks.push_back(p_task);
        mp_vertexMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = m_vertexTasks,
                .fallbackMethods = {},
                .friendlyName = "Vertex",
            });
    }
    if (supportedStages & ShaderStageFlag::Fragment) {
        m_fragmentTasks.push_back(p_task);
        mp_fragmentMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = m_fragmentTasks,
                .fallbackMethods = {},
                .friendlyName = "Fragment",
            });
    }
    if (supportedStages & ShaderStageFlag::Compute) {
        m_computeTasks.push_back(p_task);
        mp_computeMethod =
            dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
                .tasks = m_computeTasks,
                .fallbackMethods = {},
                .friendlyName = "Compute",
            });
    }
}

void MethodCollection::registerComposeTask(dynasma::FirmPtr<ComposeTask> p_task)
{
    m_composeTasks.push_back(p_task);
    mp_composeMethod =
        dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
            .tasks = m_composeTasks,
            .fallbackMethods = {},
            .friendlyName = "Compose",
        });
}

void MethodCollection::registerShaderPropertyOption(const String &outputPropertyName,
                                                    const String &optionPropertyName)
{
    m_shaderPropertyOptions[outputPropertyName].push_back(optionPropertyName);
}

void MethodCollection::registerComposePropertyOption(const String &outputPropertyName,
                                                     const String &optionPropertyName)
{
    m_composePropertyOptions[outputPropertyName].push_back(optionPropertyName);
}

dynasma::FirmPtr<const Method<ShaderTask>> MethodCollection::getVertexMethod() const
{
    return mp_vertexMethod;
}

dynasma::FirmPtr<const Method<ShaderTask>> MethodCollection::getFragmentMethod() const
{
    return mp_fragmentMethod;
}

dynasma::FirmPtr<const Method<ShaderTask>> MethodCollection::getComputeMethod() const
{
    return mp_computeMethod;
}

dynasma::FirmPtr<const Method<ComposeTask>> MethodCollection::getComposeMethod() const
{
    return mp_composeMethod;
}

std::span<const String> MethodCollection::getShaderPropertyOptions(
    StringId outputPropertyNameId) const
{
    return m_shaderPropertyOptions.at(outputPropertyNameId);
}

std::span<const String> MethodCollection::getComposePropertyOptions(
    StringId outputPropertyNameId) const
{
    return m_composePropertyOptions.at(outputPropertyNameId);
}

const StableMap<StringId, std::vector<String>> &MethodCollection::getShaderPropertyOptionsMap()
    const
{
    return m_shaderPropertyOptions;
}

const StableMap<StringId, std::vector<String>> &MethodCollection::getComposePropertyOptionsMap()
    const
{
    return m_composePropertyOptions;
}

} // namespace Vitrae