// liukun@tcl.com add this file for make log output to logcat
#include "android_log.h"

void A_DBG(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    A_VSYSLOG(A_LOG_DEBUG, format, ap);
    va_end(ap);
}

