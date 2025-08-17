#pragma once

typedef enum {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
    PANIC
} loglevel_t;

void kprint(loglevel_t level, const char *name, const char *file, int line, const char *fmt, ...);

#define kp_debug(name, fmt...) kprint(DEBUG, name, __FILE__, __LINE__, fmt)
#define kp_info(name, fmt...)  kprint(INFO, name, __FILE__, __LINE__, fmt)
#define kp_warn(name, fmt...)  kprint(WARNING, name, __FILE__, __LINE__, fmt)
#define kp_error(name, fmt...) kprint(ERROR, name, __FILE__, __LINE__, fmt)
#define kp_crit(name, fmt...)  kprint(CRITICAL, name, __FILE__, __LINE__, fmt)
#define kp_panic(name, fmt...) kprint(PANIC, name, __FILE__, __LINE__, fmt)
