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

#ifndef defiProp_h
#define defiProp_h

#include "defiKRDefs.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

// Struct holds the data for one property.
class defiProp {
public:
  defiProp(defrData *data = NULL);
  void Init();
  
  DEF_COPY_CONSTRUCTOR_H( defiProp );
  DEF_ASSIGN_OPERATOR_H( defiProp );

  void Destroy();
  ~defiProp();

  void setPropType(const char* typ, const char* string);
  void setRange(double left, double right);
  void setNumber(double num);
  void setPropInteger();
  void setPropReal();
  void setPropString();
  void setPropQString(const char* string);
  void setPropNameMapString(const char* string);
  void clear();

  const char* string() const;
  const char* propType() const;
  const char* propName() const;
  char  dataType() const;
       // either I:integer R:real S:string Q:quotedstring N:nameMapString
  int hasNumber() const;
  int hasRange() const;
  int hasString() const;
  int hasNameMapString() const;
  double number() const;
  double left() const;
  double right() const;

  void bumpSize(int size);
  void bumpName(int size);

  void print(FILE* f) const;

protected:
  char* propType_;      // "design" ...
  char* propName_;      // name.
  int nameSize_;        // allocated size of name.
  char hasRange_;       // either 0:NO or 1:YES.
  char hasNumber_;      // either 0:NO or 1:YES.
  char hasNameMapString_;
  char dataType_;       // either I:integer R:real S:string Q:quotedstring.
                        //   N:nameMapString
  char* stringData_;    // if it is a string the data is here.
  int stringLength_;    // allocated size of stringData.
  double left_, right_; // if it has a range the numbers are here.
  double d_;            // if it is a real or int the number is here.

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
