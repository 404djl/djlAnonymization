/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `LICENSE` for details.
 */

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct {
  void *udata;
  log_LockFn lock;
  FILE *fp;
  int level;
  int quiet;
} L;

static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};

static void default_lock(void *udata, int lock) {
  (void)udata;
  (void)lock;
}

// 初始化函数
static void log_init(void) __attribute__((constructor));

static void log_init(void) {
    L.udata = NULL;
    L.lock = default_lock;
    L.fp = stderr;
    L.level = LOG_INFO;
    L.quiet = 0;
}

void log_set_udata(void *udata) {
  L.udata = udata;
}

void log_set_lock(log_LockFn fn) {
  L.lock = fn ? fn : default_lock;
}

void log_set_fp(FILE *fp) {
  L.fp = fp;
}

void log_set_level(int level) {
  L.level = level;
}

void log_set_quiet(int enable) {
  L.quiet = enable;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
  if (level < L.level) return;
  
  L.lock(L.udata, 1);
  
  /* Get current time */
  time_t t = time(NULL);
  struct tm *lt = localtime(&t);
  char time_str[32];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", lt);
  
  /* Format the message */
  va_list args;
  char msg[1024];
  
  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);
  
  /* Determine max level name length */
  size_t max_level_len = 0;
  for (int i = 0; i < 6; i++) {
    size_t len = strlen(level_names[i]);
    if (len > max_level_len) {
      max_level_len = len;
    }
  }
  
  /* Build the full message */
  char full_msg[2048];
  #ifdef _WIN32
    snprintf(full_msg, sizeof(full_msg), "%s %-*s %s:%d: %s\n",
             time_str, (int)max_level_len, level_names[level],
             file, line, msg);
  #else
    snprintf(full_msg, sizeof(full_msg), "%s %s%-*s\x1b[0m %s:%d: %s\n",
             time_str, level_colors[level], (int)max_level_len, level_names[level],
             file, line, msg);
  #endif
  
  /* Write to file */
  if (L.fp) {
    fputs(full_msg, L.fp);
    fflush(L.fp);
  }
  
  /* Write to stderr */
  if (!L.quiet) {
    fputs(full_msg, stderr);
    fflush(stderr);
  }
  
  /* If fatal, abort */
  if (level == LOG_FATAL) {
    abort();
  }
  
  L.lock(L.udata, 0);
}