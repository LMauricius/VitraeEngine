#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "dynasma/pointer.hpp"

namespace Vitrae
{
class Scene;
class Camera;
class FrameStore;
class LoDSelectionParams;

/**
 * Namespace containing commonly used ParamSpecs, identifiable by their string names
 */
namespace StandardParam
{

// clang-format off

inline const ParamSpec scene     = {.name = "scene",     .typeInfo = TYPE_INFO<dynasma::FirmPtr<Scene>>};
inline const ParamSpec camera    = {.name = "camera",    .typeInfo = TYPE_INFO<dynasma::FirmPtr<Camera>>};
inline const ParamSpec fs_target = {.name = "fs_target", .typeInfo = TYPE_INFO<dynasma::FirmPtr<FrameStore>>};
inline const ParamSpec LoDParams = {.name = "LoDParams", .typeInfo = TYPE_INFO<LoDSelectionParams>};

inline const ParamSpec mat_model = {.name = "mat_model", .typeInfo = TYPE_INFO<glm::mat4>};
// clang-format on

} // namespace StandardParam
} // namespace Vitrae