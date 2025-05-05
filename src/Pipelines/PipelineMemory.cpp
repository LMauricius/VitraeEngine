#include "Vitrae/Pipelines/PipelineMemory.hpp"

namespace Vitrae
{
PipelineMemory::PipelineMemory() : m_nextAnyIndex(0) {}

std::any &PipelineMemory::createNextAny()
{
    ++m_nextAnyIndex;
    return m_anyPool.emplace_back();
}

std::any &PipelineMemory::nextAny()
{
    return m_anyPool[m_nextAnyIndex++];
}

RestartablePipelineMemory::RestartablePipelineMemory() {}

void RestartablePipelineMemory::restart() {
    m_nextAnyIndex = 0;
}

} // namespace Vitrae