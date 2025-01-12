#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"

namespace Vitrae
{

const ParamSpec ComposeClearRender::FRAME_STORE_SPEC = {
    .name = "fs_target",
    .typeInfo = TYPE_INFO<dynasma::FirmPtr<FrameStore>>,
};

const ParamList ComposeClearRender::FILTER_SPECS = {FRAME_STORE_SPEC};
}