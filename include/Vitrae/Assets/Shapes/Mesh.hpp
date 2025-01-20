#pragma once

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

    virtual ~Mesh() = default;

    virtual std::span<const Triangle> getTriangles() const = 0;

    /**
     * @tparam ElementT The type of the vertex element
     * @param elementName The name of the vertex element to get ("position", "normal", etc.)
     * @returns A span of ElementT values
     * @throws std::out_of_range if the element does not exist or a wrong type is used
     */
    template <class ElementT>
    std::span<const ElementT> getVertexElements(StringId elementName) const
    {
        Variant anyArray = getVertexData(elementName, TYPE_INFO<ElementT>);
        assert(anyArray.getAssignedTypeInfo() == TYPE_INFO<std::span<const ElementT>>);
        return anyArray.get<std::span<const ElementT>>();
    }

  protected:
    /**
     * @returns std::span<const ElementT> where ElementT is the type whose TypeInfo we passed
     */
    virtual Variant getVertexData(StringId bufferName, const TypeInfo &type) const = 0;
};

struct MeshKeeperSeed
{
    using Asset = Mesh;

    std::variant<Mesh::AssimpLoadParams> kernel;

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