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
#include "defiPropType.hpp"
#include "defiDebug.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE


defiPropType::defiPropType() {
  Init();
}


void defiPropType::Init() {
  numProperties_ = 0;
  propertiesAllocated_ = 0;
  propNames_ = 0;
  propTypes_ = 0;
}


void defiPropType::Clear() {
  int i;

  for (i = 0; i < numProperties_; i++) {
    free(propNames_[i]);
  }
  numProperties_ = 0;
  propertiesAllocated_ = 0;
}


void defiPropType::Destroy() {
  Clear();
  if (propNames_)
     free(propNames_);
  if (propTypes_)
     free(propTypes_);
}


defiPropType::~defiPropType() {
  Destroy();
}


void defiPropType::setPropType(const char* name, const char type) {
  int len;

  if (numProperties_ == propertiesAllocated_)
    bumpProps();
  len = strlen(name) + 1;
  propNames_[numProperties_] = (char*)malloc(len);
  strcpy(propNames_[numProperties_], name);
  propTypes_[numProperties_] = type;
  numProperties_ += 1;
}


void defiPropType::bumpProps() {
  int lim = propertiesAllocated_;
  int news ;
  char** newpn;
  char*   newt;
 
  news = lim ? lim + lim : 2;
 
  newpn = (char**)malloc(sizeof(char*)*news);
  newt = (char*)malloc(sizeof(char)*news);
 
  lim = propertiesAllocated_ = news;
 
  if (lim > 2) {
     int i;
     for (i = 0; i < numProperties_; i++) {
       newpn[i] = propNames_[i];
       newt[i] = propTypes_[i];
     }
     free((char*)(propNames_));
     free((char*)(propTypes_));
  }
  propNames_ = newpn;
  propTypes_ = newt;
}


char defiPropType::propType(char* name) const {
  int i;

  // Name is NULL, error
  if (!name)
     return('N');

  for (i = 0; i < numProperties_; i++) {
     if (strcmp(name, propNames_[i]) == 0)
        return(propTypes_[i]);  // found the prop name
  }
  return('N'); // Can't found the name
}
END_LEFDEF_PARSER_NAMESPACE

