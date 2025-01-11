#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Params/ArgumentGetter.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include "glm/glm.hpp"

#include <variant>

namespace Vitrae
{

class ComposeFrameToTexture : public ComposeTask
{
  public:
    struct OutputTextureParamSpec
    {
        String textureName;
        ParamSpec fragmentSpec;
    };

    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> inputTokenNames;
        String outputColorTextureName;
        String outputDepthTextureName;
        std::vector<OutputTextureParamSpec> outputs;
        ArgumentGetter<glm::vec2> size;
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

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &aliases) const override;

    void run(RenderComposeContext args) const override;
    void prepareRequiredLocalAssets(RenderComposeContext args) const override;

    StringView getFriendlyName() const override;

  protected:
    ComponentRoot &m_root;
    ParamList m_inputSpecs;
    ParamList m_consumeSpecs;
    ParamList m_outputSpecs;

    String m_outputColorTextureName, m_outputDepthTextureName;
    StringId m_outputColorTextureNameId, m_outputDepthTextureNameId;
    std::vector<OutputTextureParamSpec> m_outputTextureParamSpecs;
    ArgumentGetter<glm::vec2> m_size;
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