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

    ParamList(std::initializer_list<const ParamSpec> specs);
    template <class ContainerT>
    ParamList(const ContainerT &specs)
        requires(std::ranges::range<ContainerT> &&
                 std::convertible_to<std::ranges::range_value_t<ContainerT>, const ParamSpec &>)
    {
        for (const auto &spec : specs) {
            m_specNameIds.push_back(spec.name);
            m_specList.push_back(spec);
            m_mappedSpecs.emplace(spec.name, spec);
        }

        recalculateHash();
    }
    ParamList(const StableMap<StringId, ParamSpec> &mappedSpecs);
    ParamList(StableMap<StringId, ParamSpec> &&mappedSpecs);

    virtual ~ParamList() = default;

    ParamList &operator=(const ParamList &other);
    ParamList &operator=(ParamList &&other) = default;

    /**
     * Inserts specs from other at the end of this list, if they are not already in the list
     */
    std::size_t merge(const ParamList &other);

    /**
     * Inserts a spec at the end of the list if it is not already in the list
     */
    void insert_back(const ParamSpec &spec);

    /**
     * Erases the spec from the list if it exists
     */
    void erase(const ParamSpec &spec);

    /**
     * Erases the spec with the given name if it exists
     */
    void erase(const StringId &nameId);

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

    bool contains(StringId nameId) const;

    /*
    Comparisons
    */

    inline bool operator==(const ParamList &other) const { return m_hash == other.m_hash; }
    inline auto operator<=>(const ParamList &other) const { return m_hash <=> other.m_hash; }

  private:
    void recalculateHash();
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