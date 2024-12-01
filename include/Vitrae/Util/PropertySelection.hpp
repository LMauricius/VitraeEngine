#pragma once

#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"

#include <optional>

namespace Vitrae {

/**
 * A class that encompasses mapping between needed properties and more specific
 * properties that we select as aliases
 */
class PropertySelection {
  public:
    /**
     * Default constructor
     */
    PropertySelection();

    /**
     * Copy constructor
     */
    PropertySelection(const PropertySelection &other) = default;

    /**
     * Move constructor
     */
    PropertySelection(PropertySelection &&other) = default;

    /**
     * Constructor for parentless alias mapping
     * @param aliases A map of aliases; key = target (to choose), value = source
     * (choice)
     */
    PropertySelection(StableMap<StringId, StringId> aliases);

    /**
     * Constructor for alias mapping with inheritance
     * @param parent The parent PropertySelection
     * @param aliases A map of aliases; key = target (to choose), value = source
     * (choice)
     */
    PropertySelection(const PropertySelection &parent,
                      StableMap<StringId, StringId> aliases);

    /**
     * Copy assignment
     */
    PropertySelection &operator=(const PropertySelection &other) = default;

    /**
     * Move assignment
     */
    PropertySelection &operator=(PropertySelection &&other) = default;

    /**
     * @returns The choice for the specified target, or empty if not found
     */
    std::optional<StringId> choiceFor(StringId target) const;

    /**
     * @returns The has unique for this selection of choices. Order is
     * unimportant, just as the parent-child hierarchy
     */
    inline std::size_t hash() const { return m_hash; }

  private:
    const PropertySelection *mp_parent;
    StableMap<StringId, StringId> m_localAliases;
    std::size_t m_hash;
};

} // namespace Vitrae

namespace std {

template <> struct hash<Vitrae::PropertySelection> {
    std::size_t operator()(const Vitrae::PropertySelection &selection) const {
        return selection.hash();
    }
};
}