/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H
#include <stdio.h>
#include <stdarg.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#define LOG_VERSION "0.1.0"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef void (*log_LockFn)(void *udata, int lock);

    typedef enum
    {
        LOG_TRACE = 0,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_FATAL
    } LOG_LEVEL;
    void log_log(int level, const char *file, int line, const char *fmt, ...);
#ifdef __ANDROID__
#define LOGT(fmt...) __android_log_print(ANDROID_LOG_VERBOSE, "Anonymization", fmt)
#define LOGD(fmt...) __android_log_print(ANDROID_LOG_DEBUG, "Anonymization", fmt)
#define LOGI(fmt...) __android_log_print(ANDROID_LOG_INFO, "Anonymization", fmt)
#define LOGW(fmt...) __android_log_print(ANDROID_LOG_WARN, "Anonymization", fmt)
#define LOGE(fmt...) __android_log_print(ANDROID_LOG_ERROR, "Anonymization", fmt)
#define LOGF(fmt...) __android_log_print(ANDROID_LOG_FATAL, "Anonymization", fmt)
#else
#define LOGT(fmt...) fprintf(stderr, fmt)
#define LOGD(fmt...) fprintf(stderr, fmt)
#define LOGI(fmt...) fprintf(stderr, fmt)
#define LOGW(fmt...) fprintf(stderr, fmt)
#define LOGE(fmt...) fprintf(stderr, fmt)
#define LOGF(fmt...) fprintf(stderr, fmt)
#endif

#define log_trace(...)                                   \
    log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__); \
    LOGT(__VA_ARGS__)
#define log_debug(...)                                   \
    log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__); \
    LOGD(__VA_ARGS__)

#define log_info(...)                                   \
    log_log(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__); \
    LOGI(__VA_ARGS__)

#define log_warn(...)                                   \
    log_log(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__); \
    LOGW(__VA_ARGS__)
#define log_error(...)                                   \
    log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__); \
    LOGE(__VA_ARGS__)
#define log_fatal(...)                                   \
    log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__); \
    LOGF(__VA_ARGS__)

    void log_set_udata(void *udata);
    void log_set_lock(log_LockFn fn);
    void log_set_fp(FILE *fp);
    void log_set_level(int level);
    void log_set_quiet(int enable);

#ifdef __cplusplus
}
#endif

#endif