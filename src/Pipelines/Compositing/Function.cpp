#include "Vitrae/Pipelines/Compositing/Function.hpp"

#include "MMeter.h"

namespace Vitrae
{

ComposeFunction::ComposeFunction(const SetupParams &params)
    : mp_function(params.p_function), m_friendlyName(params.friendlyName),
      m_inputSpecs(params.inputSpecs), m_outputSpecs(params.outputSpecs)
{}

std::size_t ComposeFunction::memory_cost() const
{
    return sizeof(ComposeFunction);
}

const PropertyList &ComposeFunction::getInputSpecs(const PropertyAliases &) const
{
    return m_inputSpecs;
}

const PropertyList &ComposeFunction::getOutputSpecs() const
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

void ComposeFunction::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                       const PropertyAliases &aliases) const
{
    for (const PropertyList *p_specs :
         {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs, &m_consumingSpecs}) {
        for (const PropertySpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeFunction::extractSubTasks(std::set<const Task *> &taskSet,
                                      const PropertyAliases &aliases) const
{
    taskSet.insert(this);
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