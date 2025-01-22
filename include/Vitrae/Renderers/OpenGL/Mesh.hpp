#pragma once

#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/Typedefs.hpp"

#include "glad/glad.h"

#include <map>
#include <optional>

namespace Vitrae
{
class OpenGLRenderer;

class OpenGLMesh : public Mesh
{
  public:
    OpenGLMesh(const AssimpLoadParams &params);
    OpenGLMesh(const TriangleVerticesParams &params);
    ~OpenGLMesh();

    void prepareComponents(const ParamList &components) override;
    void loadToGPU(Renderer &rend) override;
    void unloadFromGPU();

    FrontSideOrientation getFrontSideOrientation() const override;

    BoundingBox getBoundingBox() const override;

    inline std::size_t memory_cost() const override
    {
        /// TODO: compute the real cost
        return sizeof(*this);
    }

    SharedSubBufferVariantPtr getVertexComponentBuffer(StringId componentName) const override;
    void setVertexComponentBuffer(StringId componentName,
                                  SharedSubBufferVariantPtr p_buffer) override;

    SharedBufferPtr<void, Triangle> getIndexBuffer() const override;

    void rasterize() const override;

    GLuint VAO;

  protected:
    ComponentRoot &m_root;
    String m_friendlyname;
    FrontSideOrientation m_frontSideOrientation;
    BoundingBox m_aabb;
    StableMap<StringId, SharedSubBufferVariantPtr> m_vertexComponentBuffers;
    SharedBufferPtr<void, Triangle> m_indexBuffer;

    bool m_sentToGPU;
};
} // namespace Vitrae