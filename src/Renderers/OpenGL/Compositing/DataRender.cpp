#include "Vitrae/Renderers/OpenGL/Compositing/DataRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Util/Variant.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeDataRender::OpenGLComposeDataRender(const SetupParams &params)
    : ComposeDataRender(std::span<const PropertySpec>{params.inputSpecs},
                        std::span<const PropertySpec>{{PropertySpec{
                            .name = params.displayOutputPropertyName,
                            .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}}),
      m_root(params.root),
      m_viewPositionOutputPropertyName(params.vertexPositionOutputPropertyName),
      mp_dataPointVisual(params.p_dataPointVisual), m_dataGenerator(params.dataGenerator),
      m_displayInputNameId(params.displayInputPropertyName.empty()
                               ? std::optional<StringId>()
                               : params.displayInputPropertyName),
      m_displayOutputNameId(params.displayOutputPropertyName), m_cullingMode(params.cullingMode),
      m_rasterizingMode(params.rasterizingMode), m_smoothFilling(params.smoothFilling),
      m_smoothTracing(params.smoothTracing), m_smoothDotting(params.smoothDotting)
{

    /*
    We reuse m_inputSpecs as container of the display input specs,
    but we will actually return the renderer's cached input specs for this task
    */

    if (!params.displayInputPropertyName.empty()) {
        m_inputSpecs.emplace(
            params.displayInputPropertyName,
            PropertySpec{.name = params.displayInputPropertyName,
                         .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()});
    }

    m_friendlyName = "Render data:\n";
    m_friendlyName += params.displayOutputPropertyName;
}

const StableMap<StringId, PropertySpec> &OpenGLComposeDataRender::getInputSpecs(
    const RenderSetupContext &args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(args.renderer);
    return rend.getInputDependencyCache(rend.getInputDependencyCacheID(
        this, args.p_defaultVertexMethod, args.p_defaultFragmentMethod));
}

const StableMap<StringId, PropertySpec> &OpenGLComposeDataRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

void OpenGLComposeDataRender::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeDataRender::run");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();
    auto &inputDependencies = rend.getEditableInputDependencyCache(rend.getInputDependencyCacheID(
        this, args.p_defaultVertexMethod, args.p_defaultFragmentMethod));

    bool needsRebuild = false;
    auto specifyInputDependency = [&](const PropertySpec &spec) {
        needsRebuild = needsRebuild || inputDependencies.emplace(spec.name, spec).second;
    };

    // add common inputs to renderer's cached input specs for this task
    for (auto [nameId, spec] : m_inputSpecs) {
        specifyInputDependency(spec);
    }

    // fail early if we already need to rebuild
    // DO save the output, the pipeline expects this property to be set
    if (needsRebuild) {
        args.properties.set(m_displayOutputNameId,
                            args.preparedCompositorFrameStores.at(m_displayOutputNameId));

        throw ComposeTaskRequirementsChangedException();
    }

    // extract common inputs
    auto p_mesh = mp_dataPointVisual.getLoaded();

    dynasma::FirmPtr<FrameStore> p_frame =
        m_displayInputNameId.has_value()
            ? args.properties.get(m_displayInputNameId.value()).get<dynasma::FirmPtr<FrameStore>>()
            : args.preparedCompositorFrameStores.at(m_displayOutputNameId);
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    {
        MMETER_SCOPE_PROFILER("Rendering (multipass)");

        switch (m_rasterizingMode) {
        // derivational methods (all methods for now)
        case RasterizingMode::DerivationalFillCenters:
        case RasterizingMode::DerivationalFillEdges:
        case RasterizingMode::DerivationalFillVertices:
        case RasterizingMode::DerivationalTraceEdges:
        case RasterizingMode::DerivationalTraceVertices:
        case RasterizingMode::DerivationalDotVertices: {
            frame.enterRender(args.properties, {0.0f, 0.0f}, {1.0f, 1.0f});

            {
                MMETER_SCOPE_PROFILER("OGL setup");

                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);

                switch (m_cullingMode) {
                case CullingMode::None:
                    glDisable(GL_CULL_FACE);
                    break;
                case CullingMode::Backface:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    break;
                case CullingMode::Frontface:
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    break;
                }

                // smoothing
                if (m_smoothFilling) {
                    glEnable(GL_POLYGON_SMOOTH);
                    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
                } else {
                    glDisable(GL_POLYGON_SMOOTH);
                }
                if (m_smoothTracing) {
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                } else {
                    glDisable(GL_LINE_SMOOTH);
                }
            }

            // generic function for all passes
            auto runDerivationalPass = [&]() {
                MMETER_SCOPE_PROFILER("Pass");

                // setup the shader
                auto p_mat = p_mesh->getMaterial().getLoaded();
                auto &matProperties = p_mat->getProperties();
                auto &matTextures = p_mat->getTextures();
                auto vertexMethod = p_mat->getVertexMethod();
                auto fragmentMethod = p_mat->getFragmentMethod();

                // compile shader for this material
                dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
                    shaderCacher.retrieve_asset({CompiledGLSLShader::SurfaceShaderParams(
                        args.methodCombinator.getCombinedMethod(args.p_defaultVertexMethod,
                                                                vertexMethod),
                        args.methodCombinator.getCombinedMethod(args.p_defaultFragmentMethod,
                                                                fragmentMethod),
                        m_viewPositionOutputPropertyName, frame.getRenderComponents(), m_root)});

                glUseProgram(p_compiledShader->programGLName);

                // set uniforms
                GLint glModelMatrixUniformLocation;
                {
                    MMETER_SCOPE_PROFILER("Uniform setup");

                    for (auto [propertyNameId, uniSpec] : p_compiledShader->uniformSpecs) {
                        if (propertyNameId == StandardShaderPropertyNames::INPUT_MODEL) {
                            // this is set per model
                            glModelMatrixUniformLocation = uniSpec.location;

                        } else {
                            auto matPropIt = matProperties.find(propertyNameId);
                            if (matPropIt == matProperties.end()) {
                                auto p = args.properties.getPtr(propertyNameId);
                                if (p) {
                                    rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                                        .setUniform(uniSpec.location, *p);
                                }

                                specifyInputDependency(uniSpec.srcSpec);
                            } else {
                                rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                                    .setUniform(uniSpec.location, (*matPropIt).second);
                            }
                        }
                    }

                    for (auto [propertyNameId, bindingSpec] : p_compiledShader->bindingSpecs) {
                        auto matTexIt = matTextures.find(propertyNameId);
                        if (matTexIt == matTextures.end()) {
                            auto p = args.properties.getPtr(propertyNameId);
                            if (p) {
                                rend.getTypeConversion(bindingSpec.srcSpec.typeInfo)
                                    .setBinding(bindingSpec.bindingIndex, *p);
                            }

                            specifyInputDependency(bindingSpec.srcSpec);
                        } else {
                            rend.getTypeConversion(bindingSpec.srcSpec.typeInfo)
                                .setBinding(bindingSpec.bindingIndex, (*matTexIt).second);
                        }
                    }

                    for (auto tokenPropName : p_compiledShader->tokenPropertyNames) {
                        specifyInputDependency(PropertySpec{
                            .name = tokenPropName,
                            .typeInfo = Variant::getTypeInfo<void>(),
                        });
                    }
                }

                if (!needsRebuild) {
                    // run the data generator
                    MMETER_SCOPE_PROFILER("Render data");

                    OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*p_mesh);
                    mesh.loadToGPU(rend);

                    glBindVertexArray(mesh.VAO);
                    std::size_t triCount = mesh.getTriangles().size();

                    RenderCallback renderCallback = [glModelMatrixUniformLocation,
                                                     triCount](const glm::mat4 &transform) {
                        glUniformMatrix4fv(glModelMatrixUniformLocation, 1, GL_FALSE,
                                           &(transform[0][0]));
                        glDrawElements(GL_TRIANGLES, 3 * triCount, GL_UNSIGNED_INT, 0);
                    };

                    m_dataGenerator(args, renderCallback);
                }

                glUseProgram(0);
            };

            // render filled polygons
            switch (m_rasterizingMode) {
            case RasterizingMode::DerivationalFillCenters:
            case RasterizingMode::DerivationalFillEdges:
            case RasterizingMode::DerivationalFillVertices:
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                runDerivationalPass();
                break;
            }

            // render edges
            switch (m_rasterizingMode) {
            case RasterizingMode::DerivationalFillEdges:
            case RasterizingMode::DerivationalTraceEdges:
            case RasterizingMode::DerivationalTraceVertices:
                if (m_smoothTracing) {
                    glDepthMask(GL_FALSE);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_BLEND);
                    glLineWidth(1.5);
                } else {
                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);
                    glLineWidth(1.0);
                }
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                runDerivationalPass();
                break;
            }

            // render vertices
            switch (m_rasterizingMode) {
            case RasterizingMode::DerivationalFillVertices:
            case RasterizingMode::DerivationalTraceVertices:
            case RasterizingMode::DerivationalDotVertices:
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
                glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
                runDerivationalPass();
                break;
            }

            glDepthMask(GL_TRUE);

            frame.exitRender();
            break;
        }
        }
    }

    args.properties.set(m_displayOutputNameId, p_frame);

    if (needsRebuild) {
        throw ComposeTaskRequirementsChangedException();
    }

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    glFinish();
#endif
}

void OpenGLComposeDataRender::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
    const ScopedDict &properties, const RenderSetupContext &context) const {
    // We just need to check whether the frame store is already prepared and make it input also
    if (auto it = frameStores.find(m_displayOutputNameId); it != frameStores.end()) {
        if (m_displayInputNameId.has_value()) {
            auto frame = (*it).second;
            frameStores.emplace(m_displayInputNameId.value(), std::move(frame));
        }
    } else {
        throw std::runtime_error("Frame store not found");
    }
}

StringView OpenGLComposeDataRender::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae