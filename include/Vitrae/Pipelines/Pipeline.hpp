#pragma once

#include "Vitrae/Pipelines/Method.hpp"

#include <stdexcept>

namespace Vitrae
{

class PipelineSetupException : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

template <TaskChild BasicTask> class Pipeline
{
  public:
    Pipeline() = default;
    Pipeline(Pipeline &&) = default;
    Pipeline(const Pipeline &) = default;

    Pipeline &operator=(Pipeline &&) = default;
    Pipeline &operator=(const Pipeline &) = default;

    /**
     * Constructs a pipeline using the preffered method to get desired results
     * @param method The preffered method
     * @param desiredOutputNameIds The desired outputs
     */
    Pipeline(dynasma::FirmPtr<Method<BasicTask>> p_method, const PropertyList &desiredOutputSpecs,
             const PropertySelection &selection)
    {
        // solve dependencies
        std::set<StringId> visitedOutputs;
        for (auto &outputSpec : desiredOutputSpecs.getSpecList()) {
            tryAddDependency(outputSpec, *p_method, visitedOutputs, selection);
        }

        // Process tasks' properties and add them to the pipeline
        setupPropertiesFromTasks({{}}, desiredOutputSpecs, selection);
    }

    /**
     * Constructs a partial pipeline using the preffered method to get desired
     * results. The pipeline is partial because only tasks that directly or
     * indirectly depend on parametric inputs are included
     * @note Usually used as a sub-pipeline, so that unused tasks go to the
     * parent
     * @param method The preffered method
     * @param parametricInputSpecs The inputs that the tasks in this pipeline
     * depend on
     * @param desiredOutputNameIds The desired outputs
     */
    Pipeline(dynasma::FirmPtr<Method<BasicTask>> p_method,
             std::span<const PropertySpec> parametricInputSpecs,
             std::span<const PropertySpec> desiredOutputSpecs, const PropertySelection &selection)
    {
        // solve dependencies
        std::set<StringId> visitedOutputs;
        for (auto &paramSpec : parametricInputSpecs) {
            visitedOutputs.insert(paramSpec.name);
        }

        for (auto &outputSpec : desiredOutputSpecs) {
            tryAddDependencyIfParametrized(outputSpec, *p_method, visitedOutputs, selection);
        }

        // Process tasks' properties and add them to the pipeline
        setupPropertiesFromTasks(parametricInputSpecs, desiredOutputSpecs, selection);
    }

    /**
     * The list of tasks in the pipeline
     */
    std::vector<dynasma::FirmPtr<BasicTask>> items;

    /**
     * Properties that have to be set before running the pipeline and are not modified
     */
    PropertyList inputSpecs;

    /**
     * Properties that didn't exist beforehand but are set by running the pipeline, and were
     * requested as desired outputs
     */
    PropertyList outputSpecs;

    /**
     * Properties that have to be set before running the pipeline and don't exist afterwards
     */
    PropertyList consumingSpecs;

    /**
     * Properties that have to be set before running the pipeline and exist afterwards, but might be
     * modified by running the pipeline
     */
    PropertyList filterSpecs;

    /**
     * Properties that are just passed from inputs to outputs, unprocessed
     */
    PropertyList pipethroughSpecs;

    /**
     * Properties used by the pipeline internally.
     * They don't exist beforehand but are set by running the pipeline, and weren't requested as
     * desired outputs
     */
    PropertyList localSpecs;

  protected:
    /**
     * Adds the desiredOutputSpec name to the visitedOutputs set.
     * Adds a task outputting the desired property if possible and all its dependency properties.
     * If a task is added, all its outputs are added to the visitedOutputs set
     * @param desiredOutputSpec The dependency property
     * @param method The method we use to get the task
     * @param visitedOutputs The set of visited outputs
     * @param selection The property mapping
     */
    void tryAddDependency(const PropertySpec &desiredOutputSpec, const Method<BasicTask> &method,
                          std::set<StringId> &visitedOutputs, const PropertySelection &selection)
    {
        if (visitedOutputs.find(desiredOutputSpec.name) == visitedOutputs.end()) {
            std::optional<dynasma::FirmPtr<BasicTask>> maybeTask =
                method.getTask(desiredOutputSpec.name);

            if (maybeTask.has_value()) {
                const Task &task = *maybeTask.value();

                // task outputs (store the outputs as visited)
                for (auto [nameId, spec] : task.getOutputSpecs().getMappedSpecs()) {
                    visitedOutputs.insert(nameId);
                }

                // task deps (input + filter + consuming)
                for (auto [nameId, spec] : task.getInputSpecs(selection).getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection);
                }
                for (auto [nameId, spec] : task.getFilterSpecs().getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection);
                }
                for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection);
                }

                // consume specs by removing them from the visited list
                for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                    visitedOutputs.erase(nameId);
                }

                items.push_back(maybeTask.value());
            } else {
                visitedOutputs.insert(desiredOutputSpec.name);
            }
        }
    };

    /**
     * Adds a task outputting the desired property if possible and all its dependency properties,
     * but only if it directly or indirectly depends on one of already visited outputs.
     * If a task is added, all its outputs are added to the visitedOutputs set
     * @param desiredOutputSpec The dependency property
     * @param method The method we use to get the task
     * @param visitedOutputs The set of visited outputs
     * @param selection The property mapping
     * @returns Whether the dependency is satisfied
     */
    bool tryAddDependencyIfParametrized(const PropertySpec &desiredOutputSpec,
                                        const Method<BasicTask> &method,
                                        std::set<StringId> &visitedOutputs,
                                        const PropertySelection &selection)
    {
        if (visitedOutputs.find(desiredOutputSpec.name) != visitedOutputs.end()) {
            return true;
        }

        std::optional<dynasma::FirmPtr<BasicTask>> maybeTask =
            method.getTask(desiredOutputSpec.name);

        if (maybeTask.has_value()) {
            const Task &task = *maybeTask.value();

            bool satisfiedAnyDependencies = false;

            // task deps (input + filter + consuming)
            for (auto [nameId, spec] : task.getInputSpecs(selection).getMappedSpecs()) {
                satisfiedAnyDependencies |=
                    tryAddDependencyIfParametrized(spec, method, visitedOutputs, selection);
            }
            for (auto [nameId, spec] : task.getFilterSpecs().getMappedSpecs()) {
                satisfiedAnyDependencies |=
                    tryAddDependencyIfParametrized(spec, method, visitedOutputs, selection);
            }
            for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                satisfiedAnyDependencies |=
                    tryAddDependencyIfParametrized(spec, method, visitedOutputs, selection);
            }

            if (satisfiedAnyDependencies) {
                // task outputs (store the outputs as visited)
                for (auto [nameId, spec] : task.getOutputSpecs().getMappedSpecs()) {
                    visitedOutputs.insert(nameId);
                }

                // consume specs by removing them from the visited list
                for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                    visitedOutputs.erase(nameId);
                }

                items.push_back(maybeTask.value());

                return true;
            }
        }

        return false;
    }

    /**
     * Adds all used properties in the pipeline's tasks to the correct PropertyLists.
     * (i.e this->inputSpecs, this->outputSpecs, this->filterSpecs, this->consumedSpecs,
     * this->pipethroughSpecs, this->localSpecs)
     * @param fixedInputSpecs The known input specs
     * @param desiredOutputSpecs The desired output specs
     * @param selection The property mapping
     */
    void setupPropertiesFromTasks(const PropertyList &fixedInputSpecs,
                                  const PropertyList &desiredOutputSpecs,
                                  const PropertySelection &selection)
    {
        // 4 maps/sets needed to know how we use properties
        std::set<StringId> missingPropertyNames;
        std::map<StringId, PropertySpec> everUsedProperties;
        std::set<StringId> currentPropertyNames;
        std::set<StringId> modifiedPropertyNames;

        // helper functions for controlling the maps/sets

        auto requireProperty = [&](const PropertySpec &propertySpec) {
            if (currentPropertyNames.find(propertySpec.name) == currentPropertyNames.end()) {
                if (everUsedProperties.find(propertySpec.name) == everUsedProperties.end()) {
                    missingPropertyNames.insert(propertySpec.name);
                    currentPropertyNames.insert(propertySpec.name);
                } else {
                    throw PipelineSetupException(
                        std::string("Property ") + propertySpec.name +
                        "' was consumed, but also depended on later in the pipeline");
                }
            }
        };

        auto usingProperty = [&](const PropertySpec &propertySpec, const String &byWho) {
            auto it = everUsedProperties.find(propertySpec.name);
            if (it != everUsedProperties.end()) {
                if ((*it).second.typeInfo != propertySpec.typeInfo) {
                    throw PipelineSetupException(
                        String("Property '") + propertySpec.name + "' was first used as " +
                        String((*it).second.typeInfo.getShortTypeName()) + " but late as " +
                        String(propertySpec.typeInfo.getShortTypeName()) + " by " + byWho);
                }
            } else {
                everUsedProperties.emplace(propertySpec.name, propertySpec);
            }
        };

        auto setProperty = [&](const PropertySpec &propertySpec) {
            modifiedPropertyNames.insert(propertySpec.name);
            currentPropertyNames.insert(propertySpec.name);
        };

        auto consumeProperty = [&](const PropertySpec &propertySpec) {
            currentPropertyNames.erase(propertySpec.name);
        };

        // use fixed input properties before the tasks need them
        for (auto &spec : fixedInputSpecs.getSpecList()) {
            requireProperty(spec);
            usingProperty(spec, "pipeline parameters");
        }

        // iterate over the tasks and simulate property usage
        for (auto &p_item : items) {
            const Task &task = *p_item;
            String taskContextName = "task '" + String(task.getFriendlyName()) + "'";

            for (auto &spec : task.getInputSpecs(selection).getSpecList()) {
                requireProperty(spec);
                usingProperty(spec, taskContextName);
            }

            for (auto &spec : task.getConsumingSpecs(selection).getSpecList()) {
                requireProperty(spec);
                usingProperty(spec, taskContextName);
                consumeProperty(spec);
            }

            for (auto &spec : task.getOutputSpecs().getSpecList()) {
                usingProperty(spec, taskContextName);
                setProperty(spec);
            }

            for (auto &spec : task.getFilterSpecs().getSpecList()) {
                requireProperty(spec);
                usingProperty(spec, taskContextName);
                setProperty(spec);
            }
        }

        // also use the desired outputs, even if not used by the tasks
        for (auto &spec : desiredOutputSpecs.getSpecList()) {
            requireProperty(spec);
            usingProperty(spec, "pipeline outputs");
        }

        // analyze the sets and add property specs to proper lists
        for (auto &[nameId, spec] : everUsedProperties) {
            if (missingPropertyNames.find(nameId) != missingPropertyNames.end()) {
                // property  existed beforehand
                if (currentPropertyNames.find(nameId) == currentPropertyNames.end()) {
                    // property was consumed at some point
                    consumingSpecs.insert_back(spec);
                } else if (modifiedPropertyNames.find(nameId) != modifiedPropertyNames.end()) {
                    // property was modified and still exists
                    filterSpecs.insert_back(spec);
                } else if (desiredOutputSpecs.getMappedSpecs().find(nameId) !=
                           desiredOutputSpecs.getMappedSpecs().end()) {
                    // property was desired and but only set externally
                    pipethroughSpecs.insert_back(spec);
                } else {
                    // property was externally set and has the same value afterwards
                    inputSpecs.insert_back(spec);
                }
            } else {
                // property was introduced by the pipeline
                if (desiredOutputSpecs.getMappedSpecs().find(nameId) !=
                    desiredOutputSpecs.getMappedSpecs().end()) {
                    // property was desired and is set by the pipeline
                    outputSpecs.insert_back(spec);
                } else {
                    // property was set by the pipeline, but not desired
                    localSpecs.insert_back(spec);
                }
            }
        }
    }
};
} // namespace Vitrae