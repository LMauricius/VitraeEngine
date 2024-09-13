#include "Vitrae/Util/Variant.hpp"
#include "glm/glm.hpp"

#include <regex>

// GNU compiler demangling
#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace Vitrae
{
String VariantVTable::constructShortTypeName(const std::type_info *p_id)
{
    String ret;

    if (p_id == &typeid(void))
        ret = "void";
    else if (p_id == &typeid(bool))
        ret = "bool";
    else if (p_id == &typeid(std::int8_t))
        ret = "int8";
    else if (p_id == &typeid(std::uint8_t))
        ret = "uint8";
    else if (p_id == &typeid(std::int16_t))
        ret = "int16";
    else if (p_id == &typeid(std::uint16_t))
        ret = "uint16";
    else if (p_id == &typeid(std::int32_t))
        ret = "int32";
    else if (p_id == &typeid(std::uint32_t))
        ret = "uint32";
    else if (p_id == &typeid(std::int64_t))
        ret = "int64";
    else if (p_id == &typeid(std::uint64_t))
        ret = "uint64";
    else if (p_id == &typeid(float))
        ret = "float";
    else if (p_id == &typeid(double))
        ret = "double";
    else if (p_id == &typeid(std::string))
        ret = "string";
    else if (p_id == &typeid(glm::vec2))
        ret = "vec2";
    else if (p_id == &typeid(glm::vec3))
        ret = "vec3";
    else if (p_id == &typeid(glm::vec4))
        ret = "vec4";
    else if (p_id == &typeid(glm::ivec2))
        ret = "ivec2";
    else if (p_id == &typeid(glm::ivec3))
        ret = "ivec3";
    else if (p_id == &typeid(glm::ivec4))
        ret = "ivec4";
    else if (p_id == &typeid(glm::uvec2))
        ret = "uvec2";
    else if (p_id == &typeid(glm::uvec3))
        ret = "uvec3";
    else if (p_id == &typeid(glm::uvec4))
        ret = "uvec4";
    else if (p_id == &typeid(glm::mat2))
        ret = "mat2";
    else if (p_id == &typeid(glm::mat3))
        ret = "mat3";
    else if (p_id == &typeid(glm::mat4))
        ret = "mat4";
    else {
        ret = p_id->name();

#ifdef __GNUG__
        {
            int status = 0;
            char *demangled = abi::__cxa_demangle(p_id->name(), 0, 0, &status);
            if (status == 0) {
                ret = demangled;
                free(demangled);
            }
        }
#endif

        // remove namespace qualifiers
        const std::regex namespacePattern(R"([A-Za-z_]+::)");
        std::smatch matches;

        while (std::regex_search(ret, matches, namespacePattern)) {
            ret.replace(matches.position(0), matches.length(0), "");
        }

        // simplify pointers
        const std::regex dynasmaPattern(R"((FirmPtr|LazyPtr)<(.*)>$)");
        ret = std::regex_replace(ret, dynasmaPattern, "*$2");

        // simplify buffer pointers
        const std::regex bufferPattern(R"(SharedBufferPtr<(void, )?(.*)>$)");
        ret = std::regex_replace(ret, bufferPattern, "$2[]");

        const std::regex voidArrayPattern(R"((.*), void[])");
        ret = std::regex_replace(ret, voidArrayPattern, "$1");
    }

    return ret;
}

} // namespace Vitrae