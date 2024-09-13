#pragma once

#include "Vitrae/Pipelines/Compositing/Compute.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

#include <functional>
#include <vector>

namespace Vitrae
{

class OpenGLRenderer;
class PropertyList;

class OpenGLComposeCompute : public ComposeCompute {
  public:
    using ComposeCompute::ComposeCompute;

    OpenGLComposeCompute(const SetupParams &params);

    const StableMap<StringId, PropertySpec> &getInputSpecs(
        const RenderSetupContext &args) const override;
    const StableMap<StringId, PropertySpec> &getOutputSpecs() const override;

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
                                    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
                                    const ScopedDict &properties) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    GpuComputeSetupParams m_computeSetup;
    String m_friendlyName;
    std::function<bool(RenderRunContext &)> m_executeCondition;

    dynasma::FirmPtr<const PropertyList> mp_outputComponents;
};

} // namespace Vitrae