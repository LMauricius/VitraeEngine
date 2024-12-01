#include "Vitrae/Util/PropertySelection.hpp"

#include "Vitrae/Util/Hashing.hpp"

namespace Vitrae {

PropertySelection::PropertySelection() : mp_parent(nullptr), m_hash(0) {}

PropertySelection::PropertySelection(StableMap<StringId, StringId> aliases)
    : mp_parent(nullptr), m_localAliases(std::move(aliases)), m_hash(0) {

    for (const auto &[key, value] : m_localAliases) {
        m_hash ^= combinedHashes<2>(
            {{std::hash<StringId>{}(key), std::hash<StringId>{}(value)}});
    }
}

PropertySelection::PropertySelection(const PropertySelection &parent,
                                     StableMap<StringId, StringId> aliases)
    : mp_parent(&parent), m_localAliases(std::move(aliases)),
      m_hash(parent.m_hash) {

    for (const auto &[key, value] : m_localAliases) {
        m_hash ^= combinedHashes<2>(
            {{std::hash<StringId>{}(key), std::hash<StringId>{}(value)}});
    }
}

std::optional<StringId> PropertySelection::choiceFor(StringId key) const {
    auto it = m_localAliases.find(key);
    if (it != m_localAliases.end()) {
        return (*it).second;
    } else if (mp_parent) {
        return mp_parent->choiceFor(key);
    } else {
        return {};
    }
}

} // namespace Vitrae