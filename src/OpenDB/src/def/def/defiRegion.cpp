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
#include "defiRegion.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE


//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiRegion
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiRegion::defiRegion(defrData *data)
: defData(data)
{
  Init();
}


void defiRegion::Init() {
  name_ = 0;
  nameLength_ = 0;
  type_ = 0;
  xl_ = 0;
  yl_ = 0;
  xh_ = 0;
  yh_ = 0;
  numProps_ = 0;
  propsAllocated_ = 2;
  propNames_ = (char**)malloc(sizeof(char*)*2);
  propValues_ = (char**)malloc(sizeof(char*)*2);
  propDValues_ = (double*)malloc(sizeof(double)*2);
  propTypes_ = (char*)malloc(sizeof(char)*2);
  clear();
  numRectangles_ = 0;
  rectanglesAllocated_ = 1;
  xl_ = (int*)malloc(sizeof(int)*1);
  yl_ = (int*)malloc(sizeof(int)*1);
  xh_ = (int*)malloc(sizeof(int)*1);
  yh_ = (int*)malloc(sizeof(int)*1);
}


defiRegion::~defiRegion() {
  Destroy();
}


void defiRegion::clear() {
  int i;
  for (i = 0; i < numProps_; i++) {
    free(propNames_[i]);
    free(propValues_[i]);
    propDValues_[i] = 0;
  }
  numProps_ = 0;
  numRectangles_ = 0;
  if (type_) free(type_);
  type_ = 0;
}


void defiRegion::Destroy() {
  if (name_) free(name_);
  clear();
  name_ = 0;
  nameLength_ = 0;
  free((char*)(xl_));
  free((char*)(yl_));
  free((char*)(xh_));
  free((char*)(yh_));
  free((char*)(propNames_));
  free((char*)(propValues_));
  free((char*)(propDValues_));
  free((char*)(propTypes_));
}


void defiRegion::addRect(int xl, int yl, int xh, int yh) {
  if (numRectangles_ == rectanglesAllocated_) {
    int i;
    int max = rectanglesAllocated_ = rectanglesAllocated_ * 2;
    int* newxl = (int*)malloc(sizeof(int)*max);
    int* newyl = (int*)malloc(sizeof(int)*max);
    int* newxh = (int*)malloc(sizeof(int)*max);
    int* newyh = (int*)malloc(sizeof(int)*max);
    for (i = 0; i < numRectangles_; i++) {
      newxl[i] = xl_[i];
      newyl[i] = yl_[i];
      newxh[i] = xh_[i];
      newyh[i] = yh_[i];
    }
    free((char*)(xl_));
    free((char*)(yl_));
    free((char*)(xh_));
    free((char*)(yh_));
    xl_ = newxl;
    yl_ = newyl;
    xh_ = newxh;
    yh_ = newyh;
  }
  xl_[numRectangles_] = xl;
  yl_[numRectangles_] = yl;
  xh_[numRectangles_] = xh;
  yh_[numRectangles_] = yh;
  numRectangles_ += 1;
}


void defiRegion::setup(const char* name) {
  int len = strlen(name) + 1;

  clear();

  if (len > nameLength_) {
    if (name_) free(name_);
    nameLength_ = len;
    name_ = (char*)malloc(len);
  }

  strcpy(name_, defData->DEFCASE(name));

}

void defiRegion::addProperty(const char* name, const char* value,
                             const char type) {
  int len;
  if (numProps_ == propsAllocated_) {
    int i;
    char**  nn;
    char**  nv;
    double* nd;
    char*   nt;

    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = propNames_[i];
      nv[i] = propValues_[i];
      nd[i] = propDValues_[i];
      nt[i] = propTypes_[i];
    }
    free((char*)(propNames_));
    free((char*)(propValues_));
    free((char*)(propDValues_));
    free((char*)(propTypes_));
    propNames_ = nn;
    propValues_ = nv;
    propDValues_ = nd;
    propTypes_ = nt;
  }
  len = strlen(name) + 1;
  propNames_[numProps_] = (char*)malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  propValues_[numProps_] = (char*)malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));
  propDValues_[numProps_] = 0;
  propTypes_[numProps_] = type;
  numProps_ += 1;
}

void defiRegion::addNumProperty(const char* name, const double d,
                                const char* value, const char type) {
  int len;
  if (numProps_ == propsAllocated_) {
    int i;
    char**  nn;
    char**  nv;
    double* nd;
    char*   nt;

    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = propNames_[i];
      nv[i] = propValues_[i];
      nd[i] = propDValues_[i];
      nt[i] = propTypes_[i];
    }
    free((char*)(propNames_));
    free((char*)(propValues_));
    free((char*)(propDValues_));
    free((char*)(propTypes_));
    propNames_ = nn;
    propValues_ = nv;
    propDValues_ = nd;
    propTypes_ = nt;
  }
  len = strlen(name) + 1;
  propNames_[numProps_] = (char*)malloc(len);
  strcpy(propNames_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  propValues_[numProps_] = (char*)malloc(len);
  strcpy(propValues_[numProps_], defData->DEFCASE(value));
  propDValues_[numProps_] = d;
  propTypes_[numProps_] = type;
  numProps_ += 1;
}


void defiRegion::setType(const char* type) {
  int len;
  if (type_) free(type_);
  len = strlen(type) + 1;
  type_ = (char*)malloc(len);
  strcpy(type_, defData->DEFCASE(type));
}


int defiRegion::hasType() const {
  return type_ ? 1 : 0;
}


const char* defiRegion::type() const {
  return type_;
}


int defiRegion::numRectangles() const {
  return numRectangles_;
}


int defiRegion::numProps() const {
  return numProps_;
}


const char* defiRegion::propName(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propNames_[index];
}


const char* defiRegion::propValue(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propValues_[index];
}


double defiRegion::propNumber(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propDValues_[index];
}


char defiRegion::propType(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propTypes_[index];
}

int defiRegion::propIsNumber(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propDValues_[index] ? 1 : 0;
}

int defiRegion::propIsString(int index) const {
  char msg[256];
  if (index < 0 || index >= numProps_) {
     sprintf (msg, "ERROR (DEFPARS-6130): The index number %d specified for the REGION PROPERTY is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numProps_);
     defiError(0, 6130, msg, defData);
     return 0;
  }
  return propDValues_[index] ? 0 : 1;
}

const char* defiRegion::name() const {
  return name_;
}


int defiRegion::xl(int index) const {
  char msg[256];
  if (index < 0 || index >= numRectangles_) {
     sprintf (msg, "ERROR (DEFPARS-6131): The index number %d specified for the REGION RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numRectangles_);
     defiError(0, 6131, msg, defData);
     return 0;
  }
  return xl_[index];
}


int defiRegion::yl(int index) const {
  char msg[256];
  if (index < 0 || index >= numRectangles_) {
     sprintf (msg, "ERROR (DEFPARS-6131): The index number %d specified for the REGION RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numRectangles_);
     defiError(0, 6131, msg, defData);
     return 0;
  }
  return yl_[index];
}


int defiRegion::xh(int index) const {
  char msg[256];
  if (index < 0 || index >= numRectangles_) {
     sprintf (msg, "ERROR (DEFPARS-6131): The index number %d specified for the REGION RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numRectangles_);
     defiError(0, 6131, msg, defData);
     return 0;
  }
  return xh_[index];
}


int defiRegion::yh(int index) const {
  char msg[256];
  if (index < 0 || index >= numRectangles_) {
     sprintf (msg, "ERROR (DEFPARS-6131): The index number %d specified for the REGION RECTANGLE is invalide.\nValid index number is from 0 to %d. Specify a valid index number and then try again.",
              index, numRectangles_);
     defiError(0, 6131, msg, defData);
     return 0;
  }
  return yh_[index];
}


void defiRegion::print(FILE* f) const {
  int i;
  fprintf(f, "Region '%s'", name());
  for (i = 0; i < numRectangles(); i++) {
    fprintf(f, " %d %d %d %d",
      xl(i),
      yl(i),
      xh(i),
      yh(i));
  }
  fprintf(f, "\n");
}


END_LEFDEF_PARSER_NAMESPACE

