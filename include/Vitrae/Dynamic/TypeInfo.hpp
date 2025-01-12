#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <typeinfo>

namespace Vitrae
{

class Variant;

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

    inline std::string_view getShortTypeName() const { return shortTypeNameGetter(); }

    // comparisons (just compare type_info)
    inline bool operator==(const TypeInfo &o) const { return *p_id == *o.p_id; }
    inline bool operator!=(const TypeInfo &o) const { return *p_id != *o.p_id; }
    inline bool operator<(const TypeInfo &o) const { return p_id->before(*o.p_id); }
    inline bool operator>(const TypeInfo &o) const { return !operator<=(o); }
    inline bool operator<=(const TypeInfo &o) const { return operator<(o) || operator==(o); }
    inline bool operator>=(const TypeInfo &o) const { return !operator<(o); }

    // Getter
    template <typename T> static consteval TypeInfo construct()
    {
        TypeInfo info;
        info.p_id = &typeid(T);
        if constexpr (!std::same_as<T, void>) {
            info.size = sizeof(T);
            info.alignment = alignof(T);
        } else {
            info.size = 0;
            info.alignment = 0;
        }
        info.shortTypeNameGetter = getShortTypeName<T>;
        return info;
    }

  protected:
    TypeInfo() = default;
    TypeInfo(const TypeInfo &) = default;
    TypeInfo(TypeInfo &&) = default;

    TypeInfo &operator=(const TypeInfo &) = default;
    TypeInfo &operator=(TypeInfo &&) = default;

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

template <typename T> constexpr TypeInfo TYPE_INFO = TypeInfo::construct<T>();

} // namespace Vitrae