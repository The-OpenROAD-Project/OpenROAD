// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2015, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "defiPartition.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiPartition
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiPartition::defiPartition(defrData *data)
: defData(data)
{
  Init();
}


void defiPartition::Init() {
  name_ = 0;
  nameLength_ = 0;
  pin_ = 0;
  pinLength_ = 0;
  inst_ = 0;
  instLength_ = 0;

  pinsAllocated_ = 0;
  numPins_ = 0;
  pins_ = 0;

  clear();
}


defiPartition::~defiPartition() {
  Destroy();
}


void defiPartition::Destroy() {

  if (name_) free(name_);
  name_ = 0;
  nameLength_ = 0;
  if (pin_) free(pin_);
  pin_ = 0;
  pinLength_ = 0;
  if (inst_) free(inst_);
  inst_ = 0;
  instLength_ = 0;

  clear();

  if (pins_) free((char*)(pins_));
  pins_ = 0;
  pinsAllocated_ = 0;
}


void defiPartition::clear() {
  int i;

  setup_ = ' ';
  hold_ = ' ';
  direction_ = ' ';
  type_ = ' ';
  if (name_) *(name_) = '\0';
  if (pin_) *(pin_) = '\0';
  if (inst_) *(inst_) = '\0';
  hasMin_ = 0;
  hasMax_ = 0;

  if (numPins_) {
    for (i = 0; i < numPins_; i++) {
      free(pins_[i]);
      pins_[i] = 0;
    }
    numPins_ = 0;
  }
  hasRiseMin_ = 0;
  hasFallMin_ = 0;
  hasRiseMax_ = 0;
  hasFallMax_ = 0;
  hasRiseMinRange_ = 0;
  hasFallMinRange_ = 0;  
  hasRiseMaxRange_ = 0;
  hasFallMaxRange_ = 0;
}


void defiPartition::setName(const char* name) {
  int len = strlen(name) + 1;

  clear();

  if (len > nameLength_) {
    if (name_) free(name_);
    nameLength_ = len;
    name_ = (char*)malloc(len);
  }
  strcpy(name_, defData->DEFCASE(name));

}


void defiPartition::print(FILE* f) const {
  int i;

  fprintf(f, "Partition '%s' %c\n",
       name(), direction());
  fprintf(f, "  inst %s  pin %s  type %s\n",
       instName(), pinName(),
       itemType());

  for (i = 0; i < numPins(); i++)
    fprintf(f, "  %s\n", pin(i));

  if (isSetupRise())
    fprintf(f, "  SETUP RISE\n");

  if (isSetupFall())
    fprintf(f, "  SETUP FALL\n");

  if (isHoldRise())
    fprintf(f, "  HOLD RISE\n");

  if (isHoldFall())
    fprintf(f, "  HOLD FALL\n");

  if (hasMin())
    fprintf(f, "  MIN %g\n", partitionMin());

  if (hasMax())
    fprintf(f, "  MAX %g\n", partitionMax());

  if (hasRiseMin())
    fprintf(f, "  RISE MIN %g\n", riseMin());

  if (hasFallMin())
    fprintf(f, "  FALL MIN %g\n", fallMin());

  if (hasRiseMax())
    fprintf(f, "  RISE MAX %g\n", riseMax());

  if (hasFallMax())
    fprintf(f, "  FALL MAX %g\n", fallMax());

  if (hasFallMinRange())
    fprintf(f, "  FALL MIN %g %g\n", fallMinLeft(),
                                  fallMinRight());

  if (hasRiseMinRange())
    fprintf(f, "  RISE MIN %g %g\n", riseMinLeft(),
                                  riseMinRight());

  if (hasFallMaxRange())
    fprintf(f, "  FALL MAX %g %g\n", fallMaxLeft(),
                                  fallMaxRight());

  if (hasRiseMaxRange())
    fprintf(f, "  RISE MAX %g %g\n", riseMaxLeft(),
                                  riseMaxRight());
}


const char* defiPartition::name() const {
  return name_;
}


void defiPartition::setFromIOPin(const char* pin) {
  set('F', 'I', "", pin);
}


char defiPartition::direction() const {
  return direction_;
}


const char* defiPartition::instName() const {
  return inst_;
}


const char* defiPartition::pinName() const {
  return pin_;
}


static char* ad(const char* in) {
  return (char*)in;
}


const char* defiPartition::itemType() const {
  char* c;
  if (type_ == 'L') c = ad("CLOCK");
  else if (type_ == 'I') c = ad("IO");
  else if (type_ == 'C') c = ad("COMP");
  else c = ad("BOGUS");
  return c;
}


const char* defiPartition::pin(int index) const {
  return pins_[index];
}


int defiPartition::numPins() const {
  return numPins_;
}


int defiPartition::isSetupRise() const {
  return setup_ == 'R' ? 1 : 0 ;
}


int defiPartition::isSetupFall() const {
  return setup_ == 'F' ? 1 : 0 ;
}


int defiPartition::isHoldRise() const {
  return hold_ == 'R' ? 1 : 0 ;
}


int defiPartition::isHoldFall() const {
  return hold_ == 'F' ? 1 : 0 ;
}


void defiPartition::addTurnOff(const char* setup, const char* hold) {
  if (*setup == ' ') {
    setup_ = *setup;
  } else if (*setup == 'R') {
    setup_ = *setup;
  } else if (*setup == 'F') {
    setup_ = *setup;
  } else {
    defiError(0, 6100, "ERROR (DEFPARS-6100): The value spefified for PARTITION SETUP is invalid. The valid value for SETUP is 'R' or 'F'. Specify a valid value for SETUP and then try again.", defData);
  }

  if (*hold == ' ') {
    hold_ = *hold;
  } else if (*hold == 'R') {
    hold_ = *hold;
  } else if (*hold == 'F') {
    hold_ = *hold;
  } else {
    defiError(0, 6101, "ERROR (DEFPARS-6101): The value spefified for PARTITION HOLD is invalid. The valid value for HOLD is 'R' or 'F'. Specify a valid value for HOLD and then try again.", defData);
  }

}


void defiPartition::setFromClockPin(const char* inst, const char* pin) {
  set('F', 'L', inst, pin);
}


void defiPartition::setToClockPin(const char* inst, const char* pin) {
  set('T', 'L', inst, pin);
}


void defiPartition::set(char dir, char typ, const char* inst, const char* pin) {
  int len = strlen(pin) + 1;
  direction_ = dir;
  type_ = typ;

  if (pinLength_ <= len) {
    if (pin_) free(pin_);
    pin_ = (char*)malloc(len);
    pinLength_ = len;
  }

  strcpy(pin_, defData->DEFCASE(pin));

  len = strlen(inst) + 1;
  if (instLength_ <= len) {
    if (inst_) free(inst_);
    inst_ = (char*)malloc(len);
    instLength_ = len;
  }

  strcpy(inst_, defData->DEFCASE(inst));
}


void defiPartition::setMin(double min, double max) {
  min_ = min;
  max_ = max;
  hasMin_ = 1;
}


void defiPartition::setFromCompPin(const char* inst, const char* pin) {
  set('F', 'C', inst, pin);
}


void defiPartition::setMax(double min, double max) {
  min_ = min;
  max_ = max;
  hasMax_ = 1;
}


void defiPartition::setToIOPin(const char* pin) {
  set('T', 'I', "", pin);
}


void defiPartition::setToCompPin(const char* inst, const char* pin) {
  set('T', 'C', inst, pin);
}


void defiPartition::addPin(const char* name) {
  int len;
  int i;
  char** newp;

  if (numPins_ >= pinsAllocated_) {
    pinsAllocated_ = pinsAllocated_ ? 2 * pinsAllocated_ : 8;
    newp = (char**) malloc(sizeof(char*) * pinsAllocated_);
    for (i = 0; i < numPins_; i++)
      newp[i] = pins_[i];
    if (pins_) free((char*)(pins_));
    pins_ = newp;
  }

  len = strlen(name) + 1;
  pins_[numPins_] = (char*)malloc(len);
  strcpy(pins_[numPins_], defData->DEFCASE(name));
  numPins_ += 1;
}


int defiPartition::hasMin() const {
  return(int)(hasMin_);
}


int defiPartition::hasMax() const {
  return(int)(hasMax_);
}


double defiPartition::partitionMin() const {
  return(min_);
}


double defiPartition::partitionMax() const {
  return(max_);
}

int defiPartition::hasRiseMin() const {
  return (int)(hasRiseMin_);
}


int defiPartition::hasFallMin() const {
  return (int)(hasFallMin_);
}


int defiPartition::hasRiseMax() const {
  return (int)(hasRiseMax_);
}


int defiPartition::hasFallMax() const {
  return (int)(hasFallMax_);
}


int defiPartition::hasRiseMinRange() const {
  return (int)(hasRiseMinRange_);
}


int defiPartition::hasFallMinRange() const {
  return (int)(hasFallMinRange_);
}


int defiPartition::hasRiseMaxRange() const {
  return (int)(hasRiseMaxRange_);
}


int defiPartition::hasFallMaxRange() const {
  return (int)(hasFallMaxRange_);
}


double defiPartition::riseMin() const {
  return riseMin_;
}


double defiPartition::fallMin() const {
  return fallMin_;
}


double defiPartition::riseMax() const {
  return riseMax_;
}


double defiPartition::fallMax() const {
  return fallMax_;
}


double defiPartition::riseMinLeft() const {
  return riseMinLeft_;
}


double defiPartition::fallMinLeft() const {
  return fallMinLeft_;
}


double defiPartition::riseMaxLeft() const {
  return riseMaxLeft_;
}


double defiPartition::fallMaxLeft() const {
  return fallMaxLeft_;
}


double defiPartition::riseMinRight() const {
  return riseMinRight_;
}


double defiPartition::fallMinRight() const {
  return fallMinRight_;
}


double defiPartition::riseMaxRight() const {
  return riseMaxRight_;
}


double defiPartition::fallMaxRight() const {
  return fallMaxRight_;
}


void defiPartition::addRiseMin(double d) {
  hasRiseMin_ = 1;
  riseMin_ = d;
}


void defiPartition::addRiseMax(double d) {
  hasRiseMax_ = 1;
  riseMax_ = d;
}


void defiPartition::addFallMin(double d) {
  hasFallMin_ = 1;
  fallMin_ = d;
}


void defiPartition::addFallMax(double d) {
  hasFallMax_ = 1;
  fallMax_ = d;
}


void defiPartition::addRiseMinRange(double l, double h) {
  hasRiseMinRange_ = 1;
  riseMinLeft_ = l;
  riseMinRight_ = h;
}


void defiPartition::addRiseMaxRange(double l, double h) {
  hasRiseMaxRange_ = 1;
  riseMaxLeft_ = l;
  riseMaxRight_ = h;
}


void defiPartition::addFallMinRange(double l, double h) {
  hasFallMinRange_ = 1;
  fallMinLeft_ = l;
  fallMinRight_ = h;
}


void defiPartition::addFallMaxRange(double l, double h) {
  hasFallMaxRange_ = 1;
  fallMaxLeft_ = l;
  fallMaxRight_ = h;
}


END_LEFDEF_PARSER_NAMESPACE

