#pragma once

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unordered_map>

#include "vtpin_dbg.h"
#include "vtpin_extras.h"

pthread_t heap_scan_thread;
unordered_map<unsigned long, bool> buffer1, buffer2;
#define MEM_LEAK_THRESHOLD 100000000

static unsigned long volatile current_mem_leak = 0;

// true if heap scan thread is using the first buffer,
// false if it's using the second one
std::atomic<bool> heap_using_first(true);
std::atomic<bool> heap_scan_running(false);

unsigned long volatile heap_mem_range_low[VMAX_SIZE] = {0};
unsigned long volatile heap_mem_range_high[VMAX_SIZE] = {0};
unsigned int volatile heap_mem_range_count = 0;

double program_start_time = 0;

typedef void (*real_free_t)(void *);
static real_free_t real_free = NULL;

static void heap_scan_mark_program_start() {
  struct timeval tim;
  gettimeofday(&tim, NULL);
  program_start_time = tim.tv_sec + (tim.tv_usec / 1000000.0);
}

// check if vobject exists in map, and mark true if it does
void vtpit_mark_if_dangling(const unsigned long ptr) {
  if (heap_using_first) {
    auto found = buffer1.find(ptr);
    if (found != buffer1.end()) {
      // debug("Found dangling ptr reference in buffer1: %p", ptr);
      buffer1[ptr] = true;
    }
  } else {
    auto found = buffer2.find(ptr);
    if (found != buffer2.end()) {
      // debug("Found dangling ptr reference in buffer2: %p", ptr);
      buffer2[ptr] = true;
    }
  }
}

void vtpit_remove_non_dangling_objects() {
  if (!real_free) {
    real_free = (real_free_t)dlsym(RTLD_NEXT, "free");
    if (!real_free) {
      debug("dlsym problem: %s", dlerror());
      exit(1);
    }
  }

  if (heap_using_first) {
    debug("Initial buffer1 size: %d", buffer1.size());
    for (auto it = buffer1.begin(); it != buffer1.end();) {
      // if it doesn't have a dangling pointer to it
      if (!it->second) {
        // do actual free()
        // real_free((void *) it->first);
        // it = buffer1.erase(it);
        it++;
      } else {
        it++;
      }
    }
    debug("Cleared buffer1. Final size: %d", buffer1.size());
  } else {
    debug("Initial buffer2 size: %d", buffer2.size());
    for (auto it = buffer2.begin(); it != buffer2.end();) {
      // if it doesn't have a dangling pointer to it
      if (!it->second) {
        // real_free((void *) it->first);
        // it = buffer2.erase(it);
        it++;
      } else {
        it++;
      }
    }
    debug("Cleared buffer2. Final size: %d", buffer2.size());
  }
}

/* Parses /proc/self/maps and stores only memory ranges with rw-p privileges
and no file mmap'ed to them.
example entry from /proc/self/maps:
*7f2a1473d000-7f2a14740000 r-xp 00000000 08:12 1718304 /usr/lib/libdl-2.22.so
*/
void parse_proc_maps() {
  debug("Loading areas for heap scan");

  heap_mem_range_count = 0;

  ifstream file(PROC_MAPS_DIR);
  string line;

  while (getline(file, line)) {
    size_t first_space = line.find(' ');
    string mem_map = line.substr(0, first_space);
    string remaining_line = line.substr(first_space + 1);
    first_space = remaining_line.find(' ');
    string permissions = remaining_line.substr(0, first_space);

    if (!permissions.compare("rw-p") &&
        remaining_line.find("[heap]") != std::string::npos &&
        remaining_line.find('/') == std::string::npos &&
        remaining_line.find("stack") == std::string::npos) {
      size_t first_dash = mem_map.find("-");
      string ds_start = mem_map.substr(0, first_dash);
      string ds_end = mem_map.substr(first_dash + 1);

      unsigned long lomem = strtol(ds_start.c_str(), NULL, 16);
      unsigned long himem = strtol(ds_end.c_str(), NULL, 16);
      if ((himem - lomem) != 0) {
        if (heap_mem_range_count + 1 < VMAX_SIZE) {
          heap_mem_range_low[heap_mem_range_count] = lomem;
          heap_mem_range_high[heap_mem_range_count] = himem;
          heap_mem_range_count++;
          debug("/proc/self/maps: %s", line.c_str());
          debug("lomem: %lu himem: %lu, size: %lu perm: %s", lomem, himem,
                (himem - lomem), remaining_line.c_str());
        }
      }
    }
  }
  // zero out remaining_line of elements
  int temp = heap_mem_range_count;
  while (temp + 1 < VMAX_SIZE) {
    heap_mem_range_low[temp] = 0;
    heap_mem_range_high[temp] = 0;
    temp++;
  }
  debug("Loaded areas for heap scan, heap_mem_range_count: %d", heap_mem_range_count);
}

static void scan_region(unsigned long heap_lomem, unsigned long heap_himem) {
  unsigned long *ptr, *end;
  ptr = (unsigned long *)heap_lomem;
  end = (unsigned long *)heap_himem;
  while (ptr < end) {
    vtpit_mark_if_dangling(*ptr);
    ptr++;
  }
}

void *scan_region_thread(void *param) {
  unsigned long *mem_region = (unsigned long *)param;
  debug("Scanning heap_mem_range_low: %lu, heap_mem_range_high: %lu", mem_region[0], mem_region[1]);
  scan_region(mem_region[0], mem_region[1]);

  pthread_exit(NULL);
}

static void *heap_scan_run(void *param) {
  int scan_thread_num = 2;
  debug("-------- Starting heap scan --------");

  // disable pthread cancel while scanning
  int res = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  if (res != 0)
    debug("heap_scan pthread_setcancelstate error: %d", res);

  heap_using_first = !heap_using_first; // TODO: fix non-atomic
  debug("Scanning buffer%s", (heap_using_first) ? "1" : "2");

  struct timeval tim;
  gettimeofday(&tim, NULL);
  double t1 = tim.tv_sec + (tim.tv_usec / 1000000.0);

  debug("%.6lf seconds elapsed from last heap scan\n", t1 - program_start_time);

  // parse /proc/self/maps
  parse_proc_maps();
  // scan heap areas
  for (unsigned int i = 0; i < heap_mem_range_count; i++) {
    unsigned long mem_region[2], mem_region2[2], mem_region3[2], mem_region4[2];

    unsigned long range_difference = (heap_mem_range_high[i] - heap_mem_range_low[i]) / 4;
    mem_region[0] = heap_mem_range_low[i];
    mem_region[1] = heap_mem_range_low[i] + range_difference;

    pthread_t scan_thread1, scan_thread2, scan_thread3, scan_thread4;
    int rc = pthread_create(&scan_thread1, NULL, scan_region_thread,
                            (void *)mem_region);
    if (rc) {
      debug("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }

    /*mem_region2[0] = heap_mem_range_low[i] + range_difference;
    mem_region2[1] = heap_mem_range_low[i] + (2* range_difference);

    rc = pthread_create(&scan_thread2, NULL, scan_region_thread, (void
    *)mem_region2);
    if (rc) {
        debug("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    mem_region3[0]=heap_mem_range_low[i] + (2* range_difference);
    mem_region3[1]=heap_mem_range_low[i] + (3* range_difference);

    rc = pthread_create(&scan_thread3, NULL, scan_region_thread, (void
    *)mem_region3);
    if (rc) {
        debug("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }

    mem_region4[0]=heap_mem_range_low[i] + (3* range_difference);
    mem_region4[1]=heap_mem_range_high[i];

    rc = pthread_create(&scan_thread4, NULL, scan_region_thread, (void
    *)mem_region4);
    if (rc) {
        debug("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
    }*/

    pthread_join(scan_thread1, NULL);
    // pthread_join(scan_thread2, NULL);
    // pthread_join(scan_thread3, NULL);
    // pthread_join(scan_thread4, NULL);
  }
  // free non-dangling vobjects
  vtpit_remove_non_dangling_objects();

  gettimeofday(&tim, NULL);
  double t2 = tim.tv_sec + (tim.tv_usec / 1000000.0);
  debug("%.6lf seconds elapsed\n", t2 - t1);
  program_start_time = t2;

  // re-enable cancelation
  res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (res != 0)
    debug("heap_scan pthread_setcancelstate error: %d", res);

  debug("-------- Stopped heap scan --------");

  /* sleep() is a cancellation point */
  sleep(20);
  current_mem_leak = 0;
  heap_scan_running = false;

  return NULL;
}

static void heap_scan_start_thread() {
#ifdef DO_GARBAGE_COLLECTION
  // start heap scanning thread
  int rs = pthread_create(&heap_scan_thread, NULL, heap_scan_run, NULL);
  if (rs) {
    debug("ERROR: return code from heap_scan_run pthread_create() is %d\n", rs);
    exit(-1);
  }
#endif
}

static void heap_scan_cancel_thread() {
  if (!heap_scan_running) {
    return;
  }
  void *res;
  int rs = pthread_cancel(heap_scan_thread);
  if (rs != 0)
    debug("Error in heap_scan pthread_cancel: %d", rs);

  /* Join with thread to see what its exit status was */

  rs = pthread_join(heap_scan_thread, &res);
  if (rs != 0)
    debug("Error in heap_scan pthread_join: %d", rs);

  if (res == PTHREAD_CANCELED)
    debug("heap_scan thread was canceled");
  else
    debug("heap_scan thread wasn't canceled");
}

void vtpit_add_object(const unsigned long ptr) {
#ifdef DO_GARBAGE_COLLECTION
  // record added mem leak
  size_t ptr_size = malloc_usable_size(ptr);
  current_mem_leak += ptr_size;

  if (heap_using_first) {
    buffer2[ptr] = false;
    // debug("Added element %p to buffer2, size: %d", ptr, buffer2.size());
    if (!heap_scan_running &&
        current_mem_leak >
            MEM_LEAK_THRESHOLD) {
      heap_scan_running = true;
      heap_scan_start_thread();
    }
  } else {
    buffer1[ptr] = false;
    // debug("Added element %p to buffer1, size: %d", ptr, buffer1.size());
    if (!heap_scan_running &&
        current_mem_leak >
            MEM_LEAK_THRESHOLD) {
      heap_scan_running = true;
      heap_scan_start_thread();
    }
  }
#endif
}
