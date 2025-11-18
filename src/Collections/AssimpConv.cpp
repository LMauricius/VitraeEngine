#include "Vitrae/Collections/AssimpConv.hpp"

#include "Vitrae/Params/Standard.hpp"

#include "assimp/material.h"
#include "assimp/mesh.h"

namespace Vitrae
{

AssimpConvCollection::AssimpConvCollection()
{
    addMeshBufferConv<aiVector3D>({
        StandardParam::position.name,
        [](const aiMesh &extMesh) -> const aiVector3D * {
            if (extMesh.HasPositions()) {
                return extMesh.mVertices;
            } else {
                return nullptr;
            }
        },
    });

    addMeshBufferConv<aiVector3D>({
        StandardParam::normal.name,
        [](const aiMesh &extMesh) -> const aiVector3D * {
            if (extMesh.HasNormals()) {
                return extMesh.mNormals;
            } else {
                return nullptr;
            }
        },
    });

    addMeshBufferConv<aiVector3D>({
        StandardParam::coord_base.name,
        [](const aiMesh &extMesh) -> const aiVector3D * {
            if (extMesh.HasTextureCoords(0)) {
                return extMesh.mTextureCoords[0];
            } else {
                return nullptr;
            }
        },
    });

    addMaterialPropertyConv({
        .nameId = StandardParam::is_transparent.name,
        .extractor = [](const aiMaterial &extMat) -> std::optional<Variant> {
            int flags;
            if (extMat.Get(AI_MATKEY_TEXFLAGS(aiTextureType_DIFFUSE, 0), flags) ==
                aiReturn_SUCCESS) {
                return Variant{
                    bool{flags & aiTextureFlags_UseAlpha && !(flags & aiTextureFlags_IgnoreAlpha)}};
            }
            return std::nullopt;
        },
    });
}

void AssimpConvCollection::addMaterialPropertyConv(const MaterialPropertyConv &newInfo)
{
    this->m_aiMaterialPropertyConvs.push_back(newInfo);
}

std::span<const AssimpConvCollection::MaterialPropertyConv> AssimpConvCollection::
    getMaterialPropertyConvs() const
{
    return std::span(m_aiMaterialPropertyConvs);
}

void AssimpConvCollection::addShadingModeConv(aiShadingMode aiMode, const ParamAliases &aliases)
{
    m_aiShadingModeAliases[aiMode] = aliases;
}

const ParamAliases &AssimpConvCollection::getShadingModeParamAliases(aiShadingMode aiMode) const
{
    return m_aiShadingModeAliases.at(aiMode);
}

} // namespace Vitrae