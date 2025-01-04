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

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs() const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const PropertyAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const PropertyAliases &aliases) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    glm::vec4 m_color;
    PropertyList m_outputSpecs;
    String m_friendlyName;
};

} // namespace Vitrae