#pragma once

namespace Vitrae
{
struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(NonCopyable &&) = default;

    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};
} // namespace Vitrae