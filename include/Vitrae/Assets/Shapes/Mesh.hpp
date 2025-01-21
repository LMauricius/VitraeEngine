#pragma once

#include "Vitrae/Assets/BufferUtil/Ptr.hpp"
#include "Vitrae/Assets/BufferUtil/SubVariantPtr.hpp"
#include "Vitrae/Assets/Shapes/Shape.hpp"
#include "Vitrae/Data/GraphicPrimitives.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"
#include "Vitrae/Util/NonCopyable.hpp"

#include "assimp/mesh.h"
#include "assimp/scene.h"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <filesystem>
#include <span>
#include <variant>

namespace Vitrae
{
class ComponentRoot;

/**
 * A mesh is a 3D polygonal shape defined by a set of triangles connected by shared vertices.
 * It depicts a possibly smooth surface by interpolating data between defined vertices.
 */
class Mesh : public Shape
{
  public:
    struct AssimpLoadParams
    {
        ComponentRoot &root;
        const aiMesh *p_extMesh;
    };
    struct TriangleVerticesParams
    {
        ComponentRoot &root;
        StableMap<StringId, SharedSubBufferVariantPtr> vertexComponentBuffers;
        SharedBufferPtr<void, Triangle> indexBuffer;
    };

    virtual ~Mesh() = default;

    /**
     * @returns The subbuffer of vertex components
     * @param componentName The name of the vertex component to get ("position", "normal", etc.)
     */
    virtual SharedSubBufferVariantPtr getVertexComponentBuffer(StringId componentName) const = 0;

    /**
     * @returns The index buffer
     */
    virtual SharedBufferPtr<void, Triangle> getIndexBuffer() const = 0;

    /**
     * @tparam ElementT The type of the vertex element
     * @param componentName The name of the vertex component to get ("position", "normal", etc.)
     * @returns A span of ElementT values
     * @throws std::out_of_range if the element does not exist or a wrong type is used
     */
    template <class ElementT>
    StridedSpan<const ElementT> getVertexComponentData(StringId componentName) const
    {
        return getVertexComponentBuffer(componentName).getElements<ElementT>();
    }

    /**
     * @returns A span of Triangle values with indices of the mesh vertices
     */
    inline std::span<const Triangle> getTriangles() const { return getIndexBuffer().getElements(); }
};

struct MeshKeeperSeed
{
    using Asset = Mesh;

    std::variant<Mesh::AssimpLoadParams, Mesh::TriangleVerticesParams> kernel;

    inline std::size_t load_cost() const { return 1; }
};

/**
 * Namespace containing all standard vertex buffer names
 */
namespace StandardVertexBufferNames
{
inline constexpr const char POSITION[] = "position";
inline constexpr const char NORMAL[] = "normal";
inline constexpr const char TEXTURE_COORD[] = "textureCoord0";
inline constexpr const char COLOR[] = "color0";
} // namespace StandardVertexBufferNames

// using MeshManager = dynasma::AbstractManager<MeshSeed>;
using MeshKeeper = dynasma::AbstractKeeper<MeshKeeperSeed>;
} // namespace Vitrae