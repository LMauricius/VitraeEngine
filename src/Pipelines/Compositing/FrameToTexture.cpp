#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Data/Overloaded.hpp"
#include "Vitrae/Params/Standard.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToTexture::ComposeFrameToTexture(const SetupParams &params) : m_params(params)
{
    m_friendlyName += "Fragment ";
    std::visit(Overloaded{
                   [&](const FixedRenderComponent &comp) {
                       switch (comp) {
                       case FixedRenderComponent::Depth:
                           m_friendlyName += String("depth");
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

    if (!m_params.size.isFixed()) {
        m_inputSpecs.insert_back(m_params.size.getSpec());
    }
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

    glm::uvec2 retrSize = m_params.size.get(ctx.properties);

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
    FrameStoreManager &frameManager = m_params.root.getComponent<FrameStoreManager>();
    TextureManager &textureManager = m_params.root.getComponent<TextureManager>();

    glm::uvec2 retrSize = m_params.size.get(ctx.properties);

    auto p_texture =
        textureManager
            .register_asset({Texture::EmptyParams{.root = m_params.root,
                                                  .size = retrSize,
                                                  .format = m_params.format,
                                                  .filtering = m_params.filtering,
                                                  .friendlyName = m_params.textureName}})
            .getLoaded();
    FrameStore::OutputTextureSpec outputSpec = {
        .p_texture = p_texture,
        .shaderComponent = m_params.shaderComponent,
        .clearColor = m_params.clearColor,
    };
    ctx.properties.set(m_params.textureName, p_texture);

    /*
    Now create the FB only if it didn't exist beforehand
    */
    if (!ctx.properties.has(StandardParam::fs_target.name) ||
        ctx.properties.get(StandardParam::fs_target.name).getAssignedTypeInfo() ==
            TYPE_INFO<void>) {
        auto p_frame =
            frameManager
                .register_asset_k(FrameStore::TextureBindParams{
                    .root = m_params.root,
                    .outputTextureSpecs = {outputSpec},
                    .friendlyName = ctx.aliases.choiceStringFor(StandardParam::fs_target.name),
                })
                .getLoaded();
        ctx.properties.set(StandardParam::fs_target.name, p_frame);
    } else {
        auto p_frame =
            ctx.properties.get(StandardParam::fs_target.name).get<dynasma::FirmPtr<FrameStore>>();
        p_frame->bindOutput(outputSpec);
        ctx.properties.set(StandardParam::fs_target.name, p_frame);
    }
}

StringView ComposeFrameToTexture::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae