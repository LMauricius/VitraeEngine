#include "Vitrae/Dynamic/VariantScope.hpp"

#include <stdexcept>

namespace Vitrae
{

VariantScope::VariantScope() : m_parent{nullptr} {}
VariantScope::VariantScope(const VariantScope *parent) : m_parent{parent} {}

void VariantScope::set(StringId key, const Variant &value)
{
    m_dict[key] = value;
}

void VariantScope::set(StringId key, Variant &&value)
{
    m_dict[key] = std::move(value);
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
    return m_dict.find(key) != m_dict.end() || (m_parent && m_parent->has(key));
}

void VariantScope::clear()
{
    m_dict.clear();
}

} // namespace Vitrae