#pragma once

namespace Vitrae
{

/**
 * The boolean function to use for depth and stencil testing   
*/
enum class FragmentTestFunction {
    Never,
    Always,
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
};

} // namespace Vitrae