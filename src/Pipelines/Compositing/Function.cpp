#include "Vitrae/Pipelines/Compositing/Function.hpp"

#include "MMeter.h"

namespace Vitrae
{

ComposeFunction::ComposeFunction(const SetupParams &params)
    : mp_function(params.p_function), m_friendlyName(params.friendlyName),
      m_inputSpecs(params.inputSpecs), m_outputSpecs(params.outputSpecs)
{}

const PropertyList &ComposeFunction::getInputSpecs(const PropertyAliases &) const
{
    return m_inputSpecs;
}

const PropertyList &ComposeFunction::getOutputSpecs(const PropertyAliases &) const
{
    return m_outputSpecs;
}

const PropertyList &ComposeFunction::getFilterSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const PropertyList &ComposeFunction::getConsumingSpecs(const PropertyAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeFunction::run(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeFunction::run");

    mp_function(args);
}

void ComposeFunction::prepareRequiredLocalAssets(RenderComposeContext args) const {}

StringView ComposeFunction::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae