#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Types/Typedefs.hpp"

#include "dynasma/pointer.hpp"

#include "Vitrae/Util/StableMap.hpp"

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

    void registerShaderPropertyOption(const String &outputPropertyName,
                                      const String &optionPropertyName);
    void registerComposePropertyOption(const String &outputPropertyName,
                                       const String &optionPropertyName);

    dynasma::FirmPtr<const Method<ShaderTask>> getVertexMethod() const;
    dynasma::FirmPtr<const Method<ShaderTask>> getFragmentMethod() const;
    dynasma::FirmPtr<const Method<ShaderTask>> getComputeMethod() const;
    dynasma::FirmPtr<const Method<ComposeTask>> getComposeMethod() const;

    std::span<const String> getShaderPropertyOptions(StringId outputPropertyNameId) const;
    std::span<const String> getComposePropertyOptions(StringId outputPropertyNameId) const;
    const StableMap<StringId, std::vector<String>> &getShaderPropertyOptionsMap() const;
    const StableMap<StringId, std::vector<String>> &getComposePropertyOptionsMap() const;

  protected:
    std::vector<dynasma::FirmPtr<ShaderTask>> m_vertexTasks, m_fragmentTasks, m_computeTasks;
    std::vector<dynasma::FirmPtr<ComposeTask>> m_composeTasks;
    dynasma::FirmPtr<Method<ShaderTask>> mp_vertexMethod, mp_fragmentMethod, mp_computeMethod;
    dynasma::FirmPtr<Method<ComposeTask>> mp_composeMethod;
    StableMap<StringId, std::vector<String>> m_shaderPropertyOptions;
    StableMap<StringId, std::vector<String>> m_composePropertyOptions;
};

} // namespace Vitrae