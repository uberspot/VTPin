#pragma once

#include <fstream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <string>

#include "vtpin_dbg.h"

using namespace std;

enum VTResolution { IS_VTABLE = 0, IS_NOT_VTABLE = 1, UNCLEAR_IF_VTABLE = 2 };

static const char *PROC_MAPS_DIR = "/proc/self/maps";
#define VMAX_SIZE 2048

unsigned long volatile mem_range_low[VMAX_SIZE] = {0};
unsigned long volatile mem_range_high[VMAX_SIZE] = {0};
unsigned int volatile mem_range_count = 0;

static pthread_mutex_t proc_maps_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Parses /proc/self/maps and stores only memory ranges with r-xp or r--p
privileges
example entry from /proc/self/maps:
*7f2a1473d000-7f2a14740000 r-xp 00000000 08:12 1718304 /usr/lib/libdl-2.22.so
*/
void load_vtablemaps() {
  pthread_mutex_lock(&proc_maps_mutex);
  debug("Loading vtables");

  mem_range_count = 0;

  ifstream file(PROC_MAPS_DIR);
  string line;

  while (getline(file, line)) {
    size_t first_space = line.find(' ');
    string mem_map = line.substr(0, first_space);
    string remaining_line = line.substr(first_space + 1);
    first_space = remaining_line.find(' ');
    string permissions = remaining_line.substr(0, first_space);
    if (!permissions.compare("r-xp") || !permissions.compare("r--p")) {
      size_t first_dash = mem_map.find("-");
      string ds_start = mem_map.substr(0, first_dash);
      string ds_end = mem_map.substr(first_dash + 1);

      unsigned long lomem = strtol(ds_start.c_str(), NULL, 16);
      unsigned long himem = strtol(ds_end.c_str(), NULL, 16);
      if ((himem - lomem) != 0) {
        if (mem_range_count + 1 < VMAX_SIZE) {
          mem_range_low[mem_range_count] = lomem;
          mem_range_high[mem_range_count] = himem;
          mem_range_count++;
        }
        // debug("lomem: %lu himem: %lu perm: %s", lomem, himem,
        // permissions.c_str());
      }
    }
  }
  // zero out remaining_line of elements
  int temp = mem_range_count;
  while (temp + 1 < VMAX_SIZE) {
    mem_range_low[temp] = 0;
    mem_range_high[temp] = 0;
    temp++;
  }
  pthread_mutex_unlock(&proc_maps_mutex);
  debug("Loaded vtables, mem_range_count: %d", mem_range_count);
}

/* Does binary search in mem_range_low and mem_range_high to find the if addr belongs
to any of the ranges in those structures. */
enum VTResolution is_vtable(const unsigned long addr) {
  if (addr == 0) {
    return IS_NOT_VTABLE;
  }

  int low = 0;
  int high = mem_range_count;
  while (low <= high) {
    int mid = (low + high) / 2;
    if (mem_range_low[mid] > addr) {
      high = mid - 1;
    } else if (mem_range_high[mid] < addr) {
      low = mid + 1;
    } else if (mem_range_low[mid] <= addr && addr <= mem_range_high[mid]) {
      return UNCLEAR_IF_VTABLE;
    }
  }
  return IS_NOT_VTABLE; // addr would be inserted at index "low"
}

/* ---- VTPinMemMap END ---- */

/* ---- VTPin Safe Class START ---- */

/* Class used as a Base for our "safe" class */
class VTPinBoardbase {
public:
  virtual void test_method0() { debug("test_method0()"); }
  virtual void test_method1() { debug("test_method1()"); }
  virtual void test_method2() { debug("test_method2()"); }
  virtual void test_method3() { debug("test_method3()"); }
  virtual void test_method4() { debug("test_method4()"); }
  virtual void test_method5() { debug("test_method5()"); }
  virtual void test_method6() { debug("test_method6()"); }
  virtual void test_method7() { debug("test_method7()"); }
  virtual void test_method8() { debug("test_method8()"); }
  virtual void test_method9() { debug("test_method9()"); }
  virtual void test_method10() { debug("test_method10()"); }
  virtual void test_method11() { debug("test_method11()"); }
  virtual void test_method12() { debug("test_method12()"); }
  virtual void test_method13() { debug("test_method13()"); }
  virtual void test_method14() { debug("test_method14()"); }
  virtual void test_method15() { debug("test_method15()"); }
  virtual void test_method16() { debug("test_method16()"); }
  virtual ~VTPinBoardbase() = 0;
};

/* Class implementing some "safe" virtual functions that will be called when
a previously freed virtual method is called after vtpin pins it to this class */
class VTPinBoard : public VTPinBoardbase {
public:
  void test_method0() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method1() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method2() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method3() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method4() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method5() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method6() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method7() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method8() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method9() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method10() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method11() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method12() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method13() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method14() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method15() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
  void test_method16() {
    debug("Dangling pointer called from: %p", (unsigned long)this);
  }
};

/* ---- VTPin Safe Class END ---- */


void print_stacktrace(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  backtrace_symbols_fd(array, size, STDERR_FILENO);
}