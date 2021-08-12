// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
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

#ifndef defiScanchain_h
#define defiScanchain_h

#include "defiKRDefs.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiOrdered {
public:
  defiOrdered(defrData *data);
  ~defiOrdered();

  void addOrdered(const char* inst);
  void addIn(const char* pin);
  void addOut(const char* pin);
  void setOrderedBits(int bits);        // 5.4.1
  void bump();
  void Init();
  void Destroy();
  void clear();

  int num() const;
  char** inst() const;
  char** in() const;
  char** out() const;
  int*   bits() const;                  // 5.4.1

protected:
  int num_;
  int allocated_;
  char** inst_;
  char** in_;
  char** out_;
  int*   bits_;                       // 5.4.1
    
  defrData *defData;
};


// Struct holds the data for one Scan chain.
//
class defiScanchain {
public:
  defiScanchain(defrData *data);
  void Init();

  void Destroy();
  ~defiScanchain();

  void setName(const char* name);
  void clear();

  void addOrderedList();
  void addOrderedInst(const char* inst);
  void addOrderedIn(const char* inPin);
  void addOrderedOut(const char* outPin);
  void setOrderedBits(int bits);      // 5.4.1

  void addFloatingInst(const char* inst);
  void addFloatingIn(const char* inPin);
  void addFloatingOut(const char* outPin);
  void setFloatingBits(int bits);     // 5.4.1

  void setStart(const char* inst, const char* pin);
  void setStop(const char* inst, const char* pin);
  void setCommonIn(const char* pin);
  void setCommonOut(const char* pin);
  void setPartition(const char* partName, int maxBits);    // 5.4.1

  const char* name() const;
  int hasStart() const;
  int hasStop() const;
  int hasFloating() const;
  int hasOrdered() const;
  int hasCommonInPin() const;
  int hasCommonOutPin() const;
  int hasPartition() const;           // 5.4.1
  int hasPartitionMaxBits() const;    // 5.4.1

  // If the pin part of these routines were not supplied in the DEF
  // then a NULL pointer will be returned.
  void start(char** inst, char** pin) const;
  void stop(char** inst, char** pin) const;

  // There could be many ORDERED constructs in the DEF.  The data in
  // each ORDERED construct is stored in its own array.  The numOrderedLists()
  // routine tells how many lists there are.
  int numOrderedLists() const;

  // This routine will return an array of instances and
  // an array of in and out pins.
  // The number if things in the arrays is returned in size.
  // The inPin and outPin entry is optional for each instance.
  // If an entry is not given, then that char* is NULL.
  // For example if the second instance has
  // instnam= "FOO" and IN="A", but no OUT given, then inst[1] points
  // to "FOO"  inPin[1] points to "A" and outPin[1] is a NULL pointer.
  void ordered(int index, int* size, char*** inst, char*** inPin,
                                      char*** outPin, int** bits) const;

  // All of the floating constructs in the scan chain are
  // stored in this one array.
  // If the IN or OUT of an entry is not supplied then the array will have
  // a NULL pointer in that place.
  void floating(int* size, char*** inst, char*** inPin, char*** outPin,
                                      int** bits) const;

  const char* commonInPin() const;
  const char* commonOutPin() const;

  const char* partitionName() const;        // 5.4.1
  int partitionMaxBits() const;             // 5.4.1

  void print(FILE* f) const;

protected:
  char* name_;
  char hasStart_;
  char hasStop_;
  int nameLength_;

  int numOrderedAllocated_;
  int numOrdered_;
  defiOrdered** ordered_; 

  int numFloatingAllocated_;
  int numFloating_;
  char** floatInst_;    // Array of floating names
  char** floatIn_;
  char** floatOut_;
  int*   floatBits_;    // 5.4.1

  char* stopInst_;
  char* stopPin_;

  char* startInst_;
  char* startPin_;

  char* commonInPin_;
  char* commonOutPin_;

  char  hasPartition_;  // 5.4.1
  char* partName_;      // 5.4.1
  int   maxBits_;       // 5.4.1

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
