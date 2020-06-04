#include "exception.h"

#include <errno.h>
#include <cstring>

namespace nano
{

    std::string errno_message(int err)
    {
        char buf[256];
        char *message = strerror_r(err, buf, sizeof(buf));
        if ( message != nullptr )
        {
            return std::string(message);
        }

        int len = snprintf(buf, sizeof(buf), "unknown error (errno=%d)", err);
        return std::string(buf, len);
    }

    const char *exception::what() const noexcept
    {
        return m_message.c_str();
    }

}
