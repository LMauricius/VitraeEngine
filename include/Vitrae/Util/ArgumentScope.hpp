#pragma once

#include "Vitrae/Util/PropertyAliases.hpp"
#include "Vitrae/Util/ScopedDict.hpp"

namespace Vitrae
{

/**
 * @brief ArgumentScope provides access to named variants from a ScopedDict,
 * but with support for PropertyAliases for aliased properties
 */
class ArgumentScope
{
    ScopedDict *mp_scope;
    const PropertyAliases *mp_propertySelection;

  public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty ArgumentScope without a parent.
     */
    ArgumentScope();

    /**
     * @brief Constructor with a ScopedDict.
     *
     * Creates a ArgumentScope with the specified scope and aliases.
     *
     * @param scope Pointer to the ScopedDict.
     */
    ArgumentScope(ScopedDict *p_scope, const PropertyAliases *propertySelection = nullptr);

    /// @brief Copy constructor.
    ArgumentScope(const ArgumentScope &) = default;

    /// @brief Move constructor.
    ArgumentScope(ArgumentScope &&) = default;

    /// @brief copy assignment
    ArgumentScope &operator=(const ArgumentScope &) = default;

    /// @brief move assignment
    ArgumentScope &operator=(ArgumentScope &&) = default;

    /**
     * @brief Set a key-value pair in the dictionary.
     *
     * If the key already exists in the dictionary, the corresponding value is updated.
     *
     * @param key The key.
     * @param value The value.
     */
    void set(StringId key, const Variant &value);

    /**
     * @brief Set a key-value pair in the dictionary using move semantics.
     *
     * If the key already exists in the dictionary, the corresponding value is updated.
     *
     * @param key The key.
     * @param value The value, moved into the dictionary.
     */
    void set(StringId key, Variant &&value);

    /**
     * @brief Get the value associated with a key.
     *
     * If the key is not found in the current dictionary, the search continues in the parent
     * dictionary.
     *
     * @param key The key.
     * @return The value associated with the key.
     * @throws std::runtime_error If the key is not found in any dictionary.
     */
    const Variant &get(StringId key) const;
};
} // namespace Vitrae