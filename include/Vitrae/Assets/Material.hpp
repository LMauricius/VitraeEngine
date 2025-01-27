#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Pipelines/Shading/Task.hpp"
#include "Vitrae/Util/NonCopyable.hpp"

#include "assimp/material.h"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <filesystem>
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

    void setParamAliases(const ParamAliases &aliases);
    void setProperty(StringId key, const Variant &value);
    void setProperty(StringId key, Variant &&value);
    void setTexture(StringView colorName, dynasma::FirmPtr<Texture> texture,
                    StringView coordPropertyName);
    void setTexture(StringView colorName, glm::vec4 uniformColor);

    const ParamAliases &getParamAliases() const;
    const StableMap<StringId, Variant> &getProperties() const;

  protected:
    ComponentRoot &m_root;
    ParamAliases m_externalAliases, m_aliases;
    StableMap<StringId, String> m_tobeInternalAliases;
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

} // namespace Vitrae