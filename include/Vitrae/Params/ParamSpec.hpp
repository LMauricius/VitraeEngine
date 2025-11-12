#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Dynamic/TypeInfo.hpp"
#include "Vitrae/Params/Attribute.hpp"

namespace Vitrae
{

/**
 * @brief Specifies a parameter used for dynamic pipeline construction
 *
 * @example @code
 *  AttributeWrapper attr = {Default{0}, ShadingStage{ShadingDataStage::SHAPE}};
 *
 * ParamSpec spec = {
 *      .name = "someParameter",
 *      .typeInfo = TYPE_INFO<int>,
 *      .attributes{
 *          Default{0},
 *          InherentShadingStage{ShadingStage::SHAPE},
 *      },
 * };
 * @endcode */
struct ParamSpec
{
    /// The name of the parameter
    String name;

    /// The type of the parameter
    const TypeInfo &typeInfo;

    /// Attributes placed on the parameter as opposed to its value (default none)
    AttributeWrapper attributes{};
};

} // namespace Vitrae