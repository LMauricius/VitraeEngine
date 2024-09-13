#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Mesh.hpp"

#include <iostream>

namespace Vitrae
{
ComponentRoot::ComponentRoot()
    : mInfoStream(&std::cout), mWarningStream(&std::cout), mErrStream(&std::cerr)
{
    addAiMeshBufferInfo<aiVector3D>(
        {StandardVertexBufferNames::POSITION, [](const aiMesh &extMesh) -> const aiVector3D * {
             if (extMesh.HasPositions()) {
                 return extMesh.mVertices;
             } else {
                 return nullptr;
             }
         }});

    addAiMeshBufferInfo<aiVector3D>(
        {StandardVertexBufferNames::NORMAL, [](const aiMesh &extMesh) -> const aiVector3D * {
             if (extMesh.HasNormals()) {
                 return extMesh.mNormals;
             } else {
                 return nullptr;
             }
         }});

    addAiMeshBufferInfo<aiVector3D>(
        {StandardVertexBufferNames::TEXTURE_COORD, [](const aiMesh &extMesh) -> const aiVector3D * {
             if (extMesh.HasTextureCoords(0)) {
                 return extMesh.mTextureCoords[0];
             } else {
                 return nullptr;
             }
         }});

    addAiMeshBufferInfo<aiColor4D>(
        {StandardVertexBufferNames::COLOR, [](const aiMesh &extMesh) -> const aiColor4D * {
             if (extMesh.HasVertexColors(0)) {
                 return extMesh.mColors[0];
             } else {
                 return nullptr;
             }
         }});

    /*
    Material properties
    */

    addAiMaterialPropertyInfo({StandardMaterialPropertyNames::COL_DIFFUSE,
                               [](const aiMaterial &extMat) -> std::optional<Variant> {
                                   aiColor4D color;
                                   if (extMat.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                                       return Variant(
                                           glm::vec4(color.r, color.g, color.b, color.a));
                                   } else {
                                       return Variant(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                                   }
                               }});
    addAiMaterialPropertyInfo({StandardMaterialPropertyNames::COL_SPECULAR,
                               [](const aiMaterial &extMat) -> std::optional<Variant> {
                                   aiColor4D color;
                                   if (extMat.Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
                                       return Variant(
                                           glm::vec4(color.r, color.g, color.b, color.a));
                                   } else {
                                       return Variant(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                                   }
                               }});
    addAiMaterialPropertyInfo({StandardMaterialPropertyNames::COL_EMISSIVE,
                               [](const aiMaterial &extMat) -> std::optional<Variant> {
                                   aiColor4D color;
                                   if (extMat.Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) {
                                       return Variant(
                                           glm::vec4(color.r, color.g, color.b, color.a));
                                   } else {
                                       return Variant(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                                   }
                               }});
    addAiMaterialPropertyInfo({StandardMaterialPropertyNames::SHININESS,
                               [](const aiMaterial &extMat) -> std::optional<Variant> {
                                   float shininess;
                                   if (extMat.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                                       return Variant(shininess);
                                   } else {
                                       return Variant(1.0);
                                   }
                               }});
}

ComponentRoot::~ComponentRoot()
{
    /*
    Clean memory before calling destructors.
    This is important because order of pool destructions isn't specified,
    and dangling cached seeds with references to other pool'd types can (and will)
    cause serious memory errors
    */

    cleanMemoryPools(std::numeric_limits<std::size_t>::max());
}

std::size_t ComponentRoot::cleanMemoryPools(std::size_t bytenum)
{
    /*
    We free in the loop because freeing resources from one pool can
    remove strong pointers to another kind of resource
    */

    std::size_t totalFreed = 0;
    std::size_t currentFreed;

    do {
        currentFreed = 0;
        std::size_t bytenumPerPool = (bytenum - totalFreed) / m_memoryPools.size();
        for (auto &p_pool : m_memoryPools) {
            currentFreed += p_pool->clean(bytenumPerPool);
        }

        totalFreed += currentFreed;
    } while (currentFreed > 0 && totalFreed < bytenum);

    return totalFreed;
}

void ComponentRoot::addAiMaterialShadingInfo(aiShadingMode aiMode, AiMaterialShadingInfo newInfo)
{
    mAiMaterialShadingInfo[aiMode] = newInfo;
}

const ComponentRoot::AiMaterialShadingInfo &ComponentRoot::getAiMaterialShadingInfo(
    aiShadingMode aiMode) const
{
    return mAiMaterialShadingInfo.at(aiMode);
}

void ComponentRoot::addAiMaterialTextureInfo(AiMaterialTextureInfo newInfo)
{
    mAiMaterialTextureInfos.push_back(newInfo);
}

std::span<const ComponentRoot::AiMaterialTextureInfo> ComponentRoot::getAiMaterialTextureInfos()
    const
{
    return mAiMaterialTextureInfos;
}

} // namespace Vitrae
