#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/MovableSpan.hpp"
#include "Vitrae/Util/PropertyGetter.hpp"
#include "Vitrae/Util/Variant.hpp"

#include "dynasma/cachers/abstract.hpp"
#include "dynasma/pointer.hpp"
#include "glad/glad.h"

#include <map>
#include <set>

namespace Vitrae
{
class OpenGLRenderer;
class ComponentRoot;
class PropertyList;
class Material;

class CompiledGLSLShader : public dynasma::PolymorphicBase
{
  public:
    struct ComputeCompilationSpec
    {
        PropertyGetter<std::uint32_t> invocationCountX;
        PropertyGetter<std::uint32_t> invocationCountY;
        PropertyGetter<std::uint32_t> invocationCountZ;
        glm::uvec3 groupSize;
        bool allowOutOfBoundsCompute;
    };

    struct CompilationSpec
    {
        PropertyAliases aliases;

        // prefix for output variables
        String outVarPrefix;

        // gl shader type
        GLenum shaderType;

        // for compute shaders
        std::optional<ComputeCompilationSpec> computeSpec;
    };

    class SurfaceShaderParams
    {
        const PropertyAliases &m_aliases;
        String m_vertexPositionOutputName;
        const PropertyList &m_fragmentOutputs;
        ComponentRoot *mp_root;
        std::size_t m_hash;

      public:
        SurfaceShaderParams(const PropertyAliases &aliases, String vertexPositionOutputName,
                            const PropertyList &fragmentOutputs, ComponentRoot &root);

        inline const PropertyAliases &getAliases() const { return m_aliases; }
        inline const String &getVertexPositionOutputName() const
        {
            return m_vertexPositionOutputName;
        }
        inline const PropertyList &getFragmentOutputs() const { return m_fragmentOutputs; }
        inline ComponentRoot &getRoot() const { return *mp_root; }

        inline std::size_t getHash() const { return m_hash; }

        inline bool operator==(const SurfaceShaderParams &o) const { return m_hash == o.m_hash; }
        inline auto operator<=>(const SurfaceShaderParams &o) const { return m_hash <=> o.m_hash; }
    };

    class ComputeShaderParams
    {
        const PropertyAliases &m_aliases;
        const PropertyList &m_desiredResults;
        ComponentRoot *mp_root;
        PropertyGetter<std::uint32_t> m_invocationCountX;
        PropertyGetter<std::uint32_t> m_invocationCountY;
        PropertyGetter<std::uint32_t> m_invocationCountZ;
        glm::uvec3 m_groupSize;
        bool m_allowOutOfBoundsCompute;
        std::size_t m_hash;

      public:
        ComputeShaderParams(ComponentRoot &root, const PropertyAliases &aliases,
                            const PropertyList &desiredResults,
                            PropertyGetter<std::uint32_t> invocationCountX,
                            PropertyGetter<std::uint32_t> invocationCountY,
                            PropertyGetter<std::uint32_t> invocationCountZ, glm::uvec3 groupSize,
                            bool allowOutOfBoundsCompute);

        inline ComponentRoot &getRoot() const { return *mp_root; }
        inline const PropertyAliases &getAliases() const { return m_aliases; }
        inline const PropertyList &getDesiredResults() const { return m_desiredResults; }
        inline PropertyGetter<std::uint32_t> getInvocationCountX() const
        {
            return m_invocationCountX;
        }
        inline PropertyGetter<std::uint32_t> getInvocationCountY() const
        {
            return m_invocationCountY;
        }
        inline PropertyGetter<std::uint32_t> getInvocationCountZ() const
        {
            return m_invocationCountZ;
        }
        inline glm::uvec3 getGroupSize() const { return m_groupSize; }
        inline bool getAllowOutOfBoundsCompute() const { return m_allowOutOfBoundsCompute; }

        inline std::size_t getHash() const { return m_hash; }

        inline bool operator==(const ComputeShaderParams &o) const { return m_hash == o.m_hash; }
        inline auto operator<=>(const ComputeShaderParams &o) const { return m_hash <=> o.m_hash; }
    };

    struct LocationSpec
    {
        PropertySpec srcSpec;
        GLint location;
    };

    struct BindingSpec
    {
        PropertySpec srcSpec;
        GLint location;
        GLuint bindingIndex;
    };

    CompiledGLSLShader(MovableSpan<CompilationSpec> compilationSpecs, ComponentRoot &root,
                       const PropertyList &desiredOutputs);
    CompiledGLSLShader(const SurfaceShaderParams &params);
    CompiledGLSLShader(const ComputeShaderParams &params);
    ~CompiledGLSLShader();

    inline std::size_t memory_cost() const { return 1; }

    void setupProperties(OpenGLRenderer &rend, ScopedDict &env) const;

    void setupProperties(OpenGLRenderer &rend, ScopedDict &env, const Material &material) const;
    void setupNonMaterialProperties(OpenGLRenderer &rend, ScopedDict &env,
                                    const Material &firstMaterial) const;
    void setupMaterialProperties(OpenGLRenderer &rend, const Material &material) const;

    PropertyList inputSpecs, outputSpecs, filterSpecs, consumingSpecs;
    GLuint programGLName;
    StableMap<StringId, LocationSpec> uniformSpecs;
    StableMap<StringId, BindingSpec> opaqueBindingSpecs;
    StableMap<StringId, BindingSpec> uboSpecs;
    StableMap<StringId, BindingSpec> ssboSpecs;
};

struct CompiledGLSLShaderCacherSeed
{
    using Asset = CompiledGLSLShader;

    std::variant<CompiledGLSLShader::SurfaceShaderParams, CompiledGLSLShader::ComputeShaderParams>
        kernel;

    inline std::size_t load_cost() const { return 1; }

    inline bool operator<(const CompiledGLSLShaderCacherSeed &o) const { return kernel < o.kernel; }
};

using CompiledGLSLShaderCacher = dynasma::AbstractCacher<CompiledGLSLShaderCacherSeed>;

} // namespace Vitrae