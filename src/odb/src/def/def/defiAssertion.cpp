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
#include "defiAssertion.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

struct defiAssertPath {
  char* fromPin_;
  char* toPin_;
  char* fromInst_;
  char* toInst_;
};



defiAssertion::defiAssertion(defrData *data)
 : defData(data)
{
  Init();
}


defiAssertion::~defiAssertion() {
  Destroy();
}


void defiAssertion::Init() {
  netName_ = (char*)malloc(32);
  netNameLength_ = 32;
  numItems_ = 0;
  clear();
  numItemsAllocated_ = 16;
  items_ = (int**)malloc(sizeof(char*)*16);
  itemTypes_ = (char*)malloc(16);
}


void defiAssertion::Destroy() {
  free(netName_);
  free((char*)(itemTypes_));
  free((char*)(items_));
}


void defiAssertion::clear() {
  int i;
  struct defiAssertPath* s;

  if (netName_)
     *(netName_) = '\0';
  isSum_ = 0;
  isDiff_ = 0;
  isAssertion_ = 0;
  hasRiseMin_ = 0;
  hasRiseMax_ = 0;
  hasFallMin_ = 0;
  hasFallMax_ = 0;
  isDelay_ = 0;
  isWiredlogic_ = 0;

  for (i = 0; i < numItems_; i++) {
    if (itemTypes_[i] == 'p') {
      s = (struct defiAssertPath*)(items_[i]);
      free(s->fromPin_);
      free(s->toPin_);
      free(s->fromInst_);
      free(s->toInst_);
      free((char*)s);
    } else if (itemTypes_[i] == 'n') {
      free((char*)(items_[i]));
    } else {
      defiError(0, 6009, "ERROR (DEFPARSE-6009): An invalid attribute type has encounter while cleanning the memory.", defData);
    }
    itemTypes_[i] = 'B';  // bogus
    items_[i] = 0;
  }

  numItems_ = 0;
}


void defiAssertion::setConstraintMode() {
  isAssertion_ = 0;
}


void defiAssertion::setAssertionMode() {
  isAssertion_ = 1;
}


void defiAssertion::setWiredlogicMode() {
  isWiredlogic_ = 1;
}


void defiAssertion::setWiredlogic(const char* name, double dist) {
  int len = strlen(name) + 1;
  if (isDelay_)
    defiError(0, 6201, "ERROR (DEFPARS-6201): Unable to process the DEF file. Both WIREDLOGIC and DELAY statements are defined in constraint/assertion.\nUpdate the DEF file to define either a WIREDLOGIC or DELAY statement only.", defData);
  isWiredlogic_ = 1;
  if (netNameLength_ < len) {
    free(netName_);
    netName_ = (char*)malloc(len);
    netNameLength_ = len;
  }
  strcpy(netName_, defData->DEFCASE(name));
  fallMax_ = dist;
}


void defiAssertion::setDelay() {
  if (isWiredlogic_)
    defiError(0, 6201, "ERROR (DEFPARS-6201): Unable to process the DEF file. Both WIREDLOGIC and DELAY statements are defined in constraint/assertion.\nUpdate the DEF file to define either a WIREDLOGIC or DELAY statement only.", defData);
  isDelay_ = 1;
}


void defiAssertion::setSum() {
  if (isDiff_)
     defiError(0, 6202, "ERROR (DEPPARS-6202): Unable to process the DEF file. Both SUM and DIFF statements are defined in constraint/assertion.\nUpdate the DEF file to define either a SUM or DIFF statement only.", defData);
  isSum_ = 1;
}


void defiAssertion::unsetSum() {
  isSum_ = 0;
}


void defiAssertion::setDiff() {
  if (isSum_)
     defiError(0, 6202, "ERROR (DEPPARS-6202): Unable to process the DEF file. Both SUM and DIFF statements are defined in constraint/assertion.\nUpdate the DEF file to define either a SUM or DIFF statement only.", defData);
  isDiff_ = 1;
}


const char* defiAssertion::netName() const {
  return netName_;
}


void defiAssertion::setNetName(const char* name) {
  int len = strlen(name) + 1;
  clear();
  if (len > netNameLength_) {
    free(netName_);
    netName_ = (char*)malloc(len);
    netNameLength_ = len;
  }
  strcpy(netName_, defData->DEFCASE(name));
}



int defiAssertion::isDelay() const {
  return isDelay_ ? 1 : 0;
}


int defiAssertion::isAssertion() const {
  return isAssertion_ ? 1 : 0;
}


int defiAssertion::isConstraint() const {
  return isAssertion_ ? 0 : 1;
}


int defiAssertion::isSum() const {
  return isSum_;
}


int defiAssertion::isDiff() const {
  return isDiff_;
}


int defiAssertion::isWiredlogic() const {
  return isWiredlogic_;
}


int defiAssertion::hasRiseMin() const {
  return hasRiseMin_;
}


int defiAssertion::hasRiseMax() const {
  return hasRiseMax_;
}


int defiAssertion::hasFallMin() const {
  return hasFallMin_;
}


int defiAssertion::hasFallMax() const {
  return hasFallMax_;
}


double defiAssertion::distance() const {
  return fallMax_;  // distance is stored here
}


double defiAssertion::riseMin() const {
  return riseMin_;
}


double defiAssertion::riseMax() const {
  return riseMax_;
}


double defiAssertion::fallMin() const {
  return fallMin_;
}


double defiAssertion::fallMax() const {
  return fallMax_;
}


void defiAssertion::setRiseMin(double d) {
  riseMin_ = d;
  hasRiseMin_ = 1;
}


void defiAssertion::setRiseMax(double d) {
  riseMax_ = d;
  hasRiseMax_ = 1;
}


void defiAssertion::setFallMin(double d) {
  fallMin_ = d;
  hasFallMin_ = 1;
}


void defiAssertion::setFallMax(double d) {
  fallMax_ = d;
  hasFallMax_ = 1;
}


int defiAssertion::numItems() const {
  return numItems_;
}


int defiAssertion::isPath(int index) const {
  if (index >= 0 && index < numItems_) {
    return (itemTypes_[index] == 'p') ? 1 : 0;
  }
  return 0;
}


int defiAssertion::isNet(int index) const {
  if (index >= 0 && index < numItems_) {
    return (itemTypes_[index] == 'n') ? 1 : 0;
  }
  return 0;
}


void defiAssertion::path(int index, char** fromInst, char** fromPin,
	   char** toInst, char** toPin) const {
  struct defiAssertPath* ap;

  if (index >= 0 && index < numItems_ &&
      itemTypes_[index] == 'p') {
    ap = (struct defiAssertPath*)(items_[index]);
    if (fromInst) *fromInst = ap->fromInst_;
    if (fromPin) *fromPin = ap->fromPin_;
    if (toInst) *toInst = ap->toInst_;
    if (toPin) *toPin = ap->toPin_;
  }
}


void defiAssertion::net(int index, char** netName) const {
  if (index >= 0 && index < numItems_ &&
      itemTypes_[index] == 'n') {
    if (netName) *netName = (char*)(items_[index]);
  }
}


void defiAssertion::bumpItems() {
  int i;
  char* newTypes;
  int** newItems;
  (numItemsAllocated_) *= 2;
  newTypes = (char*)malloc(numItemsAllocated_ * sizeof(char));
  newItems = (int**)malloc(numItemsAllocated_ * sizeof(int*));
  for (i = 0; i < numItems_; i++) {
    newItems[i] = items_[i];
    newTypes[i] = itemTypes_[i];
  }
  free((char*)items_);
  free((char*)itemTypes_);
  items_ = newItems;
  itemTypes_ = newTypes;
}


void defiAssertion::addNet(const char* name) {
  int i;
  char* s, *s1;

  // set wiredlogic to false
  isWiredlogic_ = 0;

  // make our own copy
  i = strlen(name) + 1;
  if (name[i-2] == ',') {
     s  = (char*)malloc(i-1);
     s1 = (char*)malloc(i-1);
     memcpy(s1, name, i-2);
     s1[i-2] = '\0';
     strcpy(s, defData->DEFCASE(s1));
     free(s1);
  } else {
     s = (char*)malloc(i);
     strcpy(s, defData->DEFCASE(name));
  }

  // make sure there is space in the array
  if (numItems_ >= numItemsAllocated_)
    bumpItems();

  // place it
  i = numItems_;
  items_[i] = (int*)s;
  itemTypes_[i] = 'n';
  numItems_ = i + 1;
  //strcpy(itemTypes_, "n");
}


void defiAssertion::addPath(const char* fromInst, const char* fromPin,
               const char* toInst, const char* toPin) {
  int i;
  struct defiAssertPath* s;

  // set wiredlogic to false
  isWiredlogic_ = 0;

  // make our own copy
  s = (struct defiAssertPath*)malloc(sizeof(struct defiAssertPath));
  i = strlen(fromInst) + 1;
  s->fromInst_ = (char*)malloc(i);
  strcpy(s->fromInst_, defData->DEFCASE(fromInst));
  i = strlen(toInst) + 1;
  s->toInst_ = (char*)malloc(i);
  strcpy(s->toInst_, defData->DEFCASE(toInst));
  i = strlen(fromPin) + 1;
  s->fromPin_ = (char*)malloc(i);
  strcpy(s->fromPin_, defData->DEFCASE(fromPin));
  i = strlen(toPin) + 1;
  s->toPin_ = (char*)malloc(i);
  strcpy(s->toPin_, defData->DEFCASE(toPin));

  // make sure there is space in the array
  if (numItems_ >= numItemsAllocated_)
    bumpItems();

  // place it
  i = numItems_;
  items_[i] = (int*)s;
  itemTypes_[i] = 'p';
  numItems_ = i + 1;
  //strcpy(itemTypes_, "p");
}


void defiAssertion::print(FILE* f) const {
  fprintf(f, "Assertion %s\n", netName());
}


END_LEFDEF_PARSER_NAMESPACE

