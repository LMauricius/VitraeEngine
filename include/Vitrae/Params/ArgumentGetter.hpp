#pragma once

#include "Vitrae/Dynamic/ArgumentScope.hpp"
#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "Vitrae/Util/Hashing.hpp"

#include <variant>

namespace Vitrae
{

/**
 * Object that either contains a fixed value, or references a property by its nameid.
 * It can dynamically retrieve the value if referenced by the name from the surrounding scope.
 */
template <class T> class ArgumentGetter
{
    struct DynamicSpec
    {
        StringId nameId;
        ParamSpec spec;
    };

    std::variant<DynamicSpec, T> m_nameOrValue;

  public:
    ArgumentGetter(String name)
        : m_nameOrValue(std::in_place_index<0>, DynamicSpec{name, {name, TYPE_INFO<T>}})
    {}
    ArgumentGetter(String name, const T &defaultValue)
        : m_nameOrValue(std::in_place_index<0>, DynamicSpec{name,
                                                            {
                                                                .name = name,
                                                                .typeInfo = TYPE_INFO<T>,
                                                                .defaultValue = defaultValue,
                                                            }})
    {}
    ArgumentGetter(const T &value) : m_nameOrValue(std::in_place_index<1>, value) {}
    ArgumentGetter(T &&value) : m_nameOrValue(std::in_place_index<1>, std::move(value)) {}

    ArgumentGetter &operator=(String name)
    {
        m_nameOrValue.template emplace<0>(DynamicSpec{name, name});
        return *this;
    }
    ArgumentGetter &operator=(const T &value)
    {
        m_nameOrValue.template emplace<1>(value);
        return *this;
    }

    T get(const VariantScope &scope) const
    {
        switch (m_nameOrValue.index()) {
        case 0:
            return scope.get(std::get<0>(m_nameOrValue).nameId).template get<T>();
        case 1:
        default:
            return std::get<1>(m_nameOrValue);
        }
    }

    T *getPtr(const VariantScope &scope) const
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

    /**
     * @returns true if the property has a fixed value
     */
    bool isFixed() const { return m_nameOrValue.index() == 1; }

    /**
     * @returns ParamSpec for a dynamic property
     * @note throws std::bad_variant_access if isFixed()
     */
    const ParamSpec &getSpec() const { return std::get<0>(m_nameOrValue).spec; }

    /**
     * @returns ParamSpec for a dynamic property
     * @note throws std::bad_variant_access if not isFixed()
     */
    const T &getFixedValue() const { return std::get<1>(m_nameOrValue); }
};

} // namespace Vitrae

namespace std
{
template <class T> struct hash<Vitrae::ArgumentGetter<T>>
{
    size_t operator()(const Vitrae::ArgumentGetter<T> &p) const
    {
        if (p.isFixed())
            return Vitrae::combinedHashes<2>({{0, hash<T>()(p.getFixedValue())}});
        else
            return Vitrae::combinedHashes<2>({{1, hash<Vitrae::StringId>{}(p.getSpec().name)}});
    }
};
} // namespace std