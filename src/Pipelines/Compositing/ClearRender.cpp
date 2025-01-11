#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"

namespace Vitrae
{

const ParamSpec ComposeClearRender::FRAME_STORE_SPEC = {
    .name = "fs_target",
    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),
};

const ParamList ComposeClearRender::FILTER_SPECS = {FRAME_STORE_SPEC};
}