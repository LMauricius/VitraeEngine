#include "Vitrae/Pipelines/Compositing/Task.hpp"

namespace Vitrae
{

class Scene;

const ParamSpec ComposeTask::FRAME_STORE_TARGET_SPEC = {
    .name = "fs_target",
    .typeInfo = TYPE_INFO<dynasma::FirmPtr<FrameStore>>,
};

const ParamSpec ComposeTask::SCENE_SPEC = {
    .name = "scene",
    .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>,
};
} // namespace Vitrae