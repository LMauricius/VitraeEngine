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
};

} // namespace Vitrae