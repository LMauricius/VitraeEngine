#pragma once

#include "Vitrae/Util/PropertyAliases.hpp"
#include "Vitrae/Util/PropertyList.hpp"
#include "Vitrae/Util/PropertySpec.hpp"
#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "Vitrae/Util/Variant.hpp"

#include "dynasma/pointer.hpp"
#include "dynasma/util/dynamic_typing.hpp"

#include <map>
#include <set>
#include <span>

namespace Vitrae
{

class Task : public dynasma::PolymorphicBase
{
  public:
    virtual ~Task() = default;

    virtual std::size_t memory_cost() const = 0;

    /**
     * @returns A specification of input properties of this task. Those properties are never
     * modified by the task and can be inputs for following tasks
     */
    virtual const PropertyList &getInputSpecs(const PropertyAliases &propMapping) const = 0;

    /**
     * @returns A specification of consuming properties of this task. These properties might be
     * modified by the task and don't exist after the task execution
     */
    virtual const PropertyList &getConsumingSpecs(const PropertyAliases &propMapping) const = 0;

    /**
     * @returns A specification of output properties of this task. Those usually don't exist before
     * running the task (although there can be collisions) and are set by the task
     */
    virtual const PropertyList &getOutputSpecs() const = 0;

    /**
     * @returns A specification of filter properties of this task. Those properties exist both
     * before and after running the task and are usually modified by it.
     */
    virtual const PropertyList &getFilterSpecs(const PropertyAliases &propMapping) const = 0;

    /**
     * Adds all types used by this task (its properties and internal execution) to the typeSet
     * @param typeSet The output set to add types to
     */
    virtual void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                                  const PropertyAliases &propMapping) const = 0;

    /**
     * Adds all subtasks used by this task to the taskSet
     */
    virtual void extractSubTasks(std::set<const Task *> &taskSet,
                                 const PropertyAliases &propMapping) const = 0;

    /**
     * @returns The user-friendly name of the task
     */
    virtual StringView getFriendlyName() const = 0;
};

template <class T>
concept TaskChild = std::is_base_of_v<Task, T>;

} // namespace Vitrae