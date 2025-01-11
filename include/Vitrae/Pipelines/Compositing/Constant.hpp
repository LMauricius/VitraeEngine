#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <variant>

namespace Vitrae
{

class ComposeConstant : public ComposeTask
{
  public:
    struct SetupParams
    {
        ParamSpec outputSpec;
        Variant value;
    };

    ComposeConstant(const SetupParams &params);
    ~ComposeConstant() = default;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;

  protected:
    ParamList m_outputSpecs;
    Variant m_value;
    String m_friendlyName;
};

struct ComposeConstantKeeperSeed
{
    using Asset = ComposeConstant;
    std::variant<ComposeConstant::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeConstantKeeper = dynasma::AbstractKeeper<ComposeConstantKeeperSeed>;

} // namespace Vitrae