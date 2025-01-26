#include "Vitrae/Pipelines/Compositing/InitFunction.hpp"

#include "MMeter.h"

namespace Vitrae
{

ComposeInitFunction::ComposeInitFunction(const SetupParams &params)
    : mp_function(params.p_function), m_friendlyName(params.friendlyName),
      m_inputSpecs(params.inputSpecs), m_outputSpecs(params.outputSpecs)
{}

std::size_t ComposeInitFunction::memory_cost() const
{
    return sizeof(ComposeInitFunction);
}

const ParamList &ComposeInitFunction::getInputSpecs(const ParamAliases &) const
{
    return m_inputSpecs;
}

const ParamList &ComposeInitFunction::getOutputSpecs() const
{
    return m_outputSpecs;
}

const ParamList &ComposeInitFunction::getFilterSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

const ParamList &ComposeInitFunction::getConsumingSpecs(const ParamAliases &) const
{
    return EMPTY_PROPERTY_LIST;
}

void ComposeInitFunction::extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                           const ParamAliases &aliases) const
{
    for (const ParamList *p_specs :
         {&m_inputSpecs, &m_outputSpecs, &m_filterSpecs, &m_consumingSpecs}) {
        for (const ParamSpec &spec : p_specs->getSpecList()) {
            typeSet.insert(&spec.typeInfo);
        }
    }
}

void ComposeInitFunction::extractSubTasks(std::set<const Task *> &taskSet,
                                          const ParamAliases &aliases) const
{
    taskSet.insert(this);
}

void ComposeInitFunction::run(RenderComposeContext args) const {}

void ComposeInitFunction::prepareRequiredLocalAssets(RenderComposeContext args) const
{
    MMETER_SCOPE_PROFILER("ComposeInitFunction::prepareRequiredLocalAssets");

    mp_function(args);
}

StringView ComposeInitFunction::getFriendlyName() const
{
    return m_friendlyName;
}

} // namespace Vitrae