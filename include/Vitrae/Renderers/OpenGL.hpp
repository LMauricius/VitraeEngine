#pragma once

#include "Vitrae/Assets/BufferUtil/Ptr.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Renderer.hpp"

#include "glad/glad.h"
// must be after glad.h
#include "GLFW/glfw3.h"

#include <cstddef>
#include <deque>
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

struct GLLayoutSpec
{
    std::size_t std140Size;
    std::size_t std140Alignment;
    std::size_t indexSize;
};

struct GLTypeSpec
{
    String valueTypeName;
    String opaqueTypeName;
    String structBodySnippet;

    GLLayoutSpec layout;

    // used only if the type is a struct and has a flexible array member
    struct FlexibleMemberSpec
    {
        const GLTypeSpec &elementGlTypeSpec;
        String memberName;
        std::size_t maxNumElements;
    };
    std::optional<FlexibleMemberSpec> flexibleMemberSpec;

    std::vector<const GLTypeSpec *> memberTypeDependencies;
};

struct GLScalarSpec
{

    /**
     * Constant such as GL_UNSIGNED_BYTE, GL_FLOAT etc.
     */
    GLenum glTypeId;

    /**
     * Whether the value is a normalized integer that maps to [0.0, 1.0] or [-1.0, 1.0]
     */
    bool isNormalized;
};

struct GLConversionSpec
{
    const GLTypeSpec &glTypeSpec;

    std::optional<GLScalarSpec> scalarSpec;

    void (*setUniform)(GLint location, const Variant &hostValue) = nullptr;
    void (*setOpaqueBinding)(int bindingIndex, const Variant &hostValue) = nullptr;
    void (*setUBOBinding)(int bindingIndex, const Variant &hostValue) = nullptr;
    void (*setSSBOBinding)(int bindingIndex, const Variant &hostValue) = nullptr;
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
    OpenGLRenderer(ComponentRoot &root);
    ~OpenGLRenderer();

    void mainThreadSetup(ComponentRoot &root) override;
    void mainThreadFree() override;
    void mainThreadUpdate() override;

    void anyThreadEnable() override;
    void anyThreadDisable() override;

    GLFWwindow *getWindow();

    /**
     * @brief Specify a GL type
     * @returns a reference to the permanent type specification struct
     * @note The returned struct is valid until the end of the renderer's lifetime
     */
    const GLTypeSpec &specifyGlType(GLTypeSpec &&newSpec);

    /**
     * @brief Registers a native to GL type conversion
     * @returns a reference to the permanent type conversion struct
     * @param hostType the native type
     * @param newSpec the conversion specification
     * @note The returned struct is valid until the end of the renderer's lifetime
     */
    const GLConversionSpec &registerTypeConversion(const TypeInfo &hostType,
                                                   GLConversionSpec &&newSpec);
    const GLConversionSpec &getTypeConversion(const TypeInfo &hostType) const;

    /**
     * @brief Automatically specified the gl type and conversion for a SharedBuffer type
     * @tparam SharedBufferT the type of the shared buffer
     * @throws GLSpecificationError if the type isn't trivially convertible
     */
    template <SharedBufferPtrInst SharedBufferT> void specifyBufferTypeAndConversionAuto();

    void specifyVertexBuffer(const ParamSpec &newElSpec) override;
    void specifyTextureSampler(StringView colorName) override;

    std::size_t getNumVertexBuffers() const;
    std::size_t getVertexBufferLayoutIndex(StringId name) const;
    const StableMap<StringId, const GLTypeSpec *> &getAllVertexBufferSpecs() const;

  protected:
    ComponentRoot &m_root;

    std::thread::id m_mainThreadId;
    std::mutex m_contextMutex;
    GLFWwindow *mp_mainWindow;

    std::deque<GLTypeSpec> m_glTypes;
    std::deque<GLConversionSpec> m_glConversions;
    StableMap<std::type_index, GLConversionSpec *> m_glConversionsByHostType;

    StableMap<StringId, std::size_t> m_vertexBufferIndices;
    std::size_t m_vertexBufferFreeIndex;
    StableMap<StringId, const GLTypeSpec *> m_vertexBufferSpecs;

    std::unordered_set<StringId> m_specifiedColorNames;

    mutable StableMap<std::size_t, StableMap<StringId, ParamSpec>> m_sceneRenderInputDependencies;

    // utility
    static void setRawBufferBinding(const RawSharedBuffer &buf, int bindingIndex);
};

template <SharedBufferPtrInst SharedBufferT>
void OpenGLRenderer::specifyBufferTypeAndConversionAuto()
{
    using ElementT = SharedBufferT::ElementT;
    using HeaderT = SharedBufferT::HeaderT;

    GLTypeSpec typeSpec = GLTypeSpec{
        .valueTypeName = "",
        .opaqueTypeName = "",
        .structBodySnippet = "",
        .layout =
            GLLayoutSpec{
                .std140Size = 0,
                .std140Alignment = 0,
                .indexSize = 0,
            },
    };

    if constexpr (SharedBufferT::HAS_HEADER) {
        auto &headerConv = getTypeConversion(TYPE_INFO<HeaderT>);
        auto &headerGlTypeSpec = headerConv.glTypeSpec;

        if (!headerGlTypeSpec.structBodySnippet.empty()) {
            typeSpec.structBodySnippet += headerGlTypeSpec.structBodySnippet;
        } else if (!headerGlTypeSpec.valueTypeName.empty()) {
            typeSpec.structBodySnippet += headerGlTypeSpec.valueTypeName + " header;\n";
        } else {
            throw GLSpecificationError("No struct body snippet or value type name for the header");
        }

        if (headerGlTypeSpec.std140Size != sizeof(HeaderT)) {
            throw GLSpecificationError(
                "Buffer headers not trivially convertible between host and GLSL types");
        }

        typeSpec.memberTypeDependencies.push_back(&headerGlTypeSpec);
    }

    if constexpr (SharedBufferT::HAS_FAM_ELEMENTS) {
        auto &elementConv = getTypeConversion(TYPE_INFO<ElementT>);
        auto &elementGlTypeSpec = elementConv.glTypeSpec;

        if constexpr (SharedBufferT::HAS_HEADER) {
            typeSpec.flexibleMemberSpec.emplace(GLTypeSpec::FlexibleMemberSpec{
                .elementGlTypeSpec = elementGlTypeSpec,
                .memberName = "elements",
                .maxNumElements = std::numeric_limits<std::uint32_t>::max(),
            });
        } else {
            typeSpec.flexibleMemberSpec.emplace(GLTypeSpec::FlexibleMemberSpec{
                .elementGlTypeSpec = elementGlTypeSpec,
                .memberName = "",
                .maxNumElements = std::numeric_limits<std::uint32_t>::max(),
            });
        }

        if (typeSpec.layout.std140Alignment < elementGlTypeSpec.layout.std140Alignment) {
            typeSpec.layout.std140Alignment = elementGlTypeSpec.layout.std140Alignment;
        }
        if (typeSpec.layout.std140Size % elementGlTypeSpec.layout.std140Alignment != 0) {
            typeSpec.layout.std140Size =
                (typeSpec.layout.std140Size / elementGlTypeSpec.layout.std140Alignment + 1) *
                elementGlTypeSpec.layout.std140Alignment;
        }

        if (typeSpec.layout.std140Size !=
            BufferLayoutInfo::getFirstElementOffset(TYPE_INFO<HeaderT>, TYPE_INFO<ElementT>)) {
            throw GLSpecificationError(
                "Buffer elements do not start at the same offset between host and GLSL types");
        }
        if (elementGlTypeSpec.layout.std140Size != sizeof(ElementT)) {
            throw GLSpecificationError(
                "Buffer elements not trivially convertible between host and GLSL types");
        }

        typeSpec.memberTypeDependencies.push_back(&elementGlTypeSpec);
    }
    const GLTypeSpec &registeredTypeSpec = specifyGlType(std::move(typeSpec));

    registerTypeConversion(
        TYPE_INFO<SharedBufferT>,
        GLConversionSpec{.glTypeSpec = registeredTypeSpec,
                         .setUniform = nullptr,
                         .setOpaqueBinding = nullptr,
                         .setUBOBinding = nullptr,
                         .setSSBOBinding = [](int bindingIndex, const Variant &hostValue) {
                             setRawBufferBinding(*hostValue.get<SharedBufferT>().getRawBuffer(),
                                                 bindingIndex);
                         }});
}

} // namespace Vitrae