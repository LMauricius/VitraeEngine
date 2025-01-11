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

const ParamList &ComposeFunction::getInputSpecs(const ParamAliases &) const
{
    return m_inputSpecs;
}

const ParamList &ComposeFunction::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &ComposeFunction::getFilterSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &ComposeFunction::getConsumingSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeFunction::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                       const ParamAliases &aliases) const
{
    for (const ParamList *p_specs :
         {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs, &m_consumingSpecs}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeFunction::extractSubTasks(std::set<const Task *> &taskSet,
                                      const ParamAliases &aliases) const
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