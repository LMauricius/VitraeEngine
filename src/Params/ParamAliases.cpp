#include "Vitrae/Params/ParamAliases.hpp"

#include "Vitrae/Util/Hashing.hpp"

namespace Vitrae {

ParamAliases::ParamAliases() : m_parentPtrs{}, m_hash(0) {}

namespace
{
StableMap<StringId, std::pair<StringId, String>> convertAliases(
    const StableMap<StringId, String> &aliases)
{
    StableMap<StringId, std::pair<StringId, String>> result;
    for (const auto &[key, value] : aliases) {
        if (key != value)
            result.emplace(key, std::make_pair<StringId, String>(StringId(value), String(value)));
    }
    return result;
}

} // namespace

ParamAliases::ParamAliases(const StableMap<StringId, String> &aliases)
    : m_parentPtrs{}, m_localAliases(convertAliases(aliases)), m_hash(0)
{
    for (const auto &[key, value] : m_localAliases) {
        m_hash ^=
            combinedHashes<2>({{std::hash<StringId>{}(key), std::hash<StringId>{}(value.first)}});
    }
}

ParamAliases::ParamAliases(std::initializer_list<std::pair<StringId, String>> aliases)
    : ParamAliases(StableMap<StringId, String>(aliases))
{}

ParamAliases::ParamAliases(StableMap<StringId, String> &&aliases)
    : m_parentPtrs{}, m_localAliases(convertAliases(aliases)), m_hash(0)
{
    for (const auto &[key, value] : m_localAliases) {
        m_hash ^=
            combinedHashes<2>({{std::hash<StringId>{}(key), std::hash<StringId>{}(value.first)}});
    }
}

ParamAliases::ParamAliases(std::span<const ParamAliases *const> parentPtrs)
    : m_parentPtrs(parentPtrs.begin(), parentPtrs.end()), m_hash(0)
{
    for (const auto *p_parent : parentPtrs) {
        m_hash ^= p_parent->hash();
    }
}

ParamAliases::ParamAliases(std::span<const ParamAliases *const> parentPtrs,
                           const StableMap<StringId, String> &aliases)
    : m_parentPtrs(parentPtrs.begin(), parentPtrs.end()), m_localAliases(convertAliases(aliases)),
      m_hash(0)
{
    for (const auto *p_parent : parentPtrs) {
        m_hash ^= p_parent->hash();
    }

    for (const auto &[key, value] : m_localAliases) {
        m_hash ^=
            combinedHashes<2>({{std::hash<StringId>{}(key), std::hash<StringId>{}(value.first)}});
    }
}

ParamAliases::ParamAliases(std::span<const ParamAliases *const> parentPtrs,
                           StableMap<StringId, String> &&aliases)
    : m_parentPtrs(parentPtrs.begin(), parentPtrs.end()), m_localAliases(convertAliases(aliases)),
      m_hash(0)
{
    for (const auto *p_parent : parentPtrs) {
        m_hash ^= p_parent->hash();
    }

    for (const auto &[key, value] : m_localAliases) {
        m_hash ^=
            combinedHashes<2>({{std::hash<StringId>{}(key), std::hash<StringId>{}(value.first)}});
    }
}

StringId ParamAliases::choiceFor(StringId target) const
{
    StringId choice = target;

    std::optional<StringId> optNextChoice;
    while ((optNextChoice = directChoiceFor(choice)).has_value()) {
        choice = optNextChoice.value();
    }

    return choice;
}

String ParamAliases::choiceStringFor(String target) const
{
    String choice = target;

    std::optional<String> optNextChoice;
    while ((optNextChoice = directChoiceStringFor(choice)).has_value()) {
        choice = optNextChoice.value();
    }

    return choice;
}

std::optional<StringId> ParamAliases::directChoiceFor(StringId key) const
{
    auto it = m_localAliases.find(key);
    if (it != m_localAliases.end()) {
        return (*it).second.first;
    } else {
        for (auto p_parent : m_parentPtrs) {
            auto c = p_parent->directChoiceFor(key);
            if (c.has_value()) {
                return c;
            }
        }
        return {};
    }
}

std::optional<String> ParamAliases::directChoiceStringFor(StringId key) const
{
    auto it = m_localAliases.find(key);
    if (it != m_localAliases.end()) {
        return (*it).second.second;
    } else {
        for (auto p_parent : m_parentPtrs) {
            auto c = p_parent->directChoiceStringFor(key);
            if (c.has_value()) {
                return c;
            }
        }
        return {};
    }
}

void ParamAliases::extractAliasStrings(std::unordered_map<StringId, String> &aliases) const
{
    std::unordered_set<StringId> targets;
    extractAliasProxyIds(targets);

    for (const auto &target : targets) {
        std::optional<String> optChoiceString = directChoiceStringFor(target);
        if (optChoiceString.has_value()) {
            String choiceString = optChoiceString.value();
            while ((optChoiceString = directChoiceStringFor(choiceString)).has_value()) {
                choiceString = optChoiceString.value();
            }
            aliases.emplace(target, choiceString);
        }
    }
}

void ParamAliases::extractAliasNameIds(std::unordered_map<StringId, StringId> &aliases) const
{
    std::unordered_set<StringId> targets;
    extractAliasProxyIds(targets);

    for (const auto &target : targets) {
        std::optional<StringId> optChoice = directChoiceFor(target);
        if (optChoice.has_value()) {
            StringId choice = optChoice.value();
            while ((optChoice = directChoiceFor(choice)).has_value()) {
                choice = optChoice.value();
            }
            aliases.emplace(target, choice);
        }
    }
}

void ParamAliases::extractAliasProxyIds(std::unordered_set<StringId> &targets) const
{
    for (const auto &[key, value] : m_localAliases) {
        targets.insert(key);
    }

    for (auto p_parent : m_parentPtrs) {
        p_parent->extractAliasProxyIds(targets);
    }
}

} // namespace Vitrae