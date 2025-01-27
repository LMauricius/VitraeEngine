#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Collections/FormGenerator.hpp"
#include "Vitrae/Collections/MeshGenerator.hpp"
#include "Vitrae/Collections/MethodCollection.hpp"

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
    Standard components
    */
    setComponent<MethodCollection>(new MethodCollection);
    setComponent<FormGeneratorCollection>(new FormGeneratorCollection);
    setComponent<MeshGeneratorCollection>(new MeshGeneratorCollection);
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

void ComponentRoot::addAiMaterialParamAliases(aiShadingMode aiMode, const ParamAliases &aliases)
{
    mAiMaterialAliases[aiMode] = aliases;
}

const ParamAliases &ComponentRoot::getAiMaterialParamAliases(aiShadingMode aiMode) const
{
    return mAiMaterialAliases.at(aiMode);
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
