#include "Vitrae/Params/ParamList.hpp"

namespace Vitrae
{

ParamList::ParamList(std::initializer_list<const ParamSpec> specs)
{
    for (const auto &spec : specs) {
        m_specNameIds.push_back(spec.name);
        m_specList.push_back(spec);
        m_mappedSpecs.emplace(spec.name, spec);
    }

    recalculateHash();
}

ParamList::ParamList(const StableMap<StringId, ParamSpec> &mappedSpecs) : m_mappedSpecs(mappedSpecs)
{
    for (auto [nameId, spec] : m_mappedSpecs) {
        m_specNameIds.push_back(nameId);
        m_specList.push_back(spec);
    }

    recalculateHash();
}

ParamList::ParamList(StableMap<StringId, ParamSpec> &&mappedSpecs)
    : m_mappedSpecs(std::move(mappedSpecs))
{
    for (auto [nameId, spec] : m_mappedSpecs) {
        m_specNameIds.push_back(nameId);
        m_specList.push_back(spec);
    }

    recalculateHash();
}

ParamList &ParamList::operator=(const ParamList &other)
{
    m_mappedSpecs = other.m_mappedSpecs;
    m_specNameIds = other.m_specNameIds;
    m_specList = std::vector<ParamSpec>(other.m_specList.begin(), other.m_specList.end());
    m_hash = other.m_hash;
    return *this;
}

std::size_t ParamList::merge(const ParamList &other)
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

void ParamList::insert_back(const ParamSpec &spec)
{
    if (m_mappedSpecs.find(spec.name) == m_mappedSpecs.end()) {
        m_mappedSpecs.emplace(spec.name, spec);
        m_specNameIds.push_back(spec.name);
        m_specList.push_back(spec);

        recalculateHash();
    }
}

void ParamList::erase(const ParamSpec &spec)
{
    erase(spec.name);
}

void ParamList::erase(const StringId &nameId)
{
    if (m_mappedSpecs.erase(nameId) > 0) {
        m_mappedSpecs.erase(nameId);
        std::size_t ind =
            std::find(m_specNameIds.begin(), m_specNameIds.end(), nameId) - m_specNameIds.begin();
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

bool ParamList::contains(StringId nameId) const
{
    return m_mappedSpecs.find(nameId) != m_mappedSpecs.end();
}

void ParamList::recalculateHash()
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

} // namespace Vitrae