#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace Vitrae
{
template <class _T> using Unique = std::unique_ptr<_T>;

template <class _T> using Shared = std::shared_ptr<_T>;

template <class _T> using Pointer = _T *;

using Byte = unsigned char;

using String = std::string;
using StringView = std::string_view;

using EmptyType = std::monostate;
} // namespace Vitrae