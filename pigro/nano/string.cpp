#include "string.h"

#include <cctype>

std::string_view nano::trim(std::string_view text)
{
    auto begin = text.begin();
    while ( begin != text.end() && std::isspace(*begin) ) ++begin;

    if ( begin == text.end() ) return {};

    auto end = text.end() - 1;
    while ( end != begin && std::isspace(*end) ) --end;

    return std::string_view(begin, end - begin + 1);
}
