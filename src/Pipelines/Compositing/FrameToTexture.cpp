#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Assets/FrameStore.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToTexture::ComposeFrameToTexture(const SetupParams &params)
    : ComposeTask(
          std::span<const PropertySpec>{{{params.frameInputPropertyName,
                                          Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}},
          std::span<const PropertySpec>{}),
      m_root(params.root), m_frameInputName(params.frameInputPropertyName),
      m_colorTextureOutputName(params.colorTextureOutputPropertyName),
      m_depthTextureOutputName(params.depthTextureOutputPropertyName),
      m_frameInputNameId(params.frameInputPropertyName),
      m_colorTextureOutputNameId(params.colorTextureOutputPropertyName),
      m_depthTextureOutputNameId(params.depthTextureOutputPropertyName),
      m_outputTexturePropertySpecs(params.outputTexturePropertySpecs), m_size(params.size),
      m_channelType(params.channelType), m_horWrap(params.horWrap), m_verWrap(params.verWrap),
      m_minFilter(params.minFilter), m_magFilter(params.magFilter), m_useMipMaps(params.useMipMaps),
      m_borderColor(params.borderColor)

{
    m_friendlyName = "To texture:";
    if (params.colorTextureOutputPropertyName != "") {
        m_outputSpecs.emplace(m_colorTextureOutputNameId,
                              PropertySpec{
                                  params.colorTextureOutputPropertyName,
                                  Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
                              });
        m_friendlyName += String("\n- shade");
    }
    if (params.depthTextureOutputPropertyName != "") {
        m_outputSpecs.emplace(m_depthTextureOutputNameId,
                              PropertySpec{
                                  params.depthTextureOutputPropertyName,
                                  Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
                              });
        m_friendlyName += String("\n- depth");
    }
    for (auto &spec : m_outputTexturePropertySpecs) {
        m_outputSpecs.emplace(spec.texturePropertyName,
                              PropertySpec{
                                  spec.texturePropertyName,
                                  Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
                              });
        m_friendlyName += String("\n- ");
        m_friendlyName += spec.fragmentPropertySpec.name;
    }

    if (!m_size.isFixed()) {
        m_inputSpecs.emplace(m_size.getSpec().name, m_size.getSpec());
    }
}

void ComposeFrameToTexture::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeFrameToTexture::run");

    if (m_colorTextureOutputNameId != "") {
        args.properties.set(m_colorTextureOutputNameId,
                            args.preparedCompositorTextures.at(m_colorTextureOutputNameId));
    }
    if (m_depthTextureOutputNameId != "") {
        args.properties.set(m_depthTextureOutputNameId,
                            args.preparedCompositorTextures.at(m_depthTextureOutputNameId));
    }
    for (auto &spec : m_outputTexturePropertySpecs) {
        args.properties.set(spec.texturePropertyName,
                            args.preparedCompositorTextures.at(spec.texturePropertyName));
    }
}

void ComposeFrameToTexture::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{
    FrameStoreManager &frameManager = m_root.getComponent<FrameStoreManager>();
    TextureManager &textureManager = m_root.getComponent<TextureManager>();

    FrameStore::TextureBindParams frameParams = {.root = m_root,
                                                 .p_colorTexture = {},
                                                 .p_depthTexture = {},
                                                 .outputTextureSpecs = {},
                                                 .friendlyName = m_frameInputName};
    glm::vec2 retrSize = m_size.get(properties);

    if (m_colorTextureOutputNameId != "") {
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
                                  .friendlyName = m_colorTextureOutputName}});
        frameParams.p_colorTexture = p_texture;
        textures.emplace(m_colorTextureOutputNameId, p_texture);
    }
    if (m_depthTextureOutputNameId != "") {
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
                                  .friendlyName = m_depthTextureOutputName}});
        frameParams.p_depthTexture = p_texture;
        textures.emplace(m_depthTextureOutputNameId, p_texture);
    }
    for (auto &spec : m_outputTexturePropertySpecs) {
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
                                  .friendlyName = spec.texturePropertyName}});
        frameParams.outputTextureSpecs.emplace_back(FrameStore::OutputTextureSpec{
            .fragmentPropertySpec = spec.fragmentPropertySpec,
            .p_texture = p_texture,
        });
        textures.emplace(spec.texturePropertyName, p_texture);
    }

    auto frame = frameManager.register_asset({frameParams});
    frameStores.emplace(m_frameInputNameId, frame);
}

StringView ComposeFrameToTexture::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae