#pragma once

#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Pipelines/Compositing/Compute.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;
class PropertyList;

class OpenGLComposeCompute : public ComposeCompute {
  public:
    OpenGLComposeCompute(const SetupParams &params);

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
    SetupParams m_params;
    String m_friendlyName;

    struct ProgramPerAliases
    {
        PropertyList inputSpecs, filterSpecs, consumeSpecs;

        StableMap<StringId, Variant> cachedDependencies;
    };

    mutable StableMap<std::size_t, std::unique_ptr<ProgramPerAliases>> m_programPerAliasHash;

    ProgramPerAliases &getProgramPerAliases(const PropertyAliases &aliases) const;
};

} // namespace Vitrae