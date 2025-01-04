#pragma once

#include "Vitrae/Pipelines/Compositing/Task.hpp"

#include "Vitrae/Pipelines/Pipeline.hpp"

#include "dynasma/keepers/abstract.hpp"

#include <functional>
#include <map>
#include <vector>

namespace Vitrae {

class MethodCollection;

/**
 * A task that supports aliasing for adapting external properties with various names to
 * tasks that require specific property names.
 * Also supports encapsulation of non-filtered properties.
 */
class ComposeAdaptTasks : public ComposeTask
{
  public:
    struct SetupParams {
        ComponentRoot &root;

        /**
         * Aliases specific for this adaptor
         */
        PropertyAliases adaptorAliases;

        /**
         * Outputs we desire. Some outputs could become filter properties with a different name
         */
        PropertyList desiredOutputs;

        /**
         * Human readable name
         */
        String friendlyName;
    };

    ComposeAdaptTasks(const SetupParams &params);
    ~ComposeAdaptTasks() = default;

    std::size_t memory_cost() const override;

    const PropertyList &getInputSpecs(const PropertyAliases &) const override;
    const PropertyList &getOutputSpecs() const override;
    const PropertyList &getFilterSpecs(const PropertyAliases &) const override;
    const PropertyList &getConsumingSpecs(const PropertyAliases &) const override;

    void extractUsedTypes(std::set<const TypeInfo *> &typeSet,
                          const PropertyAliases &aliases) const override;
    void extractSubTasks(std::set<const Task *> &taskSet,
                         const PropertyAliases &aliases) const override;

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

        PropertyList inputSpecs, filterSpecs, consumeSpecs;

        AdaptorPerAliases() = delete;
        AdaptorPerAliases(AdaptorPerAliases &&) = default;
        AdaptorPerAliases(const AdaptorPerAliases &) = default;
        AdaptorPerAliases &operator=(AdaptorPerAliases &&) = default;
        AdaptorPerAliases &operator=(const AdaptorPerAliases &) = default;

        AdaptorPerAliases(const PropertyAliases &adaptorAliases, const PropertyList &desiredOutputs,
                          const PropertyAliases &externalAliases,
                          const MethodCollection &methodCollection, StringView friendlyName);
    };

    mutable StableMap<std::size_t, AdaptorPerAliases> m_adaptorPerSelectionHash;

    const AdaptorPerAliases &getAdaptorPerAliases(const PropertyAliases &externalAliases,
                                                  const MethodCollection &methodCollection) const;
    void forgetAdaptorPerAliases(const PropertyAliases &externalAliases) const;
};

struct ComposeAdaptTasksKeeperSeed {
    using Asset = ComposeAdaptTasks;
    std::variant<ComposeAdaptTasks::SetupParams> kernel;
    inline std::size_t load_cost() const { return 1; }
};

using ComposeAdaptTasksKeeper =
    dynasma::AbstractKeeper<ComposeAdaptTasksKeeperSeed>;
} // namespace Vitrae