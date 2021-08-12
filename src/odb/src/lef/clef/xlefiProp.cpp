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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $  
// *****************************************************************************
// *****************************************************************************

#define EXTERN extern "C"

#include "lefiProp.h"
#include "lefiProp.hpp"

// Wrappers definitions.
const char* lefiProp_string (const ::lefiProp* obj) {
    return ((const LefDefParser::lefiProp*)obj)->string();
}

const char* lefiProp_propType (const ::lefiProp* obj) {
    return ((const LefDefParser::lefiProp*)obj)->propType();
}

const char* lefiProp_propName (const ::lefiProp* obj) {
    return ((const LefDefParser::lefiProp*)obj)->propName();
}

char lefiProp_dataType (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->dataType();
}

int lefiProp_hasNumber (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->hasNumber();
}

int lefiProp_hasRange (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->hasRange();
}

int lefiProp_hasString (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->hasString();
}

int lefiProp_hasNameMapString (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->hasNameMapString();
}

double lefiProp_number (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->number();
}

double lefiProp_left (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->left();
}

double lefiProp_right (const ::lefiProp* obj) {
    return ((LefDefParser::lefiProp*)obj)->right();
}

void lefiProp_print (const ::lefiProp* obj, FILE*  f) {
    ((LefDefParser::lefiProp*)obj)->print(f);
}

