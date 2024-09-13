#include "Vitrae/Assets/Model.hpp"
#include "Vitrae/Assets/Material.hpp"
#include "Vitrae/Assets/Mesh.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/TypeConversion/StringConvert.hpp"

#include "dynasma/keepers/abstract.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Vitrae
{
Model::Model(const FileLoadParams &params)
{
    const std::filesystem::path &filepath = params.filepath;
    ComponentRoot &root = params.root;

    // prepare needed managers
    MeshKeeper &meshKeeper = root.getComponent<MeshKeeper>();
    MaterialKeeper &matKeeper = root.getComponent<MaterialKeeper>();

    // load scene
    Assimp::Importer importer;

    const aiScene *extScenePtr = importer.ReadFile(
        params.filepath.c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                     aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
                                     aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);

    if (!extScenePtr) {
        params.root.getErrStream()
            << "Loading scene failed: " << importer.GetErrorString() << std::endl;
        return;
    }

    // load materials
    std::vector<dynasma::LazyPtr<Material>> matById;
    if (extScenePtr->HasMaterials()) {
        matById.reserve(extScenePtr->mNumMaterials);
        for (int i = 0; i < extScenePtr->mNumMaterials; i++) {
            auto p_mat = matKeeper.new_asset({
                Material::AssimpLoadParams{params.root, extScenePtr->mMaterials[i],
                                           params.filepath},
            });
            matById.emplace_back(p_mat);
        }
    }

    // load meshes
    if (extScenePtr->HasMeshes()) {
        for (int i = 0; i < extScenePtr->mNumMeshes; i++) {

            auto p_mesh = meshKeeper
                              .new_asset({
                                  Mesh::AssimpLoadParams{
                                      params.root,
                                      extScenePtr->mMeshes[i],
                                      filepath,
                                  },
                              })
                              .getLoaded();

            // set material
            p_mesh->setMaterial(matById[extScenePtr->mMeshes[i]->mMaterialIndex]);

            m_meshes.push_back(p_mesh);
        }
    }
}

Model::~Model() {}
} // namespace Vitrae