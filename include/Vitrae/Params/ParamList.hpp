#pragma once

#include "Vitrae/Containers/StableMap.hpp"
#include "Vitrae/Data/StringId.hpp"
#include "Vitrae/Params/ParamSpec.hpp"
#include "Vitrae/Util/Hashing.hpp"

#include "dynasma/util/dynamic_typing.hpp"

#include <map>
#include <span>
#include <vector>

namespace Vitrae
{

class ParamList : public dynasma::PolymorphicBase
{
    friend struct std::hash<ParamList>;

    StableMap<StringId, ParamSpec> m_mappedSpecs;
    std::vector<StringId> m_specNameIds;
    std::vector<ParamSpec> m_specList;

    std::size_t m_hash;

  public:
    inline ParamList() : m_hash(0) {}
    ParamList(ParamList const &) = default;
    ParamList(ParamList &&) = default;

    inline ParamList(std::initializer_list<const ParamSpec> specs)
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
    ParamList(const ContainerT &specs)
        requires(std::ranges::range<ContainerT> &&
                 std::convertible_to<std::ranges::range_value_t<ContainerT>, const ParamSpec &>)
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

    inline ParamList(const StableMap<StringId, ParamSpec> &mappedSpecs) : m_mappedSpecs(mappedSpecs)
    {
        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    inline ParamList(StableMap<StringId, ParamSpec> &&mappedSpecs)
        : m_mappedSpecs(std::move(mappedSpecs))
    {
        for (auto [nameId, spec] : m_mappedSpecs) {
            m_specNameIds.push_back(nameId);
            m_specList.push_back(spec);
        }

        recalculateHash();
    }

    virtual ~ParamList() = default;

    ParamList &operator=(const ParamList &other)
    {
        m_mappedSpecs = other.m_mappedSpecs;
        m_specNameIds = other.m_specNameIds;
        m_specList = std::vector<ParamSpec>(other.m_specList.begin(), other.m_specList.end());
        m_hash = other.m_hash;
        return *this;
    }
    ParamList &operator=(ParamList &&other) = default;

    /**
     * Inserts specs from other at the end of this list, if they are not already in the list
     */
    std::size_t merge(const ParamList &other)
    {
        std::size_t count = 0;
        for (const auto &spec : other.m_specList) {
            if (m_mappedSpecs.find(spec.name) == m_mappedSpecs.end()) {
                m_mappedSpecs.emplace(spec.name, spec);
                m_specNameIds.push_back(spec.name);
                m_specList.push_back(spec);
                ++count;
            }
        }

        recalculateHash();

        return count;
    }

    /**
     * Inserts a spec at the end of the list if it is not already in the list
     */
    void insert_back(const ParamSpec &spec)
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
    void erase(const ParamSpec &spec) { erase(spec.name); }

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

            // we need a new vector to avoid calling std::vector::erase (ParamSpec is not
            // move-assignable)
            std::vector<ParamSpec> newSpecList;
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

    inline const StableMap<StringId, ParamSpec> &getMappedSpecs() const { return m_mappedSpecs; }

    inline std::span<const StringId> getSpecNameIds() const { return m_specNameIds; }

    inline std::span<const ParamSpec> getSpecList() const { return m_specList; }

    inline std::size_t getHash() const { return m_hash; }

    /*
    Info
    */

    inline std::size_t count() const { return m_specList.size(); }

    inline bool contains(StringId nameId) const
    {
        return m_mappedSpecs.find(nameId) != m_mappedSpecs.end();
    }

    /*
    Comparisons
    */

    bool operator==(const ParamList &other) const { return m_hash == other.m_hash; }
    auto operator<=>(const ParamList &other) const { return m_hash <=> other.m_hash; }

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

inline const ParamList EMPTY_PROPERTY_LIST{};

} // namespace Vitrae

namespace std
{

template <> struct hash<Vitrae::ParamList>
{
    std::size_t operator()(const Vitrae::ParamList &pl) const { return pl.m_hash; }
};
} // namespace std