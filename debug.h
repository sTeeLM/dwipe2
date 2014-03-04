#ifndef __MINIOS_DEBUG__
#define __MINIOS_DEBUG__

#define MINIOS_LOG_INFO  1
#define MINIOS_LOG_WARN  2
#define MINIOS_LOG_ERR   3
#define MINIOS_LOG_DBG   4

extern int minios_quiet;

void _minios_print_log(int level, const char * file, int line, const char * fmt, ...);

#define SERR(fmt, args...) \
    _minios_print_log(MINIOS_LOG_ERR, __FILE__, __LINE__, fmt, ##args);

#define SWAN(fmt, args...) \
    _minios_print_log(MINIOS_LOG_WARN, __FILE__, __LINE__, fmt, ##args);

#define SINF(fmt, args...) \
    _minios_print_log(MINIOS_LOG_INFO, __FILE__, __LINE__, fmt, ##args);

#define SDBG(fmt, args...) \
    _minios_print_log(MINIOS_LOG_DBG, __FILE__, __LINE__, fmt, ##args);


#endif