#pragma once

#include "Vitrae/Data/BoundingBox.hpp"
#include "Vitrae/Data/GraphicPrimitives.hpp"
#include "Vitrae/Util/NonCopyable.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "Vitrae/Util/Variant.hpp"

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
class Material;

/**
 * A mesh is a 3D polygonal piece of geometry,
 * with an assigned Material
 */
class Mesh : public dynasma::PolymorphicBase
{
  public:
    struct AssimpLoadParams
    {
        ComponentRoot &root;
        const aiMesh *p_extMesh;
        std::filesystem::path sceneFilepath;
    };

    virtual ~Mesh() = default;

    virtual void setMaterial(dynasma::LazyPtr<Material> mat) = 0;
    virtual dynasma::LazyPtr<Material> getMaterial() const = 0;
    virtual std::span<const Triangle> getTriangles() const = 0;
    virtual BoundingBox getBoundingBox() const = 0;

    /**
     * @tparam ElementT The type of the vertex element
     * @param elementName The name of the vertex element to get ("position", "normal", etc.)
     * @returns A span of ElementT values
     * @throws std::out_of_range if the element does not exist or a wrong type is used
     */
    template <class ElementT>
    std::span<const ElementT> getVertexElements(StringId elementName) const
    {
        Variant anyArray = getVertexData(elementName, Variant::getTypeInfo<ElementT>());
        assert(anyArray.getAssignedTypeInfo() == Variant::getTypeInfo<std::span<const ElementT>>());
        return anyArray.get<std::span<const ElementT>>();
    }

    virtual std::size_t memory_cost() const = 0;

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