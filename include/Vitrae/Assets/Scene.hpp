#pragma once

#include "Vitrae/Data/Transformation.hpp"

#include "assimp/scene.h"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <filesystem>

namespace Vitrae
{
class ComponentRoot;
class Model;

class Camera : public SimpleTransformation
{
  public:
    float zNear = 1.0, zFar = 1000.0;
    float fov = 45.0;

    inline glm::mat4 getPerspectiveMatrix(float width, float height) const
    {
        return glm::perspective(glm::radians(fov), width / height, zNear, zFar);
    }
};

class DirectionalLight
{
  public:
    glm::vec3 direction = {0.3, -0.7, 0.15};
    glm::vec3 color_primary = {0.6, 0.5, 0.4}, color_ambient = {0.3, 0.35, 0.4};

    glm::mat4 getViewMatrix(const Camera &cam, float shadow_distance, float positionRounding = 0.0);
    glm::mat4 getProjectionMatrix(float shadow_distance, float shadow_above, float shadow_below);
};

struct ModelProp
{
    dynasma::FirmPtr<Model> p_model;
    SimpleTransformation transform;
};

/**
 * Represents a scene
 */
class Scene : public dynasma::PolymorphicBase
{
  public:
    struct AssimpLoadParams
    {
        ComponentRoot &root;
        const aiScene *p_extScene;
        std::filesystem::path sceneFilepath;
    };
    struct FileLoadParams
    {
        ComponentRoot &root;
        std::filesystem::path filepath;
    };

    Scene(const AssimpLoadParams &params);
    Scene(const FileLoadParams &params);

    std::size_t memory_cost() const;

    /*
    Scene parts (to be replaced with a more modular approach)
    */

    std::vector<ModelProp> modelProps;
    Camera camera;
    DirectionalLight light;

  protected:
    void loadFromAssimp(const AssimpLoadParams &params);
};

} // namespace Vitrae