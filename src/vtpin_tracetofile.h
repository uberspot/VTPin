#pragma once

#include <stdio.h>
#include <string.h>

#include "vtpin.h"

#define TRACE_BUFFER_SIZE 16384

static __thread int volatile trace_fd = 0;
static __thread char *trace_buffer;
static __thread char *trace_temp_buff;
static __thread int volatile last_pos_written_from_buffer = 0;

/* ---- TRACE FILE START ---- */

void flush_trace_buffer() {
  int ret_out = write(trace_fd, trace_buffer, last_pos_written_from_buffer);
  if (ret_out != last_pos_written_from_buffer) {
    debug("trace write error: %d", ret_out);
  }
}

void write_char_to_file(const char ch) {
  // if we've reached the end
  if (last_pos_written_from_buffer == TRACE_BUFFER_SIZE - 1) {
    // flush buffer
    flush_trace_buffer();
    last_pos_written_from_buffer = 0;
    // zero out buffer?
  }
  trace_buffer[last_pos_written_from_buffer] = ch;
  last_pos_written_from_buffer++;
}

void write_string_to_file(const char *array) {
  int i = 0;
  while (array[i] != '\0') {
    write_char_to_file(static_cast<char>(array[i++]));
  }
}

void write_num_to_file(long n) {
  if (n < 0) {
    write_char_to_file('-');
    n = -n;
  }
  if (n / 10 != 0) {
    write_num_to_file(n / 10);
  }
  write_char_to_file((n % 10) + '0');
}

void trace_to_file(const char tag, const void *p, const long size = 0) {
#ifdef TRACE_MEM_TO_FILE
  if (trace_fd != 0 && trace_fd != 1 && trace_fd != 2 &&
      vtpin_system_in_place) {
    write_char_to_file(tag);
    write_char_to_file(' ');

    // pointer to string
    sprintf(trace_temp_buff, "%p", p);
    write_string_to_file(trace_temp_buff);
    write_char_to_file(' ');

    write_num_to_file(size);
    write_char_to_file('\n');
  }
#endif
}

void trace_finalize() {
  if (last_pos_written_from_buffer != 0) {
    debug("Final flush!");
    flush_trace_buffer();
  }
  close(trace_fd);
  close(rtti_trigger_fd);
}

void trace_initialize() {
  trace_fd = open("/tmp/trace_output.txt", O_WRONLY | O_TRUNC | O_CREAT, 0644);
  trace_buffer = (char *)malloc(sizeof(char) * TRACE_BUFFER_SIZE);
  trace_temp_buff = (char *)malloc(sizeof(char) * 64);
}
/* ---- TRACE FILE END ---- */