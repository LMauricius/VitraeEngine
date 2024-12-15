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
        PropertySpec outputSpec;
        Variant value;
    };

    ComposeConstant(const SetupParams &params);
    ~ComposeConstant() = default;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs(const PropertyAliases &) const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;

  protected:
    static const PropertyList s_emptySpecs;

    PropertyList m_outputSpecs;
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