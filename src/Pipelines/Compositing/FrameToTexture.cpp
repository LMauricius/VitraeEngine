#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Data/Overloaded.hpp"
#include "Vitrae/Params/Standard.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToTexture::ComposeFrameToTexture(const AnySetupParams &params) : m_params(params)
{
    m_friendlyName += "Fragment ";
    std::visit(
        [&]<BufferType BT>(const SetupParams<BT> &params) {
            std::visit(Overloaded{
                           [&](const FixedRenderComponent &comp) {
                               switch (comp) {
                               case FixedRenderComponent::DEPTH:
                                   m_friendlyName += String("depth");
                                   break;
                               case FixedRenderComponent::DEPTH_AND_STENCIL:
                                   m_friendlyName += String("depth and stencil");
                                   break;
                               case FixedRenderComponent::STENCIL:
                                   m_friendlyName += String("stencil");
                                   break;
                               }
                           },
                           [&](const ParamSpec &spec) { m_friendlyName += String(spec.name); },
                       },
                       params.shaderComponent);

            m_friendlyName += " to texture";

            m_inputSpecs.insert_back(StandardParam::fs_target);

            for (auto &tokenName : params.inputTokenNames) {
                m_inputSpecs.insert_back({tokenName, TYPE_INFO<void>});
            }

            m_outputSpecs.insert_back({
                params.textureName,
                TYPE_INFO<dynasma::FirmPtr<Texture>>,
            });

            if (!params.size.isFixed()) {
                m_inputSpecs.insert_back(params.size.getSpec());
            }
        },
        params);
}

std::size_t ComposeFrameToTexture::memory_cost() const
{
    return sizeof(ComposeFrameToTexture);
}

const ParamList &ComposeFrameToTexture::getInputSpecs(const ParamAliases &) const
{
    return m_inputSpecs;
}

const ParamList &ComposeFrameToTexture::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &ComposeFrameToTexture::getFilterSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &ComposeFrameToTexture::getConsumingSpecs(const ParamAliases &) const
{
    return m_consumeSpecs;
}

void ComposeFrameToTexture::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                             const ParamAliases &aliases) const
{
    for (const ParamList *p_specs : {&m_inputSpecs, &m_outputSpecs, &m_consumeSpecs}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeFrameToTexture::extractSubTasks(std::set<const Task *> &taskSet,
                                            const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void ComposeFrameToTexture::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER(m_friendlyName.c_str());

    glm::uvec2 retrSize =
        std::visit([&]<BufferType BT>(
                       const SetupParams<BT> &params) { return params.size.get(ctx.properties); },
                   m_params);

    // reset the whole pipeline if the FrameStore size is invalid
    if (ctx.properties.get(StandardParam::fs_target.name)
            .get<dynasma::FirmPtr<FrameStore>>()
            ->getSize() != retrSize) {
        // Ensure the FrameStore gets deleted
        ctx.properties.set(StandardParam::fs_target.name, Variant());

        throw ComposeTaskRequirementsChangedException();
    }

    // Everything should already be set
}

void ComposeFrameToTexture::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    ComponentRoot &root = *std::visit(
        [&]<BufferType BT>(const SetupParams<BT> &params) { return &params.root; }, m_params);

    FrameStoreManager &frameManager = root.getComponent<FrameStoreManager>();

    std::visit(
        [&]<BufferType BT>(const SetupParams<BT> &params) {
            TextureManager<Texture2D<BT>> &textureManager =
                root.getComponent<TextureManager<Texture2D<BT>>>();

            glm::uvec2 retrSize = params.size.get(ctx.properties);

            auto p_texture =
                textureManager
                    .register_asset(
                        {typename Texture2D<BT>::EmptyParams{.root = root,
                                                             .size = retrSize,
                                                             .storageFormat = params.storageFormat,
                                                             .filtering = params.filtering,
                                                             .friendlyName = params.textureName}})
                    .getLoaded();
            auto outputSpec = RenderTextureSpec::fromUnsafe(p_texture, params.shaderComponent);

            /*
            Now create the FB only if it didn't exist beforehand
            */
            if (!ctx.properties.has(StandardParam::fs_target.name) ||
                ctx.properties.get(StandardParam::fs_target.name).getAssignedTypeInfo() ==
                    TYPE_INFO<void>) {
                auto p_frame = frameManager
                                   .register_asset_k(FrameStore::TextureBindParams{
                                       .root = params.root,
                                       .outputTextureSpecs{outputSpec},
                                       .friendlyName = ctx.aliases.choiceStringFor(
                                           StandardParam::fs_target.name),
                                   })
                                   .getLoaded();
                ctx.properties.set(StandardParam::fs_target.name, p_frame);
            } else {
                auto p_frame = ctx.properties.get(StandardParam::fs_target.name)
                                   .get<dynasma::FirmPtr<FrameStore>>();
                p_frame->bindOutput(outputSpec);
                ctx.properties.set(StandardParam::fs_target.name, p_frame);
            }

            ctx.properties.set(params.textureName, p_texture);
        },
        m_params);
}

StringView ComposeFrameToTexture::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae