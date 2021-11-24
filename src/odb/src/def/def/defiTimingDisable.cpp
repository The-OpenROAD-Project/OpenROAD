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
#include "defiTimingDisable.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE


//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiTimingDisable
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiTimingDisable::defiTimingDisable(defrData *data)
 : defData(data)
{
  Init();
}


void defiTimingDisable::Init() {
  fromInst_ = 0;
  fromInstLength_ = 0;
  toInst_ = 0;
  toInstLength_ = 0;
  toPin_ = 0;
  toPinLength_ = 0;
  fromPin_ = 0;
  fromPinLength_ = 0;
}


defiTimingDisable::~defiTimingDisable() {
  Destroy();
}


void defiTimingDisable::Destroy() {

  clear();

  if (fromInst_) free(fromInst_);
  fromInst_ = 0;
  fromInstLength_ = 0;

  if (toInst_) free(toInst_);
  toInst_ = 0;
  toInstLength_ = 0;

  if (toPin_) free(toPin_);
  toPin_ = 0;
  toPinLength_ = 0;

  if (fromPin_) free(fromPin_);
  fromPin_ = 0;
  fromPinLength_ = 0;

}


void defiTimingDisable::clear() {
  hasFromTo_ = 0;
  hasThru_ = 0;
  hasMacro_ = 0;
  hasReentrantPathsFlag_ = 0;
}


void defiTimingDisable::setReentrantPathsFlag() {
  hasReentrantPathsFlag_ = 1;
}


void defiTimingDisable::setFromTo(const char* fromInst, const char* fromPin,
	 const char* toInst, const char* toPin) {
  int len;

  clear();
  hasFromTo_ = 1;

  len = strlen(fromInst) + 1;
  if (len > fromInstLength_) {
    if (fromInst_) free(fromInst_);
    fromInstLength_ = len;
    fromInst_ = (char*)malloc(len);
  }
  strcpy(fromInst_,defData->DEFCASE(fromInst));

  len = strlen(fromPin) + 1;
  if (len > fromPinLength_) {
    if (fromPin_) free(fromPin_);
    fromPinLength_ = len;
    fromPin_ = (char*)malloc(len);
  }
  strcpy(fromPin_,defData->DEFCASE(fromPin));

  len = strlen(toInst) + 1;
  if (len > toInstLength_) {
    if (toInst_) free(toInst_);
    toInstLength_ = len;
    toInst_ = (char*)malloc(len);
  }
  strcpy(toInst_, toInst);

  len = strlen(toPin) + 1;
  if (len > toPinLength_) {
    if (toPin_) free(toPin_);
    toPinLength_ = len;
    toPin_ = (char*)malloc(len);
  }
  strcpy(toPin_, toPin);

}


void defiTimingDisable::setThru(const char* fromInst, const char* fromPin) {
  int len;

  clear();
  hasThru_ = 1;

  len = strlen(fromInst) + 1;
  if (len > fromInstLength_) {
    if (fromInst_) free(fromInst_);
    fromInstLength_ = len;
    fromInst_ = (char*)malloc(len);
  }
  strcpy(fromInst_,defData->DEFCASE(fromInst));

  len = strlen(fromPin) + 1;
  if (len > fromPinLength_) {
    if (fromPin_) free(fromPin_);
    fromPinLength_ = len;
    fromPin_ = (char*)malloc(len);
  }
  strcpy(fromPin_,defData->DEFCASE(fromPin));

}


void defiTimingDisable::setMacroFromTo(const char* fromPin, const char* toPin) {
  int len;

  clear();
  hasFromTo_ = 1;

  len = strlen(fromPin) + 1;
  if (len > fromPinLength_) {
    if (fromPin_) free(fromPin_);
    fromPinLength_ = len;
    fromPin_ = (char*)malloc(len);
  }
  strcpy(fromPin_,defData->DEFCASE(fromPin));

  len = strlen(toPin) + 1;
  if (len > toPinLength_) {
    if (toPin_) free(toPin_);
    toPinLength_ = len;
    toPin_ = (char*)malloc(len);
  }
  strcpy(toPin_,defData->DEFCASE(toPin));

}


void defiTimingDisable::setMacroThru(const char* thru) {
  int len;

  clear();

  hasThru_ = 1;

  len = strlen(thru) + 1;
  if (len > fromPinLength_) {
    if (fromPin_) free(fromPin_);
    fromPinLength_ = len;
    fromPin_ = (char*)malloc(len);
  }
  strcpy(fromPin_,defData->DEFCASE(thru));

}


void defiTimingDisable::setMacro(const char* name) {
  int len;

  // hasThru_ or hasFromTo_ was already set.
  // clear() was already called.
  hasMacro_ = 1;

  len = strlen(name) + 1;
  if (len > fromInstLength_) {
    if (fromInst_) free(fromInst_);
    fromInstLength_ = len;
    fromInst_ = (char*)malloc(len);
  }
  strcpy(fromInst_,defData->DEFCASE(name));
}


void defiTimingDisable::print(FILE* f) const {

  if (hasMacroFromTo()) {
    fprintf(f, "TimingDisable macro '%s' thru '%s'\n",
	fromInst_, fromPin_);

  } else if (hasMacroThru()) {
    fprintf(f, "TimingDisable macro '%s' from '%s' to '%s'\n",
	fromInst_, fromPin_, toPin_);

  } else if (hasFromTo()) {
    fprintf(f, "TimingDisable from '%s' '%s'  to '%s' '%s'\n",
      fromInst_, fromPin_, toInst_, toPin_);

  } else if (hasThru()) {
    fprintf(f, "TimingDisable thru '%s' '%s'\n",
      fromInst_, fromPin_);

  } else {
    defiError(0, 6170, "ERROR (DEFPARS-6170): The TimingDisable type is invalid. The valid types are FROMPIN, & THRUPIN. Specify the valid type and then try again.", defData);
  }
}


int defiTimingDisable::hasReentrantPathsFlag() const {
  return hasReentrantPathsFlag_;
}


int defiTimingDisable::hasMacroFromTo() const {
  return (hasMacro_ && hasFromTo_) ? 1 : 0;
}


int defiTimingDisable::hasMacroThru() const {
  return (hasMacro_ && hasThru_) ? 1 : 0;
}


int defiTimingDisable::hasThru() const {
  return (hasMacro_ == 0 && hasThru_) ? 1 : 0;
}


int defiTimingDisable::hasFromTo() const {
  return (hasMacro_ == 0 && hasFromTo_) ? 1 : 0;
}


const char* defiTimingDisable::toPin() const {
  return toPin_;
}


const char* defiTimingDisable::fromPin() const {
  return fromPin_;
}


const char* defiTimingDisable::toInst() const {
  return toInst_;
}


const char* defiTimingDisable::fromInst() const {
  return fromInst_;
}


const char* defiTimingDisable::macroName() const {
  return fromInst_;
}


const char* defiTimingDisable::thruPin() const {
  return fromPin_;
}


const char* defiTimingDisable::thruInst() const {
  return fromInst_;
}


END_LEFDEF_PARSER_NAMESPACE

