#include "Vitrae/Renderers/OpenGL/Compositing/Compute.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/Assets/Scene.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Params/ParamList.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"

#include "dynasma/standalone.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeCompute::OpenGLComposeCompute(const SetupParams &params) : m_params(params)
{
    // friendly name gen
    m_friendlyName = "Compute\n";
    for (const auto &spec : params.outputSpecs.getSpecList()) {
        m_friendlyName += "- " + spec.name + "\n";
    }
    m_friendlyName += "[";

    if (params.computeSetup.invocationCountX.isFixed()) {
        m_friendlyName += std::to_string(params.computeSetup.invocationCountX.getFixedValue());
    } else {
        m_friendlyName += params.computeSetup.invocationCountX.getSpec().name;
    }
    m_friendlyName += ", ";
    if (params.computeSetup.invocationCountY.isFixed()) {
        m_friendlyName += std::to_string(params.computeSetup.invocationCountY.getFixedValue());
    } else {
        m_friendlyName += params.computeSetup.invocationCountY.getSpec().name;
    }
    m_friendlyName += ", ";
    if (params.computeSetup.invocationCountZ.isFixed()) {
        m_friendlyName += std::to_string(params.computeSetup.invocationCountZ.getFixedValue());
    } else {
        m_friendlyName += params.computeSetup.invocationCountZ.getSpec().name;
    }

    m_friendlyName += "]";
}

std::size_t OpenGLComposeCompute::memory_cost() const
{
    /// TODO: calculate the real memory cost
    return sizeof(*this);
}

const ParamList &OpenGLComposeCompute::getInputSpecs(const ParamAliases &aliases) const
{
    return getProgramPerAliases(aliases).inputSpecs;
}

const ParamList &OpenGLComposeCompute::getOutputSpecs() const
{
    return m_params.outputSpecs;
}

const ParamList &OpenGLComposeCompute::getFilterSpecs(const ParamAliases &aliases) const
{
    return getProgramPerAliases(aliases).filterSpecs;
}

const ParamList &OpenGLComposeCompute::getConsumingSpecs(const ParamAliases &aliases) const
{
    return getProgramPerAliases(aliases).consumeSpecs;
}

void OpenGLComposeCompute::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                            const ParamAliases &aliases) const
{
    const auto &specs = getProgramPerAliases(aliases);

    for (const ParamList *p_specs :
         {&specs.inputSpecs, &m_params.outputSpecs, &specs.filterSpecs, &specs.consumeSpecs}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLComposeCompute::extractSubTasks(std::set<const Task *> &taskSet,
                                           const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void OpenGLComposeCompute::run(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER(m_friendlyName.c_str());

    ProgramPerAliases &programPerAliases = getProgramPerAliases(args.aliases);

    // determine whether we need to run in the first place
    bool needsToRun;
    if (!m_params.cacheResults) {
        needsToRun = true;
    } else {
        needsToRun = false;

        for (auto p_specs : {&programPerAliases.inputSpecs, &programPerAliases.consumeSpecs,
                             &programPerAliases.filterSpecs}) {
            for (auto nameId : p_specs->getSpecNameIds()) {
                if (programPerAliases.cachedDependencies.find(nameId) ==
                        programPerAliases.cachedDependencies.end() ||
                    args.properties.get(nameId) !=
                        programPerAliases.cachedDependencies.at(nameId)) {
                    needsToRun = true;
                    programPerAliases.cachedDependencies.at(nameId) = args.properties.get(nameId);
                }
            }
        }

        for (auto p_specs :
             {&m_params.outputSpecs, (const ParamList *)&programPerAliases.filterSpecs}) {
            for (auto nameId : p_specs->getSpecNameIds()) {
                if (!args.properties.has(nameId)) {
                    needsToRun = true;
                    break;
                }
            }
        }
    }

    if (!needsToRun) {
        return;
    }

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_params.root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_params.root.getComponent<CompiledGLSLShaderCacher>();

    // get invocation count

    glm::ivec3 specifiedGroupSize = {
        m_params.computeSetup.groupSizeX.get(args.properties),
        m_params.computeSetup.groupSizeY.get(args.properties),
        m_params.computeSetup.groupSizeZ.get(args.properties),
    };

    glm::ivec3 decidedGroupSize = {
        specifiedGroupSize.x == GROUP_SIZE_AUTO ? 64 : specifiedGroupSize.x,
        specifiedGroupSize.y == GROUP_SIZE_AUTO ? 1 : specifiedGroupSize.y,
        specifiedGroupSize.z == GROUP_SIZE_AUTO ? 1 : specifiedGroupSize.z,
    };

    // compile shader for this compute execution
    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
        shaderCacher.retrieve_asset({CompiledGLSLShader::ComputeShaderParams(
            m_params.root, args.aliases, m_params.outputSpecs,
            m_params.computeSetup.invocationCountX, m_params.computeSetup.invocationCountY,
            m_params.computeSetup.invocationCountZ, decidedGroupSize,
            m_params.computeSetup.allowOutOfBoundsCompute)});

    glUseProgram(p_compiledShader->programGLName);

    // set uniforms
    p_compiledShader->setupProperties(rend, args.properties.getUnaliasedScope());

    // compute
    glm::ivec3 invocationCount = {
        m_params.computeSetup.invocationCountX.get(args.properties),
        m_params.computeSetup.invocationCountY.get(args.properties),
        m_params.computeSetup.invocationCountZ.get(args.properties),
    };
    glDispatchCompute((invocationCount.x + decidedGroupSize.x - 1) / decidedGroupSize.x,
                      (invocationCount.y + decidedGroupSize.y - 1) / decidedGroupSize.y,
                      (invocationCount.z + decidedGroupSize.z - 1) / decidedGroupSize.z);

    // the outputs should be the same pointers as inputs
    /// TODO: allow non-SSBO outputs

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    {
        MMETER_SCOPE_PROFILER("Waiting for GL operations");

        glFinish();
    }
#endif
}

void OpenGLComposeCompute::prepareRequiredLocalAssets(RenderComposeContext args) const {}

StringView OpenGLComposeCompute::getFriendlyName() const
{
    return m_friendlyName;
}

OpenGLComposeCompute::ProgramPerAliases &OpenGLComposeCompute::getProgramPerAliases(
    const ParamAliases &aliases) const
{
    if (auto it = m_programPerAliasHash.find(aliases.hash()); it != m_programPerAliasHash.end()) {
        return *(*it).second;
    } else {
        OpenGLRenderer &rend =
            static_cast<OpenGLRenderer &>(m_params.root.getComponent<Renderer>());
        CompiledGLSLShaderCacher &shaderCacher =
            m_params.root.getComponent<CompiledGLSLShaderCacher>();

        // dummy group size
        glm::ivec3 decidedGroupSize = {1, 1, 1};

        // compile shader for this compute execution
        dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
            shaderCacher.retrieve_asset({CompiledGLSLShader::ComputeShaderParams(
                m_params.root, aliases, m_params.outputSpecs,
                m_params.computeSetup.invocationCountX, m_params.computeSetup.invocationCountY,
                m_params.computeSetup.invocationCountZ, decidedGroupSize,
                m_params.computeSetup.allowOutOfBoundsCompute)});

        return *(*m_programPerAliasHash
                      .emplace(aliases.hash(),
                               new ProgramPerAliases{
                                   .inputSpecs = p_compiledShader->inputSpecs,
                                   .filterSpecs = p_compiledShader->filterSpecs,
                                   .consumeSpecs = p_compiledShader->consumingSpecs,
                               })
                      .first)
                    .second;
    }
}

} // namespace Vitrae