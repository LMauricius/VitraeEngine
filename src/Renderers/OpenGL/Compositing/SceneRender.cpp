#include "Vitrae/Renderers/OpenGL/Compositing/SceneRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include "MMeter.h"

namespace Vitrae
{

const PropertyList OpenGLComposeSceneRender::SHARED_INPUT_PROPERTY_LIST = {SCENE_SPEC};
const PropertyList OpenGLComposeSceneRender::SHARED_FILTER_PROPERTY_LIST = {
    FRAME_STORE_TARGET_SPEC};

OpenGLComposeSceneRender::OpenGLComposeSceneRender(const SetupParams &params)
    : m_root(params.root), m_params(params)
{
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

    for (const auto &spec : params.outputTokenNames) {
        m_outputSpecs.insert_back({.name = spec, .typeInfo = Variant::getTypeInfo<void>()});
    }
}

std::size_t OpenGLComposeSceneRender::memory_cost() const
{
    return sizeof(*this);
}

const PropertyList &OpenGLComposeSceneRender::getInputSpecs(const PropertyAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->inputSpecs;
    } else {
        return SHARED_INPUT_PROPERTY_LIST;
    }
}

const PropertyList &OpenGLComposeSceneRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

const PropertyList &OpenGLComposeSceneRender::getFilterSpecs(const PropertyAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->filterSpecs;
    } else {
        return SHARED_FILTER_PROPERTY_LIST;
    }
}

const PropertyList &OpenGLComposeSceneRender::getConsumingSpecs(
    const PropertyAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->consumingSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

void OpenGLComposeSceneRender::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                                const PropertyAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        const SpecsPerAliases &specsPerAliases = *(*it).second;

        for (const PropertyList *p_specs :
             {&specsPerAliases.inputSpecs, &m_outputSpecs, &specsPerAliases.filterSpecs,
              &specsPerAliases.consumingSpecs}) {
            for (const PropertySpec &spec : p_specs->getSpecList()) {
                typeSet.insert(&spec.typeInfo);
            }
        }
    }
}

void OpenGLComposeSceneRender::extractSubTasks(std::set<const Task *> &taskSet,
                                               const PropertyAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLComposeSceneRender::run(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeSceneRender::run");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();

    // Get specs cache and init it if needed
    std::size_t specsKey = getSpecsKey(args.aliases);
    auto specsIt = m_specsPerKey.find(specsKey);
    if (specsIt == m_specsPerKey.end()) {
        specsIt = m_specsPerKey.emplace(specsKey, new SpecsPerAliases()).first;
        SpecsPerAliases &specsContainer = *(*specsIt).second;

        for (auto &tokenName : m_params.inputTokenNames) {
            specsContainer.inputSpecs.insert_back(
                {.name = tokenName, .typeInfo = Variant::getTypeInfo<void>()});
        }

        specsContainer.inputSpecs.insert_back(SCENE_SPEC);
        specsContainer.filterSpecs.insert_back(FRAME_STORE_TARGET_SPEC);
    }

    SpecsPerAliases &specsContainer = *(*specsIt).second;

    // extract common inputs
    Scene &scene = *args.properties.get(SCENE_SPEC.name).get<dynasma::FirmPtr<Scene>>();

    dynasma::FirmPtr<FrameStore> p_frame =
        args.properties.get(FRAME_STORE_TARGET_SPEC.name).get<dynasma::FirmPtr<FrameStore>>();
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    // build map of shaders to materials to mesh props
    std::map<PropertyAliases,
             std::map<dynasma::FirmPtr<const Material>, std::vector<const MeshProp *>>,
             bool (*)(const PropertyAliases &, const PropertyAliases &)>
        aliases2materials2props(
            [](const PropertyAliases &l, const PropertyAliases &r) { return l.hash() < r.hash(); });

    {

        MMETER_SCOPE_PROFILER("Scene struct gen");

        for (auto &meshProp : scene.meshProps) {
            auto mat = meshProp.p_mesh->getMaterial().getLoaded();
            OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*meshProp.p_mesh);
            mesh.loadToGPU(rend);

            const PropertyAliases *p_aliaseses[] = {&mat->getPropertyAliases(), &args.aliases};

            aliases2materials2props[PropertyAliases(p_aliaseses)][mat].push_back(&meshProp);
        }
    }

    // check for whether we have all input deps or whether we need to update the pipeline
    bool needsRebuild = false;

    {
        MMETER_SCOPE_PROFILER("Rendering (multipass)");

        switch (m_params.rasterizingMode) {
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

                switch (m_params.cullingMode) {
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
                if (m_params.smoothFilling) {
                    glEnable(GL_POLYGON_SMOOTH);
                    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
                } else {
                    glDisable(GL_POLYGON_SMOOTH);
                }
                if (m_params.smoothTracing) {
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
                for (auto &[aliases, materials2props] : aliases2materials2props) {
                    MMETER_SCOPE_PROFILER("Shader iteration");

                    auto p_firstMat = materials2props.begin()->first;

                    // compile shader for this material
                    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader;

                    {
                        MMETER_SCOPE_PROFILER("Shader loading");

                        p_compiledShader =
                            shaderCacher.retrieve_asset({CompiledGLSLShader::SurfaceShaderParams(
                                aliases, m_params.vertexPositionOutputPropertyName,
                                *frame.getRenderComponents(), m_root)});

                        // Store pipeline property specs

                        using ListConvPair = std::pair<const PropertyList *, PropertyList *>;

                        for (auto [p_specs, p_targetSpecs] :
                             {ListConvPair{&p_compiledShader->inputSpecs,
                                           &specsContainer.inputSpecs},
                              ListConvPair{&p_compiledShader->filterSpecs,
                                           &specsContainer.filterSpecs},
                              ListConvPair{&p_compiledShader->consumingSpecs,
                                           &specsContainer.consumingSpecs}}) {
                            for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
                                if (p_firstMat->getProperties().find(nameId) ==
                                        p_firstMat->getProperties().end() &&
                                    nameId != StandardShaderPropertyNames::INPUT_MODEL &&
                                    p_targetSpecs->getMappedSpecs().find(nameId) ==
                                        p_targetSpecs->getMappedSpecs().end()) {
                                    p_targetSpecs->insert_back(spec);
                                    needsRebuild = true;
                                }
                            }
                        }
                    }

                    if (!needsRebuild) {
                        // OpenGL - use the program
                        glUseProgram(p_compiledShader->programGLName);

                        // Aliases should've already been taken into account, so use properties
                        // directly
                        VariantScope &directProperties = args.properties.getUnaliasedScope();

                        // set the 'environmental' uniforms
                        // skip those that will be set by the material
                        GLint glModelMatrixUniformLocation;
                        if (auto it = p_compiledShader->uniformSpecs.find(
                                StandardShaderPropertyNames::INPUT_MODEL);
                            it != p_compiledShader->uniformSpecs.end()) {
                            glModelMatrixUniformLocation = (*it).second.location;
                        } else {
                            glModelMatrixUniformLocation = -1;
                        }

                        p_compiledShader->setupNonMaterialProperties(rend, directProperties,
                                                                     *p_firstMat);

                        // iterate over materials
                        for (auto [material, props] : materials2props) {
                            MMETER_SCOPE_PROFILER("Material iteration");

                            p_compiledShader->setupMaterialProperties(rend, *material);

                            // iterate over meshes
                            {
                                MMETER_SCOPE_PROFILER("Prop loop");

                                for (auto p_meshProp : props) {
                                    OpenGLMesh &mesh =
                                        static_cast<OpenGLMesh &>(*p_meshProp->p_mesh);

                                    glBindVertexArray(mesh.VAO);
                                    if (glModelMatrixUniformLocation != -1) {
                                        glUniformMatrix4fv(
                                            glModelMatrixUniformLocation, 1, GL_FALSE,
                                            &(p_meshProp->transform.getModelMatrix()[0][0]));
                                    }
                                    glDrawElements(GL_TRIANGLES, 3 * mesh.getTriangles().size(),
                                                   GL_UNSIGNED_INT, 0);
                                }
                            }
                        }
                    }
                }

                glUseProgram(0);
            };

            // render filled polygons
            switch (m_params.rasterizingMode) {
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
            switch (m_params.rasterizingMode) {
            case RasterizingMode::DerivationalFillEdges:
            case RasterizingMode::DerivationalTraceEdges:
            case RasterizingMode::DerivationalTraceVertices:
                if (m_params.smoothTracing) {
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
            switch (m_params.rasterizingMode) {
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

    if (needsRebuild) {
        throw ComposeTaskRequirementsChangedException();
    }

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    {
        MMETER_SCOPE_PROFILER("Waiting for GL operations");

        glFinish();
    }
#endif
}

void OpenGLComposeSceneRender::prepareRequiredLocalAssets(RenderComposeContext args) const {}

StringView OpenGLComposeSceneRender::getFriendlyName() const
{
    return m_friendlyName;
}

std::size_t OpenGLComposeSceneRender::getSpecsKey(const PropertyAliases &aliases) const
{
    return combinedHashes<2>(
        {{std::hash<StringId>{}(m_params.vertexPositionOutputPropertyName), aliases.hash()}});
}

} // namespace Vitrae