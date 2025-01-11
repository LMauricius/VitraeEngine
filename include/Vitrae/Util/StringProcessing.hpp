#pragma once

#include "Vitrae/Data/Typedefs.hpp"

namespace Vitrae
{

String clearIndents(const String &str);
String searchAndReplace(String input, const StringView search, const StringView replace);
}