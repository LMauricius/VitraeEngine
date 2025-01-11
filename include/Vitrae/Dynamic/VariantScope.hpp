#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Dynamic/Variant.hpp"

#include <map>

namespace Vitrae
{
/**
 * @brief VariantScope class provides a dictionary-like data structure with a parent-child
 * relationship.
 *
 * The VariantScope class allows you to set key-value pairs and retrieve values using keys. It also
 * supports checking if a key exists in the dictionary. The dictionary can have a parent
 * VariantScope, which allows for inheritance of values. If a key is not found in the current
 * dictionary, the search continues in the parent dictionary.
 */
class VariantScope
{
    const VariantScope *m_parent;
    StableMap<StringId, Variant> m_dict;

  public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty VariantScope without a parent.
     */
    VariantScope();

    /**
     * @brief Constructor with a parent VariantScope.
     *
     * Creates a VariantScope with the specified parent.
     *
     * @param parent Pointer to the parent VariantScope.
     */
    VariantScope(const VariantScope *parent);

    /// @brief Copy constructor.
    VariantScope(const VariantScope &) = default;

    /// @brief Move constructor.
    VariantScope(VariantScope &&) = default;

    /// @brief copy assignment
    VariantScope &operator=(const VariantScope &) = default;

    /// @brief move assignment
    VariantScope &operator=(VariantScope &&) = default;

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
     * @brief Get the value associated with a key.
     *
     * If the key is not found in the current dictionary, the search continues in the parent
     * dictionary.
     *
     * @param key The key.
     * @return The value associated with the key, or nullptr if the key is not found.
     * @throws nothing
     */
    const Variant *getPtr(StringId key) const;

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
     * @brief Erases all keys and values from the dictionary.
     */
    void clear();
};
} // namespace Vitrae