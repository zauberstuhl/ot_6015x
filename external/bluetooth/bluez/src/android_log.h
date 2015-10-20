// liukun@tcl.com add this file for make log output to logcat
#ifndef _ANDROID_LOG_H_
#define _ANDROID_LOG_H_


#include <cutils/log.h>

void A_DBG(const char *format, ...);

#define LOG_TAGS "--bluez--"
#define A_LOG_INFO  ANDROID_LOG_INFO
#define A_LOG_ERR   ANDROID_LOG_ERROR
#define A_LOG_DEBUG ANDROID_LOG_DEBUG
#define A_VSYSLOG(level, msg, ap) __android_log_vprint(level, LOG_TAGS, msg, ap);
#define A_SYSLOG(level, msg, args...) __android_log_print(level, LOG_TAGS, msg, ##args);


#endif
