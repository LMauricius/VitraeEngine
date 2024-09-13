#pragma once

#include "Vitrae/Pipelines/Method.hpp"

namespace Vitrae
{

template <TaskChild BasicTask> class Pipeline
{
  public:
    struct PipeItem
    {
        dynasma::FirmPtr<BasicTask> p_task;
        StableMap<StringId, StringId> inputToLocalVariables;
        StableMap<StringId, StringId> outputToLocalVariables;
    };

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
    Pipeline(dynasma::FirmPtr<Method<BasicTask>> p_method,
             std::span<const PropertySpec> desiredOutputSpecs,
             const BasicTask::InputSpecsDeducingContext &ctx)
    {
        // store outputs
        for (auto &outputSpec : desiredOutputSpecs) {
            outputSpecs.emplace(outputSpec.name, outputSpec);
        }

        // solve dependencies
        std::set<StringId> visitedOutputs;

        for (auto &outputSpec : desiredOutputSpecs) {
            tryAddDependency(*p_method, ctx, visitedOutputs, outputSpec, true);
        }
    }

    // pipethrough variables are those that are just passed from inputs to outputs, unprocessed
    StableMap<StringId, PropertySpec> localSpecs, inputSpecs, outputSpecs;
    std::set<StringId> pipethroughInputNames;
    std::vector<PipeItem> items;

  protected:
    void tryAddDependency(const Method<BasicTask> &method,
                          const BasicTask::InputSpecsDeducingContext &ctx,
                          std::set<StringId> &visitedOutputs, const PropertySpec &desiredOutputSpec,
                          bool isFinalOutput)
    {
        if (visitedOutputs.find(desiredOutputSpec.name) == visitedOutputs.end()) {
            std::optional<dynasma::FirmPtr<BasicTask>> task =
                method.getTask(desiredOutputSpec.name);

            if (task.has_value()) {
                if (task.value() == dynasma::FirmPtr<BasicTask>() || &*task.value() == nullptr) {
                    throw std::runtime_error("No task found for output " + desiredOutputSpec.name);
                }

                // task outputs (store the outputs as visited)
                std::map<StringId, StringId> outputToLocalVariables;
                for (auto [taskOutputNameId, taskOutputSpec] : task.value()->getOutputSpecs()) {
                    if (outputSpecs.find(taskOutputSpec.name) == outputSpecs.end() &&
                        localSpecs.find(taskOutputSpec.name) == localSpecs.end()) {
                        localSpecs.emplace(taskOutputSpec.name, taskOutputSpec);
                    }
                    visitedOutputs.insert(taskOutputSpec.name);
                    outputToLocalVariables.emplace(taskOutputSpec.name, taskOutputSpec.name);
                }

                // task inputs (also handle deps)
                std::map<StringId, StringId> inputToLocalVariables;
                for (auto [inputNameId, inputSpec] : task.value()->getInputSpecs(ctx)) {
                    if (task.value()->getOutputSpecs().find(inputNameId) !=
                        task.value()->getOutputSpecs().end()) {
                        inputSpecs.emplace(inputNameId, inputSpec);
                    } else {
                        tryAddDependency(method, ctx, visitedOutputs, inputSpec, false);
                    }
                    inputToLocalVariables.emplace(inputSpec.name, inputSpec.name);
                }

                items.push_back({task.value(), inputToLocalVariables, outputToLocalVariables});
            } else {
                inputSpecs.emplace(desiredOutputSpec.name, desiredOutputSpec);

                if (isFinalOutput) {
                    pipethroughInputNames.insert(desiredOutputSpec.name);
                }

                visitedOutputs.insert(desiredOutputSpec.name);
            }
        }
    };
};
} // namespace Vitrae