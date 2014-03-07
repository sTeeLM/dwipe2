#include "serial.h"
#include "debug.h"
#include "option.h"
#include <stdio.h>
#include <stdarg.h>

void _minios_print_log(int level, const char * file, int line, const char * fmt, ...)
{
    va_list ap;
    char buffer[4096];
    size_t len = 0;
    const char * log_str = "ERR ";

    /* 0:nothing, 1:err & warn only, 2: info & err & warn, 3: debug, info, err, warn */
    if(!(opt.debug & level))
        return;

    switch(level) {
        case MINIOS_LOG_ERR:
            log_str = "ERR"; break;
        case MINIOS_LOG_WARN:
            log_str = "WAN"; break;
        case MINIOS_LOG_INFO:
            log_str = "INF"; break;
        case MINIOS_LOG_DBG:
            log_str = "DBG"; break;
        default:
            log_str = "???"; break;

    }
    len += snprintf(buffer + len, sizeof(buffer) - len, "[%s]", log_str);
    len += snprintf(buffer + len, sizeof(buffer) - len, "[%s:%d]", file, line);

    va_start(ap, fmt);
    len += vsnprintf(buffer + len, sizeof(buffer) - len, fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s\r\n", buffer);
}