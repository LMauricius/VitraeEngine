#pragma once

#include "Vitrae/Util/Hashing.hpp"
#include "Vitrae/Util/StableMap.hpp"
#include "Vitrae/Util/StringId.hpp"
#include "Vitrae/Util/PropertySpec.hpp"

#include "dynasma/util/dynamic_typing.hpp"

#include <map>
#include <span>
#include <vector>

namespace Vitrae
{

class PropertyList : public dynasma::PolymorphicBase
{
    friend struct std::hash<PropertyList>;

    StableMap<StringId, PropertySpec> m_mappedSpecs;
    std::vector<StringId> m_specNameIds;
    std::vector<PropertySpec> m_specList;

    std::size_t m_hash;

  public:
    inline PropertyList() : m_hash(0) {}
    PropertyList(PropertyList const &) = default;
    PropertyList(PropertyList &&) = default;

    inline PropertyList(std::initializer_list<const PropertySpec> specs)
    {
        for (const auto &spec : specs) {
            m_mappedSpecs.emplace(spec.name, spec);
        }

        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    template <class ContainerT>
    PropertyList(const ContainerT &specs)
        requires(std::ranges::range<ContainerT> &&
                 std::convertible_to<std::ranges::range_value_t<ContainerT>, const PropertySpec &>)
    {
        for (const auto &spec : specs) {
            m_mappedSpecs.emplace(spec.name, spec);
        }

        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    inline PropertyList(const StableMap<StringId, PropertySpec> &mappedSpecs)
        : m_mappedSpecs(mappedSpecs)
    {
        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    inline PropertyList(StableMap<StringId, PropertySpec> &&mappedSpecs)
        : m_mappedSpecs(std::move(mappedSpecs))
    {
        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    virtual ~PropertyList() = default;

    PropertyList &operator=(const PropertyList &other) = default;
    PropertyList &operator=(PropertyList &&other) = default;

    /**
     * Inserts specs from other at the end of this list, if they are not already in the list
     */
    void merge(const PropertyList &other)
    {
        for (const auto &spec : other.m_specList) {
            if (m_mappedSpecs.find(spec.name) == m_mappedSpecs.end()) {
                m_mappedSpecs.emplace(spec.name, spec);
                m_specNameIds.push_back(spec.name);
                m_specList.push_back(spec);
            }
        }

        recalculateHash();
    }

    /**
     * Inserts a spec at the end of the list if it is not already in the list
     */
    void insert_back(const PropertySpec &spec)
    {
        if (m_mappedSpecs.find(spec.name) == m_mappedSpecs.end()) {
            m_mappedSpecs.emplace(spec.name, spec);
            m_specNameIds.push_back(spec.name);
            m_specList.push_back(spec);

            recalculateHash();
        }
    }

    /**
     * Erases the spec from the list if it exists
     */
    void erase(const PropertySpec &spec) { erase(spec.name); }

    /**
     * Erases the spec with the given name if it exists
     */
    void erase(const StringId &nameId)
    {
        if (m_mappedSpecs.erase(nameId) > 0) {
            m_mappedSpecs.erase(nameId);
            std::size_t ind = std::find(m_specNameIds.begin(), m_specNameIds.end(), nameId) -
                              m_specNameIds.begin();
            m_specNameIds.erase(m_specNameIds.begin() + ind);

            // we need a new vector to avoid calling std::vector::erase (PropertySpec is not
            // move-assignable)
            std::vector<PropertySpec> newSpecList;
            newSpecList.reserve(m_specList.size() - 1);
            for (int i = 0; i < m_specList.size(); i++) {
                if (i != ind) {
                    newSpecList.push_back(m_specList[i]);
                }
            }
            m_specList = std::move(newSpecList);

            recalculateHash();
        }
    }

    /*
    Getters
    */

    inline const StableMap<StringId, PropertySpec> &getMappedSpecs() const { return m_mappedSpecs; }

    inline std::span<const StringId> getSpecNameIds() const { return m_specNameIds; }

    inline std::span<const PropertySpec> getSpecList() const { return m_specList; }

    inline std::size_t getHash() const { return m_hash; }

    /*
    Comparisons
    */

    bool operator==(const PropertyList &other) const { return m_hash == other.m_hash; }
    bool operator!=(const PropertyList &other) const { return m_hash != other.m_hash; }
    bool operator<(const PropertyList &other) const { return m_hash < other.m_hash; }
    bool operator<=(const PropertyList &other) const { return m_hash <= other.m_hash; }
    bool operator>(const PropertyList &other) const { return m_hash > other.m_hash; }
    bool operator>=(const PropertyList &other) const { return m_hash >= other.m_hash; }

  private:
    void recalculateHash()
    {
        m_hash = 0;
        for (int i = 0; i < m_specList.size(); i++) {
            m_hash = combinedHashes<3>({{
                m_hash,
                std::hash<StringId>{}(m_specNameIds[i]),
                m_specList[i].typeInfo.p_id->hash_code(),
            }});
        }
    }
};

inline const PropertyList EMPTY_PROPERTY_LIST{};

} // namespace Vitrae

namespace std
{

template <> struct hash<Vitrae::PropertyList>
{
    std::size_t operator()(const Vitrae::PropertyList &pl) const { return pl.m_hash; }
};
} // namespace std