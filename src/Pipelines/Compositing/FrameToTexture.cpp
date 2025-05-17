#include "Vitrae/Pipelines/Compositing/FrameToTexture.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Data/Overloaded.hpp"
#include "Vitrae/Params/Standard.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToTexture::ComposeFrameToTexture(const SetupParams &params)
    : m_root(params.root), m_outputTextureParamSpecs(params.outputs), m_size(params.size)

{
    m_friendlyName = "To texture:";
    for (auto &texSpec : m_outputTextureParamSpecs) {
        m_outputSpecs.insert_back({
            texSpec.textureName,
            TYPE_INFO<dynasma::FirmPtr<Texture>>,
        });
        m_friendlyName += String("\n- ");
        std::visit(
            Overloaded{
                [&](const FixedRenderComponent &comp) {
                    switch (comp) {
                    case FixedRenderComponent::Depth:
                        m_friendlyName += String("\n- depth");
                        break;
                    }
                },
                [&](const ParamSpec &spec) { m_friendlyName += String("\n- ") + spec.name; },
            },
            texSpec.shaderComponent);
    }

    m_consumeSpecs.insert_back(StandardParam::fs_target);

    for (auto &tokenName : params.inputTokenNames) {
        m_inputSpecs.insert_back({tokenName, TYPE_INFO<void>});
    }

    if (!m_size.isFixed()) {
        m_inputSpecs.insert_back(m_size.getSpec());
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

    // Everything should already be set
}

void ComposeFrameToTexture::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    FrameStoreManager &frameManager = m_root.getComponent<FrameStoreManager>();
    TextureManager &textureManager = m_root.getComponent<TextureManager>();

    FrameStore::TextureBindParams frameParams = {
        .root = m_root,
        .outputTextureSpecs = {},
        .friendlyName = ctx.aliases.choiceStringFor(StandardParam::fs_target.name)};
    glm::uvec2 retrSize = m_size.get(ctx.properties);

    for (auto &texSpec : m_outputTextureParamSpecs) {
        auto p_texture =
            textureManager
                .register_asset({Texture::EmptyParams{.root = m_root,
                                                      .size = retrSize,
                                                      .format = texSpec.format,
                                                      .filtering = texSpec.filtering,
                                                      .friendlyName = texSpec.textureName}})
                .getLoaded();
        ;
        frameParams.outputTextureSpecs.emplace_back(FrameStore::OutputTextureSpec{
            .p_texture = p_texture,
            .shaderComponent = texSpec.shaderComponent,
            .clearColor = texSpec.clearColor,
        });
        ctx.properties.set(texSpec.textureName, p_texture);
    }

    auto frame = frameManager.register_asset({frameParams}).getLoaded();
    ctx.properties.set(StandardParam::fs_target.name, frame);
}

StringView ComposeFrameToTexture::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae