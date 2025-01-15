#pragma once

#include "Vitrae/Params/ParamList.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/Pipeline.hpp"

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

    void setParamAliases(const ParamAliases &aliases);
    void setDesiredProperties(const ParamList &properties);

    void compose();

    static const ParamSpec FRAME_STORE_TARGET_SPEC;

    VariantScope parameters;

  protected:
    ComponentRoot &m_root;
    ParamList m_desiredProperties;
    ParamAliases m_aliases;

    bool m_needsRebuild;
    bool m_needsFrameStoreRegeneration;
    Pipeline<ComposeTask> m_pipeline;
    VariantScope m_localProperties;

    void rebuildPipeline();
    void regenerateFrameStores();
};

} // namespace Vitrae