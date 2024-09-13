#include "Vitrae/Util/StringProcessing.hpp"

#include <algorithm>
#include <limits>

namespace Vitrae
{

String clearIndents(const String &str)
{
    std::size_t minIndents = std::numeric_limits<int>::max();
    std::size_t firstEmptyLines = 0;
    std::size_t i;

    // count empty lines
    i = 0;
    while (i < str.size()) {
        if (str[i] == '\n') {
            firstEmptyLines++;
        } else if (str[i] == ' ' || str[i] == '\t') {
        } else {
            break;
        }
        i++;
    }

    // count indents
    i = 0;
    std::size_t currentCount;
    bool counting = false;
    while (i < str.size()) {
        if (str[i] == '\n') {
            counting = true;
            currentCount = 0;
        } else if (str[i] == ' ' || str[i] == '\t') {
            if (counting) {
                currentCount++;
            }
        } else {
            if (counting) {
                minIndents = std::min(minIndents, currentCount);
                counting = false;
            }
        }
        i++;
    }

    // process
    String out;

    // skip empty lines
    i = 0;
    currentCount = 0;
    while (currentCount < firstEmptyLines) {
        if (str[i] == '\n') {
            currentCount++;
        }
        i++;
    }

    // skip indents
    currentCount = minIndents;
    while (i < str.size()) {
        if (str[i] == '\n') {
            currentCount = minIndents;
            out += '\n';
        } else {
            if (currentCount == 0) {
                out += str[i];
            } else {
                currentCount--;
            }
        }
        i++;
    }

    return out;
}

String searchAndReplace(String input, const StringView search, const StringView replace)
{
    std::size_t pos = input.find(search);
    while (pos != String::npos) {
        input.replace(pos, search.size(), replace);
        pos = input.find(search, pos + replace.size());
    }
    return input;
}

} // namespace Vitrae