#include "assetCollection.hpp"

#include "Methods/classic.hpp"
#include "Methods/lightSpaceStable.hpp"
#include "Methods/shadowBiLin.hpp"
#include "Methods/shadowEBR.hpp"
#include "Methods/shadowPCF.hpp"
#include "Methods/shadowRough.hpp"
#include "Methods/smResolution.hpp"

#include "Vitrae/Assets/FrameStore.hpp"
#include "Vitrae/ComponentRoot.hpp"
#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"
#include "Vitrae/Pipelines/Compositing/Function.hpp"
#include "Vitrae/Pipelines/Compositing/SceneRender.hpp"
#include "Vitrae/Pipelines/Shading/Constant.hpp"
#include "Vitrae/Pipelines/Shading/Function.hpp"
#include "Vitrae/Renderers/OpenGL.hpp"
#include "Vitrae/Renderers/OpenGL/FrameStore.hpp"
#include "Vitrae/Visuals/Compositor.hpp"
#include "Vitrae/Visuals/Scene.hpp"

#include "dynasma/keepers/naive.hpp"
#include "dynasma/standalone.hpp"

#include "glm/gtx/vector_angle.hpp"

AssetCollection::AssetCollection(ComponentRoot &root, Renderer &rend,
                                 std::filesystem::path scenePath, float sceneScale)
    : root(root), rend(rend), modeSetter(root), running(true), shouldReloadPipelines(true),
      comp(root)
{
    /*
    Shading setup
    */
    modeSetter.setModes(root);
    methodCategories = {
        {"Base shading", {std::make_shared<MethodsClassic>(root)}, 0},
        {"Light/shadow space", {std::make_shared<MethodsLSStable>(root)}, 0},
        {"Shadow filtering",
         {
             std::make_shared<MethodsShadowRough>(root),
             std::make_shared<MethodsShadowBiLin>(root),
             std::make_shared<MethodsShadowPCF>(root),
             std::make_shared<MethodsShadowEBR>(root),
         },
         2},
        {"Shadow map resolution",
         {
             std::make_shared<MethodsShadowMapResolution>(root, 256),
             std::make_shared<MethodsShadowMapResolution>(root, 512),
             std::make_shared<MethodsShadowMapResolution>(root, 1024),
             std::make_shared<MethodsShadowMapResolution>(root, 2048),
             std::make_shared<MethodsShadowMapResolution>(root, 4096),
             std::make_shared<MethodsShadowMapResolution>(root, 8192),
             std::make_shared<MethodsShadowMapResolution>(root, 16384),
         },
         3},
    };
    reapplyChoosenMethods();

    /*
    Setup window
    */
    running = true;

    p_windowFrame =
        root.getComponent<FrameStoreManager>()
            .register_asset(FrameStoreSeed{FrameStore::WindowDisplayParams{
                .root = root,
                .width = 800,
                .height = 600,
                .title = "Vitrae Showcase",
                .isFullscreen = false,
                .onClose = [&]() { running = false; },
                .onDrag =
                    [&](glm::vec2 motion, bool bLeft, bool bRight, bool bMiddle) {
                        // Camera rotation
                        if (bRight) {
                            glm::vec3 dirVec =
                                p_scene->camera.rotation * glm::vec3{0.0f, 0.0f, 1.0f};
                            float yaw = glm::orientedAngle(
                                glm::normalize(glm::vec2{dirVec.x, dirVec.z}), {0.0, 1.0});
                            float pitch = glm::orientedAngle(
                                glm::normalize(
                                    glm::vec2{glm::sqrt(dirVec.x * dirVec.x + dirVec.z * dirVec.z),
                                              dirVec.y}),
                                {1.0, 0.0});
                            yaw += 0.15f * glm::radians(motion.x);
                            pitch += 0.15f * glm::radians(motion.y);
                            p_scene->camera.rotation = glm::quat(glm::vec3(pitch, yaw, 0.0f));
                        }

                        // Camera movement
                        if (bLeft) {
                            p_scene->camera.move(p_scene->camera.rotation *
                                                 (0.02f * glm::vec3{-motion.x, 0.0, -motion.y}));
                        }

                        // Scene scaling (in case of wrong scale)
                        if (bMiddle) {
                            p_scene->camera.move(p_scene->camera.rotation *
                                                 (0.02f * glm::vec3{-motion.x, motion.y, 0.0}));
                        }
                    }}})
            .getLoaded();

    /*
    Setup assets
    */

    p_scene =
        dynasma::makeStandalone<Scene>(Scene::FileLoadParams{.root = root, .filepath = scenePath});
    p_scene->camera.position = glm::vec3(-15.0, 10.0, 1.3);
    p_scene->camera.scaling = glm::vec3(1, 1, 1);
    p_scene->camera.zNear = 0.05f;
    p_scene->camera.zFar = 1000.0f;
    p_scene->camera.rotation = glm::quatLookAt(glm::vec3(0.8, -0.5, 0), glm::vec3(0, 1, 0));
    for (auto &prop : p_scene->meshProps) {
        prop.transform.position = prop.transform.position * sceneScale;
        prop.transform.scale(glm::vec3(sceneScale));
    }

    /*
    Compositor
    */
    comp.parameters.set("scene", p_scene);
    comp.setOutput(p_windowFrame);
}

AssetCollection::~AssetCollection() {}

void AssetCollection::reapplyChoosenMethods()
{
    std::vector<dynasma::FirmPtr<Method<ShaderTask>>> choosenVertexMethods;
    std::vector<dynasma::FirmPtr<Method<ShaderTask>>> choosenFragMethods;
    std::vector<dynasma::FirmPtr<Method<ComposeTask>>> choosenComposeMethods;
    Vitrae::String vertName = "";
    Vitrae::String fragName = "";
    Vitrae::String compName = "";

    for (auto &category : methodCategories) {
        choosenVertexMethods.push_back(category.methods[category.selectedIndex]->p_vertexMethod);
        choosenFragMethods.push_back(category.methods[category.selectedIndex]->p_fragmentMethod);
        choosenComposeMethods.push_back(category.methods[category.selectedIndex]->p_composeMethod);

        if (category.methods[category.selectedIndex]->p_vertexMethod->getFriendlyName() != "") {
            vertName += category.methods[category.selectedIndex]->p_vertexMethod->getFriendlyName();
            vertName += "_";
        }

        if (category.methods[category.selectedIndex]->p_fragmentMethod->getFriendlyName() != "") {
            fragName +=
                category.methods[category.selectedIndex]->p_fragmentMethod->getFriendlyName();
            fragName += "_";
        }

        if (category.methods[category.selectedIndex]->p_composeMethod->getFriendlyName() != "") {
            compName +=
                category.methods[category.selectedIndex]->p_composeMethod->getFriendlyName();
            compName += "_";
        }
    }

    auto p_aggregateVertexMethod =
        dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
            .fallbackMethods = choosenVertexMethods, .friendlyName = vertName});

    auto p_aggregateFragmentMethod =
        dynasma::makeStandalone<Method<ShaderTask>>(Method<ShaderTask>::MethodParams{
            .fallbackMethods = choosenFragMethods, .friendlyName = fragName});

    auto p_aggregateComposeMethod =
        dynasma::makeStandalone<Method<ComposeTask>>(Method<ComposeTask>::MethodParams{
            .fallbackMethods = choosenComposeMethods, .friendlyName = compName});

    comp.setDefaultShadingMethod(p_aggregateVertexMethod, p_aggregateFragmentMethod);
    comp.setComposeMethod(p_aggregateComposeMethod);
}

void AssetCollection::render()
{
    if (shouldReloadPipelines) {
        shouldReloadPipelines = false;
        reapplyChoosenMethods();
    }
    comp.compose();
}
