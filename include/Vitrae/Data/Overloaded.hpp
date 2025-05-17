#pragma once

namespace Vitrae {

// helper type for the visitor
template <class... Ts> struct Overloaded : Ts...
{
    using Ts::operator()...;
};

}