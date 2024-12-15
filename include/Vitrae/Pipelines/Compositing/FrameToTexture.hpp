#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Util/PropertyGetter.hpp"

#include "dynasma/keepers/abstract.hpp"

#include "glm/glm.hpp"

#include <variant>

namespace Vitrae
{

class ComposeFrameToTexture : public ComposeTask
{
  public:
    struct OutputTexturePropertySpec
    {
        String textureName;
        PropertySpec fragmentSpec;
    };

    struct SetupParams
    {
        ComponentRoot &root;
        String inputFrameName;
        std::vector<String> inputTokenNames;
        String outputColorTextureName;
        String outputDepthTextureName;
        std::vector<OutputTexturePropertySpec> outputs;
        PropertyGetter<glm::vec2> size;
        Texture::ChannelType channelType = Texture::ChannelType::RGB;
        Texture::WrappingType horWrap = Texture::WrappingType::REPEAT;
        Texture::WrappingType verWrap = Texture::WrappingType::REPEAT;
        Texture::FilterType minFilter = Texture::FilterType::LINEAR;
        Texture::FilterType magFilter = Texture::FilterType::LINEAR;
        bool useMipMaps = true;
        glm::vec4 borderColor = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    ComposeFrameToTexture(const SetupParams &params);
    ~ComposeFrameToTexture() = default;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs(const PropertyAliases &) const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void run(RenderComposeContext args) const override;
    void prepareRequiredLocalAssets(RenderComposeContext args) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    PropertyList m_inputSpecs;
    PropertyList m_outputSpecs;

    String m_inputFrameName, m_outputColorTextureName, m_outputDepthTextureName;
    StringId m_inputFrameNameId, m_outputColorTextureNameId, m_outputDepthTextureNameId;
    std::vector<OutputTexturePropertySpec> m_outputTexturePropertySpecs;
    PropertyGetter<glm::vec2> m_size;
    Texture::ChannelType m_channelType;
    Texture::WrappingType m_horWrap;
    Texture::WrappingType m_verWrap;
    Texture::FilterType m_minFilter;
    Texture::FilterType m_magFilter;
    bool m_useMipMaps;
    glm::vec4 m_borderColor;
    String m_friendlyName;
};

struct ComposeFrameToTextureKeeperSeed
{
    using Asset = ComposeFrameToTexture;
    std::variant<ComposeFrameToTexture::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFrameToTextureKeeper = dynasma::AbstractKeeper<ComposeFrameToTextureKeeperSeed>;

} // namespace Vitrae