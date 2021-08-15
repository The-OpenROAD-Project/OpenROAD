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
#include "defiIOTiming.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiIOTiming
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiIOTiming::defiIOTiming(defrData *data)
 : defData(data)
{
  Init();
}


void defiIOTiming::Init() {
  inst_ = 0;
  instLength_ = 0;
  pin_ = 0;
  pinLength_ = 0;
  from_ = 0;
  fromLength_ = 0;
  to_ = 0;
  toLength_ = 0;
  driveCell_ = 0;
  driveCellLength_ = 0;
  hasVariableRise_ = 0;
  hasVariableFall_ = 0;
  hasSlewRise_ = 0;
  hasSlewFall_ = 0;
  hasCapacitance_ = 0;
  hasDriveCell_ = 0;
  hasFrom_ = 0;
  if (pin_) free(pin_);
  pin_ = 0;
  pinLength_ = 0;

  hasTo_ = 0;
  hasParallel_ = 0;
  variableFallMin_ = 0.0;
  variableRiseMin_ = 0.0;
  variableFallMax_ = 0.0;
  variableRiseMax_ = 0.0;
  slewFallMin_ = 0.0;
  slewRiseMin_ = 0.0;
  slewFallMax_ = 0.0;
  slewRiseMax_ = 0.0;
  capacitance_ = 0.0;
  parallel_ = 0.0;
}


defiIOTiming::~defiIOTiming() {
  Destroy();
}


void defiIOTiming::Destroy() {

  if (inst_) free(inst_);
  inst_ = 0;
  instLength_ = 0;

  if (pin_) free(pin_);
  pin_ = 0;
  pinLength_ = 0;

  if (from_) free(from_);
  from_ = 0;
  fromLength_ = 0;

  if (to_) free(to_);
  to_ = 0;
  toLength_ = 0;

  if (driveCell_) free(driveCell_);
  driveCell_ = 0;
  driveCellLength_ = 0;

  clear();
}


void defiIOTiming::clear() {
  hasVariableRise_ = 0;
  hasVariableFall_ = 0;
  hasSlewRise_ = 0;
  hasSlewFall_ = 0;
  hasCapacitance_ = 0;
  hasDriveCell_ = 0;
  hasFrom_ = 0;
  hasTo_ = 0;
  hasParallel_ = 0;
  variableFallMin_ = 0.0;
  variableRiseMin_ = 0.0;
  variableFallMax_ = 0.0;
  variableRiseMax_ = 0.0;
  slewFallMin_ = 0.0;
  slewRiseMin_ = 0.0;
  slewFallMax_ = 0.0;
  slewRiseMax_ = 0.0;
  capacitance_ = 0.0;
  parallel_ = 0.0;
}


void defiIOTiming::setName(const char* inst, const char* pin) {
  int len;

  clear();

  len = strlen(inst) + 1;
  if (len > instLength_) {
    if (inst_) free(inst_);
    instLength_ = len;
    inst_ = (char*)malloc(len);
  }
  strcpy(inst_, defData->DEFCASE(inst));

  len = strlen(pin) + 1;
  if (len > pinLength_) {
    if (pin_) free(pin_);
    pinLength_ = len;
    pin_ = (char*)malloc(len);
  }
  strcpy(pin_, defData->DEFCASE(pin));

}


void defiIOTiming::print(FILE* f) const {
  fprintf(f, "IOTiming '%s' '%s'\n", inst_, pin_);

  if (hasSlewRise())
    fprintf(f, "  Slew rise  %5.2f %5.2f\n",
       slewRiseMin(),
       slewRiseMax());

  if (hasSlewFall())
    fprintf(f, "  Slew fall  %5.2f %5.2f\n",
       slewFallMin(),
       slewFallMax());

  if (hasVariableRise())
    fprintf(f, "  variable rise  %5.2f %5.2f\n",
       variableRiseMin(),
       variableRiseMax());

  if (hasVariableFall())
    fprintf(f, "  variable fall  %5.2f %5.2f\n",
       variableFallMin(),
       variableFallMax());

  if (hasCapacitance())
    fprintf(f, "  capacitance %5.2f\n",
       capacitance());

  if (hasDriveCell())
    fprintf(f, "  drive cell '%s'\n",
       driveCell());

  if (hasFrom())
    fprintf(f, "  from pin '%s'\n",
       from());

  if (hasTo())
    fprintf(f, "  to pin '%s'\n",
       to());

  if (hasParallel())
    fprintf(f, "  parallel %5.2f\n",
       parallel());
}



void defiIOTiming::setVariable(const char* riseFall, double min, double max) {
  if (*riseFall == 'R') {
    hasVariableRise_ = 1;
    variableRiseMin_ = min;
    variableRiseMax_ = max;

  } else if (*riseFall == 'F') {
    hasVariableFall_ = 1;
    variableFallMin_ = min;
    variableFallMax_ = max;

  } else {
    defiError(0, 6060, "ERROR (DEFPARS-6060): Invalid value specified for IOTIMING rise/fall. The valid value for rise is 'R' and for fall is 'F'. Specify a valid value and then try again.", defData);
  }
}


void defiIOTiming::setSlewRate(const char* riseFall, double min, double max) {
  if (*riseFall == 'R') {
    hasSlewRise_ = 1;
    slewRiseMin_ = min;
    slewRiseMax_ = max;

  } else if (*riseFall == 'F') {
    hasSlewFall_ = 1;
    slewFallMin_ = min;
    slewFallMax_ = max;

  } else {
    defiError(0, 6060, "ERROR (DEFPARS-6060): Invalid value specified for IOTIMING rise/fall. The valid value for rise is 'R' and for fall is 'F'. Specify a valid value and then try again.", defData);
  }
}


void defiIOTiming::setCapacitance(double num) {
  hasCapacitance_ = 1;
  capacitance_ = num;
}


void defiIOTiming::setDriveCell(const char* name) {
  int len = strlen(name) + 1;

  if (driveCellLength_ < len) {
    if (driveCell_) free(driveCell_);
    driveCell_ = (char*) malloc(len);
    driveCellLength_ = len;
  }

  strcpy(driveCell_, defData->DEFCASE(name));
  hasDriveCell_ = 1;
}


void defiIOTiming::setFrom(const char* name) {
  int len = strlen(name) + 1;

  if (fromLength_ < len) {
    if (from_) free(from_);
    from_ = (char*) malloc(len);
    fromLength_ = len;
  }

  strcpy(from_, defData->DEFCASE(name));
  hasFrom_ = 1;
}


void defiIOTiming::setTo(const char* name) {
  int len = strlen(name) + 1;

  if (toLength_ < len) {
    if (to_) free(to_);
    to_ = (char*) malloc(len);
    toLength_ = len;
  }

  strcpy(to_, defData->DEFCASE(name));
  hasTo_ = 1;
}


void defiIOTiming::setParallel(double num) {
  hasParallel_ = 1;
  parallel_ = num;
}


int defiIOTiming::hasVariableRise() const {
  return hasVariableRise_;
}


int defiIOTiming::hasVariableFall() const {
  return hasVariableFall_;
}


int defiIOTiming::hasSlewRise() const {
  return hasSlewRise_;
}


int defiIOTiming::hasSlewFall() const {
  return hasSlewFall_;
}


int defiIOTiming::hasCapacitance() const {
  return hasCapacitance_;
}


int defiIOTiming::hasDriveCell() const {
  return hasDriveCell_;
}


int defiIOTiming::hasFrom() const {
  return hasFrom_;
}


int defiIOTiming::hasTo() const {
  return hasTo_;
}


int defiIOTiming::hasParallel() const {
  return hasParallel_;
}


const char* defiIOTiming::inst() const {
  return inst_;
}


const char* defiIOTiming::pin() const {
  return pin_;
}


double defiIOTiming::variableFallMin() const {
  return variableFallMin_;
}


double defiIOTiming::variableRiseMin() const {
  return variableRiseMin_;
}


double defiIOTiming::variableFallMax() const {
  return variableFallMax_;
}


double defiIOTiming::variableRiseMax() const {
  return variableRiseMax_;
}


double defiIOTiming::slewFallMin() const {
  return slewFallMin_;
}


double defiIOTiming::slewRiseMin() const {
  return slewRiseMin_;
}


double defiIOTiming::slewFallMax() const {
  return slewFallMax_;
}


double defiIOTiming::slewRiseMax() const {
  return slewRiseMax_;
}


double defiIOTiming::capacitance() const {
  return capacitance_;
}


const char* defiIOTiming::driveCell() const {
  return driveCell_;
}


const char* defiIOTiming::from() const {
  return from_;
}


const char* defiIOTiming::to() const {
  return to_;
}


double defiIOTiming::parallel() const {
  return parallel_;
}


END_LEFDEF_PARSER_NAMESPACE

