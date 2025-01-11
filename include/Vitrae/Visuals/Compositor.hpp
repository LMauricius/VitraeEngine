#pragma once

#include "Vitrae/Params/PropertyList.hpp"
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

    void setPropertyAliases(const PropertyAliases &aliases);
    void setDesiredProperties(const PropertyList &properties);

    void compose();

    static const PropertySpec FRAME_STORE_TARGET_SPEC;

    VariantScope parameters;

  protected:
    ComponentRoot &m_root;
    PropertyList m_desiredProperties;
    PropertyAliases m_aliases;

    bool m_needsRebuild;
    bool m_needsFrameStoreRegeneration;
    Pipeline<ComposeTask> m_pipeline;
    VariantScope m_localProperties;

    void rebuildPipeline();
    void regenerateFrameStores();
};

} // namespace Vitrae