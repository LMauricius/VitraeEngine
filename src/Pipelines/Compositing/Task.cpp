#include "Vitrae/Pipelines/Compositing/Task.hpp"

namespace Vitrae
{

class Scene;

const ParamSpec ComposeTask::FRAME_STORE_TARGET_SPEC = {
    .name = "fs_target",
    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),
};

const ParamSpec ComposeTask::SCENE_SPEC = {
    .name = "scene",
    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<Scene>>(),
};
} // namespace Vitrae