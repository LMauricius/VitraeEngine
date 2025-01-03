#include "Vitrae/Pipelines/Compositing/ClearRender.hpp"

namespace Vitrae
{

const PropertySpec ComposeClearRender::FRAME_STORE_SPEC = {
    .name = "frame_store",
    .typeInfo = Variant::getTypeInfo<dynasma::FirmPtr<FrameStore>>(),
};

const PropertyList ComposeClearRender::FILTER_SPECS = {FRAME_STORE_SPEC};
}