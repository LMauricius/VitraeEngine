#pragma once

#include "Vitrae/Assets/Texture.hpp"
#include "Vitrae/Pipelines/Method.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/NonCopyable.hpp"

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

    dynasma::FirmPtr<Method<ShaderTask>> getVertexMethod() const;
    dynasma::FirmPtr<Method<ShaderTask>> getFragmentMethod() const;
    const std::map<StringId, dynasma::FirmPtr<Texture>> &getTextures() const;

  protected:
    dynasma::FirmPtr<Method<ShaderTask>> m_vertexMethod;
    dynasma::FirmPtr<Method<ShaderTask>> m_fragmentMethod;
    std::map<StringId, dynasma::FirmPtr<Texture>> m_textures;
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
constexpr const char DIFFUSE[] = "tex_diffuse";
constexpr const char SPECULAR[] = "tex_specular";
constexpr const char EMISSIVE[] = "tex_emissive";
} // namespace StandardMaterialTextureNames

} // namespace Vitrae