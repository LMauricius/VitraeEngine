#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

namespace Vitrae
{
class FrameStore;
class ComponentRoot;

class Compositor : public dynasma::PolymorphicBase
{
  public:
    Compositor(ComponentRoot &root);
    virtual ~Compositor() = default;

    std::size_t memory_cost() const;

    void setComposeMethod(dynasma::FirmPtr<Method<ComposeTask>> p_method);
    void setDefaultShadingMethod(dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod,
                                 dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod);
    void setDefaultComputeMethod(dynasma::FirmPtr<Method<ShaderTask>> p_method);
    void setOutput(dynasma::FirmPtr<FrameStore> p_store);
    void setDesiredProperties(const PropertyList &properties);

    void compose();

    ScopedDict parameters;

  protected:
    ComponentRoot &m_root;
    bool m_needsRebuild;
    bool m_needsFrameStoreRegeneration;
    Pipeline<ComposeTask> m_pipeline;
    ScopedDict m_localProperties;
    dynasma::FirmPtr<FrameStore> mp_frameStore;
    MethodCombinator<ShaderTask> m_shadingMethodCombinator;
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> m_preparedFrameStores;
    std::set<dynasma::FirmPtr<FrameStore>> m_uniqueFrameStores;
    StableMap<StringId, dynasma::FirmPtr<Texture>> m_preparedTextures;
    dynasma::FirmPtr<Method<ComposeTask>> mp_composeMethod;
    dynasma::FirmPtr<Method<ShaderTask>> m_defaultVertexMethod, m_defaultFragmentMethod,
        m_defaultComputeMethod;
    PropertyList m_desiredProperties;

    void rebuildPipeline();
    void regenerateFrameStores();
};

} // namespace Vitrae