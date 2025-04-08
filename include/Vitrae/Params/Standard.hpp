#pragma once

#include "Vitrae/Data/LevelOfDetail.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "dynasma/pointer.hpp"

namespace Vitrae
{
class Scene;
class Camera;
class FrameStore;

/**
 * Namespace containing commonly used ParamSpecs, identifiable by their string names
 */
namespace StandardParam
{

// clang-format off

inline const ParamSpec vsync       = {"vsync",       TYPE_INFO<bool>};

inline const ParamSpec scene       = {"scene",       TYPE_INFO<dynasma::FirmPtr<Scene>>};
inline const ParamSpec camera      = {"camera",      TYPE_INFO<dynasma::FirmPtr<Camera>>};
inline const ParamSpec fs_target   = {"fs_target",   TYPE_INFO<dynasma::FirmPtr<FrameStore>>};
inline const ParamSpec fs_display  = {"fs_display",  TYPE_INFO<dynasma::FirmPtr<FrameStore>>};
inline const ParamSpec index4data  = {"index4data",  TYPE_INFO<std::uint32_t>};
inline const ParamSpec LoDParams   = {"LoDParams",   TYPE_INFO<LoDSelectionParams>};
    
inline const ParamSpec mat_model   = {"mat_model",   TYPE_INFO<glm::mat4>};
inline const ParamSpec mat_view    = {"mat_view",    TYPE_INFO<glm::mat4>};
inline const ParamSpec mat_proj    = {"mat_proj",    TYPE_INFO<glm::mat4>};

/// @brief Combined matrix of view and projection
inline const ParamSpec mat_display = {"mat_display", TYPE_INFO<glm::mat4>};

/// @brief Combined matrix of model, view and projection
inline const ParamSpec mat_mvp     = {"mat_mvp",     TYPE_INFO<glm::mat4>};

/// @brief The surface visible color 
inline const ParamSpec fragment_color = {"fragment_color",       TYPE_INFO<glm::vec4>};

/// @brief The surface color with lights applied to it
inline const ParamSpec shade       = {"shade",       TYPE_INFO<glm::vec4>};

/// @subsection Vertex properties
inline const ParamSpec position    = {"position",    TYPE_INFO<glm::vec3>};
inline const ParamSpec normal      = {"normal",      TYPE_INFO<glm::vec3>};
inline const ParamSpec coord_base  = {"coord_base",   TYPE_INFO<glm::vec3>};

// clang-format on

} // namespace StandardParam
} // namespace Vitrae