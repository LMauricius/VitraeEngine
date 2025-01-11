#pragma once

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"

namespace Vitrae
{

class OpenGLRenderer;

class OpenGLComposeClearRender : public ComposeClearRender
{
  public:
    OpenGLComposeClearRender(const SetupParams &params);

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
    glm::vec4 m_color;
    ParamList m_outputSpecs;
    String m_friendlyName;
};

} // namespace Vitrae