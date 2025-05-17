#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Params/ArgumentGetter.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Data/RenderComponents.hpp"
#include "Vitrae/Data/ClearColor.hpp"

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
        RenderComponent shaderComponent;
        BufferFormat format;
        ClearColor clearColor = glm::vec4{0.0f, 0.0f, 0.0f, 0.0f};
        TextureFilteringParams filtering;
    };

    struct SetupParams
    {
        ComponentRoot &root;
        std::vector<String> inputTokenNames;
        std::vector<OutputTextureParamSpec> outputs;
        ArgumentGetter<glm::uvec2> size;
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

    std::vector<OutputTextureParamSpec> m_outputTextureParamSpecs;
    ArgumentGetter<glm::uvec2> m_size;
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