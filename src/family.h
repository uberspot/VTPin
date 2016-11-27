#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtpin_dbg.h"

using namespace std;

class Baby {
public:
  virtual void cry() { debug("Baby/cry called"); };
  virtual void cry_loud(int i) { debug("Baby/cry loud: %d", i); };
  // Baby() { debug("Constructor/Baby called"); };
  virtual ~Baby() { debug("Destructor/Baby called"); };
};

class Girl : public Baby {
  unsigned int age;

public:
  void cry() { debug("Girl/Cry"); };
  // void cry_loud(int i) { debug("Boy/cry loud: %d", i); };
  Girl() {
    age = 2730; // 0xAAA
    debug("Constructor/Girl called");
  };
  virtual ~Girl() { debug("Destructor/Girl called"); };
};

class Boy : public Baby {
  unsigned int age, age2;

public:
  void cry() { debug("Boy/cry"); }
  void cry_loud(int i) { debug("Boy/cry loud: %d", i); };
  Boy() {
    age = age2 = 3003; // 0xBBB
    debug("Constructor/Boy called");
  };
  virtual ~Boy() { debug("Destructor/Boy called"); };
};

class Cousin : public Boy {
  unsigned int age, age2, age3;

public:
  void cry() { debug("Cousin/cry"); }
  void cry_loud(int i) { debug("Cousin/cry loud: %d", i); };
  Cousin() {
    age = age2 = age3 = 3549; // 0xDDD
    debug("Constructor/Cousin called");
  };
  virtual ~Cousin() { debug("Destructor/Cousin called"); };
};

class Aunt : public Baby {
  unsigned int age;

public:
  void cry_loud(int i) { debug("Aunt/cry loud: %d", i); };
  Aunt() {
    age = 3822; // 0xEEE
    debug("Constructor/Aunt called");
  };
  virtual ~Aunt() { debug("Destructor/Aunt called"); };
};

class Twins : public Boy, public Girl, public Aunt {
private:
  unsigned int age, age2;

public:
  void cry() { debug("Twins/cry()"); };
  void cry_loud(int i) { debug("Twins/cry_loud"); }
  Twins() {
    age = age2 = 3276; // 0xCCC
    debug("Constructor/Twins called");
  };
  virtual ~Twins() { debug("Destructor/Twins called"); };
};
