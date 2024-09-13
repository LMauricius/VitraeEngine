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

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
                                    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
                                    const ScopedDict &properties) const override;

    StringView getFriendlyName() const override;

  protected:
    StringId m_outputNameId;
    PropertySpec m_outputSpec;
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