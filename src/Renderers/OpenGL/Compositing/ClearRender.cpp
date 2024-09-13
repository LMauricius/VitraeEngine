#include "Vitrae/Renderers/OpenGL/Compositing/ClearRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/TypeConversion/StringConvert.hpp"
#include "Vitrae/Util/Variant.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeClearRender::OpenGLComposeClearRender(const SetupParams &params)
    : ComposeClearRender(
          params.displayInputPropertyName.empty()
              ? std::span<const PropertySpec>{}
              : std::span<const PropertySpec>{{PropertySpec{
                    .name = params.displayInputPropertyName,
                    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}},
          std::span<const PropertySpec>{
              {PropertySpec{.name = params.displayOutputPropertyName,
                            .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}}),
      m_root(params.root), m_displayInputNameId(params.displayInputPropertyName.empty()
                                                    ? std::optional<StringId>()
                                                    : params.displayInputPropertyName),
      m_displayOutputNameId(params.displayOutputPropertyName), m_color(params.backgroundColor),
      m_friendlyName(String("Clear to\n") + toHexString(255 * m_color.r, 2) +
                     toHexString(255 * m_color.g, 2) + toHexString(255 * m_color.b, 2) + "*" +
                     std::to_string(m_color.a))
{}

void OpenGLComposeClearRender::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeClearRender::run");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();

    dynasma::FirmPtr<FrameStore> p_frame =
        m_displayInputNameId.has_value()
            ? args.properties.get(m_displayInputNameId.value()).get<dynasma::FirmPtr<FrameStore>>()
            : args.preparedCompositorFrameStores.at(m_displayOutputNameId);
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    frame.enterRender(args.properties, {0.0f, 0.0f}, {1.0f, 1.0f});

    glClearColor(m_color.r, m_color.g, m_color.b, m_color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    frame.exitRender();

    args.properties.set(m_displayOutputNameId, p_frame);

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    glFinish();
#endif
}

void OpenGLComposeClearRender::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{
    // We just need to check whether the frame store is already prepared and make it input also
    if (auto it = frameStores.find(m_displayOutputNameId); it != frameStores.end()) {
        if (m_displayInputNameId.has_value()) {
            frameStores.emplace(m_displayInputNameId.value(), (*it).second);
        }
    } else {
        throw std::runtime_error("Frame store not found");
    }
}

StringView OpenGLComposeClearRender::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae