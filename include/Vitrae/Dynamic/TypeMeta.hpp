#pragma once

namespace Vitrae
{

/**
 * @brief A polymorphic base class of all TypeMeta classes
 * @note It isn't a pure virtual class, thus it can be instanced as-is
 */
struct PolymorphicTypeMeta
{
    virtual ~PolymorphicTypeMeta() = default;
};

/**
 * @brief A template depending on the type T, for which this value decribes properties
 * It can be specialized for various types so its class inherits other Meta component types specific
 * for the type's features
 * @note All instances' classes have to inherit from PolymorphicTypeMeta, so it can be
 * dynamic_casted to specific Meta components
 */
template <class T> inline const PolymorphicTypeMeta TYPE_META = {};

/**
 * @brief A shortcut for defining a TypeMeta class that inherits from multiple Metas,
 * along with PolymorphicTypeMeta
 */
template <class... CompMeta>
struct CompoundTypeMeta : public PolymorphicTypeMeta, public CompMeta...
{
    CompoundTypeMeta(const CompMeta &&...metaInit) : PolymorphicTypeMeta(), CompMeta(metaInit)... {}
};

} // namespace Vitrae

// Include because it includes TypeMetaStd.hpp, which depends on TypeInfo
#include "Vitrae/Dynamic/TypeInfo.hpp"