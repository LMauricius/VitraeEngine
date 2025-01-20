#include "Vitrae/Assets/Model.hpp"
#include "Vitrae/Assets/Shapes/Mesh.hpp"
#include "Vitrae/Collections/ComponentRoot.hpp"
#include "Vitrae/TypeConversion/StringCvt.hpp"

#include "dynasma/keepers/abstract.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Vitrae
{
Model::Model(const AssimpLoadParams &params) : m_root(params.root)
{
    MeshKeeper &meshKeeper = params.root.getComponent<MeshKeeper>();
    auto p_mesh =
        meshKeeper.new_asset({Mesh::AssimpLoadParams{params.root, params.p_extMesh}}).getLoaded();

    // Calculate minimum edge length
    float minEdgeLength = std::numeric_limits<float>::max();

    std::span<const Triangle> triangles = p_mesh->getTriangles();
    std::span<const glm::vec3> positions =
        p_mesh->getVertexElements<glm::vec3>(StandardVertexBufferNames::POSITION);
    for (const auto &tri : triangles) {
        float e0 = glm::distance(positions[tri.ind[0]], positions[tri.ind[1]]);
        float e1 = glm::distance(positions[tri.ind[1]], positions[tri.ind[2]]);
        float e2 = glm::distance(positions[tri.ind[2]], positions[tri.ind[0]]);

        minEdgeLength = std::min(minEdgeLength, std::min(e0, std::min(e1, e2)));
    }

    addForm("visual",
            std::unique_ptr<Vitrae::LoDMeasure>(new SmallestElementSizeMeasure(minEdgeLength)),
            p_mesh);
}

Model::~Model() {}

void Model::setMaterial(dynasma::LazyPtr<Material> p_mat)
{
    mp_material = p_mat;
}

dynasma::LazyPtr<Material> Model::getMaterial() const
{
    return mp_material;
}

dynasma::LazyPtr<Shape> Model::getBestForm(StringId purpose, const LoDSelectionParams &lodParams,
                                           const LoDContext &lodCtx) const
{
    const auto &forms = getFormsWithPurpose(purpose);

    switch (lodParams.method) {
    case LoDSelectionMethod::Minimum:
        return forms.back().second;
    case LoDSelectionMethod::Maximum:
        return forms.front().second;
    case LoDSelectionMethod::FirstBelowThreshold: {
        dynasma::LazyPtr<Shape> p_choice;

        for (const auto &[p_lodMeasure, p_shape] : forms) {
            if (!p_lodMeasure->isTooDetailed(lodCtx, lodParams.threshold)) {
                return p_shape;
            }
        }

        return forms.back().second;
    }
    case LoDSelectionMethod::FirstAboveThreshold: {
        dynasma::LazyPtr<Shape> p_choice;

        for (const auto &[p_lodMeasure, p_shape] : forms) {
            if (!p_lodMeasure->isTooDetailed(lodCtx, lodParams.threshold)) {
                break;
            } else {
                p_choice = p_shape;
            }
        }

        if (p_choice != dynasma::LazyPtr<Shape>())
            return p_choice;
        else
            return forms.back().second;
    }
    }

    throw std::out_of_range("Form not found");
}

DetailFormSpan Model::getFormsWithPurpose(StringId purpose) const
{
    auto it = m_formsByPurpose.find(purpose);

    if (it != m_formsByPurpose.end()) {
        return (*it).second;
    } else {
        const FormGeneratorCollection &generators = m_root.getComponent<FormGeneratorCollection>();
        if (auto genIt = generators.find(purpose); genIt != generators.end()) {
            it = m_formsByPurpose.emplace(purpose, (*genIt).second(*this)).first;
            return (*it).second;
        } else {
            throw std::out_of_range("Form not found and genarator for it doesn't exist");
        }
    }
}

void Model::addForm(StringId purpose, std::unique_ptr<LoDMeasure> lodMeasure,
                    dynasma::LazyPtr<Shape> p_shape)
{
    m_formsByPurpose[purpose].emplace_back(std::move(lodMeasure), p_shape);
}

void Model::setFormsWithPurpose(StringId purpose, DetailFormVector forms)
{
    m_formsByPurpose[purpose] = std::move(forms);
}

} // namespace Vitrae