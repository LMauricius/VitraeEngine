#pragma once

#include <cstring>
#include <string>
#include <string_view>

namespace Vitrae
{
/**
 * A hash of a string, for quicker comparison and mapping
 */
class StringId
{
    friend std::hash<StringId>;

    std::size_t m_hash;
#ifdef VITRAE_DEBUG_STRINGIDS
    char *m_str;
#endif

  public:
    constexpr StringId(const char *str) : StringId(std::string_view{str}) {}
    StringId(const std::string &str) : StringId(std::string_view{str}) {}
    constexpr StringId(const std::string_view str)
    {
        // fnv_hash_1a_64
        m_hash = 0xcbf29ce484222325ULL;

        for (char c : str)
            m_hash = (m_hash ^ c) * 0x100000001b3ULL;

#ifdef VITRAE_DEBUG_STRINGIDS
        m_str = new char[str.size() + 1];
        std::copy(str.begin(), str.end(), m_str);
        m_str[str.size()] = '\0';
#endif
    }
    constexpr StringId(StringId &&id)
    {
        m_hash = id.m_hash;
#ifdef VITRAE_DEBUG_STRINGIDS
        m_str = id.m_str;
        id.m_str = nullptr;
#endif
    }
    constexpr StringId(const StringId &id)
    {
        m_hash = id.m_hash;
#ifdef VITRAE_DEBUG_STRINGIDS
        m_str = new char[std::strlen(id.m_str) + 1];
        std::strcpy(m_str, id.m_str);
#endif
    }

#ifdef VITRAE_DEBUG_STRINGIDS
    constexpr ~StringId()
    {
        if (m_str)
            delete[] m_str;
    }
#endif

    constexpr StringId &operator=(StringId id)
    {
        m_hash = id.m_hash;
#ifdef VITRAE_DEBUG_STRINGIDS
        m_str = new char[std::strlen(id.m_str) + 1];
        std::strcpy(m_str, id.m_str);
#endif
        return *this;
    }

    constexpr bool operator==(StringId id) const { return m_hash == id.m_hash; }
    constexpr bool operator!=(StringId id) const { return m_hash != id.m_hash; }
    constexpr bool operator>=(StringId id) const { return m_hash >= id.m_hash; }
    constexpr bool operator<=(StringId id) const { return m_hash <= id.m_hash; }
    constexpr bool operator>(StringId id) const { return m_hash > id.m_hash; }
    constexpr bool operator<(StringId id) const { return m_hash < id.m_hash; }
};
} // namespace Vitrae

namespace std
{
template <> struct hash<Vitrae::StringId>
{
    size_t operator()(const Vitrae::StringId &x) const { return x.m_hash; }
};
} // namespace std