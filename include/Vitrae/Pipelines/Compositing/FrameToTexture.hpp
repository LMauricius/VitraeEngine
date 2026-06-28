#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Data/ClearColor.hpp"
#include "Vitrae/Data/PixelTyping.hpp"
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
    template <PixelType PIXEL_TYPE> struct SetupParams
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        PixelFormat<PIXEL_TYPE> storageFormat;
        SwizzleSpec<PIXEL_TYPE> swizzle = CommonSwizzleSpecs<PIXEL_TYPE>::NATURAL;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;
        RenderComponent shaderComponent;
        ClearColor clearColor = FixedClearColor::Default;
    };

    template <> struct SetupParams<PixelType::DEPTH>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        PixelFormat<PixelType::DEPTH> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<PixelType::DEPTH> swizzle =
            CommonSwizzleSpecs<PixelType::DEPTH>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::DEPTH;
        ClearColor clearColor = FixedClearColor::Default;
    };

    template <> struct SetupParams<PixelType::STENCIL>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        PixelFormat<PixelType::STENCIL> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<PixelType::STENCIL> swizzle =
            CommonSwizzleSpecs<PixelType::STENCIL>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::STENCIL;
        ClearColor clearColor = FixedClearColor::Default;
    };

    template <> struct SetupParams<PixelType::DEPTH_AND_STENCIL>
    {
        ComponentRoot &root;
        ArgumentGetter<glm::uvec2> size;
        PixelFormat<PixelType::DEPTH_AND_STENCIL> storageFormat;
        TextureFilteringParams filtering = FilteringCommon::INHERIT_ALL;
        String textureName;
        std::vector<String> inputTokenNames;

        constexpr static SwizzleSpec<PixelType::DEPTH_AND_STENCIL> swizzle =
            CommonSwizzleSpecs<PixelType::DEPTH_AND_STENCIL>::NATURAL;
        constexpr static RenderComponent shaderComponent = FixedRenderComponent::DEPTH_AND_STENCIL;
        ClearColor clearColor = FixedClearColor::Default;
    };

    using AnyPTSetupParams = VariantForPixelTypes<SetupParams>;

    ComposeFrameToTexture(const AnyPTSetupParams &params);
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
    AnyPTSetupParams m_params;
    ParamList m_inputSpecs;
    ParamList m_consumeSpecs;
    ParamList m_outputSpecs;

    String m_friendlyName;
};

struct ComposeFrameToTextureKeeperSeed
{
    using Asset = ComposeFrameToTexture;
    std::variant<ComposeFrameToTexture::AnyPTSetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeFrameToTextureKeeper = dynasma::AbstractKeeper<ComposeFrameToTextureKeeperSeed>;

} // namespace Vitrae