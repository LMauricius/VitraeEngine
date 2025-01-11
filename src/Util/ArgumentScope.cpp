#include "Vitrae/Dynamic/ArgumentScope.hpp"

namespace Vitrae
{

ArgumentScope::ArgumentScope() : mp_scope(nullptr), mp_propertySelection(nullptr) {}

ArgumentScope::ArgumentScope(VariantScope *scope, const ParamAliases *propertySelection)
    : mp_scope(scope), mp_propertySelection(propertySelection)
{}

void ArgumentScope::set(StringId key, const Variant &value)
{
    StringId actualKey = mp_propertySelection->choiceFor(key);
    mp_scope->set(actualKey, value);
}

void ArgumentScope::set(StringId key, Variant &&value)
{
    StringId actualKey = mp_propertySelection->choiceFor(key);
    mp_scope->set(actualKey, std::move(value));
}

const Variant &ArgumentScope::get(StringId key) const
{
    StringId actualKey = mp_propertySelection->choiceFor(key);
    return mp_scope->get(actualKey);
}
Variant ArgumentScope::move(StringId key)
{
    StringId actualKey = mp_propertySelection->choiceFor(key);
    return mp_scope->move(actualKey);
}
bool ArgumentScope::has(StringId key) const
{
    StringId actualKey = mp_propertySelection->choiceFor(key);
    return mp_scope->has(actualKey);
}
} // namespace Vitrae