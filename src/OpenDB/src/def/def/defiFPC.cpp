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
#include "defiFPC.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//   defiFPC
//
//////////////////////////////////////////////
//////////////////////////////////////////////


defiFPC::defiFPC(defrData *data)
 : defData(data)
{
  Init();
}


void defiFPC::Init() {
  name_ = 0;
  nameLength_ = 0;
  namesAllocated_ = 0;
  namesUsed_ = 0;
  names_ = 0;
  rowOrComp_ = 0;
  clear();
}


defiFPC::~defiFPC() {
  Destroy();
}


void defiFPC::Destroy() {

  clear();

  if (name_) free(name_);
  name_ = 0;
  nameLength_ = 0;

  free((char*)(names_));
  free((char*)(rowOrComp_));
  namesAllocated_ = 0;
}


void defiFPC::clear() {
  int i;

  direction_ = 0;
  hasAlign_ = 0;
  hasMin_ = 0;
  hasMax_ = 0;
  hasEqual_ = 0;
  corner_ = 0;

  for (i = 0; i < namesUsed_; i++) {
    if (names_[i]) free (names_[i]) ;
  }
  namesUsed_ = 0;
}


void defiFPC::setName(const char* name, const char* direction) {
  int len = strlen(name) + 1;

  clear();

  if (len > nameLength_) {
    if (name_) free(name_);
    nameLength_ = len;
    name_ = (char*)malloc(len);
  }
  strcpy(name_, defData->DEFCASE(name));

  if (*direction == 'H')
    direction_ = 'H';
  else if (*direction == 'V')
    direction_ = 'V';
  else
    defiError(0, 6030, "ERROR (DEFPARS-6030): Invalid direction specified with FPC name. The valid direction is either 'H' or 'V'. Specify a valid value and then try again.", defData);

}


void defiFPC::print(FILE* f) const {
  fprintf(f, "FPC '%s'\n", name_);
}


const char* defiFPC::name() const {
  return name_;
}


int defiFPC::isVertical() const {
  return direction_ == 'V' ? 1 : 0 ;
}


int defiFPC::isHorizontal() const {
  return direction_ == 'H' ? 1 : 0 ;
}


int defiFPC::hasAlign() const {
  return (int)(hasAlign_);
}


int defiFPC::hasMax() const {
  return (int)(hasMax_);
}


int defiFPC::hasMin() const {
  return (int)(hasMin_);
}


int defiFPC::hasEqual() const {
  return (int)(hasEqual_);
}


double defiFPC::alignMin() const {
  return minMaxEqual_;
}


double defiFPC::alignMax() const {
  return minMaxEqual_;
}


double defiFPC::equal() const {
  return minMaxEqual_;
}


int defiFPC::numParts() const {
  return namesUsed_;
}


void defiFPC::setAlign() {
  hasAlign_ = 0;
}


void defiFPC::setMin(double num) {
  minMaxEqual_ = num;
}


void defiFPC::setMax(double num) {
  minMaxEqual_ = num;
}


void defiFPC::setEqual(double num) {
  minMaxEqual_ = num;
}


void defiFPC::setDoingBottomLeft() {
  corner_ = 'B';
}


void defiFPC::setDoingTopRight() {
  corner_ = 'T';
}


void defiFPC::getPart(int index, int* corner, int* typ, char** name) const {
  if (index >= 0 && index <= namesUsed_) {
    // 4 for bottom left  0 for topright
    // 2 for row   0 for comps
    if (corner) *corner = (int)((rowOrComp_[index] & 4) ? 'B' : 'T') ;
    if (typ) *typ = (int)((rowOrComp_[index] & 2) ? 'R' : 'C') ;
    if (name) *name = names_[index];
  }
}


void defiFPC::addRow(const char* name) {
  addItem('R', defData->DEFCASE(name));
}


void defiFPC::addComps(const char* name) {
  addItem('C', defData->DEFCASE(name));
}


void defiFPC::addItem(char item, const char* name) {
  int len = strlen(name) + 1;

  if (namesUsed_ >= namesAllocated_) {
    char* newR;
    char** newN;
    int i;
    namesAllocated_ =
	namesAllocated_ ? namesAllocated_ * 2 : 8 ;
    newN = (char**) malloc(sizeof(char*) * namesAllocated_);
    newR = (char*) malloc(sizeof(char) * namesAllocated_);
    for (i = 0; i < namesUsed_; i++) {
      newN[i] = names_[i];
      newR[i] = rowOrComp_[i];
    }
    if (names_) free((char*)(names_));
    if (rowOrComp_) free(rowOrComp_);
    names_ = newN;
    rowOrComp_ = newR;
  }

  names_[namesUsed_] = (char*)malloc(len);
  strcpy(names_[namesUsed_], name);

  // 4 for bottomleft
  // 2 for row
  rowOrComp_[namesUsed_] = 
         (char)(((corner_ == 'B') ? 4 : 0) |
	 (item == 'R' ? 2 : 0));

  namesUsed_ += 1;
}


END_LEFDEF_PARSER_NAMESPACE

