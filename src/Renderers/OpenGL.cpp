#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Renderers/OpenGL/Compositing/ClearRender.hpp"
#include "Vitrae/Renderers/OpenGL/Compositing/Compute.hpp"
#include "Vitrae/Renderers/OpenGL/Compositing/DataRender.hpp"
#include "Vitrae/Renderers/OpenGL/Compositing/SceneRender.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Renderers/OpenGL/ShaderCompilation.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/Constant.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/Function.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/FunctionCall.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/Header.hpp"
#include "Vitrae/Renderers/OpenGL/SharedBuffer.hpp"
#include "Vitrae/Renderers/OpenGL/Texture.hpp"

#include "dynasma/cachers/basic.hpp"
#include "dynasma/keepers/naive.hpp"
#include "dynasma/managers/basic.hpp"

#include "glad/glad.h"
// must be after glad.h
#include "GLFW/glfw3.h"

namespace Vitrae
{
OpenGLRenderer::OpenGLRenderer() : m_vertexBufferFreeIndex(0)
{
    /*
    Standard GLSL ypes
    */
    // clang-format off
    specifyGlType({.glMutableTypeName = "float",  .glConstTypeName = "float",  .std140Size = 4,  .std140Alignment = 4,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "vec2",   .glConstTypeName = "vec2",   .std140Size = 8,  .std140Alignment = 8,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "vec3",   .glConstTypeName = "vec3",   .std140Size = 12, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "vec4",   .glConstTypeName = "vec4",   .std140Size = 16, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "mat2",   .glConstTypeName = "mat2",   .std140Size = 32, .std140Alignment = 16, .layoutIndexSize = 2});
    specifyGlType({.glMutableTypeName = "mat2x3", .glConstTypeName = "mat2x3", .std140Size = 48, .std140Alignment = 16, .layoutIndexSize = 2});
    specifyGlType({.glMutableTypeName = "mat2x4", .glConstTypeName = "mat2x4", .std140Size = 64, .std140Alignment = 16, .layoutIndexSize = 2});
    specifyGlType({.glMutableTypeName = "mat3",   .glConstTypeName = "mat3",   .std140Size = 48, .std140Alignment = 16, .layoutIndexSize = 3});
    specifyGlType({.glMutableTypeName = "mat3x2", .glConstTypeName = "mat3x2", .std140Size = 32, .std140Alignment = 8,  .layoutIndexSize = 3});
    specifyGlType({.glMutableTypeName = "mat3x4", .glConstTypeName = "mat3x4", .std140Size = 64, .std140Alignment = 16, .layoutIndexSize = 3});
    specifyGlType({.glMutableTypeName = "mat4",   .glConstTypeName = "mat4",   .std140Size = 64, .std140Alignment = 16, .layoutIndexSize = 4});
    specifyGlType({.glMutableTypeName = "mat4x2", .glConstTypeName = "mat4x2", .std140Size = 32, .std140Alignment = 8,  .layoutIndexSize = 4});
    specifyGlType({.glMutableTypeName = "mat4x3", .glConstTypeName = "mat4x3", .std140Size = 48, .std140Alignment = 16, .layoutIndexSize = 4});
    specifyGlType({.glMutableTypeName = "int",    .glConstTypeName = "int",    .std140Size = 4,  .std140Alignment = 4,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "ivec2",  .glConstTypeName = "ivec2",  .std140Size = 8,  .std140Alignment = 8,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "ivec3",  .glConstTypeName = "ivec3",  .std140Size = 12, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "ivec4",  .glConstTypeName = "ivec4",  .std140Size = 16, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "uint",   .glConstTypeName = "uint",   .std140Size = 4,  .std140Alignment = 4,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "uvec2",  .glConstTypeName = "uvec2",  .std140Size = 8,  .std140Alignment = 8,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "uvec3",  .glConstTypeName = "uvec3",  .std140Size = 12, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "uvec4",  .glConstTypeName = "uvec4",  .std140Size = 16, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "bool",   .glConstTypeName = "bool",   .std140Size = 4,  .std140Alignment = 4,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "bvec2",  .glConstTypeName = "bvec2",  .std140Size = 8,  .std140Alignment = 8,  .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "bvec3",  .glConstTypeName = "bvec3",  .std140Size = 12, .std140Alignment = 16, .layoutIndexSize = 1});
    specifyGlType({.glMutableTypeName = "bvec4",  .glConstTypeName = "bvec4",  .std140Size = 16, .std140Alignment = 16, .layoutIndexSize = 1});

    specifyGlType({.glMutableTypeName = "image2D", .glConstTypeName = "sampler2D", .std140Size = 0, .std140Alignment = 0, .layoutIndexSize = 1});

    /*
    Type conversions
    */
    specifyTypeConversion({.hostType = Variant::getTypeInfo<       float>(), .glTypeSpec = getGlTypeSpec("float"), .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1f(location, hostValue.get<float>()                                    );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::vec2>(), .glTypeSpec = getGlTypeSpec("vec2"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform2fv(location, 1, &hostValue.get<glm::vec2>()[0]                         );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::vec3>(), .glTypeSpec = getGlTypeSpec("vec3"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform3fv(location, 1, &hostValue.get<glm::vec3>()[0]                         );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::vec4>(), .glTypeSpec = getGlTypeSpec("vec4"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform4fv(location, 1, &hostValue.get<glm::vec4>()[0]                         );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<         int>(), .glTypeSpec = getGlTypeSpec("int"),   .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1i(location, hostValue.get<int>()                                      );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::ivec2>(), .glTypeSpec = getGlTypeSpec("ivec2"), .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform2iv(location, 1, &hostValue.get<glm::ivec2>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::ivec3>(), .glTypeSpec = getGlTypeSpec("ivec3"), .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform3iv(location, 1, &hostValue.get<glm::ivec3>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::ivec4>(), .glTypeSpec = getGlTypeSpec("ivec4"), .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform4iv(location, 1, &hostValue.get<glm::ivec4>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<unsigned int>(), .glTypeSpec = getGlTypeSpec("uint"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform1ui(location, hostValue.get<unsigned int>()                             );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::uvec2>(), .glTypeSpec = getGlTypeSpec("uvec2"), .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform2uiv(location, 1, &hostValue.get<glm::uvec2>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::uvec3>(), .glTypeSpec = getGlTypeSpec("uvec3"), .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform3uiv(location, 1, &hostValue.get<glm::uvec3>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::uvec4>(), .glTypeSpec = getGlTypeSpec("uvec4"), .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform4uiv(location, 1, &hostValue.get<glm::uvec4>()[0]                        );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<        bool>(), .glTypeSpec = getGlTypeSpec("bool"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1i(location, hostValue.get<bool>() ? 1 : 0                             );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::bvec2>(), .glTypeSpec = getGlTypeSpec("bvec2"), .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec2& val = hostValue.get<glm::bvec2>();          glUniform2i(location, val.x ? 1 : 0, val.y ? 1 : 0                              );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::bvec3>(), .glTypeSpec = getGlTypeSpec("bvec3"), .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec3& val = hostValue.get<glm::bvec3>();          glUniform3i(location, val.x ? 1 : 0, val.y ? 1 : 0, val.z ? 1 : 0               );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<  glm::bvec4>(), .glTypeSpec = getGlTypeSpec("bvec4"), .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec4& val = hostValue.get<glm::bvec4>();          glUniform4i(location, val.x ? 1 : 0, val.y ? 1 : 0, val.z ? 1 : 0, val.w ? 1 : 0);}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::mat2>(), .glTypeSpec = getGlTypeSpec("mat2"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2>()[0][0]            );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat2x3>(), .glTypeSpec = getGlTypeSpec("mat2x3"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix2x3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2x3>()[0][0]          );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat2x4>(), .glTypeSpec = getGlTypeSpec("mat2x4"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix2x4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2x4>()[0][0]          );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::mat3>(), .glTypeSpec = getGlTypeSpec("mat3"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3>()[0][0]            );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat3x2>(), .glTypeSpec = getGlTypeSpec("mat3x2"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix3x2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3x2>()[0][0]          );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat3x4>(), .glTypeSpec = getGlTypeSpec("mat3x4"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix3x4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3x4>()[0][0]          );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo<   glm::mat4>(), .glTypeSpec = getGlTypeSpec("mat4"),  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4>()[0][0]            );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat4x2>(), .glTypeSpec = getGlTypeSpec("mat4x2"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix4x2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4x2>()[0][0]          );}});
    specifyTypeConversion({.hostType = Variant::getTypeInfo< glm::mat4x3>(), .glTypeSpec = getGlTypeSpec("mat4x3"),.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix4x3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4x3>()[0][0]          );}});
    // clang-format on

    specifyTypeConversion({.hostType = Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
                           .glTypeSpec = getGlTypeSpec("image2D"),
                           .setBinding = [](int bindingIndex, const Variant &hostValue) {
                               auto p_tex = hostValue.get<dynasma::FirmPtr<Texture>>();
                               OpenGLTexture &tex = static_cast<OpenGLTexture &>(*p_tex);
                               glActiveTexture(GL_TEXTURE0 + bindingIndex);
                               glBindTexture(GL_TEXTURE_2D, tex.glTextureId);
                           }});

    /*
    Mesh vertex buffers for standard components
    */
    specifyVertexBuffer({.name = StandardVertexBufferNames::POSITION,
                         .typeInfo = Variant::getTypeInfo<glm::vec3>()});
    specifyVertexBuffer(
        {.name = StandardVertexBufferNames::NORMAL, .typeInfo = Variant::getTypeInfo<glm::vec3>()});
    specifyVertexBuffer({.name = StandardVertexBufferNames::TEXTURE_COORD,
                         .typeInfo = Variant::getTypeInfo<glm::vec2>()});
    specifyVertexBuffer(
        {.name = StandardVertexBufferNames::COLOR, .typeInfo = Variant::getTypeInfo<glm::vec4>()});
}

OpenGLRenderer::~OpenGLRenderer() {}

namespace
{
void APIENTRY globalDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                  GLsizei length, const GLchar *message, const void *userParam)
{
    const ComponentRoot &root = *reinterpret_cast<const ComponentRoot *>(userParam);

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        root.getErrStream() << "OpenGL error: " << message << std::endl;
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        root.getErrStream() << "OpenGL deprecated behavior: " << message << std::endl;
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        root.getErrStream() << "OpenGL undefined behavior: " << message << std::endl;
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        root.getErrStream() << "OpenGL portability issue: " << message << std::endl;
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        root.getErrStream() << "OpenGL performance issue: " << message << std::endl;
        break;
    }
}
} // namespace

void OpenGLRenderer::mainThreadSetup(ComponentRoot &root)
{
    // threading stuff
    m_mainThreadId = std::this_thread::get_id();
    m_contextMutex.lock();

    // clang-format off
    root.setComponent<              MeshKeeper>(new  dynasma::NaiveKeeper<              MeshKeeperSeed, std::allocator<              OpenGLMesh>>());
    root.setComponent<          MaterialKeeper>(new  dynasma::NaiveKeeper<          MaterialKeeperSeed, std::allocator<                Material>>());
    root.setComponent<          TextureManager>(new dynasma::BasicManager<                 TextureSeed, std::allocator<           OpenGLTexture>>());
    root.setComponent<       FrameStoreManager>(new dynasma::BasicManager<              FrameStoreSeed, std::allocator<        OpenGLFrameStore>>());
    root.setComponent<   RawSharedBufferKeeper>(new  dynasma::NaiveKeeper<   RawSharedBufferKeeperSeed, std::allocator<   OpenGLRawSharedBuffer>>());
    root.setComponent<    ShaderConstantKeeper>(new  dynasma::NaiveKeeper<    ShaderConstantKeeperSeed, std::allocator<    OpenGLShaderConstant>>());
    root.setComponent<    ShaderFunctionKeeper>(new  dynasma::NaiveKeeper<    ShaderFunctionKeeperSeed, std::allocator<    OpenGLShaderFunction>>());
    root.setComponent<ShaderFunctionCallKeeper>(new  dynasma::NaiveKeeper<ShaderFunctionCallKeeperSeed, std::allocator<OpenGLShaderFunctionCall>>());
    root.setComponent<      ShaderHeaderKeeper>(new  dynasma::NaiveKeeper<      ShaderHeaderKeeperSeed, std::allocator<      OpenGLShaderHeader>>());
    root.setComponent<ComposeSceneRenderKeeper>(new  dynasma::NaiveKeeper<ComposeSceneRenderKeeperSeed, std::allocator<OpenGLComposeSceneRender>>());
    root.setComponent< ComposeDataRenderKeeper>(new  dynasma::NaiveKeeper< ComposeDataRenderKeeperSeed, std::allocator< OpenGLComposeDataRender>>());
    root.setComponent<    ComposeComputeKeeper>(new  dynasma::NaiveKeeper<    ComposeComputeKeeperSeed, std::allocator<    OpenGLComposeCompute>>());
    root.setComponent<ComposeClearRenderKeeper>(new  dynasma::NaiveKeeper<ComposeClearRenderKeeperSeed, std::allocator<OpenGLComposeClearRender>>());
    root.setComponent<CompiledGLSLShaderCacher>(new  dynasma::BasicCacher<CompiledGLSLShaderCacherSeed, std::allocator<      CompiledGLSLShader>>());
    // clang-format on

    glfwInit();

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

    /*
    Texture types
    */
    root.addAiMaterialTextureInfo(
        {StandardMaterialTextureNames::DIFFUSE, aiTextureType_DIFFUSE,
         root.getComponent<TextureManager>().register_asset(
             {Texture::PureColorParams{.root = root, .color = {1.0f, 1.0f, 1.0f, 1.0f}}})});
    root.addAiMaterialTextureInfo(
        {StandardMaterialTextureNames::SPECULAR, aiTextureType_SPECULAR,
         root.getComponent<TextureManager>().register_asset(
             {Texture::PureColorParams{.root = root, .color = {0.0f, 0.0f, 0.0f, 1.0f}}})});
    root.addAiMaterialTextureInfo(
        {StandardMaterialTextureNames::EMISSIVE, aiTextureType_EMISSIVE,
         root.getComponent<TextureManager>().register_asset(
             {Texture::PureColorParams{.root = root, .color = {0.0f, 0.0f, 0.0f, 0.0f}}})});

    /*
    Main window
    */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    mp_mainWindow = glfwCreateWindow(640, 480, "", nullptr, nullptr);
    if (mp_mainWindow == nullptr) {
        fprintf(stderr, "Failed to Create OpenGL Context");
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(mp_mainWindow);
    gladLoadGL(); // seems we need to do this after setting the first context... for whatev reason

    /*
    List extensions
    */
    GLint no_of_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &no_of_extensions);

    root.getInfoStream() << "OpenGL extensions:" << std::endl;
    for (int i = 0; i < no_of_extensions; ++i)
        root.getInfoStream() << "\t" << (const char *)glGetStringi(GL_EXTENSIONS, i) << std::endl;
    root.getInfoStream() << "OpenGL end of extensions" << std::endl;

    /*
    Error handling
    */
    glDebugMessageCallback(globalDebugCallback, &root);
}

void OpenGLRenderer::mainThreadFree()
{
    glfwMakeContextCurrent(0);
    glfwDestroyWindow(mp_mainWindow);
    glfwTerminate();
}

void OpenGLRenderer::mainThreadUpdate()
{
    glfwPollEvents();
}

void OpenGLRenderer::anyThreadEnable()
{
    m_contextMutex.lock();
    glfwMakeContextCurrent(mp_mainWindow);
}

void OpenGLRenderer::anyThreadDisable()
{
    glfwMakeContextCurrent(0);
    m_contextMutex.unlock();
}

GLFWwindow *OpenGLRenderer::getWindow()
{
    return mp_mainWindow;
}

void OpenGLRenderer::specifyGlType(const GLTypeSpec &newSpec)
{
    m_glTypes.emplace(StringId(newSpec.glMutableTypeName), new GLTypeSpec(newSpec));
}

void OpenGLRenderer::specifyTypeConversion(const GLConversionSpec &newSpec)
{
    m_glConversions.emplace(std::type_index(*newSpec.hostType.p_id), new GLConversionSpec(newSpec));
}

const GLTypeSpec &OpenGLRenderer::getGlTypeSpec(StringId name) const
{
    return *m_glTypes.at(name);
}

const GLConversionSpec &OpenGLRenderer::getTypeConversion(const TypeInfo &type) const
{
    return *m_glConversions.at(std::type_index(*type.p_id));
}

const StableMap<StringId, std::unique_ptr<GLTypeSpec>> &OpenGLRenderer::getAllGlTypeSpecs() const
{
    return m_glTypes;
}

void OpenGLRenderer::specifyVertexBuffer(const PropertySpec &newElSpec)
{
    const GLTypeSpec &glTypeSpec = getTypeConversion(newElSpec.typeInfo).glTypeSpec;

    m_vertexBufferIndices.emplace(StringId(newElSpec.name), m_vertexBufferFreeIndex);
    m_vertexBufferFreeIndex += glTypeSpec.layoutIndexSize;
    m_vertexBufferSpecs.emplace(StringId(newElSpec.name), &glTypeSpec);
}

std::size_t OpenGLRenderer::getNumVertexBuffers() const
{
    return m_vertexBufferIndices.size();
}

std::size_t OpenGLRenderer::getVertexBufferLayoutIndex(StringId name) const
{
    return m_vertexBufferIndices.at(name);
}

const StableMap<StringId, const GLTypeSpec *> &OpenGLRenderer::getAllVertexBufferSpecs() const
{
    return m_vertexBufferSpecs;
}

OpenGLRenderer::GpuValueStorageMethod OpenGLRenderer::getGpuStorageMethod(
    const GLTypeSpec &spec) const
{
    if (spec.glMutableTypeName == "image1D" || spec.glMutableTypeName == "image2D") {
        return GpuValueStorageMethod::UniformBinding;
    } else if (spec.glslDefinitionSnippet.empty()) {
        return GpuValueStorageMethod::Uniform;
    } else if (spec.flexibleMemberSpec.has_value()) {
        return GpuValueStorageMethod::SSBO;
    } else {
        return GpuValueStorageMethod::UBO;
    }
}
const StableMap<StringId, PropertySpec> &OpenGLRenderer::getInputDependencyCache(
    std::size_t id) const
{
    return m_sceneRenderInputDependencies[id];
}

StableMap<StringId, PropertySpec> &OpenGLRenderer::getEditableInputDependencyCache(std::size_t id)
{
    return m_sceneRenderInputDependencies[id];
}

std::size_t OpenGLRenderer::getInputDependencyCacheID(
    const Task *p_task, dynasma::LazyPtr<Method<ShaderTask>> p_defaultVertexMethod,
    dynasma::LazyPtr<Method<ShaderTask>> p_defaultFragmentMethod) const
{
    return combinedHashes({{
        (std::size_t)p_task,
        std::hash<dynasma::LazyPtr<Method<ShaderTask>>>{}(p_defaultVertexMethod),
        std::hash<dynasma::LazyPtr<Method<ShaderTask>>>{}(p_defaultFragmentMethod),
    }});
}

void OpenGLRenderer::setRawBufferBinding(const RawSharedBuffer &buf, int bindingIndex)
{
    const OpenGLRawSharedBuffer &glbuf = static_cast<const OpenGLRawSharedBuffer &>(buf);

    if (!glbuf.isSynchronized()) {
        throw std::runtime_error("OpenGLRawSharedBuffer is not synchronized");
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, glbuf.getGlBufferHandle());
}

} // namespace Vitrae
