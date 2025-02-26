#pragma once

namespace Vitrae
{

/**
 * @brief A polymorphic base class of all TypeMeta classes
 * @note It isn't a pure virtual class, thus it can be instanced as-is
 */
class PolymorphicTypeMeta
{
  public:
    virtual ~PolymorphicTypeMeta() = default;
};

/**
 * A template depending on the type T, for which this TypeMeta decribes properties
 * This class can be specialized for various types so it inherits other Meta component types
 * specific for the type's features
 * @tparam T The type
 * @note All specializations have to inherit from PolymorphicTypeMeta, so it can be dynamic_casted
 * to specific Meta components
 */
template <class T> class TypeMeta : public PolymorphicTypeMeta
{};

/**
 * @brief A global instance of the TypeMeta class for type T
 */
template <class T> constexpr TypeMeta<T> TYPE_META = {};

} // namespace Vitrae

// Include so that we can't get TypeMeta of types for which it is defined in this header
#include "Vitrae/Dynamic/TypeMetaStd.hpp"