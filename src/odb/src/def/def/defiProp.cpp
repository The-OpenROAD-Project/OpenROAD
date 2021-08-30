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

#include <stdlib.h>
#include <string.h>
#include "lex.h"
#include "defiProp.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

defiProp::defiProp(defrData *data)
: propType_(0), propName_(0), stringData_(0), defData(data)
{
  Init();
}


void defiProp::Init() {
  stringLength_ = 16;
  stringData_ = (char*)malloc(16);
  nameSize_ = 16;
  propName_ = (char*)malloc(16);
  clear();
}


DEF_COPY_CONSTRUCTOR_C( defiProp ) {
    DEF_MALLOC_FUNC( propType_, char, sizeof(char) * (strlen(prev.propType_) +1));
    DEF_MALLOC_FUNC( propName_, char, sizeof(char) * (strlen(prev.propName_) +1));
    DEF_COPY_FUNC( nameSize_ );
    DEF_COPY_FUNC( hasRange_ );
    DEF_COPY_FUNC( hasNumber_ );
    DEF_COPY_FUNC( hasNameMapString_ );
    DEF_COPY_FUNC( dataType_ );
    DEF_MALLOC_FUNC( stringData_, char, sizeof(char) * (strlen(prev.stringData_) +1));
    DEF_COPY_FUNC( stringLength_ );
    DEF_COPY_FUNC( left_ );
    DEF_COPY_FUNC( right_ );
    DEF_COPY_FUNC( d_ );
}

DEF_ASSIGN_OPERATOR_C( defiProp ) {
    CHECK_SELF_ASSIGN
    DEF_MALLOC_FUNC( propType_, char, sizeof(char) * (strlen(prev.propType_) +1));
    DEF_MALLOC_FUNC( propName_, char, sizeof(char) * (strlen(prev.propName_) +1));
    DEF_COPY_FUNC( nameSize_ );
    DEF_COPY_FUNC( hasRange_ );
    DEF_COPY_FUNC( hasNumber_ );
    DEF_COPY_FUNC( hasNameMapString_ );
    DEF_COPY_FUNC( dataType_ );
    DEF_MALLOC_FUNC( stringData_, char, sizeof(char) * (strlen(prev.stringData_) +1));
    DEF_COPY_FUNC( stringLength_ );
    DEF_COPY_FUNC( left_ );
    DEF_COPY_FUNC( right_ );
    DEF_COPY_FUNC( d_ );
    return *this;
}


void defiProp::Destroy() {
  free(stringData_);
  free(propName_);
}


defiProp::~defiProp() {
  Destroy();
}


void defiProp::setPropType(const char* typ, const char* string) {
  int len;
  propType_ = (char*)typ;
  if ((len = strlen(string)+1) > nameSize_)
    bumpName(len);
  strcpy(propName_, defData->DEFCASE(string));
}


void defiProp::setRange(double left, double right) {
  hasRange_ = 1;
  left_ = left;
  right_ = right;
}


void defiProp::setNumber(double d) {
  hasNumber_ = 1;
  d_ = d;
}


void defiProp::setPropInteger() {
  dataType_ = 'I';
}


void defiProp::setPropReal() {
  dataType_ = 'R';
}


void defiProp::setPropString() {
  dataType_ = 'S';
}


void defiProp::setPropNameMapString(const char* string) {
  int len;
  dataType_ = 'N';
  hasNameMapString_ = 1;
  if ((len = strlen(string)+1) > stringLength_)
    bumpSize(len);
  strcpy(stringData_, defData->DEFCASE(string));
}


void defiProp::setPropQString(const char* string) {
  int len;
  dataType_ = 'Q';
  if ((len = strlen(string)+1) > stringLength_)
    bumpSize(len);
  strcpy(stringData_, defData->DEFCASE(string));
}


const char* defiProp::string() const {
  return stringData_;
}


const char* defiProp::propType() const {
  return propType_;
}


int defiProp::hasNameMapString() const {
  return (int)(hasNameMapString_);
}


int defiProp::hasNumber() const {
  return (int)(hasNumber_);
}


int defiProp::hasRange() const {
  return (int)(hasRange_);
}


double defiProp::number() const {
  return d_;
}


double defiProp::left() const {
  return left_;
}


double defiProp::right() const {
  return right_;
}


void defiProp::bumpSize(int size) {
  free(stringData_);
  stringData_ = (char*)malloc(size);
  stringLength_ = size;
  *(stringData_) = '\0';
}


void defiProp::bumpName(int size) {
  free(propName_);
  propName_ = (char*)malloc(size);
  nameSize_ = size;
  *(propName_) = '\0';
}



void defiProp::clear() {
  if (stringData_)
     *(stringData_) = '\0';
  if (propName_)
     *(propName_) = '\0';
  propType_ = 0;
  hasRange_ = 0;
  hasNumber_ = 0;
  hasNameMapString_ = 0;
  dataType_ = 'B'; /* bogus */
  d_ = left_ = right_ = 0.0;
}


int defiProp::hasString() const {
  return *(stringData_) ? 1 : 0 ;
}


const char* defiProp::propName() const {
  return (propName_);
}


char defiProp::dataType() const {
  return (dataType_);
}


void defiProp::print(FILE* f) const {
  fprintf(f, "Prop type '%s'\n", propType());
  if (hasString()) {
    fprintf(f, "  string '%s'\n", string());
  }
  if (hasNumber()) {
    fprintf(f, "  number %5.2f\n", number());
  }
  if (hasRange()) {
    fprintf(f, "  range %5.2f - %5.2f\n",
      left(), right());
  }
}


END_LEFDEF_PARSER_NAMESPACE

