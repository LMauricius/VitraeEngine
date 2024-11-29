#include "Vitrae/Renderers/OpenGL/Compositing/Compute.hpp"
#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"
#include "Vitrae/Util/PropertyList.hpp"
#include "Vitrae/Util/Variant.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include "MMeter.h"

namespace Vitrae
{

OpenGLComposeCompute::OpenGLComposeCompute(const SetupParams &params)
    : ComposeCompute(std::span<const PropertySpec>{},
                     std::span<const PropertySpec>(params.outputSpecs)),
      m_root(params.root), m_computeSetup(params.computeSetup),
      m_executeCondition(params.executeCondition)
{
    // base inputs
    if (!m_computeSetup.invocationCountX.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.invocationCountX.getSpec().name,
                             m_computeSetup.invocationCountX.getSpec());
    }
    if (!m_computeSetup.invocationCountY.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.invocationCountY.getSpec().name,
                             m_computeSetup.invocationCountY.getSpec());
    }
    if (!m_computeSetup.invocationCountZ.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.invocationCountZ.getSpec().name,
                             m_computeSetup.invocationCountZ.getSpec());
    }
    if (!m_computeSetup.groupSizeX.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.groupSizeX.getSpec().name,
                             m_computeSetup.groupSizeX.getSpec());
    }
    if (!m_computeSetup.groupSizeY.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.groupSizeY.getSpec().name,
                             m_computeSetup.groupSizeY.getSpec());
    }
    if (!m_computeSetup.groupSizeZ.isFixed()) {
        m_inputSpecs.emplace(m_computeSetup.groupSizeZ.getSpec().name,
                             m_computeSetup.groupSizeZ.getSpec());
    }

    // setup members
    mp_outputComponents = dynasma::makeStandalone<PropertyList>(m_outputSpecs.values());

    // friendly name gen
    m_friendlyName = "Compute\n";
    for (const auto &spec : params.outputSpecs) {
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

const StableMap<StringId, PropertySpec> &OpenGLComposeCompute::getInputSpecs(
    const RenderSetupContext &args) const
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(args.renderer);
    return rend.getInputDependencyCache(rend.getInputDependencyCacheID(
        this, args.p_defaultVertexMethod, args.p_defaultFragmentMethod));
}

const StableMap<StringId, PropertySpec> &OpenGLComposeCompute::getOutputSpecs() const
{
    return m_outputSpecs;
}

void OpenGLComposeCompute::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER(m_friendlyName.c_str());

    if (m_executeCondition && !m_executeCondition(args)) {
        return;
    }

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(m_root.getComponent<Renderer>());
    CompiledGLSLShaderCacher &shaderCacher = m_root.getComponent<CompiledGLSLShaderCacher>();
    auto &inputDependencies = rend.getEditableInputDependencyCache(rend.getInputDependencyCacheID(
        this, args.p_defaultVertexMethod, args.p_defaultFragmentMethod));

    bool needsRebuild = false;
    auto specifyInputDependency = [&](const PropertySpec &spec) {
        needsRebuild = needsRebuild || inputDependencies.emplace(spec.name, spec).second;
    };

    // get invocation count
    glm::ivec3 invocationCount = {
        m_computeSetup.invocationCountX.get(args.properties),
        m_computeSetup.invocationCountY.get(args.properties),
        m_computeSetup.invocationCountZ.get(args.properties),
    };

    glm::ivec3 specifiedGroupSize = {
        m_computeSetup.groupSizeX.get(args.properties),
        m_computeSetup.groupSizeY.get(args.properties),
        m_computeSetup.groupSizeZ.get(args.properties),
    };

    glm::ivec3 decidedGroupSize = {
        specifiedGroupSize.x == GROUP_SIZE_AUTO ? 64 : specifiedGroupSize.x,
        specifiedGroupSize.y == GROUP_SIZE_AUTO ? 1 : specifiedGroupSize.y,
        specifiedGroupSize.z == GROUP_SIZE_AUTO ? 1 : specifiedGroupSize.z,
    };

    // compile shader for this compute execution
    dynasma::FirmPtr<CompiledGLSLShader> p_compiledShader =
        shaderCacher.retrieve_asset({CompiledGLSLShader::ComputeShaderParams(
            m_root, args.p_defaultComputeMethod, mp_outputComponents,
            m_computeSetup.invocationCountX, m_computeSetup.invocationCountY,
            m_computeSetup.invocationCountZ, decidedGroupSize,
            m_computeSetup.allowOutOfBoundsCompute)});

    glUseProgram(p_compiledShader->programGLName);

    // set uniforms
    for (auto tokenPropName : p_compiledShader->tokenPropertyNames) {
        specifyInputDependency(PropertySpec{
            .name = tokenPropName,
            .typeInfo = Variant::getTypeInfo<void>(),
        });
    }

    for (auto [propertyNameId, uniSpec] : p_compiledShader->uniformSpecs) {
        auto p = args.properties.getPtr(propertyNameId);
        if (p) {
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo).setUniform(uniSpec.location, *p);
        }

        specifyInputDependency(uniSpec.srcSpec);
    }

    for (auto [propertyNameId, bindingSpec] : p_compiledShader->bindingSpecs) {
        auto p = args.properties.getPtr(propertyNameId);
        if (p) {
            rend.getTypeConversion(bindingSpec.srcSpec.typeInfo)
                .setBinding(bindingSpec.bindingIndex, *p);
        }

        specifyInputDependency(bindingSpec.srcSpec);
    }

    if (needsRebuild) {
        throw ComposeTaskRequirementsChangedException();
    }

    // compute
    glDispatchCompute((invocationCount.x + decidedGroupSize.x - 1) / decidedGroupSize.x,
                      (invocationCount.y + decidedGroupSize.y - 1) / decidedGroupSize.y,
                      (invocationCount.z + decidedGroupSize.z - 1) / decidedGroupSize.z);

    // the outputs should be the same pointers as inputs
    /// TODO: allow non-SSBO outputs

    // wait (for profiling)
#ifdef VITRAE_ENABLE_DETERMINISTIC_RENDERING
    glFinish();
#endif
}

void OpenGLComposeCompute::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
    const ScopedDict &properties, const RenderSetupContext &context) const {}

StringView OpenGLComposeCompute::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae