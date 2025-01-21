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
    return m_generators.at(componentName);
}

} // namespace Vitrae