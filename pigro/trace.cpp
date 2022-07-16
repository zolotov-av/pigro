#include "trace.h"

void trace::log(const char *message)
{
    const std::string thread = getThreadName().toStdString();
    char buf[4096];
    int r = snprintf(buf, sizeof(buf)-1, "%s: %s\n", thread.c_str(), message);
    buf[r] = 0;
    fputs(buf, stdout);
}
