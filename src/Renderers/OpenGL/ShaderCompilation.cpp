#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"
#include "Vitrae/Debugging/PipelineExport.hpp"
#include "Vitrae/Params/ParamList.hpp"
#include "Vitrae/Params/Standard.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"

#include "MMeter.h"

#include <fstream>

namespace Vitrae
{

CompiledGLSLShader::SurfaceShaderParams::SurfaceShaderParams(const ParamAliases &aliases,
                                                             String vertexPositionOutputName,
                                                             const ParamList &fragmentOutputs,
                                                             ComponentRoot &root)
    : m_aliases(aliases), m_vertexPositionOutputName(vertexPositionOutputName),
      m_fragmentOutputs(fragmentOutputs), mp_root(&root),
      m_hash(combinedHashes<3>({{aliases.hash(), fragmentOutputs.getHash(),
                                 std::hash<StringId>{}(StringId(vertexPositionOutputName))}}))
{}

CompiledGLSLShader::ComputeShaderParams::ComputeShaderParams(
    ComponentRoot &root, const ParamAliases &aliases, const ParamList &desiredResults,
    ArgumentGetter<std::uint32_t> invocationCountX, ArgumentGetter<std::uint32_t> invocationCountY,
    ArgumentGetter<std::uint32_t> invocationCountZ, glm::uvec3 groupSize,
    bool allowOutOfBoundsCompute)
    : mp_root(&root), m_aliases(aliases), m_desiredResults(desiredResults),
      m_invocationCountX(invocationCountX), m_invocationCountY(invocationCountY),
      m_invocationCountZ(invocationCountZ), m_groupSize(groupSize),
      m_allowOutOfBoundsCompute(allowOutOfBoundsCompute),
      m_hash(combinedHashes<9>({{
          aliases.hash(),
          desiredResults.getHash(),
          std::hash<ArgumentGetter<std::uint32_t>>{}(invocationCountX),
          std::hash<ArgumentGetter<std::uint32_t>>{}(invocationCountY),
          std::hash<ArgumentGetter<std::uint32_t>>{}(invocationCountZ),
          groupSize.x,
          groupSize.y,
          groupSize.z,
          allowOutOfBoundsCompute,
      }}))
{}

CompiledGLSLShader::CompiledGLSLShader(const SurfaceShaderParams &params)
    : CompiledGLSLShader(
          {{
              CompilationSpec{.aliases = ParamAliases(
                                  std::initializer_list{
                                      &params.getAliases(),
                                  },
                                  {
                                      {"gl_Position", params.getVertexPositionOutputName()},
                                  }),
                              .outVarPrefix = "vert_",
                              .shaderType = GL_VERTEX_SHADER},
              CompilationSpec{.aliases = ParamAliases(std::initializer_list{
                                  &params.getAliases(),
                              }),
                              .outVarPrefix = "frag_",
                              .shaderType = GL_FRAGMENT_SHADER},
          }},
          params.getRoot(), params.getFragmentOutputs())
{}

CompiledGLSLShader::CompiledGLSLShader(const ComputeShaderParams &params)
    : CompiledGLSLShader(
          {{
              CompilationSpec{
                  .aliases = ParamAliases(std::initializer_list{
                      &params.getAliases(),
                  }),
                  .outVarPrefix = "comp_",
                  .shaderType = GL_COMPUTE_SHADER,
                  .computeSpec =
                      ComputeCompilationSpec{
                          .invocationCountX = params.getInvocationCountX(),
                          .invocationCountY = params.getInvocationCountY(),
                          .invocationCountZ = params.getInvocationCountZ(),
                          .groupSize = params.getGroupSize(),
                          .allowOutOfBoundsCompute = params.getAllowOutOfBoundsCompute(),
                      },
              },
          }},
          params.getRoot(), params.getDesiredResults())
{}

CompiledGLSLShader::CompiledGLSLShader(MovableSpan<CompilationSpec> compilationSpecs,
                                       ComponentRoot &root, const ParamList &desiredOutputs)
{
    MMETER_SCOPE_PROFILER("CompiledGLSLShader");

    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(root.getComponent<Renderer>());

    // uniforms are global variables given to all shader steps
    String uniVarPrefix = "uniform_";
    String bindingVarPrefix = "bind_";
    String uboBlockPrefix = "ubo_block_";
    String uboVarPrefix = "ubo_";
    String ssboBlockPrefix = "buffer_block_";
    String ssboVarPrefix = "buffer_";
    String localVarPrefix = "tmp_";

    // mesh vertex element data is given to the vertex shader and passed through to other steps
    String elemVarPrefix = "elem_";

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
        ParamList passedVarSpecs = desiredOutputs;

        for (auto p_helper : invHelperOrder) {
            // stage-specific required outputs
            if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {
                passedVarSpecs.insert_back({
                    .name = p_helper->p_compSpec->aliases.choiceStringFor("gl_Position"),
                    .typeInfo = TYPE_INFO<glm::vec4>,
                });
            }

            // get the method
            dynasma::FirmPtr<const Method<ShaderTask>> p_method;
            switch (p_helper->p_compSpec->shaderType) {
            case GL_VERTEX_SHADER:
                p_method = root.getComponent<MethodCollection>().getVertexMethod();
                break;
            case GL_FRAGMENT_SHADER:
                p_method = root.getComponent<MethodCollection>().getFragmentMethod();
                break;
            case GL_COMPUTE_SHADER:
                p_method = root.getComponent<MethodCollection>().getComputeMethod();
                break;
            default:
                assert(false);
            }

            p_helper->pipeline =
                Pipeline<ShaderTask>(p_method, passedVarSpecs, p_helper->p_compSpec->aliases);

            // stage-specific required inputs
            if (p_helper->p_compSpec->shaderType == GL_COMPUTE_SHADER) {

                auto &computeSpec = p_helper->p_compSpec->computeSpec.value();

                if (!computeSpec.allowOutOfBoundsCompute) {
                    if (!computeSpec.invocationCountX.isFixed()) {
                        p_helper->pipeline.inputSpecs.insert_back(
                            computeSpec.invocationCountX.getSpec());
                    }
                    if (!computeSpec.invocationCountY.isFixed()) {
                        p_helper->pipeline.inputSpecs.insert_back(
                            computeSpec.invocationCountY.getSpec());
                    }
                    if (!computeSpec.invocationCountZ.isFixed()) {
                        p_helper->pipeline.inputSpecs.insert_back(
                            computeSpec.invocationCountZ.getSpec());
                    }
                }
            }

            passedVarSpecs = p_helper->pipeline.inputSpecs;
            passedVarSpecs.merge(p_helper->pipeline.filterSpecs);
            passedVarSpecs.merge(p_helper->pipeline.consumingSpecs);
            passedVarSpecs.merge(p_helper->pipeline.pipethroughSpecs);
        }
    }

    // prepare binding indexes
    std::map<StringId, GLuint> namedBindings;
    auto getBinding = [&](StringId name) -> GLuint {
        auto it = namedBindings.find(name);
        if (it == namedBindings.end()) {
            namedBindings.emplace(name, (GLuint)namedBindings.size());
            return (GLuint)(namedBindings.size() - 1);
        } else {
            return it->second;
        }
    };

    // prepare previous stage inputs
    StableMap<StringId, LocationSpec> prevStageOutputs;
    String prevStageOutVarPrefix;

    // Select uniforms, bindings, UBOs and SSBOs
    {
        MMETER_SCOPE_PROFILER("Selecting program inputs");

        auto p_helper = helperOrder[0];

        // select the input prefix
        if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {
            prevStageOutVarPrefix = elemVarPrefix;
        }

        // Input & consuming specs
        for (const ParamList *p_specs : {
                 &p_helper->pipeline.inputSpecs,
                 &p_helper->pipeline.consumingSpecs,
                 &p_helper->pipeline.filterSpecs,
                 &p_helper->pipeline.pipethroughSpecs,
             }) {
            for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
                if (spec.typeInfo == TYPE_INFO<void>) {
                    // just a token, skip
                } else if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER &&
                           rend.getAllVertexBufferSpecs().find(nameId) !=
                               rend.getAllVertexBufferSpecs().end()) {
                    // vertex shader receives inputs from the mesh
                    prevStageOutputs.emplace(
                        nameId,
                        LocationSpec{.srcSpec = spec,
                                     .location = (int)rend.getVertexBufferLayoutIndex(nameId)});
                    this->vertexComponentSpecs.insert_back(spec);
                } else {
                    // decide how to convert it
                    const GLConversionSpec &convSpec = rend.getTypeConversion(spec.typeInfo);
                    const GLTypeSpec &glTypeSpec = convSpec.glTypeSpec;

                    if (convSpec.setUniform) {
                        this->uniformSpecs.emplace(nameId, LocationSpec{
                                                               .srcSpec = spec,
                                                               .location = -1, // will be set later
                                                           });
                    } else if (convSpec.setOpaqueBinding) {
                        this->opaqueBindingSpecs.emplace(nameId,
                                                         BindingSpec{
                                                             .srcSpec = spec,
                                                             .location = -1,    // will be set later
                                                             .bindingIndex = 0, // will be set later
                                                         });
                    } else if (convSpec.setUBOBinding) {
                        this->uboSpecs.emplace(nameId, BindingSpec{
                                                           .srcSpec = spec,
                                                           .location = -1,    // will be set later
                                                           .bindingIndex = 0, // will be set later
                                                       });
                    } else if (convSpec.setSSBOBinding) {
                        this->ssboSpecs.emplace(nameId, BindingSpec{
                                                            .srcSpec = spec,
                                                            .location = -1,    // will be set later
                                                            .bindingIndex = 0, // will be set later
                                                        });
                    } else {
                        throw std::runtime_error(
                            "Shader compilation failed: unconvertible property " + spec.name);
                    }
                }
            }
        }
    }

    // build the source code
    {
        for (auto p_helper : helperOrder) {

            // Property storage choosing
            ParamList stageUniformList;
            ParamList stageOpaqueBindingList;
            ParamList stageUBOList;
            ParamList stageSSBOList;
            ParamList stageInputList;
            ParamList stageOutputList;
            ParamList stageLocalList;
            std::map<StringId, String> tobeStageAliases;
            std::vector<std::pair<String, String>> initialPipethroughList;

            {
                MMETER_SCOPE_PROFILER("Property storage choosing");

                // Separate source values - Input & consuming & filter & pipethrough specs
                for (const ParamList *p_specs : {
                         &p_helper->pipeline.inputSpecs,
                         &p_helper->pipeline.consumingSpecs,
                         &p_helper->pipeline.filterSpecs,
                         &p_helper->pipeline.pipethroughSpecs,
                     }) {
                    for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
                        if (spec.typeInfo == TYPE_INFO<void>) {
                            // just a token, skip
                        } else if (prevStageOutputs.find(nameId) != prevStageOutputs.end()) {
                            // normal input
                            stageInputList.insert_back(spec);
                            tobeStageAliases[spec.name] = prevStageOutVarPrefix + spec.name;
                        } else if (this->uniformSpecs.find(nameId) != this->uniformSpecs.end()) {
                            // uniform
                            stageUniformList.insert_back(spec);
                            tobeStageAliases[spec.name] = uniVarPrefix + spec.name;
                        } else if (this->opaqueBindingSpecs.find(nameId) !=
                                   this->opaqueBindingSpecs.end()) {
                            // opaque binding
                            stageOpaqueBindingList.insert_back(spec);
                            tobeStageAliases[spec.name] = bindingVarPrefix + spec.name;
                        } else if (this->uboSpecs.find(nameId) != this->uboSpecs.end()) {
                            // UBO
                            stageUBOList.insert_back(spec);
                            tobeStageAliases[spec.name] = uboVarPrefix + spec.name;
                        } else if (this->ssboSpecs.find(nameId) != this->ssboSpecs.end()) {
                            // SSBO
                            stageSSBOList.insert_back(spec);
                            tobeStageAliases[spec.name] = ssboVarPrefix + spec.name;
                        }
                    }
                }

                // filter specs
                for (auto [nameId, spec] : p_helper->pipeline.filterSpecs.getMappedSpecs()) {
                    if (spec.typeInfo == TYPE_INFO<void>) {
                        // just a token, skip
                    } else if ((stageOpaqueBindingList.getMappedSpecs().find(nameId) !=
                                stageOpaqueBindingList.getMappedSpecs().end()) ||
                               (stageSSBOList.getMappedSpecs().find(nameId) !=
                                stageSSBOList.getMappedSpecs().end())) {
                        // Just skip bindings that can be modified
                    } else {
                        // The value needs to be copied before it can be modified
                        // Will be output

                        // check if we can
                        const GLConversionSpec &convSpec = rend.getTypeConversion(spec.typeInfo);
                        const GLTypeSpec &glTypeSpec = convSpec.glTypeSpec;

                        if (!glTypeSpec.valueTypeName.empty()) {
                            initialPipethroughList.push_back({
                                p_helper->p_compSpec->outVarPrefix + spec.name,
                                tobeStageAliases.at(spec.name),
                            });
                            tobeStageAliases[spec.name] =
                                p_helper->p_compSpec->outVarPrefix + spec.name;
                            stageOutputList.insert_back(spec);
                        } else {
                            throw std::runtime_error("Shader compilation failed: property " +
                                                     spec.name +
                                                     " cannot be reasigned to an output");
                        }
                    }
                }

                // pipethrough specs
                for (auto [nameId, spec] : p_helper->pipeline.pipethroughSpecs.getMappedSpecs()) {
                    if (spec.typeInfo == TYPE_INFO<void>) {
                        // just a token, skip
                    } else if (stageInputList.getMappedSpecs().find(nameId) !=
                               stageInputList.getMappedSpecs().end()) {
                        // The value needs to be copied to an output

                        // check if we can
                        const GLConversionSpec &convSpec = rend.getTypeConversion(spec.typeInfo);
                        const GLTypeSpec &glTypeSpec = convSpec.glTypeSpec;

                        if (!glTypeSpec.valueTypeName.empty()) {
                            initialPipethroughList.push_back({
                                p_helper->p_compSpec->outVarPrefix + spec.name,
                                tobeStageAliases.at(spec.name),
                            });
                            stageOutputList.insert_back(spec);
                        } else {
                            throw std::runtime_error("Shader compilation failed: property " +
                                                     spec.name +
                                                     " cannot be reasigned to an output");
                        }
                    }
                }

                // output specs
                for (auto [nameId, spec] : p_helper->pipeline.outputSpecs.getMappedSpecs()) {
                    if (spec.typeInfo == TYPE_INFO<void>) {
                        // just a token, skip
                    } else {
                        // Check if we can store it as output
                        const GLConversionSpec &convSpec = rend.getTypeConversion(spec.typeInfo);
                        const GLTypeSpec &glTypeSpec = convSpec.glTypeSpec;

                        if (!glTypeSpec.valueTypeName.empty() ||
                            (!glTypeSpec.structBodySnippet.empty() &&
                             !glTypeSpec.flexibleMemberSpec.has_value())) {
                            // can be stored as normal output
                            tobeStageAliases[spec.name] =
                                p_helper->p_compSpec->outVarPrefix + spec.name;
                            stageOutputList.insert_back(spec);
                        } else {
                            throw std::runtime_error("Shader compilation failed: property " +
                                                     spec.name + " cannot be used as output");
                        }
                    }
                }

                // Local specs
                for (auto [nameId, spec] : p_helper->pipeline.localSpecs.getMappedSpecs()) {
                    if (spec.typeInfo == TYPE_INFO<void>) {
                        // just a token, skip
                    } else {
                        // Check if we can store it as a local variable
                        const GLConversionSpec &convSpec = rend.getTypeConversion(spec.typeInfo);
                        const GLTypeSpec &glTypeSpec = convSpec.glTypeSpec;

                        if (!glTypeSpec.valueTypeName.empty() ||
                            (!glTypeSpec.structBodySnippet.empty() &&
                             !glTypeSpec.flexibleMemberSpec.has_value())) {
                            // can be stored as normal output
                            tobeStageAliases[spec.name] = localVarPrefix + spec.name;
                            stageLocalList.insert_back(spec);
                        } else {
                            throw std::runtime_error("Shader compilation failed: property " +
                                                     spec.name +
                                                     " cannot be used as a local variable");
                        }
                    }
                }
            }

            // make a list of all types we need to define
            std::vector<const GLTypeSpec *> typeDeclarationOrder;
            {
                std::set<const TypeInfo *> usedTypeSet;

                for (auto p_task : p_helper->pipeline.items) {
                    p_task->extractUsedTypes(usedTypeSet, p_helper->p_compSpec->aliases);
                }

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

                for (auto p_type : usedTypeSet) {
                    if (*p_type != TYPE_INFO<void>) {
                        processTypeNameId(rend.getTypeConversion(*p_type).glTypeSpec);
                    }
                }
            }

            // === code output ===

            std::stringstream ss;
            ParamAliases stageAliases({{&p_helper->p_compSpec->aliases}},
                                         StableMap<StringId, String>(std::move(tobeStageAliases)));
            ShaderTask::BuildContext context{
                .output = ss,
                .root = root,
                .renderer = rend,
                .aliases = stageAliases,
            };

            // boilerplate stuff
            ss << "#version 460 core\n"
               << "\n"
               << "\n";

            // Specifications unique to shader stages
            if (p_helper->p_compSpec->shaderType == GL_COMPUTE_SHADER) {
                // compute shader spec
                auto &computeSpec = helperOrder[0]->p_compSpec->computeSpec.value();

                ss << "layout (local_size_x = " << computeSpec.groupSize.x
                   << ", local_size_y = " << computeSpec.groupSize.y
                   << ", local_size_z = " << computeSpec.groupSize.z << ") in;\n"
                   << "\n";
            }

            // write type definitions
            for (auto p_glType : typeDeclarationOrder) {
                if (!p_glType->valueTypeName.empty() && !p_glType->structBodySnippet.empty()) {
                    ss << "struct " << p_glType->valueTypeName << " {\n"
                       << p_glType->structBodySnippet << "\n"
                       << "};\n"
                       << "\n";
                }
            }

            // p_task function declarations
            for (auto p_task : p_helper->pipeline.items) {
                p_task->outputDeclarationCode(context);
                ss << "\n";
            }

            // Receiving values

            // uniforms
            for (auto &spec : stageUniformList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                ss << "uniform " << glTypeSpec.valueTypeName << " " << uniVarPrefix << spec.name
                   << ";\n";
            }

            ss << "\n";

            // opaque bindings
            for (auto &spec : stageOpaqueBindingList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                ss << "layout(binding=" << getBinding(spec.name) << ") " << "uniform "
                   << glTypeSpec.opaqueTypeName << " " << bindingVarPrefix << spec.name << ";\n";
            }

            ss << "\n";

            // UBOs
            for (auto &spec : stageUBOList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                ss << "layout(std430, binding=" << getBinding(spec.name) << ") ";
                ss << "uniform " << uboBlockPrefix << spec.name << " {\n";
                if (glTypeSpec.valueTypeName.empty()) {
                    ss << glTypeSpec.structBodySnippet << "\n";
                    ss << "} " << uboVarPrefix << spec.name << ";\n";
                } else {
                    ss << glTypeSpec.valueTypeName << " " << uboVarPrefix << spec.name << ";\n";
                    ss << "};\n";
                }
            }

            ss << "\n";

            // SSBOs
            for (auto &spec : stageSSBOList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                ss << "layout(std430, binding=" << getBinding(spec.name) << ") ";
                ss << "buffer " << ssboBlockPrefix << spec.name << " {\n";
                if (glTypeSpec.valueTypeName.empty()) {
                    if (glTypeSpec.structBodySnippet.empty() &&
                        glTypeSpec.flexibleMemberSpec.has_value() &&
                        glTypeSpec.flexibleMemberSpec->memberName.empty()) {
                        // The buffer is purely an array of FAM data
                        if (glTypeSpec.flexibleMemberSpec->elementGlTypeSpec.valueTypeName
                                .empty()) {
                            throw std::runtime_error("Unable to generate SSBO " + spec.name +
                                                     " because FAM type has no name");
                        }
                        ss << glTypeSpec.flexibleMemberSpec->elementGlTypeSpec.valueTypeName << " "
                           << ssboVarPrefix << spec.name << "[];\n";
                        ss << "};\n";
                    } else {
                        // The buffer has members, possibly a FAM
                        if (!glTypeSpec.structBodySnippet.empty()) {
                            ss << glTypeSpec.structBodySnippet << "\n";
                        }
                        if (glTypeSpec.flexibleMemberSpec.has_value()) {
                            if (glTypeSpec.flexibleMemberSpec->elementGlTypeSpec.valueTypeName
                                    .empty()) {
                                throw std::runtime_error("Unable to generate SSBO " + spec.name +
                                                         " because FAM type has no name");
                            }
                            ss << glTypeSpec.flexibleMemberSpec->elementGlTypeSpec.valueTypeName
                               << " " << glTypeSpec.flexibleMemberSpec->memberName << "[];\n";
                        }
                        ss << "} " << ssboVarPrefix << spec.name << ";\n";
                    }
                } else {
                    // The buffer is a named struct
                    ss << glTypeSpec.valueTypeName << " " << ssboVarPrefix << spec.name << ";\n";
                    ss << "};\n";
                }
            }

            ss << "\n";

            // Inputs
            for (auto &spec : stageInputList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {
                    const LocationSpec &locationSpec = prevStageOutputs.at(spec.name);
                    ss << "layout(location=" << locationSpec.location << ") ";
                }

                if (glTypeSpec.valueTypeName.empty()) {
                    throw std::runtime_error("Unable to generate input " + spec.name +
                                             " because type has no name");
                }

                ss << "in " << glTypeSpec.valueTypeName << " " << prevStageOutVarPrefix << spec.name
                   << ";\n";
            }

            ss << "\n";

            // Outputs
            for (auto &spec : stageOutputList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                if (p_helper->p_compSpec->shaderType == GL_FRAGMENT_SHADER) {

                    std::size_t index = 0;
                    for (auto &desiredNameId : desiredOutputs.getSpecNameIds()) {
                        if (spec.name == p_helper->p_compSpec->aliases.choiceFor(desiredNameId)) {
                            break;
                        }
                        ++index;
                    }

                    assert(index < desiredOutputs.getSpecNameIds().size());

                    ss << "layout(location=" << index << ") ";
                }

                if (glTypeSpec.valueTypeName.empty()) {
                    throw std::runtime_error("Unable to generate output " + spec.name +
                                             " because type has no name");
                }

                ss << "out " << glTypeSpec.valueTypeName << " "
                   << p_helper->p_compSpec->outVarPrefix << spec.name << ";\n";
            }

            ss << "\n";

            // Main function

            ss << "void main() {\n";

            // Local variables
            for (auto &spec : stageLocalList.getSpecList()) {
                const GLTypeSpec &glTypeSpec = rend.getTypeConversion(spec.typeInfo).glTypeSpec;

                if (glTypeSpec.valueTypeName.empty()) {
                    throw std::runtime_error("Unable to generate output " + spec.name +
                                             " because type has no name");
                }

                ss << "\t" << glTypeSpec.valueTypeName << " " << localVarPrefix << spec.name
                   << ";\n";
            }

            ss << "\n";

            // early return conditions
            if (p_helper->p_compSpec->shaderType == GL_COMPUTE_SHADER) {
                auto &computeSpec = helperOrder[0]->p_compSpec->computeSpec.value();

                if (!computeSpec.allowOutOfBoundsCompute) {
                    if (computeSpec.invocationCountX.isFixed()) {
                        if (computeSpec.invocationCountX.getFixedValue() % computeSpec.groupSize.x)
                            ss << "    if (gl_GlobalInvocationID.x >= "
                               << computeSpec.invocationCountX.getFixedValue() << ") return;\n";

                    } else {
                        if (computeSpec.groupSize.x > 1)
                            ss << "    if (gl_GlobalInvocationID.x >= "
                               << stageAliases.choiceStringFor(
                                      computeSpec.invocationCountX.getSpec().name)
                               << ") return;\n";
                    }

                    if (computeSpec.invocationCountY.isFixed()) {
                        if (computeSpec.invocationCountY.getFixedValue() % computeSpec.groupSize.y)
                            ss << "    if (gl_GlobalInvocationID.y >= "
                               << computeSpec.invocationCountY.getFixedValue() << ") return;\n";
                    } else {
                        if (computeSpec.groupSize.y > 1)
                            ss << "    if (gl_GlobalInvocationID.y >= "
                               << stageAliases.choiceStringFor(
                                      computeSpec.invocationCountY.getSpec().name)
                               << ") return;\n";
                    }

                    if (computeSpec.invocationCountZ.isFixed()) {
                        if (computeSpec.invocationCountZ.getFixedValue() % computeSpec.groupSize.z)
                            ss << "    if (gl_GlobalInvocationID.z >= "
                               << computeSpec.invocationCountZ.getFixedValue() << ") return;\n";
                    } else {
                        if (computeSpec.groupSize.z > 1)
                            ss << "    if (gl_GlobalInvocationID.z >= "
                               << stageAliases.choiceStringFor(
                                      computeSpec.invocationCountZ.getSpec().name)
                               << ") return;\n";
                    }
                }
            }

            ss << "\n";

            // pipethroughs
            for (const auto &[to, from] : initialPipethroughList) {
                ss << to << " = " << from << ";\n";
            }

            ss << "\n";

            // execution pipeline
            for (auto p_task : p_helper->pipeline.items) {
                ss << "    ";
                p_task->outputUsageCode(context);
                ss << "\n";
            }

            ss << "\n";

            // predefined outputs
            if (p_helper->p_compSpec->shaderType == GL_VERTEX_SHADER) {
                ss << "gl_Position = " << stageAliases.choiceStringFor("gl_Position") << ";\n";
            }

            ss << "}\n // main()";

            // === Compile ===

            // create the shader with the source
            std::string srcCode = ss.str();
            const char *c_code = srcCode.c_str();
            p_helper->shaderId = glCreateShader(p_helper->p_compSpec->shaderType);
            glShaderSource(p_helper->shaderId, 1, &c_code, NULL);

            // debug
            String filePrefix = std::string("shaderdebug/") + p_helper->p_compSpec->outVarPrefix +
                                getPipelineId(p_helper->pipeline, p_helper->p_compSpec->aliases);
            {
                std::ofstream file;
                String filename = filePrefix + ".dot";
                file.open(filename);
                exportPipeline(p_helper->pipeline, p_helper->p_compSpec->aliases, file);
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

            // compile
            int success;
            char cmplLog[1024];
            glCompileShader(p_helper->shaderId);

            glGetShaderInfoLog(p_helper->shaderId, sizeof(cmplLog), nullptr, cmplLog);
            glGetShaderiv(p_helper->shaderId, GL_COMPILE_STATUS, &success);
            if (!success) {
                root.getErrStream() << "Shader compilation error: " << cmplLog << std::endl;
            } else {
                root.getInfoStream() << "Shader compiled! " << cmplLog << std::endl;
            }

            // === Prepare for the next stage ===

            prevStageOutVarPrefix = p_helper->p_compSpec->outVarPrefix;
            prevStageOutputs.clear();
            for (const auto &[nameId, spec] : stageOutputList.getMappedSpecs()) {
                prevStageOutputs.emplace(nameId, LocationSpec{
                                                     .srcSpec = spec,
                                                     .location = 0, /// TODO: fixed locations
                                                 });
            }
        }
    }

    // Link shaders
    {
        int success;
        char cmplLog[1024];

        programGLName = glCreateProgram();
        for (auto p_helper : helperOrder) {
            glAttachShader(programGLName, p_helper->shaderId);
        }
        glLinkProgram(programGLName);

        glGetProgramInfoLog(programGLName, sizeof(cmplLog), nullptr, cmplLog);
        glGetProgramiv(programGLName, GL_LINK_STATUS, &success);
        if (!success) {
            root.getErrStream() << "Shader linking error: " << cmplLog << std::endl;
        } else {
            root.getInfoStream() << "Shader linked! " << cmplLog << std::endl;
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
    for (auto nameIdSpecPair : this->uniformSpecs) {
        nameIdSpecPair.second.location = glGetUniformLocation(
            programGLName, (uniVarPrefix + nameIdSpecPair.second.srcSpec.name).c_str());
    }
    for (auto nameIdSpecPair : this->opaqueBindingSpecs) {
        nameIdSpecPair.second.location = glGetUniformLocation(
            programGLName, (bindingVarPrefix + nameIdSpecPair.second.srcSpec.name).c_str());
        nameIdSpecPair.second.bindingIndex = namedBindings.at(nameIdSpecPair.first);
    }
    for (auto nameIdSpecPair : this->uboSpecs) {
        nameIdSpecPair.second.location = glGetUniformBlockIndex(
            programGLName, (uboVarPrefix + nameIdSpecPair.second.srcSpec.name).c_str());
        nameIdSpecPair.second.bindingIndex = namedBindings.at(nameIdSpecPair.first);
    }
    for (auto nameIdSpecPair : this->ssboSpecs) {
        nameIdSpecPair.second.location =
            glGetProgramResourceIndex(programGLName, GL_SHADER_STORAGE_BLOCK,
                                      (ssboVarPrefix + nameIdSpecPair.second.srcSpec.name).c_str());
        nameIdSpecPair.second.bindingIndex = namedBindings.at(nameIdSpecPair.first);
    }

    // combine the property specs
    for (auto nameIdSpecPair : desiredOutputs.getMappedSpecs()) {
        this->outputSpecs.insert_back(nameIdSpecPair.second);
    }
    for (auto p_specs : {
             &helperOrder[0]->pipeline.inputSpecs,
             &helperOrder[0]->pipeline.filterSpecs,
             &helperOrder[0]->pipeline.consumingSpecs,
             &helperOrder[0]->pipeline.pipethroughSpecs,
         }) {
        for (auto [nameId, spec] : p_specs->getMappedSpecs()) {
            if (this->uniformSpecs.find(nameId) != this->uniformSpecs.end() ||
                this->opaqueBindingSpecs.find(nameId) != this->opaqueBindingSpecs.end() ||
                this->uboSpecs.find(nameId) != this->uboSpecs.end() ||
                this->ssboSpecs.find(nameId) != this->ssboSpecs.end()) {
                // the property is used
                bool wasConsumed = false;
                bool wasModified = false;
                for (auto p_helper : helperOrder) {
                    if (p_helper->pipeline.consumingSpecs.contains(nameId)) {
                        wasConsumed = true;
                    } else if (p_helper->pipeline.filterSpecs.contains(nameId)) {
                        wasModified = true;
                    } else if (p_helper->pipeline.outputSpecs.contains(nameId)) {
                        wasConsumed = false;
                        wasModified = true;
                    }
                }

                if (wasConsumed) {
                    this->consumingSpecs.insert_back(spec);
                } else if (wasModified) {
                    this->filterSpecs.insert_back(spec);
                } else {
                    this->inputSpecs.insert_back(spec);
                }
            }
        }
    }
}

CompiledGLSLShader::~CompiledGLSLShader()
{
    glDeleteProgram(programGLName);
}

void CompiledGLSLShader::setupProperties(OpenGLRenderer &rend, VariantScope &env) const
{
    MMETER_SCOPE_PROFILER(
        "CompiledGLSLShader::setupProperties(OpenGLRenderer &rend, VariantScope &env) const");

    for (auto [propertyNameId, uniSpec] : this->uniformSpecs) {
        if (env.has(propertyNameId)) {
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                .setUniform(uniSpec.location, env.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, bindSpec] : this->opaqueBindingSpecs) {
        if (env.has(propertyNameId)) {
            rend.getTypeConversion(bindSpec.srcSpec.typeInfo)
                .setOpaqueBinding(bindSpec.location, env.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, uboSpec] : this->uboSpecs) {
        if (env.has(propertyNameId)) {
            rend.getTypeConversion(uboSpec.srcSpec.typeInfo)
                .setUBOBinding(uboSpec.location, env.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, ssboSpec] : this->ssboSpecs) {
        if (env.has(propertyNameId)) {
            rend.getTypeConversion(ssboSpec.srcSpec.typeInfo)
                .setSSBOBinding(ssboSpec.location, env.get(propertyNameId));
        }
    }
}

void CompiledGLSLShader::setupProperties(OpenGLRenderer &rend, VariantScope &envProperties,
                                         const Material &material) const
{
    MMETER_SCOPE_PROFILER("CompiledGLSLShader::setupProperties(OpenGLRenderer &rend, VariantScope "
                          "&envProperties, const Material &material) const");

    auto &matProperties = material.getProperties();

    for (auto [propertyNameId, uniSpec] : this->uniformSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // convert material variant to uniform
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                .setUniform(uniSpec.location, (*nameValIt).second);
        } else if (envProperties.has(propertyNameId)) {
            // convert context variant to uniform
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                .setUniform(uniSpec.location, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, bindSpec] : this->opaqueBindingSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant value
            rend.getTypeConversion(bindSpec.srcSpec.typeInfo)
                .setOpaqueBinding(bindSpec.bindingIndex, (*nameValIt).second);
        } else if (envProperties.has(propertyNameId)) {
            // bind context variant value
            rend.getTypeConversion(bindSpec.srcSpec.typeInfo)
                .setOpaqueBinding(bindSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, uboSpec] : this->uboSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant UBO
            rend.getTypeConversion(uboSpec.srcSpec.typeInfo)
                .setUBOBinding(uboSpec.bindingIndex, (*nameValIt).second);
        } else if (envProperties.has(propertyNameId)) {
            // bind context variant UBO
            rend.getTypeConversion(uboSpec.srcSpec.typeInfo)
                .setUBOBinding(uboSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, ssboSpec] : this->ssboSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant buffer
            rend.getTypeConversion(ssboSpec.srcSpec.typeInfo)
                .setSSBOBinding(ssboSpec.bindingIndex, (*nameValIt).second);
        } else if (envProperties.has(propertyNameId)) {
            // bind context variant buffer
            rend.getTypeConversion(ssboSpec.srcSpec.typeInfo)
                .setSSBOBinding(ssboSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }
}

void CompiledGLSLShader::setupNonMaterialProperties(OpenGLRenderer &rend, VariantScope &envProperties,
                                                    const Material &firstMaterial) const
{
    MMETER_SCOPE_PROFILER("CompiledGLSLShader::setupNonMaterialProperties(OpenGLRenderer &rend, "
                          "VariantScope &envProperties, "
                          "const Material &firstMaterial) const");

    auto &matProperties = firstMaterial.getProperties();

    for (auto [propertyNameId, uniSpec] : this->uniformSpecs) {
        if (matProperties.find(propertyNameId) == matProperties.end() &&
            envProperties.has(propertyNameId)) {
            // convert context variant to uniform
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                .setUniform(uniSpec.location, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, bindSpec] : this->opaqueBindingSpecs) {
        if (matProperties.find(propertyNameId) == matProperties.end() &&
            envProperties.has(propertyNameId)) {
            // bind context variant
            rend.getTypeConversion(bindSpec.srcSpec.typeInfo)
                .setOpaqueBinding(bindSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, uboSpec] : this->uboSpecs) {
        if (matProperties.find(propertyNameId) == matProperties.end() &&
            envProperties.has(propertyNameId)) {
            // bind context variant UBO
            rend.getTypeConversion(uboSpec.srcSpec.typeInfo)
                .setUBOBinding(uboSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }

    for (auto [propertyNameId, ssboSpec] : this->ssboSpecs) {
        if (matProperties.find(propertyNameId) == matProperties.end() &&
            envProperties.has(propertyNameId)) {
            // bind context variant buffer
            rend.getTypeConversion(ssboSpec.srcSpec.typeInfo)
                .setSSBOBinding(ssboSpec.bindingIndex, envProperties.get(propertyNameId));
        }
    }
}

void CompiledGLSLShader::setupMaterialProperties(OpenGLRenderer &rend,
                                                 const Material &material) const
{
    MMETER_SCOPE_PROFILER("CompiledGLSLShader::setupMaterialProperties(OpenGLRenderer &rend, const "
                          "Material &material) const");

    auto &matProperties = material.getProperties();

    for (auto [propertyNameId, uniSpec] : this->uniformSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // convert material variant to uniform
            rend.getTypeConversion(uniSpec.srcSpec.typeInfo)
                .setUniform(uniSpec.location, (*nameValIt).second);
        }
    }

    for (auto [propertyNameId, bindSpec] : this->opaqueBindingSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant
            rend.getTypeConversion(bindSpec.srcSpec.typeInfo)
                .setOpaqueBinding(bindSpec.bindingIndex, (*nameValIt).second);
        }
    }

    for (auto [propertyNameId, uboSpec] : this->uboSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant UBO
            rend.getTypeConversion(uboSpec.srcSpec.typeInfo)
                .setUBOBinding(uboSpec.bindingIndex, (*nameValIt).second);
        }
    }

    for (auto [propertyNameId, ssboSpec] : this->ssboSpecs) {
        if (auto nameValIt = matProperties.find(propertyNameId); nameValIt != matProperties.end()) {
            // bind material variant buffer
            rend.getTypeConversion(ssboSpec.srcSpec.typeInfo)
                .setSSBOBinding(ssboSpec.bindingIndex, (*nameValIt).second);
        }
    }
}

} // namespace Vitrae