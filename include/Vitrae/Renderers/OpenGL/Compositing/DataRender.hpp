#pragma once

#include "Vitrae/Pipelines/Compositing/DataRender.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;

class OpenGLComposeDataRender : public ComposeDataRender
{
  public:
    OpenGLComposeDataRender(const SetupParams &params);

    std::size_t memory_cost() const override;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs(const PropertyAliases &) const override;
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

    SetupParams m_params;
    String m_friendlyName;

    struct SpecsPerAliases
    {
        PropertyList inputSpecs, outputSpecs, filterSpecs, consumingSpecs;
    };

    mutable StableMap<std::size_t, SpecsPerAliases> m_specsPerKey;

    std::size_t getSpecsKey(const PropertyAliases &aliases) const;
};

} // namespace Vitrae