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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lex.h"
#include "defiScanchain.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

defiOrdered::defiOrdered(defrData *data)
: defData(data)
{}

defiOrdered::~defiOrdered()
{
    Destroy();
}

void defiOrdered::clear() {
  int i;
  for (i = 0; i < num_; i++) {
    free((char*)(inst_[i]));
    if (in_[i]) free((char*)(in_[i]));
    if (out_[i]) free((char*)(out_[i]));
  }
  num_ = 0;
}


void defiOrdered::Destroy() {
  int i;
  for (i = 0; i < num_; i++) {
     free((char*)(inst_[i]));
     free((char*)(in_[i]));
     free((char*)(out_[i]));
  }
  free((char*)(inst_));
  free((char*)(in_));
  free((char*)(out_));
  free((char*)(bits_));
}


void defiOrdered::Init() {
  num_ = 0;
  allocated_ = 32;
  inst_ = (char**)malloc(sizeof(char*)*32);
  in_   = (char**)malloc(sizeof(char*)*32);
  out_  = (char**)malloc(sizeof(char*)*32);
  bits_ = (int*)malloc(sizeof(int)*32);
}


void defiOrdered::bump() {
  int max = allocated_ * 2;
  int i;
  char** nin = (char**)malloc(sizeof(char*)*max);
  char** nout = (char**)malloc(sizeof(char*)*max);
  char** ninst = (char**)malloc(sizeof(char*)*max);
  int*   nbits = (int*)malloc(sizeof(int)*max);

  for (i = 0; i < num_; i++) {
    nin[i]  = in_[i];
    nout[i]  = out_[i];
    ninst[i] = inst_[i];
    nbits[i] = bits_[i];
  }
  free((char*)(inst_));
  free((char*)(in_));
  free((char*)(out_));
  free((char*)(bits_));
  allocated_ = max;
  inst_ = ninst;
  in_ = nin;
  out_ = nout;
  bits_ = nbits;
}


void defiOrdered::addOrdered(const char* inst) {
  if (num_ == allocated_) bump();
  inst_[num_] = (char*)malloc(strlen(inst)+1);
  strcpy(inst_[num_], defData->DEFCASE(inst));
  in_[num_]   = 0;
  out_[num_]  = 0;
  bits_[num_] = -1;
  num_ += 1;
}


void defiOrdered::addIn(const char* pin) {
  in_[num_-1] = (char*)malloc(strlen(pin)+1);
  strcpy(in_[num_-1], defData->DEFCASE(pin));
}


void defiOrdered::addOut(const char* pin) {
  out_[num_-1] = (char*)malloc(strlen(pin)+1);
  strcpy(out_[num_-1], defData->DEFCASE(pin));
}


void defiOrdered::setOrderedBits(int bits) {
  bits_[num_-1] = bits;
}


int defiOrdered::num() const {
  return num_;
}


char** defiOrdered::inst() const {
  return inst_;
}


char** defiOrdered::in() const {
  return in_;
}


char** defiOrdered::out() const {
  return out_;
}


int* defiOrdered::bits() const {
  return bits_;
}


defiScanchain::defiScanchain(defrData *data)
 : defData(data)
{
  Init();
}


void defiScanchain::Init() {
  name_ = (char*)malloc(32);
  nameLength_ = 32;

  numOrdered_ = 0;
  numOrderedAllocated_ = 4;
  ordered_ = (defiOrdered**)malloc(sizeof(defiOrdered*)*4);

  numFloating_ = 0;
  numFloatingAllocated_ = 4;
  floatInst_ = (char**)malloc(sizeof(char*)*4);
  floatIn_   = (char**)malloc(sizeof(char*)*4);
  floatOut_  = (char**)malloc(sizeof(char*)*4);
  floatBits_ = (int*)malloc(sizeof(int)*4);

  stopInst_     = 0;
  stopPin_      = 0;
  startInst_    = 0;
  startPin_     = 0;
  hasStart_     = 0;
  hasStop_      = 0;
  commonInPin_  = 0;
  commonOutPin_ = 0;
  hasPartition_ = 0;
  partName_     = 0;
  maxBits_      = -1;
}


void defiScanchain::clear() {
  int i;

  for (i = 0; i < numOrdered_; i++) {
    delete ordered_[i];
    ordered_[i] = 0;
  }

  numOrdered_ = 0;

  for (i = 0; i < numFloating_; i++) {
    if (floatIn_[i]) free(floatIn_[i]);
    if (floatOut_[i]) free(floatOut_[i]);
    free(floatInst_[i]);
    floatInst_[i] = 0;
    floatBits_[i] = -1;
  }
  numFloating_ = 0;

  if (stopInst_) free(stopInst_);
  if (stopPin_) free(stopPin_);
  if (startInst_) free(startInst_);
  if (startPin_) free(startPin_);
  if (commonInPin_) free(commonInPin_);
  if (commonOutPin_) free(commonOutPin_);

  stopInst_ = 0;
  stopPin_ = 0;
  startInst_ = 0;
  startPin_ = 0;
  hasStart_ = 0;
  hasStop_ = 0;
  commonInPin_ = 0;
  commonOutPin_ = 0;
  hasPartition_ = 0;
  if (partName_)
    free((char*)(partName_));
  partName_ = 0;
  maxBits_  = -1; 
}


void defiScanchain::Destroy() {
  clear();
  free(name_);
  free((char*)(ordered_));
  free((char*)(floatInst_));
  free((char*)(floatIn_));
  free((char*)(floatOut_));
  free((char*)(floatBits_));
}


defiScanchain::~defiScanchain() {
  Destroy();
}


void defiScanchain::setName(const char* name) {
  int len = strlen(name) + 1;

  clear();

  if (len > nameLength_) {
    free(name_);
    name_ = (char*)malloc(len);
    nameLength_ = len;
  }
  strcpy(name_, defData->DEFCASE(name));
}


const char* defiScanchain::name() const {
  return name_;
}


void defiScanchain::addFloatingInst(const char* name) {

  if (numFloating_ >= numFloatingAllocated_) {
    int max = 2 * numFloatingAllocated_;
    int i;
    char** ninst = (char**)malloc(sizeof(char*)*max);
    char** nin = (char**)malloc(sizeof(char*)*max);
    char** nout = (char**)malloc(sizeof(char*)*max);
    int*   nbits = (int*)malloc(sizeof(int)*max);
    for (i = 0; i < numFloating_; i++) {
      ninst[i] = floatInst_[i];
      nin[i] = floatIn_[i];
      nout[i] = floatOut_[i];
      nbits[i] = floatBits_[i];
    }
    free((char*)(floatInst_));
    free((char*)(floatIn_));
    free((char*)(floatOut_));
    free((char*)(floatBits_));
    floatInst_ = ninst;
    floatOut_ = nout;
    floatIn_ = nin;
    floatBits_ = nbits;
    numFloatingAllocated_ = max;
  }

  floatInst_[numFloating_] =
     (char*)malloc(strlen(name) + 1);
  strcpy(floatInst_[numFloating_], defData->DEFCASE(name));
  floatIn_[numFloating_] = 0;
  floatOut_[numFloating_] = 0;
  floatBits_[numFloating_] = -1;
  numFloating_ += 1;
}


void defiScanchain::addFloatingIn(const char* name) {
  int len = strlen(name) + 1;
  floatIn_[numFloating_-1] = (char*)malloc(len);
  strcpy(floatIn_[numFloating_-1], defData->DEFCASE(name));
}


void defiScanchain::addFloatingOut(const char* name) {
  int len = strlen(name) + 1;
  floatOut_[numFloating_-1] = (char*)malloc(len);
  strcpy(floatOut_[numFloating_-1], defData->DEFCASE(name));
}


void defiScanchain::setFloatingBits(int bits) {
  floatBits_[numFloating_-1] = bits;
}


void defiScanchain::addOrderedIn(const char* name) {
  defiOrdered* o = ordered_[numOrdered_-1];
  o->addIn(name);
}


void defiScanchain::addOrderedOut(const char* name) {
  defiOrdered* o = ordered_[numOrdered_-1];
  o->addOut(name);
}


void defiScanchain::addOrderedInst(const char* name) {
  defiOrdered* o = ordered_[numOrdered_-1];
  o->addOrdered(name);
}


void defiScanchain::setOrderedBits(int bits) {
  defiOrdered* o = ordered_[numOrdered_-1];
  o->setOrderedBits(bits);
}


void defiScanchain::addOrderedList() {
  defiOrdered* o;

  if (numOrdered_ == numOrderedAllocated_) {
    int max = 2 * numOrderedAllocated_;
    int i;
    defiOrdered** no = (defiOrdered**)malloc(sizeof(defiOrdered*)*max);
    for (i = 0; i < numOrdered_; i++) {
      no[i] = ordered_[i];
    }
    free((char*)(ordered_));
    ordered_ = no;
    numOrderedAllocated_ = max;
  }

  o = new defiOrdered(defData);
  ordered_[numOrdered_] = o;
  o->Init();
  numOrdered_ += 1;
}


void defiScanchain::setStart(const char* inst, const char* pin) {
   int len;
   if (startInst_)
      defiError(0, 6150, "ERROR (DEFPARS-6150): The START statement in the SCANCHAINS has defined more than one time in the SCANCHAINS statement.\nUpdate the DEF file to only one START statement and then try again.", defData);
   len = strlen(inst) + 1;
   startInst_ = (char*)malloc(len);
   strcpy(startInst_, defData->DEFCASE(inst));
   len = strlen(pin) + 1;
   startPin_ = (char*)malloc(len);
   strcpy(startPin_, defData->DEFCASE(pin));
   hasStart_ = 1;
}


void defiScanchain::setStop(const char* inst, const char* pin) {
   int len;
   if (stopInst_)
      defiError(0, 6151, "ERROR (DEFPARS-6151): The STOP statment in the SCANCHAINS has defined more than one time in the SCANCHAINS statement.\nUpdate the DEF file to only one STOP statement and then try again.", defData);
   len = strlen(inst) + 1;
   stopInst_ = (char*)malloc(len);
   strcpy(stopInst_, defData->DEFCASE(inst));
   len = strlen(pin) + 1;
   stopPin_ = (char*)malloc(len);
   strcpy(stopPin_, defData->DEFCASE(pin));
   hasStop_ = 1;
}


// 5.4.1
void defiScanchain::setPartition(const char* partName, int maxBits) {
  if (partName_)
     free(partName_);
  partName_ = (char*)malloc(strlen(partName) + 1);
  strcpy(partName_, defData->DEFCASE(partName));
  maxBits_ = maxBits;
  hasPartition_ = 1;
}


int defiScanchain::hasStart() const {
  return (int)(hasStart_);
}


int defiScanchain::hasStop() const {
  return (int)(hasStop_);
}


int defiScanchain::hasFloating() const {
  return numFloating_ ? 1 : 0 ;
}


int defiScanchain::hasOrdered() const {
  return numOrdered_ ? 1 : 0 ;
}


// 5.4.1
int defiScanchain::hasPartition() const {
  return hasPartition_;
}


// 5.4.1
int defiScanchain::hasPartitionMaxBits() const {
  return (maxBits_ != -1) ? 1 : 0;
}


// 5.4.1
const char* defiScanchain::partitionName() const {
  return partName_;
}


// 5.4.1
int defiScanchain::partitionMaxBits() const {
  return maxBits_;
}


void defiScanchain::start(char** inst, char** pin) const {
  if (inst) *inst = startInst_;
  if (pin) *pin = startPin_;
}


void defiScanchain::stop(char** inst, char** pin) const {
  if (inst) *inst = stopInst_;
  if (pin) *pin = stopPin_;
}


int defiScanchain::numOrderedLists() const {
  return numOrdered_;
}


void defiScanchain::ordered(int index, int* size, char*** inst,
                        char*** inPin, char*** outPin, int** bits) const {
  defiOrdered* o;
  if (index >= 0 && index <= numOrdered_) {
    o = ordered_[index];
    *size   = o->num();
    *inst   = o->inst();
    *inPin  = o->in();
    *outPin = o->out();
    *bits   = o->bits();
  } else {
    *size = 10;
    *inst = 0;
  }
}


void defiScanchain::floating(int* size, char*** inst,
                 char*** inPin, char*** outPin, int** bits) const {
  *size   = numFloating_;
  *inst   = floatInst_;
  *inPin  = floatIn_;
  *outPin = floatOut_;
  *bits   = floatBits_;
}


void defiScanchain::setCommonOut(const char* pin) {
  int len = strlen(pin) + 1;
  commonOutPin_ = (char*)malloc(len);
  strcpy(commonOutPin_, defData->DEFCASE(pin));
}


void defiScanchain::setCommonIn(const char* pin) {
  int len = strlen(pin) + 1;
  commonInPin_ = (char*)malloc(len);
  strcpy(commonInPin_, defData->DEFCASE(pin));
}


int defiScanchain::hasCommonInPin() const {
  return commonInPin_ ? 1 : 0;
}


int defiScanchain::hasCommonOutPin() const {
  return commonOutPin_ ? 1 : 0;
}


const char* defiScanchain::commonInPin() const {
  return commonInPin_;
}


const char* defiScanchain::commonOutPin() const {
  return commonOutPin_;
}


void defiScanchain::print(FILE* f) const {
  char* a;
  char* b;

  char** inst;
  char** in;
  char** out;
  int*   bits;
  int num;
  int i;
  int h;

  fprintf(f, "Scan chain '%s'  %d things\n",
      name(),
      hasStart() +
      hasStop() +
      numFloating_ +
      numOrderedLists());

  if (hasStart()) {
    start(&a, &b);
    fprintf(f, "  start inst '%s' pin '%s'\n", a, b);
  }

  if (hasStop()) {
    stop(&a, &b);
    fprintf(f, "  stop inst '%s' pin '%s'\n", a, b);
  }

  floating(&num, &inst, &in, &out, &bits);
  for (i = 0; i < num; i++) {
    fprintf(f, "  floating '%s' IN=%s  OUT=%s BITS=%d\n", inst[i],
       in[i]?in[i]:"NIL", out[i]?out[i]:"NIL", bits[i]?bits[i]:0);
  }

  for (i = 0; i < numOrderedLists(); i++) {
    ordered(i, &num, &inst, &in, &out, &bits);
    fprintf(f, "  %d ordered\n", i);
    for (h = 0; h < num; h++) {
      fprintf(f, "        '%s' IN=%s OUT=%s BITS=%d\n", inst[h],
       in[h]?in[h]:"NIL", out[h]?out[h]:"NIL", bits[h]?bits[h]:0);
    }
  }

  if (hasCommonInPin())
    fprintf(f, "  common in pin %s\n", commonInPin());

  if (hasCommonOutPin())
    fprintf(f, "  common out pin %s\n", commonOutPin());
}


END_LEFDEF_PARSER_NAMESPACE

