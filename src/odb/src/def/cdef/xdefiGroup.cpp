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

#include "defiGroup.h"
#include "defiGroup.hpp"

// Wrappers definitions.
const char* defiGroup_name(const ::defiGroup* obj)
{
  return ((const DefParser::defiGroup*) obj)->name();
}

const char* defiGroup_regionName(const ::defiGroup* obj)
{
  return ((const DefParser::defiGroup*) obj)->regionName();
}

int defiGroup_hasRegionBox(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->hasRegionBox();
}

int defiGroup_hasRegionName(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->hasRegionName();
}

int defiGroup_hasMaxX(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->hasMaxX();
}

int defiGroup_hasMaxY(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->hasMaxY();
}

int defiGroup_hasPerim(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->hasPerim();
}

void defiGroup_regionRects(const ::defiGroup* obj,
                           int* size,
                           int** xl,
                           int** yl,
                           int** xh,
                           int** yh)
{
  ((DefParser::defiGroup*) obj)->regionRects(size, xl, yl, xh, yh);
}

int defiGroup_maxX(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->maxX();
}

int defiGroup_maxY(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->maxY();
}

int defiGroup_perim(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->perim();
}

int defiGroup_numProps(const ::defiGroup* obj)
{
  return ((DefParser::defiGroup*) obj)->numProps();
}

const char* defiGroup_propName(const ::defiGroup* obj, int index)
{
  return ((const DefParser::defiGroup*) obj)->propName(index);
}

const char* defiGroup_propValue(const ::defiGroup* obj, int index)
{
  return ((const DefParser::defiGroup*) obj)->propValue(index);
}

double defiGroup_propNumber(const ::defiGroup* obj, int index)
{
  return ((DefParser::defiGroup*) obj)->propNumber(index);
}

const char defiGroup_propType(const ::defiGroup* obj, int index)
{
  return ((const DefParser::defiGroup*) obj)->propType(index);
}

int defiGroup_propIsNumber(const ::defiGroup* obj, int index)
{
  return ((DefParser::defiGroup*) obj)->propIsNumber(index);
}

int defiGroup_propIsString(const ::defiGroup* obj, int index)
{
  return ((DefParser::defiGroup*) obj)->propIsString(index);
}

void defiGroup_print(const ::defiGroup* obj, FILE* f)
{
  ((DefParser::defiGroup*) obj)->print(f);
}
