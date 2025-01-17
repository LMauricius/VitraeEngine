#include "Vitrae/Renderers/OpenGL/Compositing/SceneRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Scene.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeSceneRender::OpenGLComposeSceneRender(const SetupParams &params)
    : m_root(params.root), m_params(params)
{
    m_friendlyName = "Render scene:\n";
    m_friendlyName += params.rasterizing.vertexPositionOutputPropertyName;
    switch (params.rasterizing.cullingMode) {
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
    switch (params.rasterizing.rasterizingMode) {
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
    if (params.rasterizing.smoothFilling || params.rasterizing.smoothTracing ||
        params.rasterizing.smoothDotting) {
        m_friendlyName += "\n- smooth";
        if (params.rasterizing.smoothFilling) {
            m_friendlyName += " filling";
        }
        if (params.rasterizing.smoothTracing) {
            m_friendlyName += " tracing";
        }
        if (params.rasterizing.smoothDotting) {
            m_friendlyName += " dotting";
        }
    }

    for (const auto &spec : params.inputTokenNames) {
        m_params.ordering.inputSpecs.insert_back({.name = spec, .typeInfo = TYPE_INFO<void>});
    }
    for (const auto &spec : params.outputTokenNames) {
        m_outputSpecs.insert_back({.name = spec, .typeInfo = TYPE_INFO<void>});
    }

    m_params.ordering.inputSpecs.insert_back(SCENE_SPEC);
    m_params.ordering.filterSpecs.insert_back(FRAME_STORE_TARGET_SPEC);
}

std::size_t OpenGLComposeSceneRender::memory_cost() const
{
    return sizeof(*this);
}

const ParamList &OpenGLComposeSceneRender::getInputSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->inputSpecs;
    } else {
        return m_params.ordering.inputSpecs;
    }
}

const ParamList &OpenGLComposeSceneRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &OpenGLComposeSceneRender::getFilterSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->filterSpecs;
    } else {
        return m_params.ordering.filterSpecs;
    }
}

const ParamList &OpenGLComposeSceneRender::getConsumingSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->consumingSpecs;
    } else {
        return m_params.ordering.consumingSpecs;
    }
}

void OpenGLComposeSceneRender::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                                const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        const SpecsPerAliases &specsPerAliases = *(*it).second;

        for (const ParamList *p_specs :
             {&specsPerAliases.inputSpecs, &m_outputSpecs, &specsPerAliases.filterSpecs,
              &specsPerAliases.consumingSpecs}) {
            for (const ParamSpec &spec : p_specs->getSpecList()) {
                typeSet.insert(&spec.typeInfo);
            }
        }
    }
}

void OpenGLComposeSceneRender::extractSubTasks(std::set<const Task *> &taskSet,
                                               const ParamAliases &aliases) const
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
            specsContainer.inputSpecs.insert_back({.name = tokenName, .typeInfo = TYPE_INFO<void>});
        }

        specsContainer.inputSpecs.insert_back(SCENE_SPEC);
        specsContainer.filterSpecs.insert_back(FRAME_STORE_TARGET_SPEC);

        specsContainer.inputSpecs.merge(m_params.ordering.inputSpecs);
        specsContainer.consumingSpecs.merge(m_params.ordering.consumingSpecs);
        specsContainer.filterSpecs.merge(m_params.ordering.filterSpecs);
    }

    SpecsPerAliases &specsContainer = *(*specsIt).second;

    // extract common inputs
    Scene &scene = *args.properties.get(SCENE_SPEC.name).get<dynasma::FirmPtr<Scene>>();

    dynasma::FirmPtr<FrameStore> p_frame =
        args.properties.get(FRAME_STORE_TARGET_SPEC.name).get<dynasma::FirmPtr<FrameStore>>();
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    // build map of shaders to materials to mesh props
    /*std::map<ParamAliases,
             std::map<dynasma::FirmPtr<const Material>, std::vector<const MeshProp *>>,
             bool (*)(const ParamAliases &, const ParamAliases &)>
        aliases2materials2props(
            [](const ParamAliases &l, const ParamAliases &r) { return l.hash() < r.hash(); });
*/
    /*{

        MMETER_SCOPE_PROFILER("Scene struct gen");

        for (auto &meshProp : scene.meshProps) {
            auto mat = meshProp.p_mesh->getMaterial().getLoaded();
            OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*meshProp.p_mesh);
            mesh.loadToGPU(rend);

            const ParamAliases *p_aliaseses[] = {&mat->getParamAliases(), &args.aliases};

            aliases2materials2props[ParamAliases(p_aliaseses)][mat].push_back(&meshProp);
        }
    }*/

    auto [meshFilter, meshComparator] = m_params.ordering.generateFilterAndSort(scene, args);

    m_sortedMeshProps.clear();
    m_sortedMeshProps.reserve(scene.meshProps.size());

    for (auto &meshProp : scene.meshProps) {
        if (meshFilter(meshProp)) {
            OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*meshProp.p_mesh);
            mesh.loadToGPU(rend);

            m_sortedMeshProps.push_back(&meshProp);
        }
    }

    std::sort(m_sortedMeshProps.begin(), m_sortedMeshProps.end(),
              [&](const MeshProp *l, const MeshProp *r) { return meshComparator(*l, *r); });

    // check for whether we have all input deps or whether we need to update the pipeline
    bool needsRebuild = false;

    {
        MMETER_SCOPE_PROFILER("Rendering (multipass)");

        switch (m_params.rasterizing.rasterizingMode) {
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

                switch (m_params.rasterizing.cullingMode) {
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
                if (m_params.rasterizing.smoothFilling) {
                    glEnable(GL_POLYGON_SMOOTH);
                    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
                } else {
                    glDisable(GL_POLYGON_SMOOTH);
                }
                if (m_params.rasterizing.smoothTracing) {
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
                dynasma::FirmPtr<const Material> p_currentMaterial;
                dynasma::FirmPtr<CompiledGLSLShader> p_currentShader;
                std::size_t currentShaderHash = 0;
                GLint glModelMatrixUniformLocation;

                for (auto p_meshProp : m_sortedMeshProps) {
                    dynasma::FirmPtr<const Material> p_nextMaterial =
                        p_meshProp->p_mesh->getMaterial();

                    if (p_nextMaterial != p_currentMaterial) {
                        MMETER_SCOPE_PROFILER("Material iteration");

                        p_currentMaterial = p_nextMaterial;

                        if (p_currentMaterial->getParamAliases().hash() != currentShaderHash) {
                            MMETER_SCOPE_PROFILER("Shader change");

                            {
                                MMETER_SCOPE_PROFILER("Shader loading");

                                currentShaderHash = p_currentMaterial->getParamAliases().hash();

                                const ParamAliases *p_aliaseses[] = {
                                    &p_currentMaterial->getParamAliases(), &args.aliases};

                                ParamAliases aliases(p_aliaseses);

                                p_currentShader = shaderCacher.retrieve_asset(
                                    {CompiledGLSLShader::SurfaceShaderParams(
                                        aliases,
                                        m_params.rasterizing.vertexPositionOutputPropertyName,
                                        *frame.getRenderComponents(), m_root)});

                                // Store pipeline property specs

                                using ListConvPair = std::pair<const ParamList *, ParamList *>;

                                for (auto [p_specs, p_targetSpecs] :
                                     {ListConvPair{&p_currentShader->inputSpecs,
                                                   &specsContainer.inputSpecs},
                                      ListConvPair{&p_currentShader->filterSpecs,
                                                   &specsContainer.filterSpecs},
                                      ListConvPair{&p_currentShader->consumingSpecs,
                                                   &specsContainer.consumingSpecs}}) {
                                    for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
                                        if (p_currentMaterial->getProperties().find(nameId) ==
                                                p_currentMaterial->getProperties().end() &&
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
                                MMETER_SCOPE_PROFILER("Shader setup");

                                // OpenGL - use the program
                                glUseProgram(p_currentShader->programGLName);

                                // Aliases should've already been taken into account, so use
                                // properties directly
                                VariantScope &directProperties =
                                    args.properties.getUnaliasedScope();

                                // set the 'environmental' uniforms
                                // skip those that will be set by the material
                                if (auto it = p_currentShader->uniformSpecs.find(
                                        StandardShaderPropertyNames::INPUT_MODEL);
                                    it != p_currentShader->uniformSpecs.end()) {
                                    glModelMatrixUniformLocation = (*it).second.location;
                                } else {
                                    glModelMatrixUniformLocation = -1;
                                }

                                p_currentShader->setupNonMaterialProperties(rend, directProperties,
                                                                            *p_currentMaterial);
                            }
                        }

                        if (!needsRebuild) {
                            p_currentShader->setupMaterialProperties(rend, *p_currentMaterial);
                        }
                    }

                    if (!needsRebuild) {
                        MMETER_SCOPE_PROFILER("Mesh draw");

                        OpenGLMesh &mesh = static_cast<OpenGLMesh &>(*p_meshProp->p_mesh);

                        glBindVertexArray(mesh.VAO);
                        if (glModelMatrixUniformLocation != -1) {
                            glUniformMatrix4fv(glModelMatrixUniformLocation, 1, GL_FALSE,
                                               &(p_meshProp->transform.getModelMatrix()[0][0]));
                        }
                        glDrawElements(GL_TRIANGLES, 3 * mesh.getTriangles().size(),
                                       GL_UNSIGNED_INT, 0);
                    }
                }

                glUseProgram(0);
            };

            auto getBlending = [](BlendingFunction blending) {
                switch (blending) {
                case BlendingFunction::Zero:
                    return GL_ZERO;
                case BlendingFunction::One:
                    return GL_ONE;
                case BlendingFunction::SourceColor:
                    return GL_SRC_COLOR;
                case BlendingFunction::OneMinusSourceColor:
                    return GL_ONE_MINUS_SRC_COLOR;
                case BlendingFunction::DestinationColor:
                    return GL_DST_COLOR;
                case BlendingFunction::OneMinusDestinationColor:
                    return GL_ONE_MINUS_DST_COLOR;
                case BlendingFunction::SourceAlpha:
                    return GL_SRC_ALPHA;
                case BlendingFunction::OneMinusSourceAlpha:
                    return GL_ONE_MINUS_SRC_ALPHA;
                case BlendingFunction::DestinationAlpha:
                    return GL_DST_ALPHA;
                case BlendingFunction::OneMinusDestinationAlpha:
                    return GL_ONE_MINUS_DST_ALPHA;
                case BlendingFunction::ConstantColor:
                    return GL_CONSTANT_COLOR;
                case BlendingFunction::OneMinusConstantColor:
                    return GL_ONE_MINUS_CONSTANT_COLOR;
                case BlendingFunction::ConstantAlpha:
                    return GL_CONSTANT_ALPHA;
                case BlendingFunction::OneMinusConstantAlpha:
                    return GL_ONE_MINUS_CONSTANT_ALPHA;
                case BlendingFunction::SourceAlphaSaturated:
                    return GL_SRC_ALPHA_SATURATE;
                }
                return GL_ZERO;
            };
            auto setBlending = [&](const RasterizingSetupParams &params) {
                glDepthMask(params.writeDepth);
                glBlendFunc(getBlending(params.sourceBlending),
                            getBlending(params.destinationBlending));
                if (params.sourceBlending == BlendingFunction::One &&
                    params.destinationBlending == BlendingFunction::Zero) {
                    glDisable(GL_BLEND);
                } else {
                    glEnable(GL_BLEND);
                }
            };

            // render filled polygons
            switch (m_params.rasterizing.rasterizingMode) {
            case RasterizingMode::DerivationalFillCenters:
            case RasterizingMode::DerivationalFillEdges:
            case RasterizingMode::DerivationalFillVertices:
                setBlending(m_params.rasterizing);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                runDerivationalPass();
                break;
            }

            // render edges
            switch (m_params.rasterizing.rasterizingMode) {
            case RasterizingMode::DerivationalFillEdges:
            case RasterizingMode::DerivationalTraceEdges:
            case RasterizingMode::DerivationalTraceVertices:
                if (m_params.rasterizing.smoothTracing) {
                    glDepthMask(GL_FALSE);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glEnable(GL_BLEND);
                    glLineWidth(1.5);
                } else {
                    setBlending(m_params.rasterizing);
                    glLineWidth(1.0);
                }
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                runDerivationalPass();
                break;
            }

            // render vertices
            switch (m_params.rasterizing.rasterizingMode) {
            case RasterizingMode::DerivationalFillVertices:
            case RasterizingMode::DerivationalTraceVertices:
            case RasterizingMode::DerivationalDotVertices:
                setBlending(m_params.rasterizing);
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

std::size_t OpenGLComposeSceneRender::getSpecsKey(const ParamAliases &aliases) const
{
    return combinedHashes<2>(
        {{std::hash<StringId>{}(m_params.rasterizing.vertexPositionOutputPropertyName),
          aliases.hash()}});
}

} // namespace Vitrae