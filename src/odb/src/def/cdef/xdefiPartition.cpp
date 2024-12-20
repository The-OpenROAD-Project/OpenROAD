// *****************************************************************************
// *****************************************************************************
// ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!
// *****************************************************************************
// *****************************************************************************
// Copyright 2012, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the \"License\");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an \"AS IS\" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author: xxx $
//  $Revision: xxx $
//  $Date: xxx $
//  $State: xxx $
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "defiPartition.h"
#include "defiPartition.hpp"

// Wrappers definitions.
const char* defiPartition_name(const ::defiPartition* obj)
{
  return ((const DefParser::defiPartition*) obj)->name();
}

char defiPartition_direction(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->direction();
}

const char* defiPartition_itemType(const ::defiPartition* obj)
{
  return ((const DefParser::defiPartition*) obj)->itemType();
}

const char* defiPartition_pinName(const ::defiPartition* obj)
{
  return ((const DefParser::defiPartition*) obj)->pinName();
}

const char* defiPartition_instName(const ::defiPartition* obj)
{
  return ((const DefParser::defiPartition*) obj)->instName();
}

int defiPartition_numPins(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->numPins();
}

const char* defiPartition_pin(const ::defiPartition* obj, int index)
{
  return ((const DefParser::defiPartition*) obj)->pin(index);
}

int defiPartition_isSetupRise(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->isSetupRise();
}

int defiPartition_isSetupFall(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->isSetupFall();
}

int defiPartition_isHoldRise(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->isHoldRise();
}

int defiPartition_isHoldFall(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->isHoldFall();
}

int defiPartition_hasMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasMin();
}

int defiPartition_hasMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasMax();
}

int defiPartition_hasRiseMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasRiseMin();
}

int defiPartition_hasFallMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasFallMin();
}

int defiPartition_hasRiseMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasRiseMax();
}

int defiPartition_hasFallMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasFallMax();
}

int defiPartition_hasRiseMinRange(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasRiseMinRange();
}

int defiPartition_hasFallMinRange(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasFallMinRange();
}

int defiPartition_hasRiseMaxRange(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasRiseMaxRange();
}

int defiPartition_hasFallMaxRange(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->hasFallMaxRange();
}

double defiPartition_partitionMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->partitionMin();
}

double defiPartition_partitionMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->partitionMax();
}

double defiPartition_riseMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMin();
}

double defiPartition_fallMin(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMin();
}

double defiPartition_riseMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMax();
}

double defiPartition_fallMax(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMax();
}

double defiPartition_riseMinLeft(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMinLeft();
}

double defiPartition_fallMinLeft(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMinLeft();
}

double defiPartition_riseMaxLeft(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMaxLeft();
}

double defiPartition_fallMaxLeft(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMaxLeft();
}

double defiPartition_riseMinRight(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMinRight();
}

double defiPartition_fallMinRight(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMinRight();
}

double defiPartition_riseMaxRight(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->riseMaxRight();
}

double defiPartition_fallMaxRight(const ::defiPartition* obj)
{
  return ((DefParser::defiPartition*) obj)->fallMaxRight();
}

void defiPartition_print(const ::defiPartition* obj, FILE* f)
{
  ((DefParser::defiPartition*) obj)->print(f);
}
