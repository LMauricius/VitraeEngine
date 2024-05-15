#pragma once

#include "Vitrae/Pipelines/Task.h"

namespace Vitrae {
template<TaskChild BasicTask>
class Filter : public BasicTask
{
  public:
    using FilterConditionType = bool;

  protected:
    dynasma::FirmPtr<BasicTask> m_task;
    StringId m_conditionProperty;

  public:
    Filter(dynasma::FirmPtr<BasicTask> containedTask, StringId conditionProperty)
        : m_containedTask(containedTask), m_conditionProperty(conditionProperty)
    {
        this->m_inputSpecs = m_containedTask->m_inputSpecs;
        this->m_outputSpecs = m_containedTask->m_outputSpecs;

        if (this->m_inputSpecs != this->m_outputSpecs)
        {
            throw std::runtime_error("Input and output specs must be equal");
        }

        this->m_inputSpecs.insert({m_conditionProperty, m_trueValue.type()});
  }

  ~Filter() = default;
};
} // namespace Vitrae