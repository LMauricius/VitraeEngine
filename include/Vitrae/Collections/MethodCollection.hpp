#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"

#include "dynasma/pointer.hpp"

#include "Vitrae/Containers/StableMap.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace Vitrae
{

using ShaderStageFlags = uint32_t;
namespace ShaderStageFlag
{
inline constexpr ShaderStageFlags Vertex = 1 << 0;
inline constexpr ShaderStageFlags Fragment = 1 << 1;
inline constexpr ShaderStageFlags Compute = 1 << 2;
} // namespace ShaderStageFlag

class MethodCollection
{
  public:
    MethodCollection();

    void registerShaderTask(dynasma::FirmPtr<ShaderTask> p_task, ShaderStageFlags supportedStages);
    void registerComposeTask(dynasma::FirmPtr<ComposeTask> p_task);

    void registerPropertyOption(const String &outputPropertyName, const String &optionPropertyName);

    void registerCompositorOutput(const String &outputPropertyName);

    dynasma::FirmPtr<const Method<ShaderTask>> getVertexMethod() const;
    dynasma::FirmPtr<const Method<ShaderTask>> getFragmentMethod() const;
    dynasma::FirmPtr<const Method<ShaderTask>> getComputeMethod() const;
    dynasma::FirmPtr<const Method<ComposeTask>> getComposeMethod() const;

    std::span<const String> getPropertyOptions(String outputPropertyNameId) const;
    const StableMap<String, std::vector<String>> &getPropertyOptionsMap() const;

    const std::vector<String> &getCompositorOutputs() const { return m_compositorOutputs; }

  protected:
    std::vector<dynasma::FirmPtr<ShaderTask>> m_vertexTasks, m_fragmentTasks, m_computeTasks;
    std::vector<dynasma::FirmPtr<ComposeTask>> m_composeTasks;
    dynasma::FirmPtr<Method<ShaderTask>> mp_vertexMethod, mp_fragmentMethod, mp_computeMethod;
    dynasma::FirmPtr<Method<ComposeTask>> mp_composeMethod;
    StableMap<String, std::vector<String>> m_propertyOptions;
    std::vector<String> m_compositorOutputs;
};

} // namespace Vitrae