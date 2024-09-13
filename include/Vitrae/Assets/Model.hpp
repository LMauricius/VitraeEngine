#pragma once

#include "Vitrae/Assets/Mesh.hpp"

#include "assimp/mesh.h"
#include "assimp/scene.h"
#include "dynasma/keepers/abstract.hpp"
#include "dynasma/managers/abstract.hpp"
#include "dynasma/pointer.hpp"

#include <filesystem>
#include <span>
#include <vector>

namespace Vitrae
{
class ComponentRoot;

/**
 * A mesh is a 3D polygonal piece of geometry,
 * with an assigned Material
 */
class Model : public dynasma::PolymorphicBase
{
  public:
    struct FileLoadParams
    {
        std::filesystem::path filepath;
        ComponentRoot &root;
    };

    Model(const FileLoadParams &params);
    virtual ~Model();

    inline std::span<dynasma::LazyPtr<Mesh>> getMeshes() { return m_meshes; }
    inline std::span<const dynasma::LazyPtr<Mesh>> getMeshes() const { return m_meshes; }

  protected:
    std::vector<dynasma::LazyPtr<Mesh>> m_meshes;
};

struct ModelSeed
{
    using Asset = Model;

    inline std::size_t load_cost() const { return 1; }

    std::variant<Model::FileLoadParams> kernel;
};

using ModelManager = dynasma::AbstractManager<ModelSeed>;
// using ModelKeeper = dynasma::AbstractKeeper<ImmediateModelSeed>;
} // namespace Vitrae