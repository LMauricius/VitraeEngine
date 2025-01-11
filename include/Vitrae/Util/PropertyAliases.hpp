#pragma once

#include "Vitrae/Data/Typedefs.hpp"
#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"

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
class PropertyAliases
{
  public:
    /**
     * Default constructor
     */
    PropertyAliases();

    /**
     * Copy constructor
     */
    PropertyAliases(const PropertyAliases &other) = default;

    /**
     * Move constructor
     */
    PropertyAliases(PropertyAliases &&other) = default;

    /**
     * Constructor for parentless alias mapping
     * @param aliases A map of aliases; key = proxy, value = provider
     * (choice)
     */
    PropertyAliases(const StableMap<StringId, String> &aliases);
    PropertyAliases(std::initializer_list<std::pair<StringId, String>> aliases);

    PropertyAliases(StableMap<StringId, String> &&aliases);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent PropertyAliases
     * @warning The parent pointers are non-owning, and MUST exist until this object's destruction
     */
    PropertyAliases(std::span<const PropertyAliases *const> parentPtrs);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent PropertyAliases
     * @param aliases A map of aliases; key = proxy, value = provider
     * (choice)
     * @warning The parent pointers are non-owning, and MUST exist until this object's destruction
     */
    PropertyAliases(std::span<const PropertyAliases *const> parentPtrs,
                    const StableMap<StringId, String> &aliases);

    PropertyAliases(std::span<const PropertyAliases *const> parentPtrs,
                    StableMap<StringId, String> &&aliases);

    /**
     * Copy assignment
     */
    PropertyAliases &operator=(const PropertyAliases &other) = default;

    /**
     * Move assignment
     */
    PropertyAliases &operator=(PropertyAliases &&other) = default;

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
    std::vector<const PropertyAliases *> m_parentPtrs;
    StableMap<StringId, std::pair<StringId, String>> m_localAliases;
    std::size_t m_hash;
};

} // namespace Vitrae

namespace std {

template <> struct hash<Vitrae::PropertyAliases>
{
    std::size_t operator()(const Vitrae::PropertyAliases &selection) const
    {
        return selection.hash();
    }
};
}