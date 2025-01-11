#pragma once

#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Util/StableMap.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace Vitrae {

/**
 * A class that encompasses mapping between required properties and more specific
 * properties that we choose as providers of the functionality
 * The required property is called a proxy
 * The selected property is called a provider
 * An alias is one direct mapping between a proxy and a provider
 * The collection of all aliases is called a selection
 * Providers can also be proxies to other providers
 * No cycles are allowed
 */
class ParamAliases
{
  public:
    /**
     * Default constructor
     */
    ParamAliases();

    /**
     * Copy constructor
     */
    ParamAliases(const ParamAliases &other) = default;

    /**
     * Move constructor
     */
    ParamAliases(ParamAliases &&other) = default;

    /**
     * Constructor for parentless alias mapping
     * @param aliases A map of aliases; key = proxy, value = provider
     * (choice)
     */
    ParamAliases(const StableMap<StringId, String> &aliases);
    ParamAliases(std::initializer_list<std::pair<StringId, String>> aliases);

    ParamAliases(StableMap<StringId, String> &&aliases);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent ParamAliases
     * @warning The parent pointers are non-owning, and MUST exist until this object's destruction
     */
    ParamAliases(std::span<const ParamAliases *const> parentPtrs);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent ParamAliases
     * @param aliases A map of aliases; key = proxy, value = provider
     * (choice)
     * @warning The parent pointers are non-owning, and MUST exist until this object's destruction
     */
    ParamAliases(std::span<const ParamAliases *const> parentPtrs,
                 const StableMap<StringId, String> &aliases);

    ParamAliases(std::span<const ParamAliases *const> parentPtrs,
                 StableMap<StringId, String> &&aliases);

    /**
     * Copy assignment
     */
    ParamAliases &operator=(const ParamAliases &other) = default;

    /**
     * Move assignment
     */
    ParamAliases &operator=(ParamAliases &&other) = default;

    /**
     * @returns The provider for the specified proxy, or proxy if not found
     * @note supports aliases to aliases
     */
    StringId choiceFor(StringId proxy) const;

    /**
     * @returns The provider for the specified proxy, or proxy if not found;
     * in full string form
     * @note supports aliases to aliases
     */
    String choiceStringFor(String proxy) const;

    /**
     * @returns The directly specified provider for the specified proxy, or empty if not found
     */
    std::optional<StringId> directChoiceFor(StringId proxy) const;

    /**
     * @returns The directly specified provider for the specified proxy, or empty if not found;
     * in full string form
     */
    std::optional<String> directChoiceStringFor(StringId proxy) const;

    /**
     * @returns The has unique for this selection of providers. Order is
     * unimportant, just as the parent-child hierarchy
     */
    inline std::size_t hash() const { return m_hash; }

    /**
     * @brief Extracts all aliases used in this selection, including parents
     * @note The map is expected to not have any aliases of this selection already
     * @param aliases A map of aliases using Strings for providers; key = proxy, value = provider
     */
    void extractAliasStrings(std::unordered_map<StringId, String> &aliases) const;

    /**
     * @brief Extracts all aliases used in this selection, including parents
     * @note The map is expected to not have any aliases of this selection already
     * @param aliases A map of aliases using StringIds for providers; key = proxy, value = provider
     */
    void extractAliasNameIds(std::unordered_map<StringId, StringId> &aliases) const;

    /**
     * @brief Extracts all aliases proxys used in this selection, including parents
     * @param proxys A set of aliases proxys
     */
    void extractAliasProxyIds(std::unordered_set<StringId> &proxys) const;

  private:
    std::vector<const ParamAliases *> m_parentPtrs;
    StableMap<StringId, std::pair<StringId, String>> m_localAliases;
    std::size_t m_hash;
};

} // namespace Vitrae

namespace std {

template <> struct hash<Vitrae::ParamAliases>
{
    std::size_t operator()(const Vitrae::ParamAliases &selection) const { return selection.hash(); }
};
}