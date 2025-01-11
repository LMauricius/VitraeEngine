#pragma once

#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Pipelines/Compositing/Compute.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;
class ParamList;

class OpenGLComposeCompute : public ComposeCompute {
  public:
    OpenGLComposeCompute(const SetupParams &params);

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
    SetupParams m_params;
    String m_friendlyName;

    struct ProgramPerAliases
    {
        ParamList inputSpecs, filterSpecs, consumeSpecs;

        StableMap<StringId, Variant> cachedDependencies;
    };

    mutable StableMap<std::size_t, std::unique_ptr<ProgramPerAliases>> m_programPerAliasHash;

    ProgramPerAliases &getProgramPerAliases(const ParamAliases &aliases) const;
};

} // namespace Vitrae