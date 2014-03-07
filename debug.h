#ifndef __MINIOS_DEBUG__
#define __MINIOS_DEBUG__


/* err warn info debug
0x1(0001) err
0x3(0011) err & warn
0x7(0111) err & warn & info
0xf(1111) err & warn & info & debug
*/
#define MINIOS_LOG_ERR   1
#define MINIOS_LOG_WARN  2
#define MINIOS_LOG_INFO   4
#define MINIOS_LOG_DBG   8

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