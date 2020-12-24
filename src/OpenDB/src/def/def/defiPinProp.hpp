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

#ifndef defiPinProp_h
#define defiPinProp_h

#include "defiKRDefs.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiPinProp {
public:
  defiPinProp(defrData *data);
  void Init();

  ~defiPinProp();
  void Destroy();

  void clear();

  void setName(const char* inst, const char* pin);
  void addProperty(const char* name, const char* value, const char type);
  void addNumProperty(const char* name, const double d,
                      const char* value, const char type);

  int isPin() const;
  const char* instName() const;
  const char* pinName() const;

  int numProps() const;
  const char*  propName(int index) const;
  const char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int    propIsNumber(int index) const;
  int    propIsString(int index) const;

  void print(FILE* f) const;

protected:
  char isPin_;
  int instNameSize_;
  char* instName_;
  int pinNameSize_;
  char* pinName_;

  int numProps_;
  int propsAllocated_;
  char**  propNames_;
  char**  propValues_;
  double* propDValues_;
  char*   propTypes_;

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
