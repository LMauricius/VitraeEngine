#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include "MMeter.h"

#include <fstream>

namespace Vitrae
{

CompiledGLSLShader::SurfaceShaderParams::SurfaceShaderParams(
    dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod,
    dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod, String vertexPositionOutputName,
    dynasma::FirmPtr<const PropertyList> p_fragmentOutputs, ComponentRoot &root)
    : mp_vertexMethod(p_vertexMethod), mp_fragmentMethod(p_fragmentMethod),
      m_vertexPositionOutputName(vertexPositionOutputName), mp_fragmentOutputs(p_fragmentOutputs),
      mp_root(&root),
      m_hash(combinedHashes<4>({{p_vertexMethod->getHash(), p_fragmentMethod->getHash(),
                                 std::hash<StringId>{}(StringId(vertexPositionOutputName)),
                                 p_fragmentOutputs->getHash()}}))
{}

CompiledGLSLShader::ComputeShaderParams::ComputeShaderParams(
    dynasma::FirmPtr<Method<ShaderTask>> p_computeMethod,
    dynasma::FirmPtr<const PropertyList> p_desiredResults, ComponentRoot &root)
    : mp_computeMethod(p_computeMethod), mp_desiredResults(p_desiredResults), mp_root(&root),
      m_hash(combinedHashes<2>({{p_computeMethod->getHash(), p_desiredResults->getHash()}}))
{}

CompiledGLSLShader::CompiledGLSLShader(const SurfaceShaderParams &params)
    : CompiledGLSLShader(
          {{
              CompilationSpec{.p_method = params.getVertexMethodPtr(),
                              .predefinedVarsToOutputs =
                                  {
                                      {"gl_Position", params.getVertexPositionOutputName()},
                                  },
                              .outVarPrefix = "vert_",
                              .shaderType = GL_VERTEX_SHADER},
              CompilationSpec{.p_method = params.getFragmentMethodPtr(),
                              .outVarPrefix = "frag_",
                              .shaderType = GL_FRAGMENT_SHADER},
          }},
          params.getRoot(), *params.getFragmentOutputsPtr())
{}

CompiledGLSLShader::CompiledGLSLShader(const ComputeShaderParams &params)
    : CompiledGLSLShader({{
                             CompilationSpec{.p_method = params.getComputeMethodPtr(),
                                             .outVarPrefix = "comp_",
                                             .shaderType = GL_COMPUTE_SHADER},
                         }},
                         params.getRoot(), *params.getDesiredResultsPtr())
{}

CompiledGLSLShader::CompiledGLSLShader(MovableSpan<CompilationSpec> compilationSpecs,
                                       ComponentRoot &root, const PropertyList &desiredOutputs)
{
    MMETER_SCOPE_PROFILER("CompiledGLSLShader");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(root.getComponent<Renderer>());

    // util lambdas
    auto specToMutableGlName = [&](const TypeInfo &typeInfo) -> std::string_view {
        return rend.getTypeConversion(typeInfo).glTypeSpec.glMutableTypeName;
    };
    auto specToConstGlName = [&](const TypeInfo &typeInfo) -> std::string_view {
        return rend.getTypeConversion(typeInfo).glTypeSpec.glConstTypeName;
    };

    // uniforms are global variables given to all shader steps
    String uniVarPrefix = "uniform_";
    String sharedVarPrefix = "shared_";
    String bufferVarPrefix = "buffer_";
    std::map<StringId, PropertySpec> uniformVarSpecs;

    // mesh vertex element data is given to the vertex shader and passed through to other steps
    String elemVarPrefix = "elem_";
    std::map<StringId, PropertySpec> elemVarSpecs;

    // local variables are intermediate results
    String localVarPrefix = "m_";

    struct CompilationHelp
    {
        // source specification for this stage
        CompilationSpec *p_compSpec;

        // items converted to OpenGLShaderTask
        Pipeline<ShaderTask> pipeline;

        // id of the compiled shader
        GLuint shaderId;
    };

    std::vector<CompilationHelp> helpers;
    std::vector<CompilationHelp *> helperOrder;
    std::vector<CompilationHelp *> invHelperOrder;
    for (auto &comp_spec : compilationSpecs) {
        helpers.push_back(CompilationHelp{.p_compSpec = &comp_spec});
    }
    for (int i = 0; i < helpers.size(); i++) {
        helperOrder.push_back(&helpers[i]);
        invHelperOrder.push_back(&helpers[helpers.size() - 1 - i]);
    };

    // generate pipelines, from the end result to the first pipeline
    {
        std::vector<PropertySpec> passedVarSpecs(desiredOutputs.getSpecList().begin(),
                                                 desiredOutputs.getSpecList().end());

        for (auto p_helper : invHelperOrder) {
            if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {
                passedVarSpecs.push_back(PropertySpec{
                    .name = p_helper->p_compSpec->predefinedVarsToOutputs.at("gl_Position"),
                    .typeInfo = StandardShaderPropertyTypes::VERTEX_OUTPUT});
            }

            p_helper->pipeline =
                Pipeline<ShaderTask>(p_helper->p_compSpec->p_method, passedVarSpecs, {});

            passedVarSpecs.clear();
            for (auto [reqNameId, reqSpec] : p_helper->pipeline.inputSpecs) {
                passedVarSpecs.push_back(reqSpec);
            }
        }
    }

    // separate inputs into uniform and vertex layout variables
    for (auto [nameId, spec] : helperOrder[0]->pipeline.inputSpecs) {
        if (rend.getAllVertexBufferSpecs().find(spec.name) !=
            rend.getAllVertexBufferSpecs().end()) {
            elemVarSpecs.emplace(nameId, spec);
        } else {
            uniformVarSpecs.emplace(nameId, spec);
        }
    }
    if (helperOrder[0]->p_compSpec->shaderType == GL_COMPUTE_SHADER) {
        for (auto [nameId, spec] : helperOrder[0]->pipeline.outputSpecs) {
            // uniformVarSpecs.emplace(nameId, spec);
        }
    }

    // make a list of all types we need to define
    std::vector<const GLTypeSpec *> typeDeclarationOrder;

    {
        std::set<const GLTypeSpec *> mentionedTypes;

        std::function<void(const GLTypeSpec &)> processTypeNameId =
            [&](const GLTypeSpec &glTypeSpec) -> void {
            if (mentionedTypes.find(&glTypeSpec) == mentionedTypes.end()) {
                mentionedTypes.insert(&glTypeSpec);

                for (auto p_dependencyTypeSpec : glTypeSpec.memberTypeDependencies) {
                    processTypeNameId(*p_dependencyTypeSpec);
                }

                typeDeclarationOrder.push_back(&glTypeSpec);
            }
        };

        for (auto varSpecs : {uniformVarSpecs, elemVarSpecs}) {
            for (auto [varNameId, varSpec] : varSpecs) {
                processTypeNameId(rend.getTypeConversion(varSpec.typeInfo).glTypeSpec);
            }
        }

        for (auto p_helper : helperOrder) {
            std::set<const TypeInfo *> usedTypeSet;

            for (auto &pipeItem : p_helper->pipeline.items) {
                pipeItem.p_task->extractUsedTypes(usedTypeSet);
            }

            for (auto p_type : usedTypeSet) {
                processTypeNameId(rend.getTypeConversion(*p_type).glTypeSpec);
            }
        }
    }

    // build the source code
    {
        String prevVarPrefix = "";
        for (auto p_helper : helperOrder) {
            // code output
            std::stringstream ss;
            ShaderTask::BuildContext context{.output = ss, .renderer = rend};
            std::map<StringId, String> inputParametersToGlobalVars;
            std::map<StringId, String> outputParametersToGlobalVars;

            // boilerplate stuff
            ss << "#version 460 core\n"
               << "\n"
               << "\n";

            // type definitions
            for (auto p_glType : typeDeclarationOrder) {
                if (!p_glType->glslDefinitionSnippet.empty()) {
                    // skip structs with FAMs because they would cause issues, and they are only
                    // used in SSBO blocks
                    if (!p_glType->flexibleMemberSpec.has_value()) {
                        ss << p_glType->glslDefinitionSnippet << "\n";
                    }
                }
            }

            // p_task function declarations
            for (auto &pipeItem : p_helper->pipeline.items) {
                pipeItem.p_task->outputDeclarationCode(context);
                ss << "\n";
            }

            // predefined variables
            auto &predefinedInputParameterMap = p_helper->p_compSpec->inputsToPredefinedVars;

            ss << "\n"
               << "\n";

            // uniforms
            for (auto [nameId, spec] : uniformVarSpecs) {

                // UNIFORMS

                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                std::string_view glslMemberList = "";
                if (glTypeSpec.glslDefinitionSnippet.find_first_of("struct") != std::string::npos) {
                    glslMemberList =
                        std::string_view(glTypeSpec.glslDefinitionSnippet)
                            .substr(glTypeSpec.glslDefinitionSnippet.find_first_of('{') + 1,
                                    glTypeSpec.glslDefinitionSnippet.find_last_of('}') -
                                        glTypeSpec.glslDefinitionSnippet.find_first_of('{') - 1);
                }

                OpenGLRenderer::GpuValueStorageMethod minimalMethod =
                    rend.getGpuStorageMethod(glTypeSpec);

                if (minimalMethod == OpenGLRenderer::GpuValueStorageMethod::SSBO) {

                    // SSBOs
                    ss << "layout(std430) ";

                    if (glslMemberList.empty()) {
                        ss << "buffer " << spec.name << " {\n";
                        ss << "\t" << specToMutableGlName(spec.typeInfo) << " value" << ";\n";
                        ss << "} " << bufferVarPrefix << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId,
                                                            bufferVarPrefix + spec.name + ".value");
                    } else {
                        ss << "buffer " << spec.name << " {";
                        ss << glslMemberList;
                        ss << "} " << bufferVarPrefix << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId, bufferVarPrefix + spec.name);
                    }
                } else if (p_helper->pipeline.outputSpecs.find(nameId) !=
                               p_helper->pipeline.outputSpecs.end() &&
                           p_helper->p_compSpec->shaderType == GL_COMPUTE_SHADER) {
                    switch (minimalMethod) {
                    case OpenGLRenderer::GpuValueStorageMethod::UniformBinding:

                        // EDITABLE IMAGE UNIFORMs

                        ss << "uniform " << specToMutableGlName(spec.typeInfo) << " "
                           << uniVarPrefix << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId, uniVarPrefix + spec.name);
                        break;
                    case OpenGLRenderer::GpuValueStorageMethod::Uniform:
                    case OpenGLRenderer::GpuValueStorageMethod::UBO:

                        // (also) SSBOs
                        ss << "layout(std430) ";

                        if (glslMemberList.empty()) {
                            ss << "buffer " << spec.name << " {\n";
                            ss << "\t" << specToMutableGlName(spec.typeInfo) << " value" << ";\n";
                            ss << "} " << uniVarPrefix << spec.name << ";\n";

                            inputParametersToGlobalVars.emplace(nameId, bufferVarPrefix +
                                                                            spec.name + ".value");
                        } else {
                            ss << "buffer " << spec.name << " {";
                            ss << glslMemberList;
                            ss << "} " << uniVarPrefix << spec.name << ";\n";

                            inputParametersToGlobalVars.emplace(nameId,
                                                                bufferVarPrefix + spec.name);
                        }
                        break;
                    }
                } else {
                    switch (minimalMethod) {
                    case OpenGLRenderer::GpuValueStorageMethod::Uniform:
                        if (p_helper->pipeline.localSpecs.find(nameId) !=
                            p_helper->pipeline.localSpecs.end()) {

                            // SHARED LOCAL GROUP VARIABLES

                            ss << "shared " << specToMutableGlName(spec.typeInfo) << " "
                               << sharedVarPrefix << spec.name << ";\n";

                            inputParametersToGlobalVars.emplace(nameId,
                                                                sharedVarPrefix + spec.name);
                            break;
                        } else {
                            // fallthrough
                        }
                    case OpenGLRenderer::GpuValueStorageMethod::UniformBinding:

                        // UNIFORMS

                        ss << "uniform " << specToConstGlName(spec.typeInfo) << " " << uniVarPrefix
                           << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId, uniVarPrefix + spec.name);
                        break;
                    case OpenGLRenderer::GpuValueStorageMethod::UBO:

                        // UNIFORM BUFFER OBJECTS

                        if (glslMemberList.empty()) {
                            ss << "uniform " << uniVarPrefix << spec.name << " {\n";
                            ss << "\t" << specToConstGlName(spec.typeInfo) << " value" << ";\n";
                            ss << "} " << spec.name << ";\n";

                            inputParametersToGlobalVars.emplace(nameId, uniVarPrefix + spec.name +
                                                                            ".value");
                        } else {
                            ss << "uniform " << uniVarPrefix << spec.name << " {\n";
                            ss << glslMemberList << "\n";
                            ss << "} " << spec.name << ";\n";

                            inputParametersToGlobalVars.emplace(nameId, uniVarPrefix + spec.name);
                        }
                        break;
                    }
                }
            }

            // input variables
            for (auto [nameId, spec] : p_helper->pipeline.inputSpecs) {
                if (spec.typeInfo != Variant::getTypeInfo<void>() &&
                    uniformVarSpecs.find(nameId) == uniformVarSpecs.end() &&
                    predefinedInputParameterMap.find(nameId) == predefinedInputParameterMap.end()) {

                    if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {

                        // VERTEX ATTRIBUTES

                        ss << "layout(location = " << rend.getVertexBufferLayoutIndex(nameId)
                           << ") in " << specToConstGlName(spec.typeInfo) << " " << elemVarPrefix
                           << prevVarPrefix << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId, elemVarPrefix + spec.name);
                    } else {

                        // PREVIOUS STAGE OUTPUTS

                        ss << "in " << specToMutableGlName(spec.typeInfo) << " " << prevVarPrefix
                           << spec.name << ";\n";

                        inputParametersToGlobalVars.emplace(nameId, prevVarPrefix + spec.name);
                    }
                }
            }
            inputParametersToGlobalVars.merge(std::move(predefinedInputParameterMap));

            ss << "\n";

            // output variables
            for (auto [nameId, spec] : p_helper->pipeline.outputSpecs) {
                if (spec.typeInfo != Variant::getTypeInfo<void>() &&
                    uniformVarSpecs.find(nameId) == uniformVarSpecs.end()) {

                    // NORMAL OUTPUTS

                    if (p_helper->p_compSpec->shaderType == GL_FRAGMENT_SHADER) {
                        std::size_t index =
                            std::find(desiredOutputs.getSpecNameIds().begin(),
                                      desiredOutputs.getSpecNameIds().end(), nameId) -
                            desiredOutputs.getSpecNameIds().begin();

                        assert(index < desiredOutputs.getSpecNameIds().size());

                        ss << "layout(location = " << index << ") out "
                           << specToMutableGlName(spec.typeInfo) << " "
                           << p_helper->p_compSpec->outVarPrefix << spec.name << ";\n";
                    } else {
                        ss << "out " << specToMutableGlName(spec.typeInfo) << " "
                           << p_helper->p_compSpec->outVarPrefix << spec.name << ";\n";
                    }
                    inputParametersToGlobalVars.emplace(nameId, p_helper->p_compSpec->outVarPrefix +
                                                                    spec.name);
                    outputParametersToGlobalVars.emplace(
                        nameId, p_helper->p_compSpec->outVarPrefix + spec.name);
                }
            }

            ss << "\n";

            // p_task function definitions
            for (auto &pipeItem : p_helper->pipeline.items) {
                pipeItem.p_task->outputDefinitionCode(context);
                ss << "\n";
            }

            ss << "\n";

            // local variables
            ss << "void main() {\n";
            for (auto [nameId, spec] : p_helper->pipeline.localSpecs) {
                if (spec.typeInfo != Variant::getTypeInfo<void>()) {
                    ss << "    " << specToMutableGlName(spec.typeInfo) << " " << localVarPrefix
                       << spec.name << ";\n";
                }
                inputParametersToGlobalVars.emplace(nameId, localVarPrefix + spec.name);
                outputParametersToGlobalVars.emplace(nameId, localVarPrefix + spec.name);
            }

            // execution pipeline
            for (auto &pipeItem : p_helper->pipeline.items) {
                ss << "    ";
                pipeItem.p_task->outputUsageCode(context, inputParametersToGlobalVars,
                                                 outputParametersToGlobalVars);
                ss << "\n";
            }

            // pipethrough variables
            for (auto nameId : p_helper->pipeline.pipethroughInputNames) {
                if (uniformVarSpecs.find(nameId) == uniformVarSpecs.end()) {
                    ss << "    " << outputParametersToGlobalVars.at(nameId) << " = "
                       << inputParametersToGlobalVars.at(nameId) << ";\n";
                }
            }

            // built-in variables
            for (auto [predefName, outNameId] : p_helper->p_compSpec->predefinedVarsToOutputs) {
                if (auto it = outputParametersToGlobalVars.find(outNameId);
                    it != outputParametersToGlobalVars.end()) {
                    ss << "    " << predefName << " = " << it->second << ";\n";
                } else {
                    auto &globVarName = inputParametersToGlobalVars.at(outNameId);
                    ss << "    " << predefName << " = " << globVarName << ";\n";
                }
            }

            ss << "}\n";

            // create the shader with the source
            std::string srcCode = ss.str();
            const char *c_code = srcCode.c_str();
            p_helper->shaderId = glCreateShader(p_helper->p_compSpec->shaderType);
            glShaderSource(p_helper->shaderId, 1, &c_code, NULL);

            // debug
            String filePrefix = std::string("shaderdebug/") +
                                String(p_helper->p_compSpec->p_method->getFriendlyName()) + "_" +
                                std::to_string(desiredOutputs.getHash()) +
                                p_helper->p_compSpec->outVarPrefix;
            {
                std::ofstream file;
                String filename = filePrefix + ".dot";
                file.open(filename);
                exportPipeline(p_helper->pipeline, file);
                file.close();

                root.getInfoStream() << "Graph stored to: '" << std::filesystem::current_path()
                                     << "/" << filename << "'" << std::endl;
            }
            {
                std::ofstream file;
                String filename = filePrefix + ".glsl";
                file.open(filename);
                file << srcCode;
                file.close();

                root.getInfoStream() << "Shader stored to: '" << std::filesystem::current_path()
                                     << "/" << filename << "'" << std::endl;
            }

            prevVarPrefix = p_helper->p_compSpec->outVarPrefix;
        }
    }

    // compile shaders
    {
        int success;
        char cmplLog[1024];

        programGLName = glCreateProgram();
        for (auto p_helper : helperOrder) {
            glCompileShader(p_helper->shaderId);

            glGetShaderInfoLog(p_helper->shaderId, sizeof(cmplLog), nullptr, cmplLog);
            glGetShaderiv(p_helper->shaderId, GL_COMPILE_STATUS, &success);
            if (!success) {
                root.getErrStream() << "Shader compilation error: " << cmplLog << std::endl;
            } else {
                root.getInfoStream() << "Shader compiled: " << cmplLog << std::endl;
            }

            glAttachShader(programGLName, p_helper->shaderId);
        }
        glLinkProgram(programGLName);

        glGetProgramInfoLog(programGLName, sizeof(cmplLog), nullptr, cmplLog);
        glGetProgramiv(programGLName, GL_LINK_STATUS, &success);
        if (!success) {
            root.getErrStream() << "Shader linking error: " << cmplLog << std::endl;
        } else {
            root.getInfoStream() << "Shader linked: " << cmplLog << std::endl;
        }

        for (auto p_helper : helperOrder) {
            glDeleteShader(p_helper->shaderId);
        }
    }

    // delete shaders (they will continue to exist while attached to program)
    for (auto p_helper : helperOrder) {
        glDeleteShader(p_helper->shaderId);
    }

    // store uniform indices
    for (auto [uniNameId, uniSpec] : uniformVarSpecs) {
        std::string uniFullName = uniVarPrefix + uniSpec.name;

        auto minimalStorageMethod =
            rend.getGpuStorageMethod(rend.getTypeConversion(uniSpec.typeInfo).glTypeSpec);

        if (std::find(desiredOutputs.getSpecNameIds().begin(),
                      desiredOutputs.getSpecNameIds().end(),
                      uniNameId) != desiredOutputs.getSpecNameIds().end()) {
            if (minimalStorageMethod == OpenGLRenderer::GpuValueStorageMethod::UniformBinding) {
                bindingSpecs.emplace(
                    uniNameId, VariableSpec{.srcSpec = uniSpec,
                                            .glNameId = glGetUniformLocation(programGLName,
                                                                             uniFullName.c_str())});
            } else {
                GLint index = glGetProgramResourceIndex(programGLName, GL_SHADER_STORAGE_BLOCK,
                                                        uniSpec.name.c_str());
                ssboSpecs.emplace(uniNameId, VariableSpec{.srcSpec = uniSpec, .glNameId = index});
                glUniformBlockBinding(programGLName, index, index);
            }
        } else {

            switch (minimalStorageMethod) {
            case OpenGLRenderer::GpuValueStorageMethod::Uniform:
                uniformSpecs.emplace(
                    uniNameId, VariableSpec{.srcSpec = uniSpec,
                                            .glNameId = glGetUniformLocation(programGLName,
                                                                             uniFullName.c_str())});
                break;
            case OpenGLRenderer::GpuValueStorageMethod::UniformBinding:
                bindingSpecs.emplace(
                    uniNameId, VariableSpec{.srcSpec = uniSpec,
                                            .glNameId = glGetUniformLocation(programGLName,
                                                                             uniFullName.c_str())});
                break;
            case OpenGLRenderer::GpuValueStorageMethod::UBO:
                uboSpecs.emplace(uniNameId, VariableSpec{.srcSpec = uniSpec,
                                                         .glNameId = (GLint)glGetUniformBlockIndex(
                                                             programGLName, uniFullName.c_str())});
                break;
            case OpenGLRenderer::GpuValueStorageMethod::SSBO:
                GLint index = glGetProgramResourceIndex(programGLName, GL_SHADER_STORAGE_BLOCK,
                                                        uniSpec.name.c_str());
                ssboSpecs.emplace(uniNameId, VariableSpec{.srcSpec = uniSpec, .glNameId = index});
                glUniformBlockBinding(programGLName, index, index);
                break;
            }
        }
    }
}

CompiledGLSLShader::~CompiledGLSLShader()
{
    glDeleteProgram(programGLName);
}

} // namespace Vitrae