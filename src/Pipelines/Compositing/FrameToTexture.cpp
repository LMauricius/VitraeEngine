#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Assets/FrameStore.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToTexture::ComposeFrameToTexture(const SetupParams &params)
    : m_root(params.root), m_inputFrameName(params.inputFrameName),
      m_outputColorTextureName(params.outputColorTextureName),
      m_outputDepthTextureName(params.outputDepthTextureName),
      m_inputFrameNameId(params.inputFrameName),
      m_outputColorTextureNameId(params.outputColorTextureName),
      m_outputDepthTextureNameId(params.outputDepthTextureName),
      m_outputTexturePropertySpecs(params.outputs), m_size(params.size),
      m_channelType(params.channelType), m_horWrap(params.horWrap), m_verWrap(params.verWrap),
      m_minFilter(params.minFilter), m_magFilter(params.magFilter), m_useMipMaps(params.useMipMaps),
      m_borderColor(params.borderColor)

{
    m_friendlyName = "To texture:";
    if (params.outputColorTextureName != "") {
        m_outputSpecs.insert_back({
            params.outputColorTextureName,
            Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
        });
        m_friendlyName += String("\n- shade");
    }
    if (params.outputDepthTextureName != "") {
        m_outputSpecs.insert_back({
            params.outputDepthTextureName,
            Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
        });
        m_friendlyName += String("\n- depth");
    }
    for (auto &spec : m_outputTexturePropertySpecs) {
        m_outputSpecs.insert_back({
            spec.textureName,
            Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
        });
        m_friendlyName += String("\n- ");
        m_friendlyName += spec.fragmentSpec.name;
    }

    m_inputSpecs.insert_back({
        m_inputFrameName,
        Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),
    });

    for (auto &tokenName : params.inputTokenNames) {
        m_inputSpecs.insert_back({tokenName, Variant::getTypeInfo<void>()});
    }

    if (!m_size.isFixed()) {
        m_inputSpecs.insert_back(m_size.getSpec());
    }
}

const PropertyList &ComposeFrameToTexture::getInputSpecs(const PropertyAliases &) const
{
    return m_inputSpecs;
}

const PropertyList &ComposeFrameToTexture::getOutputSpecs(const PropertyAliases &) const
{
    return m_outputSpecs;
}

const PropertyList &ComposeFrameToTexture::getFilterSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &ComposeFrameToTexture::getConsumingSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeFrameToTexture::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER("ComposeFrameToTexture::run");

    // Everything should already be set
}

void ComposeFrameToTexture::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    FrameStoreManager &frameManager = m_root.getComponent<FrameStoreManager>();
    TextureManager &textureManager = m_root.getComponent<TextureManager>();

    FrameStore::TextureBindParams frameParams = {.root = m_root,
                                                 .p_colorTexture = {},
                                                 .p_depthTexture = {},
                                                 .outputTextureSpecs = {},
                                                 .friendlyName = m_inputFrameName};
    glm::vec2 retrSize = m_size.get(ctx.properties);

    if (m_outputColorTextureNameId != "") {
        auto p_texture = textureManager.register_asset(
            {Texture::EmptyParams{.root = m_root,
                                  .size = retrSize,
                                  .channelType = m_channelType,
                                  .horWrap = m_horWrap,
                                  .verWrap = m_verWrap,
                                  .minFilter = m_minFilter,
                                  .magFilter = m_magFilter,
                                  .useMipMaps = m_useMipMaps,
                                  .borderColor = m_borderColor,
                                  .friendlyName = m_outputColorTextureName}});
        frameParams.p_colorTexture = p_texture;
        ctx.properties.set(m_outputColorTextureNameId, p_texture);
    }
    if (m_outputDepthTextureNameId != "") {
        auto p_texture = textureManager.register_asset(
            {Texture::EmptyParams{.root = m_root,
                                  .size = retrSize,
                                  .channelType = Texture::ChannelType::DEPTH,
                                  .horWrap = m_horWrap,
                                  .verWrap = m_verWrap,
                                  .minFilter = m_minFilter,
                                  .magFilter = m_magFilter,
                                  .useMipMaps = m_useMipMaps,
                                  .borderColor = {1.0f, 1.0f, 1.0f, 1.0f},
                                  .friendlyName = m_outputDepthTextureName}});
        frameParams.p_depthTexture = p_texture;
        ctx.properties.set(m_outputDepthTextureNameId, p_texture);
    }
    for (auto &spec : m_outputTexturePropertySpecs) {
        auto p_texture =
            textureManager.register_asset({Texture::EmptyParams{.root = m_root,
                                                                .size = retrSize,
                                                                .channelType = m_channelType,
                                                                .horWrap = m_horWrap,
                                                                .verWrap = m_verWrap,
                                                                .minFilter = m_minFilter,
                                                                .magFilter = m_magFilter,
                                                                .useMipMaps = m_useMipMaps,
                                                                .borderColor = m_borderColor,
                                                                .friendlyName = spec.textureName}});
        frameParams.outputTextureSpecs.emplace_back(FrameStore::OutputTextureSpec{
            .fragmentSpec = spec.fragmentSpec,
            .p_texture = p_texture,
        });
        ctx.properties.set(spec.textureName, p_texture);
    }

    auto frame = frameManager.register_asset({frameParams});
    ctx.properties.set(m_inputFrameNameId, frame);
}

StringView ComposeFrameToTexture::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae