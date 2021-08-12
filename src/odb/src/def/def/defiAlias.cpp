// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2014, Cadence Design Systems
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

#include "defiAlias.hpp"
#include "defrData.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

extern defrContext defContext;

class defAliasIterator {
public:
    std::map<std::string, std::string, defCompareStrings>::iterator me; 
}; 

defiAlias_itr::defiAlias_itr(defrData *data) 
: iterator(NULL),
  first(1),
  defData(data ? data : defContext.data)
{
    defiAlias_itr::Init();
}


void defiAlias_itr::Init() {
    first = 1;
    iterator = new defAliasIterator();
}
 

void defiAlias_itr::Destroy() {
    delete iterator;
    iterator = NULL;
}


defiAlias_itr::~defiAlias_itr() {
    defiAlias_itr::Destroy();
}

 
int defiAlias_itr::Next() {
    if (first) {
        first = 0;
        iterator->me = defData->def_alias_set.begin();
    } else {
        iterator->me++;
    }

    if (iterator->me == defData->def_alias_set.end()) {
        return 0;
    }

    return 1;
}


const char* defiAlias_itr::Key() {
    if (iterator->me == defData->def_alias_set.end()) {
        return NULL;
    }
    
    return iterator->me->first.c_str();
}


const char* defiAlias_itr::Data() {
    if (iterator->me == defData->def_alias_set.end()) {
        return NULL;
    }

    // First char is reserved for 'marked' symbol ('0' or '1')
    return iterator->me->second.c_str() + 1;
}

 
int defiAlias_itr::Marked() {
    const char *value = iterator->me->second.c_str();

    if ((value == NULL) || (value[0] == '0')) {
        return 0;
    }else {
        return 1;
    }
}

END_LEFDEF_PARSER_NAMESPACE

