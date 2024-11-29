#include "Vitrae/Pipelines/Compositing/Constant.hpp"

#include "MMeter.h"

namespace Vitrae
{
ComposeConstant::ComposeConstant(const SetupParams &params)
    : ComposeTask({}, {{params.outputSpec.name, params.outputSpec}}),
      m_outputNameId(params.outputSpec.name), m_outputSpec(params.outputSpec),
      m_value(params.value), m_friendlyName(String("Const ") + params.value.toString())
{}

void ComposeConstant::run(RenderRunContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeConstant::run");

    args.properties.set(m_outputNameId, m_value);
}

void ComposeConstant::prepareRequiredLocalAssets(
    StableMap<StringId, dynasma::FirmPtr<FrameStore>> &frameStores,
    StableMap<StringId, dynasma::FirmPtr<Texture>> &textures,
    const ScopedDict &properties, const RenderSetupContext &context) const {}

StringView ComposeConstant::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae