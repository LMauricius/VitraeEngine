#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Data/BufferFormat.hpp"
#include "Vitrae/Data/RenderComponents.hpp"
#include "Vitrae/Params/ArgumentGetter.hpp"
#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <glm/glm.hpp>

#include <variant>

namespace Vitrae
{

class ComposeFrameToTexture : public ComposeTask
{
  public:
    template <BufferType BUFFER_TYPE> struct SetupParams
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        BufferFormat<BUFFER_TYPE> storageFormat;
        SwizzleSpec<BUFFER_TYPE> swizzle = CommonSwizzleSpecs<BUFFER_TYPE>::NATURAL;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;
        RenderComponent shaderComponent;
    };

    template <> struct SetupParams<BufferType::DEPTH>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        BufferFormat<BufferType::DEPTH> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<BufferType::DEPTH> swizzle =
            CommonSwizzleSpecs<BufferType::DEPTH>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::DEPTH;
    };

    template <> struct SetupParams<BufferType::STENCIL>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        BufferFormat<BufferType::STENCIL> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<BufferType::STENCIL> swizzle =
            CommonSwizzleSpecs<BufferType::STENCIL>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::STENCIL;
    };

    template <> struct SetupParams<BufferType::DEPTH_AND_STENCIL>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        BufferFormat<BufferType::DEPTH_AND_STENCIL> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<BufferType::DEPTH_AND_STENCIL> swizzle =
            CommonSwizzleSpecs<BufferType::DEPTH_AND_STENCIL>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::DEPTH_AND_STENCIL;
    };

    using AnySetupParams = VariantForBufferTypes<SetupParams>;

    ComposeFrameToTexture(const AnySetupParams &params);
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
    AnySetupParams m_params;
    ParamList m_inputSpecs;
    ParamList m_consumeSpecs;
    ParamList m_outputSpecs;

    String m_friendlyName;
};

struct ComposeFrameToTextureKeeperSeed
{
    using Asset = ComposeFrameToTexture;
    std::variant<ComposeFrameToTexture::AnySetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFrameToTextureKeeper = dynasma::AbstractKeeper<ComposeFrameToTextureKeeperSeed>;

} // namespace Vitrae