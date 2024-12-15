#pragma once

#include "Vitrae/Util/ArgumentScope.hpp"
#include "Vitrae/Util/Hashing.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Object that either contains a fixed value, or references a property by its nameid.
 * It can dynamically retrieve the value if referenced by the name from the surrounding scope.
 */
template <class T> class PropertyGetter
{
    struct DynamicSpec
    {
        StringId nameId;
        String name;
    };

    std::variant<DynamicSpec, T> m_nameOrValue;

  public:
    PropertyGetter(String name) : m_nameOrValue(std::in_place_index<0>, DynamicSpec{name, name}) {}
    PropertyGetter(const T &value) : m_nameOrValue(std::in_place_index<1>, value) {}
    PropertyGetter(T &&value) : m_nameOrValue(std::in_place_index<1>, std::move(value)) {}

    PropertyGetter &operator=(String name)
    {
        m_nameOrValue.template emplace<0>(DynamicSpec{name, name});
        return *this;
    }
    PropertyGetter &operator=(const T &value)
    {
        m_nameOrValue.template emplace<1>(value);
        return *this;
    }

    T get(const ScopedDict &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            return scope.get(std::get<0>(m_nameOrValue).nameId).template get<T>();
        case 1:
        default:
            return std::get<1>(m_nameOrValue);
        }
    }

    T *getPtr(const ScopedDict &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            const Variant *p = scope.getPtr(std::get<0>(m_nameOrValue).nameId);
            if (p)
                return &(p->get<T>());
            else
                return nullptr;
        case 1:
        default:
            return &(std::get<1>(m_nameOrValue));
        }
    }

    T get(const ArgumentScope &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            return scope.get(std::get<0>(m_nameOrValue).nameId).template get<T>();
        case 1:
        default:
            return std::get<1>(m_nameOrValue);
        }
    }

    T *getPtr(const ArgumentScope &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            const Variant *p = scope.getPtr(std::get<0>(m_nameOrValue).nameId);
            if (p)
                return &(p->get<T>());
            else
                return nullptr;
        case 1:
        default:
            return &(std::get<1>(m_nameOrValue));
        }
    }

    /**
     * @returns true if the property has a fixed value
     */
    bool isFixed() const { return m_nameOrValue.index() == 1; }

    /**
     * @returns PropertySpec for a dynamic property
     * @note throws std::bad_variant_access if isFixed()
     */
    PropertySpec getSpec() const
    {
        return PropertySpec{.name = std::get<0>(m_nameOrValue).name,
                            .typeInfo = Variant::getTypeInfo<T>()};
    }

    /**
     * @returns PropertySpec for a dynamic property
     * @note throws std::bad_variant_access if not isFixed()
     */
    const T &getFixedValue() const { return std::get<1>(m_nameOrValue); }
};

} // namespace Vitrae

namespace std
{
template <class T> struct hash<Vitrae::PropertyGetter<T>>
{
    size_t operator()(const Vitrae::PropertyGetter<T> &p) const
    {
        if (p.isFixed())
            return Vitrae::combinedHashes<2>({{0, hash<T>()(p.getFixedValue())}});
        else
            return Vitrae::combinedHashes<2>({{1, hash<Vitrae::StringId>{}(p.getSpec().name)}});
    }
};
} // namespace std