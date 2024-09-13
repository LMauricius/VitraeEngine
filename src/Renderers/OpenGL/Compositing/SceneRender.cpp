#include "Vitrae/Renderers/OpenGL/Compositing/SceneRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Util/Variant.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeSceneRender::OpenGLComposeSceneRender(const SetupParams &params)
    : ComposeSceneRender(
          std::span<const PropertySpec>{{
              PropertySpec{.name = params.sceneInputPropertyName,
                           .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>()},
          }},
          std::span<const PropertySpec>{
              {PropertySpec{.name = params.displayOutputPropertyName,
                            .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()}}}),
      m_root(params.root),
      m_viewPositionOutputPropertyName(params.vertexPositionOutputPropertyName),
      m_sceneInputNameId(params.sceneInputPropertyName),
      m_displayInputNameId(params.displayInputPropertyName.empty()
                               ? std::optional<StringId>()
                               : params.displayInputPropertyName),
      m_displayOutputNameId(params.displayOutputPropertyName), m_cullingMode(params.cullingMode),
      m_rasterizingMode(params.rasterizingMode), m_smoothFilling(params.smoothFilling),
      m_smoothTracing(params.smoothTracing), m_smoothDotting(params.smoothDotting)
{

    /*
    We reuse m_inputSpecs as container of the scene and display input specs,
    but we will actually return the renderer's cached input specs for this task
    */

    if (!params.displayInputPropertyName.empty()) {
        m_inputSpecs.emplace(
            params.displayInputPropertyName,
            PropertySpec{.name = params.displayInputPropertyName,
                         .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>()});
    }

    m_friendlyName = "Render scene:\n";
    m_friendlyName += params.vertexPositionOutputPropertyName;
    switch (params.cullingMode) {
    case CullingMode::None:
        m_friendlyName += "\n- all faces";
        break;
    case CullingMode::Backface:
        m_friendlyName += "\n- front faces";
        break;
    case CullingMode::Frontface:
        m_friendlyName += "\n- back faces";
        break;
    }
    switch (params.rasterizingMode) {
    case RasterizingMode::DerivationalFillCenters:
    case RasterizingMode::DerivationalFillEdges:
    case RasterizingMode::DerivationalFillVertices:
        m_friendlyName += "\n- filled polygons";
        break;
    case RasterizingMode::DerivationalTraceEdges:
    case RasterizingMode::DerivationalTraceVertices:
        m_friendlyName += "\n- wireframe";
        break;
    case RasterizingMode::DerivationalDotVertices:
        m_friendlyName += "\n- dots";
        break;
    }
    if (params.smoothFilling || params.smoothTracing || params.smoothDotting) {
        m_friendlyName += "\n- smooth";
        if (params.smoothFilling) {
            m_friendlyName += " filling";
        }
        if (params.smoothTracing) {
            m_friendlyName += " tracing";
        }
        if (params.smoothDotting) {
            m_friendlyName += " dotting";
        }
    }
}

const StableMap<StringId, PropertySpec> &OpenGLComposeSceneRender::getInputSpecs(
    const RenderSetupContext &args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(args.renderer);
    return rend.getInputDependencyCache(rend.getInputDependencyCacheID(
        this, args.p_defaultVertexMethod, args.p_defaultFragmentMethod));
}

const StableMap<StringId, PropertySpec> &OpenGLComposeSceneRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

void OpenGLComposeSceneRender::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeSceneRender::run");

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
    Scene &scene = *args.properties.get(m_sceneInputNameId).get<dynasma::FirmPtr<Scene>>();

    dynasma::FirmPtr<FrameStore> p_frame =
        m_displayInputNameId.has_value()
            ? args.properties.get(m_displayInputNameId.value()).get<dynasma::FirmPtr<FrameStore>>()
            : args.preparedCompositorFrameStores.at(m_displayOutputNameId);
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    // build map of shaders to materials to mesh props
    std::map<std::pair<dynasma::FirmPtr<Method<ShaderTask>>, dynasma::FirmPtr<Method<ShaderTask>>>,
             std::map<dynasma::FirmPtr<const Material>, std::vector<const MeshProp *>>>
        methods2materials2props;

    {

        MMETER_SCOPE_PROFILER("Scene struct gen");

        for (auto &meshProp : scene.meshProps) {
            auto mat = meshProp.p_mesh->getMaterial().getLoaded();
            OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*meshProp.p_mesh);
            mesh.loadToGPU(rend);

            methods2materials2props[{mat->getVertexMethod(), mat->getFragmentMethod()}]
                                   [meshProp.p_mesh->getMaterial()]
                                       .push_back(&meshProp);
        }
    }

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

                // render the scene
                // iterate over shaders
                for (auto &[methods, materials2props] : methods2materials2props) {
                    MMETER_SCOPE_PROFILER("Shader iteration");

                    auto [vertexMethod, fragmentMethod] = methods;

                    // compile shader for this material
                    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
                        shaderCacher.retrieve_asset({CompiledGLSLShader::SurfaceShaderParams(
                            args.methodCombinator.getCombinedMethod(args.p_defaultVertexMethod,
                                                                    vertexMethod),
                            args.methodCombinator.getCombinedMethod(args.p_defaultFragmentMethod,
                                                                    fragmentMethod),
                            m_viewPositionOutputPropertyName, frame.getRenderComponents(),
                            m_root)});

                    glUseProgram(p_compiledShader->programGLName);

                    // set the 'environmental' uniforms
                    // skip those that will be set by the material
                    auto p_firstMat = materials2props.begin()->first;
                    auto &firstMatProperties = p_firstMat->getProperties();
                    auto &firstMatTextures = p_firstMat->getTextures();
                    GLint glModelMatrixUniformLocation;

                    {
                        MMETER_SCOPE_PROFILER("Uniform setup");

                        for (auto [propertyNameId, uniSpec] : p_compiledShader->uniformSpecs) {
                            if (propertyNameId == StandardShaderPropertyNames::INPUT_MODEL) {
                                // this is set per model
                                glModelMatrixUniformLocation = uniSpec.location;

                            } else {
                                if (firstMatProperties.find(propertyNameId) ==
                                    firstMatProperties.end()) {
                                    auto p = args.properties.getPtr(propertyNameId);
                                    if (p) {
                                        rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                                            .setUniform(uniSpec.location, *p);
                                    }

                                    specifyInputDependency(uniSpec.srcSpec);
                                }
                            }
                        }

                        for (auto [propertyNameId, bindingSpec] : p_compiledShader->bindingSpecs) {
                            if (firstMatProperties.find(propertyNameId) ==
                                    firstMatProperties.end() &&
                                firstMatTextures.find(propertyNameId) == firstMatTextures.end()) {
                                auto p = args.properties.getPtr(propertyNameId);
                                if (p) {
                                    rend.getTypeConversion(bindingSpec.srcSpec.typeInfo)
                                        .setBinding(bindingSpec.bindingIndex, *p);
                                }

                                specifyInputDependency(bindingSpec.srcSpec);
                            }
                        }

                        for (auto tokenPropName : p_compiledShader->tokenPropertyNames) {
                            specifyInputDependency(PropertySpec{
                                .name = tokenPropName,
                                .typeInfo = Variant::getTypeInfo<void>(),
                            });
                        }
                    }

                    // helper for material properties
                    auto setPropertyToShader = [&](StringId nameId, const Variant &value) {
                        if (auto it = p_compiledShader->uniformSpecs.find(nameId);
                            it != p_compiledShader->uniformSpecs.end()) {
                            rend.getTypeConversion((*it).second.srcSpec.typeInfo)
                                .setUniform((*it).second.location, value);
                        } else if (auto it = p_compiledShader->bindingSpecs.find(nameId);
                                   it != p_compiledShader->bindingSpecs.end()) {
                            rend.getTypeConversion((*it).second.srcSpec.typeInfo)
                                .setBinding((*it).second.bindingIndex, value);
                        }
                    };

                    if (!needsRebuild) {

                        // iterate over materials
                        for (auto [material, props] : materials2props) {
                            MMETER_SCOPE_PROFILER("Material iteration");

                            // get the textures and properties
                            {
                                MMETER_SCOPE_PROFILER("Material setup");

                                for (auto [nameId, p_texture] : material->getTextures()) {
                                    setPropertyToShader(nameId, p_texture);
                                }
                                for (auto [nameId, value] : material->getProperties()) {
                                    setPropertyToShader(nameId, value);
                                }
                            }

                            // iterate over meshes
                            {
                                MMETER_SCOPE_PROFILER("Prop loop");

                                for (auto p_meshProp : props) {
                                    OpenGLMesh &mesh =
                                        static_cast<OpenGLMesh &>(*p_meshProp->p_mesh);

                                    glBindVertexArray(mesh.VAO);
                                    glUniformMatrix4fv(
                                        glModelMatrixUniformLocation, 1, GL_FALSE,
                                        &(p_meshProp->transform.getModelMatrix()[0][0]));
                                    glDrawElements(GL_TRIANGLES, 3 * mesh.getTriangles().size(),
                                                   GL_UNSIGNED_INT, 0);
                                }
                            }
                        }
                    }

                    glUseProgram(0);
                }
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

void OpenGLComposeSceneRender::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures, const ScopedDict &properties) const
{
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

StringView OpenGLComposeSceneRender::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae