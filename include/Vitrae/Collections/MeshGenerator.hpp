#pragma once

#include "Vitrae/Assets/BufferUtil/SubVariantPtr.hpp"
#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/StringId.hpp"

#include <functional>
#include <span>

namespace Vitrae
{
class ComponentRoot;
class Mesh;

/**
 * A function that generates mesh vertex component (sub)buffers for certain component names
 * @param mesh The mesh to generate the buffers for
 * @returns A map of component names to subbuffers. Will be merged into the existing buffers.
 */
using MeshGenerator = std::function<StableMap<StringId, SharedSubBufferVariantPtr>(
    ComponentRoot &root, const Mesh &mesh)>;

/**
 * A collection of functions that generate mesh data
 */
class MeshGeneratorCollection
{
  public:
    /**
     * Registers a generator for one or more components
     * @param componentNames The names of the components to generate
     * @param generator The generator
     */
    void registerGeneratorForComponents(std::span<const StringId> componentNames,
                                        MeshGenerator generator);

    /**
     * @returns The generator for a specific component, or empty function if not found
     * @param componentName The name of the component we need
     */
    MeshGenerator getGeneratorForComponent(StringId componentName);

  protected:
    StableMap<StringId, MeshGenerator> m_generators;
};

} // namespace Vitrae