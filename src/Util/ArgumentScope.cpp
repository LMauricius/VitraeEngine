#include "Vitrae/Util/ArgumentScope.hpp"

namespace Vitrae
{

ArgumentScope::ArgumentScope() : mp_scope(nullptr), mp_propertySelection(nullptr) {}

ArgumentScope::ArgumentScope(const ScopedDict *scope, const PropertySelection *propertySelection)
    : m_parent(scope), mp_propertySelection(propertySelection)
{}

void ArgumentScope::set(StringId key, const Variant &value)
{
    StringId actualKey = mp_propertySelection->choiceFor(key).value_or(key);
    mp_scope->set(actualKey, value);
}

void ArgumentScope::set(StringId key, Variant &&value)
{
    StringId actualKey = mp_propertySelection->choiceFor(key).value_or(key);
    mp_scope->set(actualKey, std::move(value));
}

const Variant &ArgumentScope::get(StringId key) const
{
    StringId actualKey = mp_propertySelection->choiceFor(key).value_or(key);
    return mp_scope->get(actualKey);
}
} // namespace Vitrae