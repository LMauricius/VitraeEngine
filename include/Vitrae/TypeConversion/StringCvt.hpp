#pragma once

#include "Vitrae/Data/Typedefs.hpp"

#include "assimp/types.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

#include <codecvt>
#include <locale>
#include <sstream>
#include <type_traits>
#include <utility>

namespace Vitrae
{
namespace StringConvertibleChooses
{
template <class T>
concept std_to_string = requires(T t) { std::to_string(t); };

template <class T>
concept glm_to_string = !std_to_string<T> && requires(T t) { glm::to_string(t); };

template <class T>
concept std_sstream = !glm_to_string<T> && requires(T t, std::stringstream ss) { ss << t; };

} // namespace StringConvertibleChooses

template <class T>
    requires StringConvertibleChooses::std_sstream<T>
String toString(const T &val)
{
    std::stringstream ss;
    ss << val;
    return ss.str();
}

template <class T>
    requires StringConvertibleChooses::glm_to_string<T>
String toString(const T &val)
{
    return glm::to_string(val);
}

template <class T>
    requires StringConvertibleChooses::std_to_string<T>
String toString(const T &val)
{
    return std::to_string(val);
}

inline String toString(const std::wstring &str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(str);
}
inline String toString(const aiString &str)
{
    return String(str.data, str.length);
}

inline String toHexString(std::size_t num, std::size_t minlen = 0)
{
    String ret;
    ret.reserve(minlen);

    while (num > 0) {
        ret += "0123456789ABCDEF"[num % 16];
        num /= 16;
    }
    while (ret.size() < minlen) {
        ret += '0';
    }

    std::reverse(ret.begin(), ret.end());
    return ret;
}

template <class T>
    requires(!requires(T t) { toString(t); })
String toStringOrErr(const T &val)
{
    return "<error in conversion>";
}
template <class T>
    requires requires(T t) { toString(t); }
String toStringOrErr(const T &val)
{
    return toString(val);
}
} // namespace Vitrae