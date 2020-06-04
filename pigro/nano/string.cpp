#include "string.h"

#include <cctype>

std::string_view nano::trim(std::string_view text)
{

    auto begin = text.begin();
    while ( begin != text.end() && std::isspace(*begin) ) ++begin;

    auto end = text.end();
    while ( end != begin && std::isspace(*end) ) --end;

    return std::string_view(begin, end - begin);
}
