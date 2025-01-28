#pragma once

#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;

class OpenGLComposeSceneRender : public ComposeSceneRender
{
  public:
    OpenGLComposeSceneRender(const SetupParams &params);

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &aliases) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;

    SetupParams m_params;
    ParamList m_outputSpecs;
    String m_friendlyName;

    struct SpecsPerAliases
    {
        ParamList inputSpecs, filterSpecs, consumingSpecs;
    };

    mutable StableMap<std::size_t, std::unique_ptr<SpecsPerAliases>> m_specsPerKey;

    std::size_t getSpecsKey(const ParamAliases &aliases) const;
};

} // namespace Vitrae