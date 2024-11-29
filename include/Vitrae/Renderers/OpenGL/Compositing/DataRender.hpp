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
    using ComposeDataRender::ComposeDataRender;

    OpenGLComposeDataRender(const SetupParams &params);

    const StableMap<StringId, PropertySpec> &getInputSpecs(
        const RenderSetupContext &args) const override;
    const StableMap<StringId, PropertySpec> &getOutputSpecs() const override;

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(
        StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
        StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
        const ScopedDict &properties,
        const RenderSetupContext &context) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    String m_viewPositionOutputPropertyName;
    dynasma::LazyPtr<Mesh> mp_dataPointVisual;
    DataGeneratorFunction m_dataGenerator;
    StringId m_displayOutputNameId;
    std::optional<StringId> m_displayInputNameId;

    CullingMode m_cullingMode;
    RasterizingMode m_rasterizingMode;
    bool m_smoothFilling, m_smoothTracing, m_smoothDotting;
    String m_friendlyName;
};

} // namespace Vitrae