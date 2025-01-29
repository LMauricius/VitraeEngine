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

    /**
     * @returns the input specs required by the built pipeline
     */
    const ParamList &getInputSpecs() const;

    /**
     * Composes and displays the result by running the pipeline
     */
    void compose();

    /**
     * Rebuilds the pipeline after changes
     */
    void rebuildPipeline();

    /**
     * Parameters for the pipeline run. Modifying them might not affect the run until the pipeline
     * is rebuilt, depending on the tasks used
     * @note This member is only modified externally. The compositor can internally cache some data.
     */
    VariantScope parameters;

  protected:
    ComponentRoot &m_root;
    ParamList m_desiredProperties;
    ParamAliases m_aliases;

    bool m_needsRebuild;
    bool m_needsFrameStoreRegeneration;
    Pipeline<ComposeTask> m_pipeline;
    VariantScope m_localProperties;
    void regenerateFrameStores();
};

} // namespace Vitrae