/*
 * Copyright 2016, Pawel Sarbinowski, Elias Athanasopoulos
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <dlfcn.h>
#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtpin_dbg.h"
#include "vtpin_extras.h"
#include "vtpin.h"

#ifdef HANDLE_SEGFAULTS

sighandler_t __sys_signal_handler = NULL;
void (*__sys_sigaction_handler)(int, siginfo_t *, void *) = NULL;

static pthread_mutex_t sigaction_check = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t signal_check = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t vtpin_in_exception = 0;
volatile sig_atomic_t vtpin_testing_rtti_primitive = 0;

class segmentation_fault {
public:
  segmentation_fault() {
  } // debug("Segfault constructor: %p", (unsigned long)this); }
  ~segmentation_fault() {
  } // debug("Segfault destructor: %p", (unsigned long)this); }
};

/* signal(2) hook. */
extern "C" sighandler_t signal(int signum, sighandler_t handler) {
    sighandler_t __prev_handler; // previous signal handler

    typedef sighandler_t (*fptr)(int signum, sighandler_t handler);
    static fptr __sys_signal = NULL;

    if (!__sys_signal && !(__sys_signal = (fptr)dlsym(RTLD_NEXT, "signal"))) {
        perror("VTPin: failed to locate signal(2)");
        exit(EXIT_FAILURE);
    }

    // do it
    if (signum != SIGSEGV)
        return __sys_signal(signum, handler);

    // save the signal handler
    __prev_handler = __sys_signal_handler;
    __sys_signal_handler = handler;

    // return the previous signal handler
    return __prev_handler;
}


 * sigaction(2) hook.

extern "C" int sigaction(int signum, const struct sigaction *act, struct
sigaction *oldact) {
    typedef int (*fptr)(int signum, const struct sigaction *act, struct
sigaction *oldact);
    static fptr __sys_sigaction = NULL;

    if (!__sys_sigaction && !(__sys_sigaction = (fptr)dlsym(RTLD_NEXT,
"sigaction"))) {
        perror("VTPin: failed to locate sigaction(2)");
        exit(EXIT_FAILURE);
    }

    // do it
    if (signum != SIGSEGV)
        return __sys_sigaction(((signum == -SIGSEGV) ? -signum : signum), act,
oldact);

    // save the signal handler
    if (act)
        __sys_sigaction_handler = act->sa_sigaction;

    // success
    return __sys_sigaction(signum, NULL, oldact);
}

void propagate_segfault(int signum, siginfo_t *siginfo, void *context) {
#ifdef DEBUG
// print_stacktrace(signum);
#endif
  // debug("vtpin_testing_rtti_primitive: %s", vtpin_testing_rtti_primitive == 1
  // ? "true" : "false");
  /*if (!vtpin_testing_rtti_primitive && __sys_sigaction_handler != NULL) {
      //debug("app segfault sigaction");
      __sys_sigaction_handler(signum, siginfo, context);
  } else if (!vtpin_testing_rtti_primitive && __sys_signal_handler != NULL) {
      //debug("app segfault signal");
      __sys_signal_handler(signum);
  } else {*/

  debug("my segfault %d self: %d pid: %d ppid: %d", signum, pthread_self(),
        ::getpid(), ::getppid());
  vtpin_in_exception = 1;
  vtpin_testing_rtti_primitive = 0;
  sigset_t x;
  sigemptyset(&x);
  sigaddset(&x, SIGSEGV);
  sigprocmask(SIG_UNBLOCK, &x, NULL); // for single threaded
  // pthread_sigmask(SIG_UNBLOCK, &x, NULL); // for multi threaded

  throw new segmentation_fault();
  //}
}

#endif 

/* Declares signal handling as follows:
SIGSEGV --> propagate_segfault()
SIGTERM and SIGINT --> handle_termination() */
void establish_sighandler(void) {
#ifdef HANDLE_SEGFAULTS
  debug("Establishing sighandler");
  struct sigaction newSEGV;

  memset(&newSEGV, 0, sizeof newSEGV);
  sigemptyset(&newSEGV.sa_mask);
  newSEGV.sa_sigaction = propagate_segfault;
  newSEGV.sa_flags = SA_SIGINFO;

  int res = sigaction(SIGSEGV, &newSEGV, NULL); //-SIGSEGV
  if (res) {
    debug("Sigaction returned: %d", res);
    exit(1);
  }

  struct sigaction newTERM;

  memset(&newTERM, 0, sizeof newTERM);
  sigemptyset(&newTERM.sa_mask);
  newTERM.sa_sigaction = handle_termination;
  newTERM.sa_flags = SA_SIGINFO;

  res = sigaction(SIGTERM, &newTERM, NULL);
  if (res) {
    debug("Sigaction returned: %d", res);
    exit(1);
  }
  struct sigaction newINT;

  memset(&newINT, 0, sizeof newINT);
  sigemptyset(&newINT.sa_mask);
  newINT.sa_sigaction = handle_termination;
  newINT.sa_flags = SA_SIGINFO;

  res = sigaction(SIGINT, &newINT, NULL);
  if (res) {
    debug("Sigaction returned: %d", res);
    exit(1);
  }
  debug("Established sighandler");
#endif
}

