#include <exception>
#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <typeinfo>
#include <unistd.h>
#include <vector>

#include "family.h"
#include "vtpin_rtti.h"
#include "vtpin_dbg.h"
#include "vtpin_test.h"
#include "xfamily.h"

using namespace std;

static void test_free_calls_for_mult_inheritance(void) {
  Twins *twins = new Twins();

  // Call same function on same instance
  twins->cry();

  // Delete initial instance, *boy should now point to a free place in memory
  debug("Deleting twins");
  delete twins;
  // Debug should print output each time free() is called
}

static void test_use_after_free(void) {
  Boy *boy;
  Girl *girl;
  Twins *twins = new Twins();

  boy = static_cast<Boy *>(twins);   /* OR */ // (Boy *)twins;
  girl = static_cast<Girl *>(twins); /* OR */ // (Boy *)twins;

  // Call same function on same instance
  twins->cry_loud(42);
  boy->cry();
  girl->cry();

  // Delete initial instance, *boy should now point to a free place in memory
  debug("Deleting twins");
  delete twins;
  sleep(3);
  // these should be caught by VTPin, or otherwise crash
  boy->cry();
  twins->cry_loud(42);
  girl->cry();
  boy->cry_loud(43);
  twins->cry();

  // cleanup
  delete boy;
}

static void test_object_inside_class(void) {
  XTwins *xtwins = new XTwins();

  xtwins->cry();

  debug("Deleting twins");
  delete xtwins;
}

static void test_position_of_vtables(void) {
  Boy *boy = new Boy();
  Boy *boy2 = new Boy();
  XTwins *xtwins = new XTwins();

  // breakpoint here, check vtables
  debug("Vtable of boy2: %p , Vtable of boy: %p", DEREFERENCE(boy2),
        DEREFERENCE(boy));
  debug("VTable difference %u", &DEREFERENCE(boy2) - &DEREFERENCE(boy));
  debug("-------");

  debug("Size of all vobject pointers: %zu",
        (sizeof(boy2) + sizeof(boy) + sizeof(xtwins)));

  debug("Deleting vobjects.");
  delete xtwins;
  delete boy2;
  delete boy;
}

static void test_segfault_in_non_vobjects(void) {
  // compile test with -fno-rtti for this to segfault
  Boy *boy = new Boy();

  delete boy;
}

static void *test_thread(void *threadid) {
  for (int i = 0; i < 2; i++) {
    test_segfault_in_non_vobjects();
  }
  pthread_exit(NULL);
}

static int test_allocations(void *arg) {
  for (int i = 0; i < 2; i++) {
    // test_segfault_in_non_vobjects();
    try {
      volatile int *m = NULL;
      *m += 1;
    } catch (...) {
      printf("Caught exception \n");
    }
  }
  return 0;
}

static int test_threading(void *arg) {
  int thread_num = 2;
  pthread_t threads[thread_num];
  for (long t = 0; t < thread_num; t++) {
    printf("In test: creating thread %ld\n", t);
    int rc = pthread_create(&threads[t], NULL, test_thread, (void *)t);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  /* Last thing that main() should do */
  pthread_exit(NULL);
  return 0;
}

static void test_fork() {
  pid_t pID = fork();
  if (pID != 0) {
    debug("Child process!");
    test_segfault_in_non_vobjects();
    _exit(0);
  } else if (pID < 0) {
    debug("Failed to fork!");
  } else {
    debug("Parent process!");
    test_segfault_in_non_vobjects();

    system("LD_PRELOAD=./libvtpin.so LD_LIBRARY_PATH=. ./vtpin_test 5");

    char *argv[] = {"./vtpin_test", "5", 0};
    char *envp[] = {"LD_PRELOAD=./libvtpin.so", "LD_LIBRARY_PATH=.", 0};
    debug("Doing EXEC");
    execve(argv[0], &argv[0], envp);
  }
  debug("done\n");
}

static void test_heap_scan() {
  std::vector<Boy *> vObjects;

  debug("Growing heap");
  for (int i = 0; i < 16000000; i++) {
    vObjects.push_back(new Boy());
  }

  for (int i = 0; i < 1000002; i++) {
    delete vObjects.back();
    vObjects.pop_back();
  }

  debug("Delete vobjects, trigger heap scan");
  // vObjects.erase(vObjects.begin(),vObjects.begin() + 50);
  vObjects.shrink_to_fit();

  sleep(3);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    debug("Choose test to run!");
    return -1;
  }
  switch (*argv[1]) {
  case '1':
    debug("###################### test_free_calls_for_mult_inheritance "
          "######################");
    test_free_calls_for_mult_inheritance();
    break;
  case '2':
    debug("###################### test_use_after_free ######################");
    test_use_after_free();
    break;
  case '3':
    debug("###################### test_object_inside_class "
          "######################");
    test_object_inside_class();
    break;
  case '4':
    debug("###################### test_position_of_vtables "
          "######################");
    test_position_of_vtables();
    break;
  case '5':
    debug("###################### test_segfault_in_non_vobjects "
          "######################");
    test_segfault_in_non_vobjects();
    break;
  case '6':
    debug("###################### test_threading ######################");
    test_threading(NULL);
    break;
  case '7':
    debug("###################### test_fork ######################");
    test_fork();
    break;
  case '8':
    debug("###################### test_heap_scan ######################");
    test_heap_scan();
    break;
  default:
    debug("No test ran.");
    break;
  }

  debug("###################### end tests ######################");
  return 0;
}
