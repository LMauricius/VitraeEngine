#pragma once

#include "Vitrae/Assets/SharedBuffer.hpp"
#include "Vitrae/Pipelines/Task.hpp"
#include "Vitrae/Renderer.hpp"
#include "Vitrae/TypeConversion/AssimpTypeConvert.hpp"
#include "Vitrae/TypeConversion/GLTypeInfo.hpp"
#include "Vitrae/TypeConversion/VectorTypeInfo.hpp"
#include "Vitrae/Types/GraphicPrimitives.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "assimp/mesh.h"

#include "glad/glad.h"
// must be after glad.h
#include "GLFW/glfw3.h"

#include <functional>
#include <map>
#include <optional>
#include <thread>
#include <typeindex>
#include <vector>

namespace Vitrae
{
class Mesh;
class Texture;
class RawSharedBuffer;
class ComposeTask;

struct GLTypeSpec
{
    String glMutableTypeName;
    String glConstTypeName;

    String glslDefinitionSnippet;
    std::vector<const GLTypeSpec *> memberTypeDependencies;

    std::size_t std140Size;
    std::size_t std140Alignment;
    std::size_t layoutIndexSize;

    // used only if the type is a struct and has a flexible array member
    struct FlexibleMemberSpec
    {
        const GLTypeSpec &elementGlTypeSpec;
        std::size_t maxNumElements;
    };
    std::optional<FlexibleMemberSpec> flexibleMemberSpec;
};

struct GLConversionSpec
{
    static void std140ToHostIdentity(const GLConversionSpec *spec, const void *src, void *dst);
    static void hostToStd140Identity(const GLConversionSpec *spec, const void *src, void *dst);
    static dynasma::FirmPtr<RawSharedBuffer> getSharedBufferRaw(const GLConversionSpec *spec,
                                                                const void *src);

    const TypeInfo &hostType;
    const GLTypeSpec &glTypeSpec;

    void (*setUniform)(GLint location, const Variant &hostValue) = nullptr;
    void (*setBinding)(int bindingIndex, const Variant &hostValue) = nullptr;

    // used only if the type has a flexible array member
    struct FlexibleMemberConversion
    {
        std::size_t (*getNumElements)(const Variant &hostValue);
    };
    std::optional<FlexibleMemberConversion> flexibleMemberSpec;
};

/**
 * @brief Thrown when there is an error in type specification
 */
struct GLSpecificationError : public std::invalid_argument
{
    using std::invalid_argument::invalid_argument;
};

class OpenGLRenderer : public Renderer
{
  public:
    OpenGLRenderer();
    ~OpenGLRenderer();

    void mainThreadSetup(ComponentRoot &root) override;
    void mainThreadFree() override;
    void mainThreadUpdate() override;

    void anyThreadEnable() override;
    void anyThreadDisable() override;

    GLFWwindow *getWindow();

    void specifyGlType(const GLTypeSpec &newSpec);
    const GLTypeSpec &getGlTypeSpec(StringId glslName) const;
    void specifyTypeConversion(const GLConversionSpec &newSpec);
    const GLConversionSpec &getTypeConversion(const TypeInfo &hostType) const;
    const StableMap<StringId, std::unique_ptr<GLTypeSpec>> &getAllGlTypeSpecs() const;

    /**
     * @brief Automatically specified the gl type and conversion for a SharedBuffer type
     * @tparam SharedBufferT the type of the shared buffer
     * @param glname the name of the glsl type
     * @throws GLSpecificationError if the type isn't trivially convertible
     */
    template <SharedBufferPtrInst SharedBufferT>
    void specifyBufferTypeAndConversionAuto(StringView glname);

    void specifyVertexBuffer(const PropertySpec &newElSpec);
    template <class T> void specifyVertexBufferAuto()
    {
        specifyVertexBuffer(getTypeConversion(Variant::getTypeInfo<T>().p_id->name()));
    }
    std::size_t getNumVertexBuffers() const;
    std::size_t getVertexBufferLayoutIndex(StringId name) const;
    const StableMap<StringId, const GLTypeSpec *> &getAllVertexBufferSpecs() const;

    enum class GpuValueStorageMethod {
        Uniform,
        UniformBinding,
        UBO,
        SSBO
    };
    GpuValueStorageMethod getGpuStorageMethod(const GLTypeSpec &spec) const;

    const StableMap<StringId, PropertySpec> &getInputDependencyCache(std::size_t id) const;
    StableMap<StringId, PropertySpec> &getEditableInputDependencyCache(std::size_t id);
    std::size_t getInputDependencyCacheID(
        const Task *p_task, dynasma::LazyPtr<Method<ShaderTask>> p_defaultVertexMethod,
        dynasma::LazyPtr<Method<ShaderTask>> p_defaultFragmentMethod) const;

  protected:
    std::thread::id m_mainThreadId;
    std::mutex m_contextMutex;
    GLFWwindow *mp_mainWindow;

    StableMap<StringId, std::unique_ptr<GLTypeSpec>> m_glTypes;
    StableMap<std::type_index, std::unique_ptr<GLConversionSpec>> m_glConversions;

    StableMap<StringId, std::size_t> m_vertexBufferIndices;
    std::size_t m_vertexBufferFreeIndex;
    StableMap<StringId, const GLTypeSpec *> m_vertexBufferSpecs;

    mutable StableMap<std::size_t, StableMap<StringId, PropertySpec>>
        m_sceneRenderInputDependencies;

    // utility
    static void setRawBufferBinding(const RawSharedBuffer &buf, int bindingIndex);
};

template <SharedBufferPtrInst SharedBufferT>
void OpenGLRenderer::specifyBufferTypeAndConversionAuto(StringView glname)
{
    using ElementT = SharedBufferT::ElementT;
    using HeaderT = SharedBufferT::HeaderT;

    GLTypeSpec typeSpec = GLTypeSpec{
        .glMutableTypeName = String(glname),
        .glConstTypeName = String(glname),
        .glslDefinitionSnippet = "struct " + String(glname) + " {\n",
        .std140Size = 0,
        .std140Alignment = 0,
        .layoutIndexSize = 0,
    };

    if constexpr (SharedBufferT::HAS_HEADER) {
        auto &headerConv = getTypeConversion(Variant::getTypeInfo<HeaderT>());
        auto &headerGlTypeSpec = headerConv.glTypeSpec;

        typeSpec.glslDefinitionSnippet +=
            String("    ") + headerGlTypeSpec.glMutableTypeName + " header;\n";

        typeSpec.std140Size = headerGlTypeSpec.std140Size;
        typeSpec.std140Alignment = headerGlTypeSpec.std140Alignment;
        typeSpec.layoutIndexSize = headerGlTypeSpec.layoutIndexSize;

        if (headerGlTypeSpec.std140Size != sizeof(HeaderT)) {
            throw GLSpecificationError(
                "Buffer headers not trivially convertible between host and GLSL types");
        }

        typeSpec.memberTypeDependencies.push_back(&headerGlTypeSpec);
    }

    if constexpr (SharedBufferT::HAS_FAM_ELEMENTS) {
        auto &elementConv = getTypeConversion(Variant::getTypeInfo<ElementT>());
        auto &elementGlTypeSpec = elementConv.glTypeSpec;

        typeSpec.glslDefinitionSnippet +=
            String("    ") + elementGlTypeSpec.glMutableTypeName + " elements[];\n";

        typeSpec.flexibleMemberSpec.emplace(GLTypeSpec::FlexibleMemberSpec{
            .elementGlTypeSpec = elementGlTypeSpec,
            .maxNumElements = std::numeric_limits<std::uint32_t>::max(),
        });

        if (typeSpec.std140Alignment < elementGlTypeSpec.std140Alignment) {
            typeSpec.std140Alignment = elementGlTypeSpec.std140Alignment;
        }
        if (typeSpec.std140Size % elementGlTypeSpec.std140Alignment != 0) {
            typeSpec.std140Size = (typeSpec.std140Size / elementGlTypeSpec.std140Alignment + 1) *
                                  elementGlTypeSpec.std140Alignment;
        }

        if (typeSpec.std140Size != SharedBufferT::getFirstElementOffset()) {
            throw GLSpecificationError(
                "Buffer elements do not start at the same offset between host and GLSL types");
        }
        if (elementGlTypeSpec.std140Size != sizeof(ElementT)) {
            throw GLSpecificationError(
                "Buffer elements not trivially convertible between host and GLSL types");
        }

        typeSpec.memberTypeDependencies.push_back(&elementGlTypeSpec);
    }

    typeSpec.glslDefinitionSnippet += "};\n";

    specifyGlType(typeSpec);

    GLConversionSpec convSpec = GLConversionSpec{
        .hostType = Variant::getTypeInfo<SharedBufferT>(),
        .glTypeSpec = getGlTypeSpec(glname),
        .setUniform = nullptr,
        .setBinding = [](int bindingIndex, const Variant &hostValue) {
            setRawBufferBinding(*hostValue.get<SharedBufferT>().getRawBuffer(), bindingIndex);
        }};

    if constexpr (SharedBufferT::HAS_FAM_ELEMENTS) {
        convSpec.flexibleMemberSpec = GLConversionSpec::FlexibleMemberConversion{
            .getNumElements =
                [](const Variant &hostValue) {
                    return hostValue.get<SharedBufferT>().numElements();
                },
        };
    }

    specifyTypeConversion(convSpec);
}

} // namespace Vitrae