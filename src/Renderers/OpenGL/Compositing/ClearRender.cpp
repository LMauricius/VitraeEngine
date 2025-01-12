#include "Vitrae/Renderers/OpenGL/Compositing/ClearRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/TypeConversion/StringCvt.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeClearRender::OpenGLComposeClearRender(const SetupParams &params)
    : m_root(params.root), m_color(params.backgroundColor),
      m_friendlyName(String("Clear to\n") + toHexString(255 * params.backgroundColor.r, 2) +
                     toHexString(255 * params.backgroundColor.g, 2) +
                     toHexString(255 * params.backgroundColor.b, 2) + "*" +
                     std::to_string(params.backgroundColor.a))
{
    for (auto &tokenName : params.outputTokenNames) {
        m_outputSpecs.insert_back({.name = tokenName, .typeInfo = TYPE_INFO<void>});
    }
}

std::size_t OpenGLComposeClearRender::memory_cost() const
{
    /// TODO: calculate the real memory cost
    return sizeof(*this);
}

const ParamList &OpenGLComposeClearRender::getInputSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &OpenGLComposeClearRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &OpenGLComposeClearRender::getFilterSpecs(const ParamAliases &) const
{
    return FILTER_SPECS;
}

const ParamList &OpenGLComposeClearRender::getConsumingSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void OpenGLComposeClearRender::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                                const ParamAliases &aliases) const
{
    for (const ParamList *p_specs : {&m_outputSpecs, &FILTER_SPECS}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLComposeClearRender::extractSubTasks(std::set<const Task *> &taskSet,
                                               const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLComposeClearRender::run(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeClearRender::run");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();

    dynasma::FirmPtr<FrameStore> p_frame =
        args.properties.get(FRAME_STORE_SPEC.name).get<dynasma::FirmPtr<FrameStore>>();
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    frame.enterRender(args.properties, {0.0f, 0.0f}, {1.0f, 1.0f});

    glClearColor(m_color.r, m_color.g, m_color.b, m_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    frame.exitRender();

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    {
        MMETER_SCOPE_PROFILER("Waiting for GL operations");

        glFinish();
    }
#endif
}

void OpenGLComposeClearRender::prepareRequiredLocalAssets(RenderComposeContext ctx) const {}

StringView OpenGLComposeClearRender::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae