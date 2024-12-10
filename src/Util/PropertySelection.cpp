#include "Vitrae/Util/PropertySelection.hpp"

#include "Vitrae/Util/Hashing.hpp"

namespace Vitrae {

PropertySelection::PropertySelection() : m_parentPtrs{}, m_hash(0) {}

PropertySelection::PropertySelection(StableMap<StringId, StringId> aliases)
    : m_parentPtrs{}, m_localAliases(std::move(aliases)), m_hash(0)
{
    for (const auto &[key, value] : m_localAliases) {
        m_hash ^= combinedHashes<2>(
            {{std::hash<StringId>{}(key), std::hash<StringId>{}(value)}});
    }
}

PropertySelection::PropertySelection(std::span<const PropertySelection *> parentPtrs,
                                     StableMap<StringId, StringId> aliases)
    : m_parentPtrs(parentPtrs), m_localAliases(std::move(aliases)), m_hash(0)
{
    for (const auto *p_parent : parentPtrs) {
        m_hash ^= p_parent->hash();
    }

    for (const auto &[key, value] : m_localAliases) {
        m_hash ^= combinedHashes<2>(
            {{std::hash<StringId>{}(key), std::hash<StringId>{}(value)}});
    }
}

StringId PropertySelection::choiceFor(StringId target) const
{
    StringId choice = target;

    std::optional<StringId> optNextChoice;
    while ((optNextChoice = directChoiceFor(choice)).has_value()) {
        choice = optNextChoice.value();
    }

    return choice;
}

std::optional<StringId> PropertySelection::directChoiceFor(StringId key) const
{
    auto it = m_localAliases.find(key);
    if (it != m_localAliases.end()) {
        return (*it).second;
    } else if (mp_parent) {
        return mp_parent->directChoiceFor(key);
    } else {
        return {};
    }
}

} // namespace Vitrae