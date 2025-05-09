#pragma once

#include "Vitrae/Dynamic/TypeMeta.hpp"
#include "Vitrae/TypeConversion/VectorCvt.hpp"

#include <concepts>
#include <string>
#include <string_view>
#include <typeinfo>

namespace Vitrae
{

class Variant;
class TypeInfo;

/**
 * A structure that contains type info and function pointers to compare and convert properties.
 * Used to implement type-specific functionality in a generic way.
 * Similar to a vtable, except for Variant type
 */
class TypeInfo
{
  public:
    const std::type_info *p_id;
    std::size_t size;
    std::size_t alignment;

    /**
     * A TypeMeta for this type.
     * It can be used to test the functionality of the type, retrieve metadata and perform
     * operations on instances of it, by dynamic_casting this value to desired Meta.
     */
    const PolymorphicTypeMeta &metaDetail;

    inline std::string_view getShortTypeName() const { return shortTypeNameGetter(); }

    // comparisons (just compare type_info)
    inline bool operator==(const TypeInfo &o) const { return *p_id == *o.p_id; }
    inline std::weak_ordering operator<=>(const TypeInfo &o) const
    {
        if (*p_id == *o.p_id) {
            return std::weak_ordering::equivalent;
        } else if (p_id->before(*o.p_id)) {
            return std::weak_ordering::less;
        } else {
            return std::weak_ordering::greater;
        }
    }

    // Getter
    template <typename T> static consteval TypeInfo construct()
    {
        const std::type_info *p_id = &typeid(T);

        std::size_t size, alignment;
        if constexpr (!std::same_as<T, void>) {
            size = sizeof(T);
            alignment = alignof(T);
        } else {
            size = 0;
            alignment = 0;
        }

        std::string_view (*shortTypeNameGetter)() = getShortTypeName<T>;

        return TypeInfo(p_id, size, alignment, TYPE_META<T>, shortTypeNameGetter);
    }

    template <typename T> static constexpr const TypeInfo GLOBAL_TYPE_INFO = construct<T>();

  protected:
    TypeInfo() = default;
    TypeInfo(const TypeInfo &) = default;
    TypeInfo(TypeInfo &&) = default;

    TypeInfo &operator=(const TypeInfo &) = default;
    TypeInfo &operator=(TypeInfo &&) = default;

    constexpr TypeInfo(const std::type_info *p_id, std::size_t size, std::size_t alignment,
                       const PolymorphicTypeMeta &metaDetail,
                       std::string_view (*shortTypeNameGetter)())
        : p_id(p_id), size(size), alignment(alignment), metaDetail(metaDetail),
          shortTypeNameGetter(shortTypeNameGetter)
    {}

    // meta
    std::string_view (*shortTypeNameGetter)();
    // utilities
    static std::string constructShortTypeName(const std::type_info *p_id);
    template <class T> static std::string_view getShortTypeName()
    {
        static std::string name = constructShortTypeName(&typeid(T));
        return name;
    }
};

template <typename T> constexpr const TypeInfo &TYPE_INFO = TypeInfo::GLOBAL_TYPE_INFO<T>;

} // namespace Vitrae

// Include so that we can't get TypeMeta of types for which it is defined in this header
#include "Vitrae/Dynamic/TypeMetaStd.hpp"