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

#ifndef defiPartition_h
#define defiPartition_h

#include <stdio.h>
#include "defiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiPartition {
public:
  defiPartition(defrData *data);
  void Init();

  void Destroy();
  ~defiPartition();

  void clear();

  void setName(const char* name);
  void addTurnOff(const char* setup, const char* hold);
  void setFromClockPin(const char* inst, const char* pin);
  void setFromCompPin(const char* inst, const char* pin);
  void setFromIOPin(const char* inst);
  void setToClockPin(const char* inst, const char* pin);
  void setToCompPin(const char* inst, const char* pin);
  void set(char dir, char typ, const char* inst, const char* pin);
  void setToIOPin(const char* inst);
  void setMin(double min, double max);
  void setMax(double min, double max);
  void addPin(const char* name);
  void addRiseMin(double d);
  void addRiseMax(double d);
  void addFallMin(double d);
  void addFallMax(double d);
  void addRiseMinRange(double l, double h);
  void addRiseMaxRange(double l, double h);
  void addFallMinRange(double l, double h);
  void addFallMaxRange(double l, double h);

  const char* name() const;
  char direction() const;
  const char* itemType() const;  // "CLOCK" or "IO" or "COMP"
  const char* pinName() const;
  const char* instName() const;

  int numPins() const;
  const char* pin(int index) const;

  int isSetupRise() const;
  int isSetupFall() const;
  int isHoldRise() const;
  int isHoldFall() const;
  int hasMin() const;
  int hasMax() const;
  int hasRiseMin() const;
  int hasFallMin() const;
  int hasRiseMax() const;
  int hasFallMax() const;
  int hasRiseMinRange() const;
  int hasFallMinRange() const;
  int hasRiseMaxRange() const;
  int hasFallMaxRange() const;

  double partitionMin() const;
  double partitionMax() const;

  double riseMin() const;
  double fallMin() const;
  double riseMax() const;
  double fallMax() const;

  double riseMinLeft() const;
  double fallMinLeft() const;
  double riseMaxLeft() const;
  double fallMaxLeft() const;
  double riseMinRight() const;
  double fallMinRight() const;
  double riseMaxRight() const;
  double fallMaxRight() const;

  // debug print
  void print(FILE* f) const;

protected:
  char* name_;
  int nameLength_;
  char setup_;
  char hold_;
  char hasMin_;
  char hasMax_;
  char direction_;   // 'F' or 'T'
  char type_;        // 'L'-clock   'I'-IO  'C'-comp
  char* inst_;
  int instLength_;
  char* pin_;
  int pinLength_;
  double min_, max_;

  int numPins_;
  int pinsAllocated_;
  char** pins_;

  char hasRiseMin_;
  char hasFallMin_;
  char hasRiseMax_;
  char hasFallMax_;
  char hasRiseMinRange_;
  char hasFallMinRange_;
  char hasRiseMaxRange_;
  char hasFallMaxRange_;
  double riseMin_;
  double fallMin_;
  double riseMax_;
  double fallMax_;
  double riseMinLeft_;
  double fallMinLeft_;
  double riseMaxLeft_;
  double fallMaxLeft_;
  double riseMinRight_;
  double fallMinRight_;
  double riseMaxRight_;
  double fallMaxRight_;

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
