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

#include <iostream>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typeinfo>
#include <unistd.h>

#include "family.h"
#include "vtpin_rtti.h"
#include "vtpin_dbg.h"

using namespace std;

static void test_use_after_free(void) {
  Boy *boy = new Boy();
  Aunt *aunt = new Aunt();
  Girl *girl = new Girl();
  Twins *twins = new Twins();
  Cousin *cousin = new Cousin();

  asm("nop");
  aunt = (Aunt *)(twins);
  asm("nop");
  girl = dynamic_cast<Girl *>(twins);
  asm("nop");
  boy = static_cast<Boy *>(twins);
  asm("nop");
  twins = dynamic_cast<Twins *>(girl);
  asm("nop");

  // Call same function on same instance
  twins->cry_loud(42);
  asm("nop");
  girl->cry();
  asm("nop");
  cousin->cry();
  asm("nop");

  // Delete initial instance, *boy should now point to a free place in memory
  debug("Deleting twins");
  delete twins;
  /*delete boy;
  delete girl;
  delete cousin;
  delete aunt;
*/
  // these should be caught by VTPin, or otherwise crash
  boy->cry();         // caught by vtpin
  girl->cry();        // calls Baby/cry
  aunt->cry_loud(33); // segfaults if not overwritten
                      // cousin->cry_loud(10);
}

int main(int argc, char *argv[]) {
  debug("###################### test_use_after_free ######################");
  test_use_after_free();
  debug("###################### end test ######################");

  return 0;
}