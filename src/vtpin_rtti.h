#pragma once

#include "vtpin_extras.h"
#include <fcntl.h> /* For O_RDWR */
#include <malloc.h>
#include <unistd.h>

#define DEREFERENCE(o) *(uintptr_t *)o
#define PTR_MINUS_ONE(o) (uintptr_t *)(o - sizeof(uintptr_t *))
#define PTR_PLUS_ONE(o) (uintptr_t *)(o + sizeof(uintptr_t *))
#define PTR_PLUS_TWO(o) (uintptr_t *)(o + (2 * sizeof(uintptr_t *)))

// heuristic based on how gcc positions the rtti metadata (just before the
// object itself)
// From http://refspecs.linuxbase.org/cxxabi-1.83.html#rtti 2.9.5 RTTI Layout
//  Every virtual table shall contain one entry that is a pointer to an object
//  derived from
//  std::type_info. This entry is located at the word preceding the location
//  pointed to by the
//  virtual pointer (i.e., entry "-1"). The entry is allocated in all virtual
//  tables; for classes
//  having virtual bases but no virtual functions, the entry is zero.
#define VTPIN_TYPEID(o) *(PTR_MINUS_ONE(DEREFERENCE(o)))
#define VTPIN_TYPE_INFO_NAME(o) *(PTR_PLUS_ONE(VTPIN_TYPEID(o)))
#define VTPIN_TYPE_INFO_BASE_COUNT(o) *(PTR_PLUS_TWO(VTPIN_TYPEID(o)))

// TODO Use __has_feature(cxx_rtti) to determine if C++ RTTI has been enabled.
// compiling code with -fno-rtti disables the use of RTTI.

/* MemMap with /proc/self/maps used as a first step to identify vtables */

static int rtti_trigger_fd{0};
#define RTTI_TRIGGER_INPUT_FILE "/dev/random"

void rtti_initialize() {
  rtti_trigger_fd = open(RTTI_TRIGGER_INPUT_FILE, O_WRONLY);
}

int is_mapped_address(const void *ptr) {
  if (!ptr) {
    return false;
  }
  int ret = true;
  if (!rtti_trigger_fd) {
    // failsafe, should never be called
    rtti_trigger_fd = open(RTTI_TRIGGER_INPUT_FILE, O_WRONLY);
  }

  ssize_t ret_write = write(rtti_trigger_fd, ptr, 1);
  if (ret_write < 0 || ret_write == EFAULT) {
    ret = false;
    // debug("Not a valid pointer! Write failed %zd", ret_write);
  }
  return ret;
}

void trigger_non_rtti_segfault_pre_cpp11(const void *ptr) {
  type_info *rtti;
  rtti = (type_info *)VTPIN_TYPEID(ptr);

  // throws segfault on purpose if it's not an actual instance 
  // of an object with a vtable
  const char *name = rtti->name();
}

/* returns true if the ptr DOESN'T have a valid (i.e. in paged memory) type_info
 * structure */
bool trigger_non_rtti_segfault_post_cpp11(const void *ptr) {
  // check ptr's vtable (1st dereference)
  if (!is_mapped_address((void *)DEREFERENCE(ptr))) {
    // debug("Invalid vtable ptr");
    return true;
  }

  // check rtti ptr (= vtable ptr -1 position)
  uintptr_t *ptr_m_one = PTR_MINUS_ONE(DEREFERENCE(ptr));
  if (!is_mapped_address((void *)ptr_m_one)) {
    // debug("Invalid vtable -1 ptr");
    return true;
  }

  // if (((unsigned long)p & (sizeof(uintptr_t*)- 1)) == 0) {
  //    return true;
  //}

  // check rtti pointer (dereference of vtable ptr -1 position)
  type_info *rtti;
  rtti = (type_info *)VTPIN_TYPEID(ptr); 
            // ^ FIX: UBSan reports "load of misaligned address 0x7fe1437521fc
            // for type 'uintptr_t', which requires 8 byte alignment", use one byte char ptr instead
  if (!is_mapped_address((void *)rtti)) {
    // debug("Invalid rtti ptr");
    return true;
  }

  // check rtti dereference ptr (it points to the type_info struct)
  if (!is_mapped_address((void *)DEREFERENCE(rtti))) {
    // debug("Invalid rtti dereference ptr");
    return true;
  }

  // check if the type_info vtable (a.k.a. the double dereference of the rtti
  // pointer)
  // is in r-- or r-x memory. The vtable shouldn't be writable.
  VTResolution vtr = is_vtable(DEREFERENCE(DEREFERENCE(rtti)));

  if (vtr == IS_NOT_VTABLE) {
    // debug("RTTI doesn't have vtable");
    return true;
  } else if (vtr == IS_VTABLE || vtr == UNCLEAR_IF_VTABLE) {
    // debug("*RTTI might be a vtable");
  }

  // check rtti ptr (= vtable ptr -1 position)
  if (!is_mapped_address((void *)(PTR_PLUS_ONE(DEREFERENCE(rtti))))) {
    // debug("Invalid vtable -1 ptr");
    return true;
  }

  // check if the type_info::name string is in paged memory
  if (!is_mapped_address((void *)VTPIN_TYPE_INFO_NAME(ptr))) {
    // debug("Invalid rtti name ptr");
    return true;
  }

  return false;
}

int try_multi_inherit(void *ptr, const uintptr_t *safe_vtable_ptr) {
  // get object size from (unsafe) dereference:
  // size_t obj_size = (unsigned long) *((void **)ptr - 1);

  // get object size from allocator api:
  size_t obj_size = malloc_usable_size(ptr);

  obj_size /= 8; // todo: replace hardcoded sizes
  int free_multi_inherit_vptrs = 0;

  unsigned long orig_vtable_ptr = (unsigned long)DEREFERENCE(ptr);
  unsigned long vtable_offset = 96; // next vtable at a +-96 offset
  unsigned long lower_offset = orig_vtable_ptr - vtable_offset;
  unsigned long upper_offset = orig_vtable_ptr + vtable_offset;
  // start from 1 cause you skip the first vtable pointer. We already know that
  // exists
  for (unsigned long i = 1; i < obj_size - 1; i++) {
    unsigned long position = (unsigned long)*((void **)ptr + i);
    if (position >= lower_offset && position <= upper_offset) {
      /* debug("Found multi inheritance at vtable pointer: %p", (void *) *((void **)ptr + i)); */
      memcpy((void *)((void **)ptr + i), safe_vtable_ptr, sizeof(void *));
      free_multi_inherit_vptrs++;
    }
  }

  if (free_multi_inherit_vptrs > 0) {
    memcpy((void *)((void **)ptr), safe_vtable_ptr, sizeof(void *));
  }

  return free_multi_inherit_vptrs;
}