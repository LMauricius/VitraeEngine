#include "Vitrae/Collections/MeshGenerator.hpp"

namespace Vitrae
{
void MeshGeneratorCollection::registerGeneratorForComponents(
    std::span<const StringId> componentNames, MeshGenerator generator)
{
    for (auto componentName : componentNames) {
        m_generators[componentName] = generator;
    }
}

MeshGenerator MeshGeneratorCollection::getGeneratorForComponent(StringId componentName)
{
    if (auto it = m_generators.find(componentName); it != m_generators.end()) {
        return (*it).second;
    }
    return {};
}

} // namespace Vitrae