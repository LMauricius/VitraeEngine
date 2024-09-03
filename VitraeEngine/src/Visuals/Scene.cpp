#include "Vitrae/Visuals/Scene.hpp"

#include "Vitrae/ComponentRoot.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Vitrae
{
Scene::Scene(const AssimpLoadParams &params)
{
    loadFromAssimp(params);
}

Scene::Scene(const FileLoadParams &params)
{
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        params.filepath.c_str(), aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                                     aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
                                     aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);

    if (!scene) {
        params.root.getErrStream() << importer.GetErrorString();
        return;
    }

    loadFromAssimp({.root = params.root, .p_extScene = scene, .sceneFilepath = params.filepath});

    importer.FreeScene();
}

std::size_t Scene::memory_cost() const
{
    return sizeof(Scene);
}

void Scene::loadFromAssimp(const AssimpLoadParams &params)
{
    MeshKeeper &meshKeeper = params.root.getComponent<MeshKeeper>();
    MaterialKeeper &matKeeper = params.root.getComponent<MaterialKeeper>();

    // load materials
    std::vector<dynasma::LazyPtr<Material>> matById;
    if (params.p_extScene->HasMaterials()) {
        matById.reserve(params.p_extScene->mNumMaterials);
        for (int i = 0; i < params.p_extScene->mNumMaterials; i++) {
            auto p_mat = matKeeper.new_asset({
                Material::AssimpLoadParams{params.root, params.p_extScene->mMaterials[i],
                                           params.sceneFilepath},
            });
            matById.emplace_back(p_mat);
        }
    }

    // load meshes
    if (params.p_extScene->HasMeshes()) {
        for (unsigned int i = 0; i < params.p_extScene->mNumMeshes; ++i) {
            dynasma::FirmPtr<Mesh> p_mesh = meshKeeper.new_asset({Mesh::AssimpLoadParams{
                params.root, params.p_extScene->mMeshes[i], params.sceneFilepath}});

            // set material
            p_mesh->setMaterial(matById[params.p_extScene->mMeshes[i]->mMaterialIndex]);

            meshProps.emplace_back(MeshProp{p_mesh, SimpleTransformation{.position = {0, 0, 0},
                                                                         .rotation = glm::quat(),
                                                                         .scaling = {1, 1, 1}}});
        }
    }

    // load the camera
    if (params.p_extScene->HasCameras()) {
        const aiCamera *p_ext_camera = params.p_extScene->mCameras[0];

        camera.position = glm::vec3(p_ext_camera->mPosition.x, p_ext_camera->mPosition.y,
                                    p_ext_camera->mPosition.z);
        camera.rotation = glm::lookAt(camera.position,
                                      camera.position + glm::vec3(p_ext_camera->mLookAt.x,
                                                                  p_ext_camera->mLookAt.y,
                                                                  p_ext_camera->mLookAt.z),
                                      glm::vec3(0, 1, 0));
    }
}

glm::mat4 DirectionalLight::getViewMatrix(const Camera &cam, float roundingStep)
{
    float worldRoundingStep = shadow_distance * 2 * roundingStep;

    glm::vec3 cameraRoundedPos;
    if (roundingStep == 0.0) {
        cameraRoundedPos = cam.position;
    } else {
        glm::mat3 shadowAffineMat =
            glm::mat3(glm::lookAt(glm::vec3(0, 0, 0), direction, glm::vec3(0, 1, 0)));

        glm::vec3 cameraShadowPos = shadowAffineMat * cam.position;
        cameraRoundedPos = glm::inverse(shadowAffineMat) *
                           (glm::round(cameraShadowPos / worldRoundingStep) * worldRoundingStep);
    }

    return glm::lookAt(cameraRoundedPos, cameraRoundedPos + direction, glm::vec3(0, 1, 0));
}

glm::mat4 DirectionalLight::getProjectionMatrix()
{
    return glm::ortho(-shadow_distance, shadow_distance, -shadow_distance, shadow_distance,
                      -shadow_above, shadow_below);
}

} // namespace Vitrae