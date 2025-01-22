#include "Vitrae/Renderers/OpenGL/Mesh.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Collections/MeshGenerator.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/SharedBuffer.hpp"
#include "Vitrae/TypeConversion/AssimpCvt.hpp"
#include "Vitrae/TypeConversion/StringCvt.hpp"

#include <map>
#include <vector>

namespace Vitrae
{
OpenGLMesh::OpenGLMesh(const AssimpLoadParams &params)
    : m_root(params.root), m_sentToGPU(false), m_aabb{{}, {}}
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(params.root.getComponent<Renderer>());

    // properties
    m_frontSideOrientation = FrontSideOrientation::CounterClockwise;

    // prepare vertices
    m_indexBuffer = makeBuffer<void, Triangle>(
        params.root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW,
        3 * params.p_extMesh->mNumFaces);

    auto cpuTriangles = m_indexBuffer.getMutableElements();

    // load triangles
    if (params.p_extMesh->HasFaces()) {
        for (int i = 0; i < params.p_extMesh->mNumFaces; i++) {
            for (int j = 0; j < params.p_extMesh->mFaces[i].mNumIndices; j++) {
                cpuTriangles[i].ind[j] = params.p_extMesh->mFaces[i].mIndices[j];
            }
        }
    }

    // load vertices
    auto extractVertexData =
        [&]<class aiType, class glmType = typename aiTypeCvt<aiType>::glmType>(
            std::span<const ComponentRoot::AiMeshBufferInfo<aiType>> aiBufferInfos) {
            for (auto &info : aiBufferInfos) {
                // get buffers
                std::size_t layoutInd = rend.getVertexBufferLayoutIndex(info.name);
                const aiType *src = info.extractor(*params.p_extMesh);

                // fill buffers
                if (src != nullptr) {
                    // construct buffer
                    SharedBufferPtr<void, glmType> p_buffer = makeBuffer<void, glmType>(
                        params.root, BufferUsageHint::HOST_INIT | BufferUsageHint::GPU_DRAW,
                        params.p_extMesh->mNumVertices);
                    m_vertexComponentBuffers[info.name] = p_buffer;

                    std::span<glmType> cpuBuffer = p_buffer.getMutableElements();

                    for (int i = 0; i < params.p_extMesh->mNumVertices; i++) {
                        cpuBuffer[i] = aiTypeCvt<aiType>::toGlmVal(src[i]);
                    }
                }
            }
        };

    extractVertexData(params.root.getAiMeshBufferInfos<aiVector2D>());
    extractVertexData(params.root.getAiMeshBufferInfos<aiVector3D>());
    extractVertexData(params.root.getAiMeshBufferInfos<aiColor3D>());
    extractVertexData(params.root.getAiMeshBufferInfos<aiColor4D>());

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

OpenGLMesh::OpenGLMesh(const TriangleVerticesParams &params)
    : m_root(params.root), m_friendlyname(params.friendlyname),
      m_frontSideOrientation(FrontSideOrientation::CounterClockwise), m_aabb{{}, {}},
      m_vertexComponentBuffers(params.vertexComponentBuffers), m_indexBuffer(params.indexBuffer),
      m_sentToGPU(false)
{}

OpenGLMesh::~OpenGLMesh()
{
    unloadFromGPU();
}

void OpenGLMesh::prepareComponents(const ParamList &components)
{
    MeshGeneratorCollection &meshGenerators = m_root.getComponent<MeshGeneratorCollection>();

    for (auto componentname : components.getSpecNameIds()) {
        if (m_vertexComponentBuffers.find(componentname) == m_vertexComponentBuffers.end()) {
            MeshGenerator generator = meshGenerators.getGeneratorForComponent(componentname);
            if (generator) {
                unloadFromGPU();

                auto newComponentBuffers = generator(m_root, *this);

                for (auto [name, p_buffer] : newComponentBuffers) {
                    m_vertexComponentBuffers[name] = p_buffer;
                }
            }
        }
    }
}

void OpenGLMesh::loadToGPU(Renderer &r)
{
    OpenGLRenderer &rend = static_cast<OpenGLRenderer &>(r);

    if (!m_sentToGPU) {
        m_sentToGPU = true;

        // prepare OpenGL buffers
        glGenVertexArrays(1, &VAO);

        // load vertices
        glBindVertexArray(VAO);

        for (auto [name, p_buffer] : m_vertexComponentBuffers) {
            OpenGLRawSharedBuffer &rawBuffer =
                static_cast<OpenGLRawSharedBuffer &>(*(p_buffer.getRawBuffer()));

            // get type info and conversion
            const TypeInfo &compType = p_buffer.getHeaderTypeInfo();
            const TypeInfo &subCompType = (compType.componentTypeInfo == TYPE_INFO<void>)
                                              ? compType
                                              : compType.componentTypeInfo;
            std::size_t numSubComponents =
                (compType.componentTypeInfo == TYPE_INFO<void>) ? 1 : compType.numComponents;

            const GLConversionSpec &convSpec = rend.getTypeConversion(subCompType);

            // assert that the type is convertible to a vertex array
            // OpenGL supports up to 4 components per vertex attribute
            /// TODO: Allow bigger types by separating them into different locations
            if (numSubComponents == 0 || numSubComponents > 4 ||
                convSpec.glTypeSpec.layout.indexSize != 1) {
                throw std::runtime_error(
                    "Unsupported component type " + String(compType.getShortTypeName()) +
                    " for mesh " + m_friendlyname +
                    "  due to invalid number of components or locations taken.");
            }
            if (!convSpec.scalarSpec.has_value()) {
                throw std::runtime_error(
                    "Unsupported component type " + String(compType.getShortTypeName()) +
                    " for mesh " + m_friendlyname +
                    "  due its component value type not being primitive according to conversion.");
            }
            const GLScalarSpec &scalarSpec = convSpec.scalarSpec.value();

            // send to OpenGL
            std::size_t layoutInd = rend.getVertexBufferLayoutIndex(name);
            glBindBuffer(GL_ARRAY_BUFFER, rawBuffer.getGlBufferHandle());
            glVertexAttribPointer(layoutInd,                        // layout pos
                                  numSubComponents,                 // data structure info
                                  scalarSpec.glTypeId,              //
                                  scalarSpec.isNormalized,          //
                                  p_buffer.getBytesStride(),        // data subbuffer location info
                                  (void *)p_buffer.getBytesOffset() //
            );
            glEnableVertexAttribArray(layoutInd);
        }

        OpenGLRawSharedBuffer &rawIndexBuffer =
            static_cast<OpenGLRawSharedBuffer &>(*(m_indexBuffer.getRawBuffer()));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rawIndexBuffer.getGlBufferHandle());

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
    }
}

FrontSideOrientation OpenGLMesh::getFrontSideOrientation() const
{
    return m_frontSideOrientation;
}

BoundingBox OpenGLMesh::getBoundingBox() const
{
    return m_aabb;
}

SharedSubBufferVariantPtr OpenGLMesh::getVertexComponentBuffer(StringId componentName) const
{
    return m_vertexComponentBuffers.at(componentName);
}

void OpenGLMesh::setVertexComponentBuffer(StringId componentName,
                                          SharedSubBufferVariantPtr p_buffer)
{
    unloadFromGPU();
    m_vertexComponentBuffers[componentName] = p_buffer;
}

SharedBufferPtr<void, Triangle> OpenGLMesh::getIndexBuffer() const
{
    return m_indexBuffer;
}

void OpenGLMesh::rasterize() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 3 * m_indexBuffer.numElements(), GL_UNSIGNED_INT, 0);
}

} // namespace Vitrae