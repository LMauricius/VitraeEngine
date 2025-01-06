#pragma once

#include "Vitrae/Pipelines/Pipeline.hpp"
#include "Vitrae/Pipelines/Task.hpp"

namespace Vitrae
{

/**
 * @brief A polymorphic interface class for retrieval of contained pipelines
 */
template <TaskChild BasicTask> class PipelineContainer
{
  public:
    virtual ~PipelineContainer() = default;

    virtual const Pipeline<BasicTask> &getContainedPipeline(
        const PropertyAliases &aliases) const = 0;

    /**
     * @returns The aliases used by the contained pipeline.
     * @note This object is valid until either this Task or the passed aliases are destroyed
     */
    virtual PropertyAliases constructContainedPipelineAliases(
        const PropertyAliases &aliases) const = 0;

    /**
     * Notifies that the contained pipeline should be rebuilt
     */
    virtual void rebuildContainedPipeline(const PropertyAliases &aliases) const = 0;
};

} // namespace Vitrae