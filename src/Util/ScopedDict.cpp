#include "Vitrae/Util/ScopedDict.hpp"

#include <stdexcept>

namespace Vitrae
{

ScopedDict::ScopedDict() : m_parent{nullptr} {}
ScopedDict::ScopedDict(const ScopedDict *parent) : m_parent{parent} {}

void ScopedDict::set(StringId key, const Variant &value)
{
    m_dict[key] = value;
}

void ScopedDict::set(StringId key, Variant &&value)
{
    m_dict[key] = std::move(value);
}

const Variant &ScopedDict::get(StringId key) const
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return (*it).second;

    if (m_parent)
        return m_parent->get(key);

    throw std::runtime_error{"Key not found"};
}

const Variant *ScopedDict::getPtr(StringId key) const
{
    auto it = m_dict.find(key);
    if (it != m_dict.end())
        return &((*it).second);

    if (m_parent)
        return m_parent->getPtr(key);

    return nullptr;
}

bool ScopedDict::has(StringId key) const
{
    return m_dict.find(key) != m_dict.end() || (m_parent && m_parent->has(key));
}

void ScopedDict::clear()
{
    m_dict.clear();
}

} // namespace Vitrae