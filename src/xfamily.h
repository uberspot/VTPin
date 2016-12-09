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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "family.h"
#include "vtpin_dbg.h"

using namespace std;

class XBaby {
public:
  virtual void cry(){};
  virtual void cry_loud(int i){};
  XBaby() { debug("Constructor/XBaby called"); };
  virtual ~XBaby() { debug("Destructor/XBaby called"); };
};

class XBoy : public XBaby {
  unsigned int age;

public:
  XBoy() {
    age = 0;
    debug("Constructor/XBoy called");
  };
  void cry() {
    age = 0;
    debug("It's an xboy");
  };
  void cry_loud(int i) { debug("cry loud: %d", i); };
  ~XBoy() { debug("Destructor/XBoy called"); };
};

class XGirl : public XBaby {
private:
  unsigned int age;

public:
  void cry() {
    age = 0;
    debug("It's an xgirl");
  };
  ~XGirl() { debug("Constructor/XGirl called"); };
  XGirl() { debug("Destructor/XGirl called"); };
};

class XTwins : public XBoy, public XGirl {
private:
  unsigned int age;
  Twins twins;

public:
  void cry() {
    age = 0;
    debug("It's xtwins");
  };
  ~XTwins() { debug("Constructor/XTwins called"); };
  XTwins() { debug("Destructor/XTwins called"); };
};
