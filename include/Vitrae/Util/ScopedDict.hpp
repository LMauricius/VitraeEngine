#pragma once

#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "Vitrae/Util/Variant.hpp"

#include <map>

namespace Vitrae
{
/**
 * @brief ScopedDict class provides a dictionary-like data structure with a parent-child
 * relationship.
 *
 * The ScopedDict class allows you to set key-value pairs and retrieve values using keys. It also
 * supports checking if a key exists in the dictionary. The dictionary can have a parent ScopedDict,
 * which allows for inheritance of values. If a key is not found in the current dictionary, the
 * search continues in the parent dictionary.
 */
class ScopedDict
{
    const ScopedDict *m_parent;
    StableMap<StringId, Variant> m_dict;

  public:
    /**
     * @brief Default constructor.
     *
     * Creates an empty ScopedDict without a parent.
     */
    ScopedDict();

    /**
     * @brief Constructor with a parent ScopedDict.
     *
     * Creates a ScopedDict with the specified parent.
     *
     * @param parent Pointer to the parent ScopedDict.
     */
    ScopedDict(const ScopedDict *parent);

    /// @brief Copy constructor.
    ScopedDict(const ScopedDict &) = default;

    /// @brief Move constructor.
    ScopedDict(ScopedDict &&) = default;

    /// @brief copy assignment
    ScopedDict &operator=(const ScopedDict &) = default;

    /// @brief move assignment
    ScopedDict &operator=(ScopedDict &&) = default;

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
    Variant &&move(StringId key);

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