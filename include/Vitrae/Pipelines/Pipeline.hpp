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

/**
 * @brief Describes how dependency task generation works in relation to pipeline parameters
 * @note A parametrized task is one that depends on a pipeline parameter property
 */
enum class PipelineParametrizationPolicy {
    AllDependencies,                  // Generates all output and indirect dependency tasks
    ParametrizedOrDirectDependencies, // Generates output tasks and paramatrized dependency tasks
    ParametrizedDependencies,         // Generates only paramatrized dependency tasks
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
    Pipeline(dynasma::FirmPtr<const Method<BasicTask>> p_method,
             const PropertyList &desiredOutputSpecs, const PropertyAliases &selection)
    {
        std::map<StringId, String> wipUsedSelection;

        // desired spec aliases (optimization and later setup)
        PropertyList actualDesiredOutputSpecs;
        for (auto &outputSpec : desiredOutputSpecs.getSpecList()) {
            actualDesiredOutputSpecs.insert_back({
                .name = selection.choiceStringFor(outputSpec.name),
                .typeInfo = outputSpec.typeInfo,
            });
        }

        // solve dependencies
        std::set<StringId> visitedOutputs;
        for (auto &outputSpec : actualDesiredOutputSpecs.getSpecList()) {
            tryAddDependency(outputSpec, *p_method, visitedOutputs, selection, wipUsedSelection);
        }

        // Process tasks' properties and add them to the pipeline
        setupPropertiesFromTasks(actualDesiredOutputSpecs, selection);

        // Add used selections
        usedSelection = PropertyAliases(StableMap<StringId, String>(std::move(wipUsedSelection)));
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
    Pipeline(dynasma::FirmPtr<const Method<BasicTask>> p_method,
             std::span<const StringId> parametricInputIds,
             PipelineParametrizationPolicy parametrizationPolicy,
             const PropertyList &desiredOutputSpecs, const PropertyAliases &selection)
    {
        std::map<StringId, String> wipUsedSelection;

        // desired spec aliases (optimization and later setup)
        PropertyList actualDesiredOutputSpecs;
        for (auto &outputSpec : desiredOutputSpecs.getSpecList()) {
            actualDesiredOutputSpecs.insert_back({
                .name = selection.choiceStringFor(outputSpec.name),
                .typeInfo = outputSpec.typeInfo,
            });
        }

        // solve dependencies
        std::set<StringId> visitedOutputs;
        for (auto &nameId : parametricInputIds) {
            visitedOutputs.insert(nameId);
        }

        for (auto &outputSpec : actualDesiredOutputSpecs.getSpecList()) {
            tryAddDependencyIfParametrized(outputSpec, *p_method, visitedOutputs,
                                           parametrizationPolicy, selection, wipUsedSelection);
        }

        // Process tasks' properties and add them to the pipeline
        setupPropertiesFromTasks(actualDesiredOutputSpecs, selection);

        // Add used selections
        usedSelection = PropertyAliases(wipUsedSelection);
    }

    /**
     * The list of tasks in the pipeline
     */
    std::vector<dynasma::FirmPtr<BasicTask>> items;

    /**
     * Map of relevant property selections
     * key=required property name (to choose), value=actual property name (choice)
     */
    PropertyAliases usedSelection;

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
     * @param outUsedSelection The used property mapping
     */
    void tryAddDependency(const PropertySpec &desiredOutputSpec, const Method<BasicTask> &method,
                          std::set<StringId> &visitedOutputs, const PropertyAliases &selection,
                          std::map<StringId, String> &outUsedSelection)
    {
        String actualOutputName = selection.choiceStringFor(desiredOutputSpec.name);

        if (actualOutputName != desiredOutputSpec.name) {
            outUsedSelection[desiredOutputSpec.name] = actualOutputName;
        }

        if (visitedOutputs.find(actualOutputName) == visitedOutputs.end()) {
            std::optional<dynasma::FirmPtr<BasicTask>> maybeTask = method.getTask(actualOutputName);

            if (maybeTask.has_value()) {
                const Task &task = *maybeTask.value();

                // task outputs (store the outputs as visited)
                for (auto [nameId, spec] : task.getOutputSpecs().getMappedSpecs()) {
                    visitedOutputs.insert(nameId);
                }

                // task deps (input + filter + consuming)
                for (auto [nameId, spec] : task.getInputSpecs(selection).getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection, outUsedSelection);
                }
                for (auto [nameId, spec] : task.getFilterSpecs(selection).getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection, outUsedSelection);
                }
                for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                    tryAddDependency(spec, method, visitedOutputs, selection, outUsedSelection);
                }

                // consume specs by removing them from the visited list
                for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                    visitedOutputs.erase(nameId);
                }

                items.push_back(maybeTask.value());
            } else {
                visitedOutputs.insert(actualOutputName);
            }
        }
    };

    /**
     * Adds a task outputting the desired property if possible and all its dependency properties,
     * but only if it directly or indirectly depends on one of already visited outputs.
     * If a task is added, all its outputs are added to the visitedOutputs set.
     * If policy==AllDependencies; then even non-parametrized dependencies are added;
     * If policy==ParametrizedOrDirectDependencies; then this dependency is added even if it is
     * non-parametrized, but its dependencies are added with policy=ParametrizedDependencies;
     * If policy==ParametrizedDependencies; then only parametrized dependencies are added.
     * @param desiredOutputSpec The dependency property
     * @param method The method we use to get the task
     * @param visitedOutputs The set of visited outputs
     * @param parametrizationPolicy The parametrization policy
     * @param selection The property mapping
     * @param outUsedSelection The used property mapping
     * @returns Whether the dependency is satisfied
     */
    bool tryAddDependencyIfParametrized(const PropertySpec &desiredOutputSpec,
                                        const Method<BasicTask> &method,
                                        std::set<StringId> &visitedOutputs,
                                        PipelineParametrizationPolicy parametrizationPolicy,
                                        const PropertyAliases &selection,
                                        std::map<StringId, String> &outUsedSelection)
    {
        String actualOutputName = selection.choiceStringFor(desiredOutputSpec.name);

        if (actualOutputName != desiredOutputSpec.name) {
            outUsedSelection[desiredOutputSpec.name] = actualOutputName;
        }

        if (visitedOutputs.find(actualOutputName) != visitedOutputs.end()) {
            return true;
        }

        std::optional<dynasma::FirmPtr<BasicTask>> maybeTask = method.getTask(actualOutputName);

        if (maybeTask.has_value()) {
            const Task &task = *maybeTask.value();

            bool satisfiedAnyDependencies = false;
            PipelineParametrizationPolicy indirectDepPolicy =
                parametrizationPolicy == PipelineParametrizationPolicy::AllDependencies
                    ? PipelineParametrizationPolicy::AllDependencies
                    : PipelineParametrizationPolicy::ParametrizedDependencies;

            // task deps (input + filter + consuming)
            for (auto [nameId, spec] : task.getInputSpecs(selection).getMappedSpecs()) {
                satisfiedAnyDependencies |= tryAddDependencyIfParametrized(
                    spec, method, visitedOutputs, indirectDepPolicy, selection, outUsedSelection);
            }
            for (auto [nameId, spec] : task.getFilterSpecs(selection).getMappedSpecs()) {
                satisfiedAnyDependencies |= tryAddDependencyIfParametrized(
                    spec, method, visitedOutputs, indirectDepPolicy, selection, outUsedSelection);
            }
            for (auto [nameId, spec] : task.getConsumingSpecs(selection).getMappedSpecs()) {
                satisfiedAnyDependencies |= tryAddDependencyIfParametrized(
                    spec, method, visitedOutputs, indirectDepPolicy, selection, outUsedSelection);
            }

            if (satisfiedAnyDependencies ||
                parametrizationPolicy != PipelineParametrizationPolicy::ParametrizedDependencies) {

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
    void setupPropertiesFromTasks(const PropertyList &desiredOutputSpecs,
                                  const PropertyAliases &selection)
    {
        // 4 maps/sets needed to know how we use properties
        std::set<StringId> missingPropertyNames;
        std::map<StringId, PropertySpec> everUsedProperties;
        std::set<StringId> currentPropertyNames;
        std::set<StringId> modifiedPropertyNames;

        // helper functions for controlling the maps/sets

        auto requireProperty = [&](const PropertySpec &propertySpec) {
            StringId actualName = selection.choiceFor(propertySpec.name);

            if (currentPropertyNames.find(actualName) == currentPropertyNames.end()) {
                if (everUsedProperties.find(actualName) == everUsedProperties.end()) {
                    missingPropertyNames.insert(actualName);
                    currentPropertyNames.insert(actualName);
                } else {
                    throw PipelineSetupException(
                        std::string("Property ") + selection.choiceStringFor(propertySpec.name) +
                        "' was consumed, but also depended on later in the pipeline");
                }
            }
        };

        auto usingProperty = [&](const PropertySpec &propertySpec, const String &byWho) {
            StringId actualName = selection.choiceFor(propertySpec.name);

            auto it = everUsedProperties.find(actualName);
            if (it != everUsedProperties.end()) {
                if ((*it).second.typeInfo != propertySpec.typeInfo) {
                    throw PipelineSetupException(
                        String("Property '") + selection.choiceStringFor(propertySpec.name) +
                        "' was first used as " + String((*it).second.typeInfo.getShortTypeName()) +
                        " but late as " + String(propertySpec.typeInfo.getShortTypeName()) +
                        " by " + byWho);
                }
            } else {
                everUsedProperties.emplace(actualName, propertySpec);
            }
        };

        auto setProperty = [&](const PropertySpec &propertySpec) {
            StringId actualName = selection.choiceFor(propertySpec.name);

            modifiedPropertyNames.insert(actualName);
            currentPropertyNames.insert(actualName);
        };

        auto consumeProperty = [&](const PropertySpec &propertySpec) {
            StringId actualName = selection.choiceFor(actualName);

            currentPropertyNames.erase(actualName);
        };

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

            for (auto &spec : task.getFilterSpecs(selection).getSpecList()) {
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