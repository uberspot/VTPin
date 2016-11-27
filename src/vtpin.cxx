#include <assert.h>
#include <bits/pthreadtypes.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h> /* For O_RDWR */
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <typeinfo>
#include <unistd.h>

#include "vtpin_heapscan.h"
#include "vtpin_rtti.h"
#include "vtpin_segfault.h"
#include "vtpin.h"
#include "vtpin_extras.h"
#include "vtpin_read_mempeak.h"
#include "vtpin_tracetofile.h"

/* Statistics  */
static unsigned long volatile stats_free_all = 0;
static unsigned long volatile stats_free_vobject = 0;
static unsigned long volatile stats_free_non_vobject = 0;
static unsigned long volatile stats_free_possible_vobject = 0;
static unsigned long volatile stats_free_exception = 0;
static unsigned long volatile stats_free_unhandled = 0;
static unsigned long volatile stats_free_null = 0;
static unsigned long volatile stats_free_multi_inherit = 0;
static unsigned long volatile stats_free_multi_inherit_vptrs = 0;

static unsigned long volatile stats_total_size_of_vobjects = 0;
static unsigned long volatile stats_total_malloc_size = 0;

// Malloc hook hack STUFF
static char tmpbuff[4096] = {0};
static unsigned long tmppos = 0;

/* ---- MEMBERS START ---- */

/* Safe object for pinning freed vtables */
VTPinBoard *vtpin_board;

/* Pointer for safe vtable object */
uintptr_t *vtpin_board_vtable;

static pthread_mutex_t vcheck = PTHREAD_MUTEX_INITIALIZER;

/* ---- MEMBERS END ---- */

/* Destructor:
    Print stats when destroying vtpin object */
__attribute__((destructor)) static void print_vtpin_stats(void) {
  vtpin_system_in_place = false;

  unsigned long stats_recorded = stats_free_non_vobject + stats_free_vobject +
                                 stats_free_unhandled + stats_free_null +
                                 stats_free_exception;

  fprintf(stderr, "VTPin destroyed. Stats:\n");
  fprintf(stderr, "free() calls: %d\n", stats_free_all);
  fprintf(stderr, "free() calls recorded: %d\n", stats_recorded);
  fprintf(stderr, "free() calls on vobjects: %lu\n", stats_free_vobject);
  fprintf(stderr, "free() calls on non vobjects: %lu\n",
          stats_free_non_vobject);
  fprintf(stderr, "free() calls on possible vobjects: %d\n",
          stats_free_possible_vobject);
  fprintf(stderr, "free() calls on exceptions: %lu\n", stats_free_exception);
  fprintf(stderr, "free() calls unhandled: %d\n", stats_free_unhandled);
  fprintf(stderr, "free() calls on null pointers: %d\n", stats_free_null);
  fprintf(stderr, "free() calls on multi inherit obj: %lu\n",
          stats_free_multi_inherit);
  fprintf(stderr, "free() vtprs in multi inherit obj: %lu\n",
          stats_free_multi_inherit_vptrs);
  fprintf(stderr, "free() calls on multi inherit: %g %%\n",
          ((double)stats_free_multi_inherit / stats_free_vobject) * 100);
  fprintf(stderr, "free() total malloc_usable_size of vobjects: %lu Bytes\n",
          stats_total_size_of_vobjects);
  fprintf(stderr, "free() total size of malloc'ed chunks: %lu Bytes\n",
          stats_total_malloc_size);
  fprintf(stderr, "free() peak physical memory: %zu Bytes\n", getPeakRSS());
  fprintf(stderr, "VObject overhead percentage(peakRSS): %g %% \n",
          (((double)stats_total_size_of_vobjects) / getPeakRSS()) * 100);
  fprintf(stderr, "VObject overhead percentage: %g %% \n",
          (((double)stats_total_size_of_vobjects) / stats_total_malloc_size) *
              100);

  trace_finalize();

  heap_scan_cancel_thread();
}

/* Prints vtpin stats when app is terminated */
void handle_termination(int isignal, siginfo_t *pssiginfo, void *psContext) {
  print_vtpin_stats();
}

/* ---- SETUP START ---- */

__attribute__((constructor)) void setup() {
  establish_sighandler();

  rtti_initialize();

  vtpin_board = new VTPinBoard();
  vtpin_board_vtable = (uintptr_t *)(vtpin_board);

  load_vtablemaps();

  trace_initialize();

  vtpin_system_in_place = true;

  heap_scan_start_thread();
  heap_scan_mark_program_start();

  debug("Setup done");
}

/* ---- SETUP END ---- */

/* ---- EXTERN C START ---- */
extern "C" {

/* ---- MALLOC START ---- */

#ifdef TRACE_MEM_TO_FILE

typedef void *(*real_malloc_t)(size_t);
static real_malloc_t real_malloc = nullptr;

void *malloc(size_t size) throw() {
  static bool initializing = false;
  if (!real_malloc) {
    if (!initializing) {
      initializing = true;
      real_malloc = (real_malloc_t)dlsym(RTLD_NEXT, "malloc");
      if (!real_malloc) {
        debug("malloc problem: %s", dlerror());
        exit(1);
      }
      initializing = false;

    } else {
      if (tmppos + size < sizeof(tmpbuff)) {
        void *retptr = tmpbuff + tmppos;
        tmppos += size;
        return retptr;
      } else {
        exit(1);
      }
    }
  }

  void *p = (void *)real_malloc(size);
  if (p != NULL) {
    stats_total_malloc_size += size;
    trace_to_file('M', p, size);
  }

  return p;
}
#endif

/* ---- MALLOC END ---- */

/* ---- DLOPEN START ---- */

typedef void *(*real_dlopen_t)(const char *, int);
static real_dlopen_t real_dlopen = nullptr;

void *dlopen(const char *filename, int flag) throw() {
  if (!real_dlopen) {
    real_dlopen = (real_dlopen_t)dlsym(RTLD_NEXT, "dlopen");
    if (!real_dlopen) {
      debug("dlsym problem: %s", dlerror());
      exit(1);
    }
  }

  void *ret = real_dlopen(filename, flag);

  load_vtablemaps();

  return ret;
}

/* ---- DLOPEN END ---- */

/* ---- FREE START ---- */

void free(void *ptr) throw() {
  stats_free_all++;
  if (ptr == NULL) {
    stats_free_null++;
    return;
  }

#ifdef TRACE_MEM_TO_FILE
  if (ptr >= (void *)tmpbuff && ptr <= (void *)(tmpbuff + tmppos)) {
    debug("Freeing temp memory\n");
    return;
  }
#endif

  // debug("------- free(%p) --------", ptr);
  if (!real_free) {
    real_free = (real_free_t)dlsym(RTLD_NEXT, "free");
    if (!real_free) {
      debug("dlsym problem: %s", dlerror());
      exit(1);
    }
  }

  if (ptr == 0) {
    trace_to_file('F', ptr);
    real_free(ptr);
    return;
  }

  if (!vtpin_system_in_place) {
    stats_free_unhandled++;
    trace_to_file('F', ptr);
    real_free(ptr);
    return;
  }

#ifdef HANDLE_SEGFAULTS
  /* this one is called from exception handler.  */
  if (vtpin_in_exception) {
    // debug("----- IS IN EXCEPTION ------");
    vtpin_in_exception = 0;
    trace_to_file('F', ptr);
    real_free(ptr);
    return;
  }
#endif

  VTResolution vtr = is_vtable(DEREFERENCE(ptr));
  if (vtr == IS_VTABLE) {
    goto release_vobject;
  }

  if (vtr == IS_NOT_VTABLE) {
    stats_free_non_vobject++;
    trace_to_file('F', ptr);
    real_free(ptr);
    return;
  }
  stats_free_possible_vobject++;

// Otherwise if it's UNCLEAR_IF_VTABLE
// use second primitive
#ifdef HANDLE_SEGFAULTS
  pthread_mutex_lock(&vcheck);
  try {
    vtpin_testing_rtti_primitive = 1;
#endif
#if __cplusplus > 199711L
    // debug("----- Triggering rtti primitive in post c++11 std -----");
    if (trigger_non_rtti_segfault_post_cpp11(ptr)) {
      // Not a real object instance so delete it
      stats_free_non_vobject++;
      stats_free_exception++;
      trace_to_file('F', ptr);
      real_free(ptr);
      pthread_mutex_unlock(&vcheck);
      return;
    }
#else
  // debug("----- Triggering rtti primitive in pre c++11 std -----");
  trigger_non_rtti_segfault_pre_cpp11(ptr); // IGNORE FOR NOW
#endif

#ifdef HANDLE_SEGFAULTS
  } catch (...) {
    // Not an object instance so delete it
    // debug("----- Segfaulted! -----");
    stats_free_non_vobject++;
    vtpin_testing_rtti_primitive = 0;
    stats_free_exception++;
    trace_to_file('F', ptr);
    real_free(ptr);
    pthread_mutex_unlock(&vcheck);
    return;
  }

  vtpin_testing_rtti_primitive = 0;
  pthread_mutex_unlock(&vcheck);
#endif

release_vobject:
  // debug("Releasing vobject: %p", ptr);
  stats_free_vobject++;

  size_t ptr_size = malloc_usable_size(ptr);
  // debug("malloc_useable_size(ptr) : %d\n", ptr_size);
  stats_total_size_of_vobjects += ptr_size;

  int tmp_vtable_ptrs = try_multi_inherit(ptr, vtpin_board_vtable);
  if (tmp_vtable_ptrs == 0) {
    // test reallocing instead of actually freeing
    trace_to_file('R', ptr, 8);
    vtpit_add_object((unsigned long)ptr);
    void *res = realloc(ptr, sizeof(void *));
    // assert(res == ptr); // verify reallocation happens on the same address
    memcpy(res, vtpin_board_vtable, sizeof(void *));
  } else {
    vtpit_add_object((unsigned long)ptr); /* OR */ // real_free(ptr);
    trace_to_file('F', ptr);
    stats_free_multi_inherit++;
    stats_free_multi_inherit_vptrs += tmp_vtable_ptrs;
    stats_free_multi_inherit_vptrs++; 
  }

  // debug("----- End of free() -----");
  return;
}

/* ---- FREE END ---- */

} /* ---- EXTERN C END ---- */