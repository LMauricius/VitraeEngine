#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/NonCopyable.hpp"
#include "Vitrae/Util/StableMap.hpp"

#include "assimp/material.h"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <variant>

namespace Vitrae
{
class ComponentRoot;
class Texture;

class Material : public dynasma::PolymorphicBase
{
  public:
    struct AssimpLoadParams
    {
        ComponentRoot &root;
        const aiMaterial *p_extMaterial;
        std::filesystem::path sceneFilepath;
    };

    Material(const AssimpLoadParams &params);
    virtual ~Material();

    std::size_t memory_cost() const;

    void setMethods(dynasma::FirmPtr<Method<ShaderTask>> vertexMethod,
                    dynasma::FirmPtr<Method<ShaderTask>> fragmentMethod);
    void setProperty(StringId key, const Variant &value);
    void setProperty(StringId key, Variant &&value);

    dynasma::FirmPtr<Method<ShaderTask>> getVertexMethod() const;
    dynasma::FirmPtr<Method<ShaderTask>> getFragmentMethod() const;
    const StableMap<StringId, dynasma::FirmPtr<Texture>> &getTextures() const;
    const StableMap<StringId, Variant> &getProperties() const;

  protected:
    dynasma::FirmPtr<Method<ShaderTask>> m_vertexMethod;
    dynasma::FirmPtr<Method<ShaderTask>> m_fragmentMethod;
    StableMap<StringId, dynasma::FirmPtr<Texture>> m_textures;
    StableMap<StringId, Variant> m_properties;
};

struct MaterialKeeperSeed
{
    using Asset = Material;

    std::variant<Material::AssimpLoadParams> kernel;

    inline std::size_t load_cost() const { return 1; }
};

// using MaterialManager = dynasma::AbstractManager<MaterialSeed>;
using MaterialKeeper = dynasma::AbstractKeeper<MaterialKeeperSeed>;

/**
 * Namespace containing all standard material texture names
 */
namespace StandardMaterialTextureNames
{
static constexpr const char DIFFUSE[] = "tex_diffuse";
static constexpr const char SPECULAR[] = "tex_specular";
static constexpr const char EMISSIVE[] = "tex_emissive";
} // namespace StandardMaterialTextureNames

namespace StandardMaterialPropertyNames
{
static constexpr const char COL_DIFFUSE[] = "color_diffuse";
static constexpr const char COL_SPECULAR[] = "color_specular";
static constexpr const char COL_EMISSIVE[] = "color_emissive";
static constexpr const char SHININESS[] = "shininess";
} // namespace StandardMaterialPropertyNames

namespace StandardMaterialPropertyTypes
{
static constexpr const TypeInfo &COL_DIFFUSE = Variant::getTypeInfo<glm::vec4>();
static constexpr const TypeInfo &COL_SPECULAR = Variant::getTypeInfo<glm::vec4>();
static constexpr const TypeInfo &COL_EMISSIVE = Variant::getTypeInfo<glm::vec4>();
static constexpr const TypeInfo &SHININESS = Variant::getTypeInfo<float>();
} // namespace StandardMaterialPropertyTypes

} // namespace Vitrae