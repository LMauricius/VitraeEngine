#pragma once

#include "Vitrae/Dynamic/VariantScope.hpp"
#include "Vitrae/Params/ParamAliases.hpp"

namespace Vitrae
{

/**
 * @brief ArgumentScope provides access to named variants from a VariantScope,
 * but with support for ParamAliases for aliased properties
 */
class ArgumentScope
{
    VariantScope *mp_scope;
    const ParamAliases *mp_propertySelection;

  public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty ArgumentScope without a parent.
     */
    ArgumentScope();

    /**
     * @brief Constructor with a VariantScope.
     *
     * Creates a ArgumentScope with the specified scope and aliases.
     *
     * @param scope Pointer to the VariantScope.
     */
    ArgumentScope(VariantScope *p_scope, const ParamAliases *propertySelection = nullptr);

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

    /**
     * @brief Get a move-reference to the value associated with a key.
     *
     * If the key is not found in the current dictionary, the search continues in the parent
     * dictionary. The parent value will then be copied.
     *
     * @note The key is not removed from a dictionary, but the Variant can become void
     *
     * @param key The key.
     * @return A move-reference to the value associated with the key.
     * @throws std::runtime_error If the key is not found in any dictionary.
     */
    Variant move(StringId key);

    /**
     * @brief Check if a key exists in the dictionary.
     *
     * If the key is not found in the current dictionary, the search continues in the parent
     * dictionary.
     *
     * @param key The key.
     * @return True if the key exists, false otherwise.
     */
    bool has(StringId key) const;

    /**
     * @return The underlying VariantScope
     */
    inline VariantScope &getUnaliasedScope() { return *mp_scope; }
    inline const VariantScope &getUnaliasedScope() const { return *mp_scope; }
};
} // namespace Vitrae