#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class ComposeInitFunction : public ComposeTask
{
    std::function<void(const RenderComposeContext &)> mp_function;
    String m_friendlyName;
    ParamList m_inputSpecs, m_outputSpecs, m_filterSpecs, m_consumingSpecs;

  public:
    struct SetupParams
    {
        ParamList inputSpecs;
        ParamList outputSpecs;
        ParamList filterSpecs;
        ParamList consumingSpecs;
        std::function<void(const RenderComposeContext &)> p_function;
        String friendlyName;
    };

    ComposeInitFunction(const SetupParams &params);
    ~ComposeInitFunction() = default;

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &propMapping) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &propMapping) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;
};

struct ComposeInitFunctionKeeperSeed
{
    using Asset = ComposeInitFunction;
    std::variant<ComposeInitFunction::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeInitFunctionKeeper = dynasma::AbstractKeeper<ComposeInitFunctionKeeperSeed>;
} // namespace Vitrae