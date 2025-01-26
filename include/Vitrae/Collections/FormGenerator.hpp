#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Data/StringId.hpp"

#include "dynasma/pointer.hpp"

#include <memory>
#include <span>
#include <vector>

namespace Vitrae
{
class Shape;
class Model;
class ComponentRoot;

using DetailFormVector =
    std::vector<std::pair<std::shared_ptr<LoDMeasure>, dynasma::LazyPtr<Shape>>>;

using DetailFormSpan =
    std::span<const std::pair<std::shared_ptr<LoDMeasure>, dynasma::LazyPtr<Shape>>>;

/**
 * Generator function for a list of shapes with varying levels of detail
 * @param model The model which we use as a basis for new forms to be generated
 */
using FormGenerator = std::function<DetailFormVector(ComponentRoot &root, const Model &model)>;

/**
 * A collection of form generators
 * The key is the purpose of the form, and the value is the form generator
 */
using FormGeneratorCollection = StableMap<StringId, FormGenerator>;

} // namespace Vitrae