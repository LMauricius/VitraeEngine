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
#include "Vitrae/Renderers/OpenGL/Shading/FunctionCall.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/Header.hpp"
#include "Vitrae/Renderers/OpenGL/Shading/Snippet.hpp"
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
    const GLTypeSpec& floatSpec  = specifyGlType({.valueTypeName = "float",  .layout = {.std140Size = 4,  .std140Alignment = 4,  .indexSize = 1}});
    const GLTypeSpec& vec2Spec   = specifyGlType({.valueTypeName = "vec2",   .layout = {.std140Size = 8,  .std140Alignment = 8,  .indexSize = 1}});
    const GLTypeSpec& vec3Spec   = specifyGlType({.valueTypeName = "vec3",   .layout = {.std140Size = 12, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& vec4Spec   = specifyGlType({.valueTypeName = "vec4",   .layout = {.std140Size = 16, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& mat2Spec   = specifyGlType({.valueTypeName = "mat2",   .layout = {.std140Size = 32, .std140Alignment = 16, .indexSize = 2}});
    const GLTypeSpec& mat2x3Spec = specifyGlType({.valueTypeName = "mat2x3", .layout = {.std140Size = 48, .std140Alignment = 16, .indexSize = 2}});
    const GLTypeSpec& mat2x4Spec = specifyGlType({.valueTypeName = "mat2x4", .layout = {.std140Size = 64, .std140Alignment = 16, .indexSize = 2}});
    const GLTypeSpec& mat3Spec   = specifyGlType({.valueTypeName = "mat3",   .layout = {.std140Size = 48, .std140Alignment = 16, .indexSize = 3}});
    const GLTypeSpec& mat3x2Spec = specifyGlType({.valueTypeName = "mat3x2", .layout = {.std140Size = 32, .std140Alignment = 8,  .indexSize = 3}});
    const GLTypeSpec& mat3x4Spec = specifyGlType({.valueTypeName = "mat3x4", .layout = {.std140Size = 64, .std140Alignment = 16, .indexSize = 3}});
    const GLTypeSpec& mat4Spec   = specifyGlType({.valueTypeName = "mat4",   .layout = {.std140Size = 64, .std140Alignment = 16, .indexSize = 4}});
    const GLTypeSpec& mat4x2Spec = specifyGlType({.valueTypeName = "mat4x2", .layout = {.std140Size = 32, .std140Alignment = 8,  .indexSize = 4}});
    const GLTypeSpec& mat4x3Spec = specifyGlType({.valueTypeName = "mat4x3", .layout = {.std140Size = 48, .std140Alignment = 16, .indexSize = 4}});
    const GLTypeSpec& intSpec    = specifyGlType({.valueTypeName = "int",    .layout = {.std140Size = 4,  .std140Alignment = 4,  .indexSize = 1}});
    const GLTypeSpec& ivec2Spec  = specifyGlType({.valueTypeName = "ivec2",  .layout = {.std140Size = 8,  .std140Alignment = 8,  .indexSize = 1}});
    const GLTypeSpec& ivec3Spec  = specifyGlType({.valueTypeName = "ivec3",  .layout = {.std140Size = 12, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& ivec4Spec  = specifyGlType({.valueTypeName = "ivec4",  .layout = {.std140Size = 16, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& uintSpec   = specifyGlType({.valueTypeName = "uint",   .layout = {.std140Size = 4,  .std140Alignment = 4,  .indexSize = 1}});
    const GLTypeSpec& uvec2Spec  = specifyGlType({.valueTypeName = "uvec2",  .layout = {.std140Size = 8,  .std140Alignment = 8,  .indexSize = 1}});
    const GLTypeSpec& uvec3Spec  = specifyGlType({.valueTypeName = "uvec3",  .layout = {.std140Size = 12, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& uvec4Spec  = specifyGlType({.valueTypeName = "uvec4",  .layout = {.std140Size = 16, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& boolSpec   = specifyGlType({.valueTypeName = "bool",   .layout = {.std140Size = 4,  .std140Alignment = 4,  .indexSize = 1}});
    const GLTypeSpec& bvec2Spec  = specifyGlType({.valueTypeName = "bvec2",  .layout = {.std140Size = 8,  .std140Alignment = 8,  .indexSize = 1}});
    const GLTypeSpec& bvec3Spec  = specifyGlType({.valueTypeName = "bvec3",  .layout = {.std140Size = 12, .std140Alignment = 16, .indexSize = 1}});
    const GLTypeSpec& bvec4Spec  = specifyGlType({.valueTypeName = "bvec4",  .layout = {.std140Size = 16, .std140Alignment = 16, .indexSize = 1}});

    const GLTypeSpec& sampler2DSpec = specifyGlType({.opaqueTypeName = "sampler2D", .layout = {.std140Size = 0, .std140Alignment = 0, .indexSize = 1}});

    /*
    Type conversions
    */
    registerTypeConversion(Variant::getTypeInfo<       float>(), {.glTypeSpec = floatSpec, .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1f(location, hostValue.get<float>()                                    );}});
    registerTypeConversion(Variant::getTypeInfo<   glm::vec2>(), {.glTypeSpec = vec2Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform2fv(location, 1, &hostValue.get<glm::vec2>()[0]                         );}});
    registerTypeConversion(Variant::getTypeInfo<   glm::vec3>(), {.glTypeSpec = vec3Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform3fv(location, 1, &hostValue.get<glm::vec3>()[0]                         );}});
    registerTypeConversion(Variant::getTypeInfo<   glm::vec4>(), {.glTypeSpec = vec4Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform4fv(location, 1, &hostValue.get<glm::vec4>()[0]                         );}});
    registerTypeConversion(Variant::getTypeInfo<         int>(), {.glTypeSpec = intSpec,   .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1i(location, hostValue.get<int>()                                      );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::ivec2>(), {.glTypeSpec = ivec2Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform2iv(location, 1, &hostValue.get<glm::ivec2>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::ivec3>(), {.glTypeSpec = ivec3Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform3iv(location, 1, &hostValue.get<glm::ivec3>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::ivec4>(), {.glTypeSpec = ivec4Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform4iv(location, 1, &hostValue.get<glm::ivec4>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<unsigned int>(), {.glTypeSpec = uintSpec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                              glUniform1ui(location, hostValue.get<unsigned int>()                             );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::uvec2>(), {.glTypeSpec = uvec2Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform2uiv(location, 1, &hostValue.get<glm::uvec2>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::uvec3>(), {.glTypeSpec = uvec3Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform3uiv(location, 1, &hostValue.get<glm::uvec3>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::uvec4>(), {.glTypeSpec = uvec4Spec, .setUniform = [](GLint location, const Variant &hostValue) {                                                             glUniform4uiv(location, 1, &hostValue.get<glm::uvec4>()[0]                        );}});
    registerTypeConversion(Variant::getTypeInfo<        bool>(), {.glTypeSpec = boolSpec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                               glUniform1i(location, hostValue.get<bool>() ? 1 : 0                             );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::bvec2>(), {.glTypeSpec = bvec2Spec, .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec2& val = hostValue.get<glm::bvec2>();          glUniform2i(location, val.x ? 1 : 0, val.y ? 1 : 0                              );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::bvec3>(), {.glTypeSpec = bvec3Spec, .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec3& val = hostValue.get<glm::bvec3>();          glUniform3i(location, val.x ? 1 : 0, val.y ? 1 : 0, val.z ? 1 : 0               );}});
    registerTypeConversion(Variant::getTypeInfo<  glm::bvec4>(), {.glTypeSpec = bvec4Spec, .setUniform = [](GLint location, const Variant &hostValue) { const glm::bvec4& val = hostValue.get<glm::bvec4>();          glUniform4i(location, val.x ? 1 : 0, val.y ? 1 : 0, val.z ? 1 : 0, val.w ? 1 : 0);}});
    registerTypeConversion(Variant::getTypeInfo<   glm::mat2>(), {.glTypeSpec = mat2Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2>()[0][0]            );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat2x3>(), {.glTypeSpec = mat2x3Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix2x3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2x3>()[0][0]          );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat2x4>(), {.glTypeSpec = mat2x4Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix2x4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat2x4>()[0][0]          );}});
    registerTypeConversion(Variant::getTypeInfo<   glm::mat3>(), {.glTypeSpec = mat3Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3>()[0][0]            );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat3x2>(), {.glTypeSpec = mat3x2Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix3x2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3x2>()[0][0]          );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat3x4>(), {.glTypeSpec = mat3x4Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix3x4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat3x4>()[0][0]          );}});
    registerTypeConversion(Variant::getTypeInfo<   glm::mat4>(), {.glTypeSpec = mat4Spec,  .setUniform = [](GLint location, const Variant &hostValue) {                                                        glUniformMatrix4fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4>()[0][0]            );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat4x2>(), {.glTypeSpec = mat4x2Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix4x2fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4x2>()[0][0]          );}});
    registerTypeConversion(Variant::getTypeInfo< glm::mat4x3>(), {.glTypeSpec = mat4x3Spec,.setUniform = [](GLint location, const Variant &hostValue) {                                                      glUniformMatrix4x3fv(location, 1, GL_FALSE, &hostValue.get<glm::mat4x3>()[0][0]          );}});
    // clang-format on

    registerTypeConversion(Variant::getTypeInfo<dynasma::FirmPtr<Texture>>(),
                           {.glTypeSpec = sampler2DSpec,
                            .setOpaqueBinding = [](int bindingIndex, const Variant &hostValue) {
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
    root.setComponent<     ShaderSnippetKeeper>(new  dynasma::NaiveKeeper<     ShaderSnippetKeeperSeed, std::allocator<     OpenGLShaderSnippet>>());
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

const GLTypeSpec &OpenGLRenderer::specifyGlType(GLTypeSpec &&newSpec)
{
    m_glTypes.emplace_back(std::move(newSpec));
    return m_glTypes.back();
}

const GLConversionSpec &OpenGLRenderer::registerTypeConversion(const TypeInfo &hostType,
                                                               GLConversionSpec &&newSpec)
{
    GLConversionSpec &registeredSpec = m_glConversions.emplace_back(std::move(newSpec));
    m_glConversionsByHostType.emplace(std::type_index(*hostType.p_id), &registeredSpec);
    return registeredSpec;
}

const GLConversionSpec &OpenGLRenderer::getTypeConversion(const TypeInfo &type) const
{
    return *m_glConversionsByHostType.at(std::type_index(*type.p_id));
}

void OpenGLRenderer::specifyVertexBuffer(const ParamSpec &newElSpec)
{
    const GLTypeSpec &glTypeSpec = getTypeConversion(newElSpec.typeInfo).glTypeSpec;

    m_vertexBufferIndices.emplace(StringId(newElSpec.name), m_vertexBufferFreeIndex);
    m_vertexBufferFreeIndex += glTypeSpec.layout.indexSize;
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

void OpenGLRenderer::setRawBufferBinding(const RawSharedBuffer &buf, int bindingIndex)
{
    const OpenGLRawSharedBuffer &glbuf = static_cast<const OpenGLRawSharedBuffer &>(buf);

    if (!glbuf.isSynchronized()) {
        throw std::runtime_error("OpenGLRawSharedBuffer is not synchronized");
    }

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingIndex, glbuf.getGlBufferHandle());
}

} // namespace Vitrae
