#pragma once

#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Types/Typedefs.hpp"
#include "Vitrae/Util/MovableSpan.hpp"
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

class CompiledGLSLShader : public dynasma::PolymorphicBase
{
  public:
    struct CompilationSpec
    {
        // the specified shading method
        dynasma::FirmPtr<Method<ShaderTask>> p_method;

        // property mapping (for built-in variables),
        std::map<StringId, String> inputsToPredefinedVars;
        std::map<String, String> predefinedVarsToOutputs;

        // prefix for output variables
        String outVarPrefix;

        // gl shader type
        GLenum shaderType;
    };

    class SurfaceShaderParams
    {
        dynasma::FirmPtr<Method<ShaderTask>> mp_vertexMethod;
        dynasma::FirmPtr<Method<ShaderTask>> mp_fragmentMethod;
        String m_vertexPositionOutputName;
        dynasma::FirmPtr<const PropertyList> mp_fragmentOutputs;
        ComponentRoot *mp_root;
        std::size_t m_hash;

      public:
        SurfaceShaderParams(dynasma::FirmPtr<Method<ShaderTask>> p_vertexMethod,
                            dynasma::FirmPtr<Method<ShaderTask>> p_fragmentMethod,
                            String vertexPositionOutputName,
                            dynasma::FirmPtr<const PropertyList> p_fragmentOutputs,
                            ComponentRoot &root);

        inline auto getVertexMethodPtr() const { return mp_vertexMethod; }
        inline auto getFragmentMethodPtr() const { return mp_fragmentMethod; }
        inline const String &getVertexPositionOutputName() const
        {
            return m_vertexPositionOutputName;
        }
        inline auto getFragmentOutputsPtr() const { return mp_fragmentOutputs; }
        inline ComponentRoot &getRoot() const { return *mp_root; }

        inline std::size_t getHash() const { return m_hash; }

        inline bool operator==(const SurfaceShaderParams &o) const { return m_hash == o.m_hash; }
        inline auto operator<=>(const SurfaceShaderParams &o) const { return m_hash <=> o.m_hash; }
    };

    class ComputeShaderParams
    {
        dynasma::FirmPtr<Method<ShaderTask>> mp_computeMethod;
        dynasma::FirmPtr<const PropertyList> mp_desiredResults;
        ComponentRoot *mp_root;
        glm::uvec3 m_groupSize;
        bool m_allowOutOfBoundsCompute;
        std::size_t m_hash;

      public:
        ComputeShaderParams(ComponentRoot &root,
                            dynasma::FirmPtr<Method<ShaderTask>> p_computeMethod,
                            dynasma::FirmPtr<const PropertyList> p_desiredResults,
                            glm::uvec3 groupSize, bool allowOutOfBoundsCompute);

        inline auto getComputeMethodPtr() const { return mp_computeMethod; }
        inline auto getDesiredResultsPtr() const { return mp_desiredResults; }
        inline ComponentRoot &getRoot() const { return *mp_root; }

        inline std::size_t getHash() const { return m_hash; }

        inline bool operator==(const ComputeShaderParams &o) const { return m_hash == o.m_hash; }
        inline auto operator<=>(const ComputeShaderParams &o) const { return m_hash <=> o.m_hash; }
    };

    struct VariableSpec
    {
        PropertySpec srcSpec;
        GLint glNameId;
    };

    CompiledGLSLShader(MovableSpan<CompilationSpec> compilationSpecs, ComponentRoot &root,
                       const PropertyList &desiredOutputs);
    CompiledGLSLShader(const SurfaceShaderParams &params);
    CompiledGLSLShader(const ComputeShaderParams &params);
    ~CompiledGLSLShader();

    inline std::size_t memory_cost() const { return 1; }

    GLuint programGLName;
    StableMap<StringId, VariableSpec> uniformSpecs;
    StableMap<StringId, VariableSpec> bindingSpecs;
    StableMap<StringId, VariableSpec> uboSpecs;
    StableMap<StringId, VariableSpec> ssboSpecs;
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