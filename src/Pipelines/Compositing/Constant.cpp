#include "Vitrae/Pipelines/Compositing/Constant.hpp"

#include "MMeter.h"

namespace Vitrae
{
ComposeConstant::ComposeConstant(const SetupParams &params)
    : m_outputSpecs{params.outputSpec}, m_value(params.value),
      m_friendlyName(String("Const ") + params.value.toString())
{}

const PropertyList &ComposeConstant::getInputSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &ComposeConstant::getOutputSpecs(const PropertyAliases &) const
{
    return m_outputSpecs;
}

const PropertyList &ComposeConstant::getFilterSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &ComposeConstant::getConsumingSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeConstant::run(RenderComposeContext ctx) const
{
    MMETER_SCOPE_PROFILER("ComposeConstant::run");

    // we have only 1 output spec
    StringId outputNameId = m_outputSpecs.getSpecNameIds().front();

    ctx.properties.set(outputNameId, m_value);
}

void ComposeConstant::prepareRequiredLocalAssets(RenderComposeContext ctx) const {}

StringView ComposeConstant::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae