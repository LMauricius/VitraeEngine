#pragma once

#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"

namespace Vitrae
{

class OpenGLRenderer;

class OpenGLComposeClearRender : public ComposeClearRender
{
  public:
    using ComposeClearRender::ComposeClearRender;

    OpenGLComposeClearRender(const SetupParams &params);

    void run(RenderRunContext args) const override;
    void prepareRequiredLocalAssets(StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
                                    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
                                    const ScopedDict &properties) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    glm::vec4 m_color;
    StringId m_displayOutputNameId;
    std::optional<StringId> m_displayInputNameId;
    String m_friendlyName;
};

} // namespace Vitrae