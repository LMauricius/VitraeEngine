#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/TypeConversion/AssimpCvt.hpp"
#include "Vitrae/TypeConversion/GLCvt.hpp"
#include "Vitrae/TypeConversion/StringCvt.hpp"
#include "Vitrae/TypeConversion/VectorCvt.hpp"

#include <map>
#include <vector>

namespace Vitrae
{
OpenGLMesh::OpenGLMesh() : m_sentToGPU(false), m_aabb{{}, {}} {}

OpenGLMesh::OpenGLMesh(const AssimpLoadParams &params) : OpenGLMesh()
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(params.root.getComponent<Renderer>());

    // prepare vertices
    mTriangles.resize(params.p_extMesh->mNumFaces);

    // load triangles
    if (params.p_extMesh->HasFaces()) {
        for (int i = 0; i < params.p_extMesh->mNumFaces; i++) {
            for (int j = 0; j < params.p_extMesh->mFaces[i].mNumIndices; j++) {
                mTriangles[i].ind[j] = params.p_extMesh->mFaces[i].mIndices[j];
            }
        }
    }

    // load vertices
    auto extractVertexData =
        [&]<class aiType, class glmType = typename aiTypeCvt<aiType>::glmType>(
            std::span<const ComponentRoot::AiMeshBufferInfo<aiType>> aiBufferInfos,
            StableMap<StringId, std::vector<glmType>> &namedBuffers) {
            for (auto &info : aiBufferInfos) {
                // get buffers
                std::size_t layoutInd = rend.getVertexBufferLayoutIndex(info.name);
                const aiType *src = info.extractor(*params.p_extMesh);
                GLuint &vbo = VBOs[layoutInd];

                // fill buffers
                if (src != nullptr) {
                    std::vector<glmType> &buffer = namedBuffers[info.name];
                    buffer.resize(params.p_extMesh->mNumVertices);

                    for (int i = 0; i < params.p_extMesh->mNumVertices; i++) {
                        buffer[i] = aiTypeCvt<aiType>::toGlmVal(src[i]);
                    }
                }
            }
        };

    extractVertexData(params.root.getAiMeshBufferInfos<aiVector2D>(), namedVec2Buffers);
    extractVertexData(params.root.getAiMeshBufferInfos<aiVector3D>(), namedVec3Buffers);
    extractVertexData(params.root.getAiMeshBufferInfos<aiColor3D>(), namedVec3Buffers);
    extractVertexData(params.root.getAiMeshBufferInfos<aiColor4D>(), namedVec4Buffers);

    m_aabb = {
        {
            params.p_extMesh->mAABB.mMin.x,
            params.p_extMesh->mAABB.mMin.y,
            params.p_extMesh->mAABB.mMin.z,
        },
        {
            params.p_extMesh->mAABB.mMax.x,
            params.p_extMesh->mAABB.mMax.y,
            params.p_extMesh->mAABB.mMax.z,
        },
    };

    // debug
    m_friendlyname = toString(params.p_extMesh->mName);
}

OpenGLMesh::~OpenGLMesh() {}

void OpenGLMesh::loadToGPU(OpenGLRenderer &rend)
{
    if (!m_sentToGPU) {
        m_sentToGPU = true;

        // prepare OpenGL buffers
        glGenVertexArrays(1, &VAO);
        VBOs.resize(rend.getNumVertexBuffers());
        glGenBuffers(rend.getNumVertexBuffers(), VBOs.data());
        glGenBuffers(1, &EBO);

        // load vertices
        glBindVertexArray(VAO);

        auto sendVertexData =
            [&]<class glmType>(const StableMap<StringId, std::vector<glmType>> &namedBuffers) {
                for (auto [name, buffer] : namedBuffers) {
                    std::size_t layoutInd = rend.getVertexBufferLayoutIndex(name);
                    GLuint &vbo = VBOs[layoutInd];

                    // send to OpenGL
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(glmType) * buffer.size(), &(buffer[0]),
                                 GL_STATIC_DRAW);
                    glVertexAttribPointer(
                        layoutInd, // layout pos
                        VectorTypeInfo<glmType>::NumComponents,
                        GLTypeInfo<typename VectorTypeInfo<glmType>::value_type>::GlTypeId,
                        GL_FALSE,                  // data structure info
                        sizeof(glmType), (void *)0 // data location info
                    );
                    glEnableVertexAttribArray(layoutInd);
                }
            };

        sendVertexData(namedVec2Buffers);
        sendVertexData(namedVec3Buffers);
        sendVertexData(namedVec3Buffers);
        sendVertexData(namedVec4Buffers);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangle) * mTriangles.size(),
                     (void *)(mTriangles.data()), GL_STATIC_DRAW);

        glBindVertexArray(0);

        // debug
        String glLabel = String("mesh ") + String(m_friendlyname);
        glObjectLabel(GL_VERTEX_ARRAY, VAO, glLabel.size(), glLabel.data());
    }
}

void OpenGLMesh::unloadFromGPU()
{
    if (m_sentToGPU) {
        m_sentToGPU = false;
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(VBOs.size(), VBOs.data());
        glDeleteBuffers(1, &EBO);
    }
}

void OpenGLMesh::setMaterial(dynasma::LazyPtr<Material> mat)
{
    mMaterial = mat;
}

dynasma::LazyPtr<Material> OpenGLMesh::getMaterial() const
{
    return mMaterial.value();
}

std::span<const Triangle> OpenGLMesh::getTriangles() const
{
    return mTriangles;
}

BoundingBox OpenGLMesh::getBoundingBox() const
{
    return m_aabb;
}

Variant OpenGLMesh::getVertexData(StringId bufferName, const TypeInfo &type) const
{
    if (type == TYPE_INFO<glm::vec1>) {
        auto &buf = namedVec1Buffers.at(bufferName);
        return std::span<const glm::vec1>(buf);
    } else if (type == TYPE_INFO<glm::vec2>) {
        auto &buf = namedVec2Buffers.at(bufferName);
        return std::span<const glm::vec2>(buf);
    } else if (type == TYPE_INFO<glm::vec3>) {
        auto &buf = namedVec3Buffers.at(bufferName);
        return std::span<const glm::vec3>(buf);
    } else if (type == TYPE_INFO<glm::vec4>) {
        auto &buf = namedVec4Buffers.at(bufferName);
        return std::span<const glm::vec4>(buf);
    } else {
        throw std::out_of_range("Unused vertex element type");
    }
}

} // namespace Vitrae