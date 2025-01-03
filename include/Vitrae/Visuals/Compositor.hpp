#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Util/PropertyList.hpp"
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

    void setPropertyAliases(const PropertyAliases &aliases);
    void setDesiredProperties(const PropertyList &properties);

    void compose();

    static const PropertySpec FRAME_STORE_TARGET_SPEC;

    ScopedDict parameters;

  protected:
    ComponentRoot &m_root;
    PropertyList m_desiredProperties;
    PropertyAliases m_aliases;

    bool m_needsRebuild;
    bool m_needsFrameStoreRegeneration;
    Pipeline<ComposeTask> m_pipeline;
    ScopedDict m_localProperties;

    void rebuildPipeline();
    void regenerateFrameStores();
};

} // namespace Vitrae