#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Dynamic/Variant.hpp"

#include <stdexcept>

namespace Vitrae
{

VariantScope::VariantScope() : m_parent{nullptr} {}
VariantScope::VariantScope(const VariantScope *parent) : m_parent{parent} {}

void VariantScope::set(StringId key, const Variant &value)
{
    // value may refer to an element of m_dict (e.g. from get()); operator[] would dangle it
    m_dict.insert_or_assign(key, value);
}

void VariantScope::set(StringId key, Variant &&value)
{
    m_dict.insert_or_assign(key, std::move(value));
}

const Variant &VariantScope::get(StringId key) const
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return (*it).second;

    if (m_parent)
        return m_parent->get(key);

    throw std::runtime_error{"Key not found"};
}

void VariantScope::erase(StringId key)
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        (*it).second.reset();
    else if (m_parent && m_parent->has(key))
        m_dict[key];
    else
        throw std::runtime_error{"Key not found"};
}

const Variant &VariantScope::get(StringId key, const Variant &defaultValue) const
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return (*it).second;

    if (m_parent)
        return m_parent->get(key, defaultValue);

    return defaultValue;
}

Variant VariantScope::move(StringId key)
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return std::move((*it).second);

    if (m_parent) {
        it = m_dict.emplace(key, m_parent->get(key)).first;
        return std::move((*it).second);
    }

    throw std::runtime_error{"Key not found"};
}

const Variant *VariantScope::getPtr(StringId key) const
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return &((*it).second);

    if (m_parent)
        return m_parent->getPtr(key);

    return nullptr;
}

bool VariantScope::has(StringId key) const
{
    auto it = m_dict.find(key);
    return (it != m_dict.end() && (*it).second.hasValue()) || (m_parent && m_parent->has(key));
}

bool VariantScope::hasEverHad(StringId key) const
{
    auto it = m_dict.find(key);
    return it != m_dict.end() || (m_parent && m_parent->hasEverHad(key));
}

void VariantScope::clear()
{
    m_dict.clear();
}

} // namespace Vitrae