#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"
#include "Vitrae/Pipelines/PipelineContainer.hpp"

#include "Vitrae/Pipelines/Pipeline.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <map>
#include <vector>

namespace Vitrae {

class MethodCollection;

/**
 * A task that sets up properties only once and caches them, by running adapted tasks only when properties are missing.
 * Like ComposeAdaptTasks, it supports aliasing for adapting external properties with various names to
 * tasks that require specific property names.
 * Also supports encapsulation of non-filtered properties.
 */
class ComposeCacheTasks : public ComposeTask, public PipelineContainer<ComposeTask>
{
  public:
    struct SetupParams {
        ComponentRoot &root;

        /**
         * Aliases specific for this adaptor
         */
        ParamAliases adaptorAliases;

        /**
         * Outputs we desire. Some outputs could become filter properties with a different name
         */
        ParamList desiredOutputs;

        /**
         * Human readable name
         */
        String friendlyName;
    };

    ComposeCacheTasks(const SetupParams &params);
    ~ComposeCacheTasks() = default;

    std::size_t memory_cost() const override;

    const ParamList &getInputSpecs(const ParamAliases &) const override;
    const ParamList &getOutputSpecs() const override;
    const ParamList &getFilterSpecs(const ParamAliases &) const override;
    const ParamList &getConsumingSpecs(const ParamAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const ParamAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const ParamAliases &aliases) const override;

    const Pipeline<ComposeTask> &getContainedPipeline(const ParamAliases &aliases) const override;

    ParamAliases constructContainedPipelineAliases(const ParamAliases &aliases) const override;

    void rebuildContainedPipeline(const ParamAliases &aliases) const override;

    void run(RenderComposeContext ctx) const override;
    void prepareRequiredLocalAssets(RenderComposeContext ctx) const override;

    StringView getFriendlyName() const override;

  private:
    SetupParams m_params;

    struct AdaptorPerAliases
    {
        /**
         * Mapping of internal property names to external ones, performed after running the pipeline
         */
        StableMap<StringId, StringId> finishingMapping;

        Pipeline<ComposeTask> pipeline;

        ParamList inputSpecs, filterSpecs, consumeSpecs;

        AdaptorPerAliases() = delete;
        AdaptorPerAliases(AdaptorPerAliases &&) = default;
        AdaptorPerAliases(const AdaptorPerAliases &) = default;
        AdaptorPerAliases &operator=(AdaptorPerAliases &&) = default;
        AdaptorPerAliases &operator=(const AdaptorPerAliases &) = default;

        AdaptorPerAliases(const ParamAliases &adaptorAliases, const ParamList &desiredOutputs,
                          const ParamAliases &externalAliases,
                          const MethodCollection &methodCollection, StringView friendlyName);
    };

    mutable StableMap<std::size_t, std::unique_ptr<AdaptorPerAliases>> m_adaptorPerSelectionHash;

    const AdaptorPerAliases &getAdaptorPerAliases(const ParamAliases &externalAliases,
                                                  const MethodCollection &methodCollection) const;
    void forgetAdaptorPerAliases(const ParamAliases &externalAliases) const;

    struct MyMemory
    {
        const AdaptorPerAliases *adaptor = nullptr;
        RestartablePipelineMemory subPipelineMemory;
        StableMap<StringId, Variant> cachedProperties;
    };
};

struct ComposeCacheTasksKeeperSeed {
    using Asset = ComposeCacheTasks;
    std::variant<ComposeCacheTasks::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeCacheTasksKeeper =
    dynasma::AbstractKeeper<ComposeCacheTasksKeeperSeed>;
} // namespace Vitrae