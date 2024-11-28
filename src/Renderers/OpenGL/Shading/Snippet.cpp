#include "Vitrae/Renderers/OpenGL/Shading/Snippet.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/ConstantDefs.hpp"
#include "Vitrae/Util/StringProcessing.hpp"

#include <fstream>

namespace Vitrae {

OpenGLShaderSnippet::OpenGLShaderSnippet(const StringParams &params)
    : ShaderSnippet(params) {

    m_bodySnippet = clearIndents(params.snippet);
    m_functionName = String(GLSL_SHADER_INTERNAL_FUNCTION_PREFIX) + "calc";

    for (const auto &spec : params.inputSpecs) {
        if (spec.typeInfo != Variant::getTypeInfo<void>()) {
            m_inputOrder.emplace_back(spec.name);
        }
    }
    for (const auto &spec : params.outputSpecs) {
        if (spec.typeInfo != Variant::getTypeInfo<void>()) {
            m_outputOrder.emplace_back(spec.name);
        }

        m_functionName += "_" + spec.name;
    }
}

std::size_t OpenGLShaderSnippet::memory_cost() const {
    /// TODO: Calculate the cost of the function
    return 1;
}

void OpenGLShaderSnippet::extractUsedTypes(
    std::set<const TypeInfo *> &typeSet) const {
    for (auto &specs : {m_inputSpecs, m_outputSpecs}) {
        for (auto [nameId, spec] : specs) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void OpenGLShaderSnippet::extractSubTasks(
    std::set<const Task *> &taskSet) const {
    taskSet.insert(this);
}

void OpenGLShaderSnippet::outputDeclarationCode(BuildContext args) const {
    outputFunctionDeclaration(args);
    args.output << ";";
}

void OpenGLShaderSnippet::outputDefinitionCode(BuildContext args) const {
    outputFunctionDeclaration(args);
    args.output << " {\n";
    args.output << m_bodySnippet;
    args.output << "}";
}

void OpenGLShaderSnippet::outputUsageCode(
    BuildContext args,
    const StableMap<StringId, String> &inputParamsToSharedVariables,
    const StableMap<StringId, String> &outputParamsToSharedVariables) const {
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);

    args.output << m_functionName << "(";
    bool hadFirstArg = false;
    for (const auto &nameId : m_inputOrder) {
        const PropertySpec &spec = m_inputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec =
            renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (renderer.getGpuStorageMethod(glTypeSpec) !=
            OpenGLRenderer::GpuValueStorageMethod::SSBO) {
            if (hadFirstArg) {
                args.output << ", ";
            }
            args.output << inputParamsToSharedVariables.at(nameId);
            hadFirstArg = true;
        }
    }
    for (const auto &nameId : m_outputOrder) {
        const PropertySpec &spec = m_outputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec =
            renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (renderer.getGpuStorageMethod(glTypeSpec) !=
            OpenGLRenderer::GpuValueStorageMethod::SSBO) {
            if (hadFirstArg) {
                args.output << ", ";
            }
            args.output << outputParamsToSharedVariables.at(nameId);
            hadFirstArg = true;
        }
    }
    args.output << ");";
}

StringView OpenGLShaderSnippet::getFriendlyName() const {
    return m_functionName;
}

void OpenGLShaderSnippet::outputFunctionDeclaration(BuildContext args) const {
    OpenGLRenderer &renderer = static_cast<OpenGLRenderer &>(args.renderer);

    args.output << "void " << m_functionName << "(";
    bool hadFirstArg = false;
    for (const auto &nameId : m_inputOrder) {
        const PropertySpec &spec = m_inputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec =
            renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (renderer.getGpuStorageMethod(glTypeSpec) !=
            OpenGLRenderer::GpuValueStorageMethod::SSBO) {
            if (hadFirstArg) {
                args.output << ", ";
            }
            args.output << glTypeSpec.glConstTypeName;
            hadFirstArg = true;
        }
    }
    for (const auto &nameId : m_outputOrder) {
        const PropertySpec &spec = m_outputSpecs.at(nameId);
        const GLTypeSpec &glTypeSpec =
            renderer.getTypeConversion(spec.typeInfo).glTypeSpec;
        if (renderer.getGpuStorageMethod(glTypeSpec) !=
            OpenGLRenderer::GpuValueStorageMethod::SSBO) {
            if (hadFirstArg) {
                args.output << ", ";
            }
            args.output << "out " << glTypeSpec.glMutableTypeName;
            hadFirstArg = true;
        }
    }
    args.output << ")";
}

} // namespace Vitrae
