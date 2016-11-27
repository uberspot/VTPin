#pragma once

/* Mostly from http://c.learncodethehardway.org/book/ex20.html */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"

#ifndef DEBUG

#define debug(M, ...)

#else

#define debug(M, ...)                                                          \
  fprintf(stderr,                                                              \
          ANSI_COLOR_YELLOW "[DEBUG - %s:%d:%s() ]: " ANSI_COLOR_RESET M "\n", \
          __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#endif
