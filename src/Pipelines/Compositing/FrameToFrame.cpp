#include "Vitrae/Pipelines/Compositing/FrameToFrame.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Data/Overloaded.hpp"
#include "Vitrae/Params/Standard.hpp"

#include "MMeter.h"

#include <span>

namespace Vitrae
{
ComposeFrameToFrame::ComposeFrameToFrame(const SetupParams &params) : m_params(params)
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

    m_friendlyName += " to another frame";

    m_inputSpecs.insert_back(ParamSpec{
        m_params.inputFrameStoreName,
        TYPE_INFO<dynasma::FirmPtr<FrameStore>>,
    });
    m_filterSpecs.insert_back(ParamSpec{
        m_params.targetFrameStoreName,
        TYPE_INFO<dynasma::FirmPtr<FrameStore>>,
    });

    for (auto &tokenName : params.outputTokenNames) {
        m_outputSpecs.insert_back({tokenName, TYPE_INFO<void>});
    }
}

std::size_t ComposeFrameToFrame::memory_cost() const
{
    return sizeof(ComposeFrameToFrame);
}

const ParamList &ComposeFrameToFrame::getInputSpecs(const ParamAliases &) const
{
    return m_inputSpecs;
}

const ParamList &ComposeFrameToFrame::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &ComposeFrameToFrame::getFilterSpecs(const ParamAliases &) const
{
    return m_filterSpecs;
}

const ParamList &ComposeFrameToFrame::getConsumingSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeFrameToFrame::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                           const ParamAliases &aliases) const
{
    for (const ParamList *p_specs : {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeFrameToFrame::extractSubTasks(std::set<const Task *> &taskSet,
                                          const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void ComposeFrameToFrame::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER(m_friendlyName.c_str());

    // Everything should already be set
}

void ComposeFrameToFrame::prepareRequiredLocalAssets(RenderComposeContext ctx) const
{
    FrameStoreManager &frameManager = m_params.root.getComponent<FrameStoreManager>();
    TextureManager &textureManager = m_params.root.getComponent<TextureManager>();

    auto p_targetFrame =
        ctx.properties.get(m_params.targetFrameStoreName).get<dynasma::FirmPtr<FrameStore>>();

    dynasma::FirmPtr<Texture> p_texture;
    ClearColor clearColor;

    Overloaded nameGetter{
        [&](FixedRenderComponent comp) { return std::to_string((int)comp); },
        [&](const ParamSpec &spec) { return spec.name; },
    };

    for (auto& texSpec : p_targetFrame->getOutputTextureSpecs()) {
        if (std::visit(nameGetter, m_params.shaderComponent) ==
                std::visit(nameGetter, texSpec.shaderComponent) &&
            texSpec.p_texture.has_value()) {
            p_texture = texSpec.p_texture.value();
            clearColor = texSpec.clearColor;
        }
    }

    FrameStore::OutputTextureSpec outputSpec = {
        .p_texture = p_texture,
        .shaderComponent = m_params.shaderComponent,
        .clearColor = clearColor,
    };

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

StringView ComposeFrameToFrame::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae