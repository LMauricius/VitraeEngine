#pragma once

#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"

#include <optional>

namespace Vitrae {

/**
 * A class that encompasses mapping between needed properties and more specific
 * properties that we select as aliases
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
     * @param aliases A map of aliases; key = target (to choose), value = source
     * (choice)
     */
    PropertyAliases(const StableMap<StringId, StringId> &aliases);

    PropertyAliases(StableMap<StringId, StringId> &&aliases);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent PropertyAliases
     */
    PropertyAliases(std::span<const PropertyAliases *> parentPtrs);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent PropertyAliases
     * @param aliases A map of aliases; key = target (to choose), value = source
     * (choice)
     */
    PropertyAliases(std::span<const PropertyAliases *> parentPtrs,
                    const StableMap<StringId, StringId> &aliases);

    PropertyAliases(std::span<const PropertyAliases *> parentPtrs,
                    StableMap<StringId, StringId> &&aliases);

    /**
     * Copy assignment
     */
    PropertyAliases &operator=(const PropertyAliases &other) = default;

    /**
     * Move assignment
     */
    PropertyAliases &operator=(PropertyAliases &&other) = default;

    /**
     * @returns The choice for the specified target, or target if not found
     * @note supports aliases to aliases
     */
    StringId choiceFor(StringId target) const;

    /**
     * @returns The directly specified choice for the specified target, or empty if not found
     */
    std::optional<StringId> directChoiceFor(StringId target) const;

    /**
     * @returns The has unique for this selection of choices. Order is
     * unimportant, just as the parent-child hierarchy
     */
    inline std::size_t hash() const { return m_hash; }

  private:
    std::span<const PropertyAliases *> m_parentPtrs;
    const PropertyAliases *mp_parent;
    StableMap<StringId, StringId> m_localAliases;
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