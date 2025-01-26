#pragma once

#include "Vitrae/Assets/Shapes/Shape.hpp"
#include "Vitrae/Collections/FormGenerator.hpp"
#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Data/Typedefs.hpp"

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
class Mesh;

/**
 * A model is a visible object with an assigned Material,
 * and one or more forms, defined by Shapes, that are chosen depending on the render's purpose and
 * desired level of detail
 */
class Model : public dynasma::PolymorphicBase
{
  public:
    struct AssimpLoadParams
    {
        ComponentRoot &root;
        const aiMesh *p_extMesh;
    };

    Model(const AssimpLoadParams &params);
    virtual ~Model();

    void setMaterial(dynasma::LazyPtr<Material> p_mat);
    dynasma::LazyPtr<Material> getMaterial() const;

    /**
     * Returns the best suited form of the specified purpose, for the desired LoD
     * @param purpose The purpose of the form
     * @param lodParams The desired level of detail
     * @note If the forms with the specified purpose aren't added, it tries to construct them on
     * the fly using the FormGenerator from the FormGeneratorCollection
     * @throws std::out_of_range if no forms with the specified purpose are available
     */
    dynasma::LazyPtr<Shape> getBestForm(StringId purpose, const LoDSelectionParams &lodParams,
                                        const LoDContext &lodCtx) const;

    /**
     * Returns all the forms with the given purpose
     * @param purpose The purpose of the forms
     * @note If the forms with the specified purpose aren't available, it tries to construct them on
     * the fly using the FormGenerator from the FormGeneratorCollection
     * @throws std::out_of_range if no forms with the specified purpose are available
     */
    DetailFormSpan getFormsWithPurpose(StringId purpose) const;

    /**
     * Adds a form to the model
     * @param purpose The purpose of the form
     * @param lodMeasure The level of detail measure of the form
     * @param p_shape The shape of the form
     */
    void addForm(StringId purpose, std::shared_ptr<LoDMeasure> lodMeasure,
                 dynasma::LazyPtr<Shape> p_shape);

    /**
     * Replaces all the forms with the given purpose
     * @param purpose The purpose of the form
     * @param forms The new forms
     */
    void setFormsWithPurpose(StringId purpose, DetailFormVector forms);

    inline const BoundingBox &getBoundingBox() const { return m_boundingBox; }

  protected:
    ComponentRoot &m_root;
    dynasma::LazyPtr<Material> mp_material;
    BoundingBox m_boundingBox;
    mutable StableMap<StringId, DetailFormVector> m_formsByPurpose;
};

struct ModelSeed
{
    using Asset = Model;

    inline std::size_t load_cost() const { return 1; }

    std::variant<Model::AssimpLoadParams> kernel;
};

using ModelKeeper = dynasma::AbstractKeeper<ModelSeed>;

} // namespace Vitrae