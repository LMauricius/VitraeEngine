#pragma once

#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/Types/Typedefs.hpp"
#include "Vitrae/Util/StringId.hpp"

#include "glad/glad.h"

#include <map>
#include <optional>

namespace Vitrae
{
class OpenGLRenderer;

class OpenGLMesh : public Mesh
{
  public:
    OpenGLMesh();
    OpenGLMesh(const AssimpLoadParams &params);
    ~OpenGLMesh();

    void loadToGPU(OpenGLRenderer &rend);
    void unloadFromGPU();

    void setMaterial(dynasma::LazyPtr<Material> mat) override;
    dynasma::LazyPtr<Material> getMaterial() const override;
    std::span<const Triangle> getTriangles() const override;
    BoundingBox getBoundingBox() const override;

    inline std::size_t memory_cost() const override
    {
        /// TODO: compute the real cost
        return sizeof(*this);
    }

    GLuint VAO;
    std::vector<GLuint> VBOs;
    GLuint EBO;

  protected:
    String m_friendlyname;
    std::optional<dynasma::LazyPtr<Material>> mMaterial;
    std::vector<Triangle> mTriangles;
    BoundingBox m_aabb;
    StableMap<StringId, std::vector<glm::vec1>> namedVec1Buffers;
    StableMap<StringId, std::vector<glm::vec2>> namedVec2Buffers;
    StableMap<StringId, std::vector<glm::vec3>> namedVec3Buffers;
    StableMap<StringId, std::vector<glm::vec4>> namedVec4Buffers;

    bool m_sentToGPU;

    Variant getVertexData(StringId bufferName, const TypeInfo &type) const override;
};
} // namespace Vitrae