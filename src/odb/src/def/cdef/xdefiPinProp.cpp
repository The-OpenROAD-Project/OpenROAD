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

#include "defiPinProp.h"
#include "defiPinProp.hpp"

// Wrappers definitions.
int defiPinProp_isPin (const ::defiPinProp* obj) {
    return ((LefDefParser::defiPinProp*)obj)->isPin();
}

const char* defiPinProp_instName (const ::defiPinProp* obj) {
    return ((const LefDefParser::defiPinProp*)obj)->instName();
}

const char* defiPinProp_pinName (const ::defiPinProp* obj) {
    return ((const LefDefParser::defiPinProp*)obj)->pinName();
}

int defiPinProp_numProps (const ::defiPinProp* obj) {
    return ((LefDefParser::defiPinProp*)obj)->numProps();
}

const char* defiPinProp_propName (const ::defiPinProp* obj, int  index) {
    return ((const LefDefParser::defiPinProp*)obj)->propName(index);
}

const char* defiPinProp_propValue (const ::defiPinProp* obj, int  index) {
    return ((const LefDefParser::defiPinProp*)obj)->propValue(index);
}

double defiPinProp_propNumber (const ::defiPinProp* obj, int  index) {
    return ((LefDefParser::defiPinProp*)obj)->propNumber(index);
}

const char defiPinProp_propType (const ::defiPinProp* obj, int  index) {
    return ((const LefDefParser::defiPinProp*)obj)->propType(index);
}

int defiPinProp_propIsNumber (const ::defiPinProp* obj, int  index) {
    return ((LefDefParser::defiPinProp*)obj)->propIsNumber(index);
}

int defiPinProp_propIsString (const ::defiPinProp* obj, int  index) {
    return ((LefDefParser::defiPinProp*)obj)->propIsString(index);
}

void defiPinProp_print (const ::defiPinProp* obj, FILE*  f) {
    ((LefDefParser::defiPinProp*)obj)->print(f);
}

