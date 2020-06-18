#ifndef NANO_STRING_H
#define NANO_STRING_H

#include <string_view>

namespace nano
{

    using string = std::string;

    std::string_view trim(std::string_view text);

}

#endif // NANO_STRING_H
