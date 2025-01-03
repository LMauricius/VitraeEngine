#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class ComposeFunction : public ComposeTask
{
    std::function<void(const RenderComposeContext &)> mp_function;
    String m_friendlyName;
    PropertyList m_inputSpecs, m_outputSpecs, m_filterSpecs, m_consumingSpecs;

  public:
    struct SetupParams
    {
        PropertyList inputSpecs;
        PropertyList outputSpecs;
        PropertyList filterSpecs;
        PropertyList consumingSpecs;
        std::function<void(const RenderComposeContext &)> p_function;
        String friendlyName;
    };

    ComposeFunction(const SetupParams &params);
    ~ComposeFunction() = default;

    std::size_t memory_cost() const override;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs(const PropertyAliases &) const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const PropertyAliases &propMapping) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const PropertyAliases &propMapping) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;
};

struct ComposeFunctionKeeperSeed
{
    using Asset = ComposeFunction;
    std::variant<ComposeFunction::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFunctionKeeper = dynasma::AbstractKeeper<ComposeFunctionKeeperSeed>;
} // namespace Vitrae