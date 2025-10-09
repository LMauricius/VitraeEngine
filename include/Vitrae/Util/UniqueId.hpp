#pragma once

#include <cstdlib>

namespace Vitrae
{
/**
 * @returns a unique ID across the entire program run
 */
inline std::size_t getUniqueID()
{
    static std::size_t ctr = 0;
    ctr++;
    return ctr;
}

/**
 * @returns a unique ID for a class, across this program run
 */
template <class T> std::size_t getClassID()
{
    static std::size_t id = getUniqueID();
    return id;
}

/**
 * @returns a unique ID for all calls with this ScopeTokenT in this program run
 * @tparam ScopeTokenT a scope 'token' i.e. type used for differentiating scopes
 */
template <class ScopeTokenT> std::size_t getScopedUniqueID()
{
    static std::size_t ctr = 0;
    ctr++;
    return ctr;
}

/**
 * @returns a unique ID for the class T, across all calls with this ScopeTokenT in this program run
 * @tparam ScopeTokenT a scope 'token' i.e. type used for differentiating scopes
 */
template <class ScopeTokenT, class T> std::size_t getScopedClassID()
{
    static std::size_t id = getScopedUniqueID<ScopeTokenT>();
    return id;
}

} // namespace Vitrae