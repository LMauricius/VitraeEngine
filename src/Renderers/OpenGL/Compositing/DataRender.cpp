#include "Vitrae/Renderers/OpenGL/Compositing/DataRender.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeDataRender::OpenGLComposeDataRender(const SetupParams &params)
    : m_root(params.root), m_params(params)
{
    m_friendlyName = "Render data:";
    for (const auto &spec : params.outputTokenNames) {
        m_friendlyName += "\n- " + spec;
        m_outputSpecs.insert_back({.name = spec, .typeInfo = Variant::getTypeInfo<void>()});
    }
}

std::size_t OpenGLComposeDataRender::memory_cost() const
{
    /// TODO: compute the real cost
    return sizeof(OpenGLComposeDataRender);
}

const ParamList &OpenGLComposeDataRender::getInputSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->inputSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

const ParamList &OpenGLComposeDataRender::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &OpenGLComposeDataRender::getFilterSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->filterSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

const ParamList &OpenGLComposeDataRender::getConsumingSpecs(const ParamAliases &aliases) const
{
    if (auto it = m_specsPerKey.find(getSpecsKey(aliases)); it != m_specsPerKey.end()) {
        return (*it).second->consumingSpecs;
    } else {
        return EMPTY_PROPERTY_LIST;
    }
}

void OpenGLComposeDataRender::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
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

void OpenGLComposeDataRender::extractSubTasks(std::set<const Task *> &taskSet,
                                              const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLComposeDataRender::run(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER("OpenGLComposeDataRender::run");

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
    }

    SpecsPerAliases &specsContainer = *(*specsIt).second;

    // extract common inputs
    dynasma::FirmPtr<FrameStore> p_frame =
        args.properties.get(FRAME_STORE_TARGET_SPEC.name).get<dynasma::FirmPtr<FrameStore>>();
    OpenGLFrameStore &frame = static_cast<OpenGLFrameStore &>(*p_frame);

    auto p_mesh = m_params.p_dataPointMesh.getLoaded();

    auto p_mat = p_mesh->getMaterial().getLoaded();

    const ParamAliases *p_aliaseses[] = {&p_mat->getParamAliases(), &args.aliases};
    ParamAliases combinedAliases(p_aliaseses);

    // Compile and setup the shader
    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader;
    GLint glModelMatrixUniformLocation;

    {
        MMETER_SCOPE_PROFILER("Shader setup");

        // compile shader for this material
        {
            MMETER_SCOPE_PROFILER("Shader loading");

            p_compiledShader = shaderCacher.retrieve_asset({CompiledGLSLShader::SurfaceShaderParams(
                combinedAliases, m_params.vertexPositionOutputPropertyName,
                *frame.getRenderComponents(), m_root)});

            // Aliases should've already been taken into account, so use properties directly
            VariantScope &directProperties = args.properties.getUnaliasedScope();

            // Store pipeline property specs
            bool needsRebuild = false;
            needsRebuild |= (specsContainer.inputSpecs.merge(p_compiledShader->inputSpecs) > 0);
            needsRebuild |= (specsContainer.filterSpecs.merge(p_compiledShader->filterSpecs) > 0);
            needsRebuild |=
                (specsContainer.consumingSpecs.merge(p_compiledShader->consumingSpecs) > 0);

            if (needsRebuild) {
                throw ComposeTaskRequirementsChangedException();
            }

            glUseProgram(p_compiledShader->programGLName);
        }

        // Aliases should've already been taken into account, so use properties directly
        VariantScope &directProperties = args.properties.getUnaliasedScope();

        {
            MMETER_SCOPE_PROFILER("Uniform setup");

            p_compiledShader->setupProperties(rend, directProperties, *p_mat);
        }
    }

    // render
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
                MMETER_SCOPE_PROFILER("Render data");

                // run the data generator

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

                m_params.dataGenerator(args, renderCallback);

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

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    {
        MMETER_SCOPE_PROFILER("Waiting for GL operations");

        glFinish();
    }
#endif
}

void OpenGLComposeDataRender::prepareRequiredLocalAssets(RenderComposeContext ctx) const {}

StringView OpenGLComposeDataRender::getFriendlyName() const
{
    return m_friendlyName;
}

std::size_t OpenGLComposeDataRender::getSpecsKey(const ParamAliases &aliases) const
{
    return combinedHashes<2>(
        {{std::hash<StringId>{}(m_params.vertexPositionOutputPropertyName), aliases.hash()}});
}

} // namespace Vitrae