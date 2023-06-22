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

#include "defiRegion.h"
#include "defiRegion.hpp"

// Wrappers definitions.
const char* defiRegion_name (const ::defiRegion* obj) {
    return ((const LefDefParser::defiRegion*)obj)->name();
}

int defiRegion_numProps (const ::defiRegion* obj) {
    return ((LefDefParser::defiRegion*)obj)->numProps();
}

const char* defiRegion_propName (const ::defiRegion* obj, int  index) {
    return ((const LefDefParser::defiRegion*)obj)->propName(index);
}

const char* defiRegion_propValue (const ::defiRegion* obj, int  index) {
    return ((const LefDefParser::defiRegion*)obj)->propValue(index);
}

double defiRegion_propNumber (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->propNumber(index);
}

const char defiRegion_propType (const ::defiRegion* obj, int  index) {
    return ((const LefDefParser::defiRegion*)obj)->propType(index);
}

int defiRegion_propIsNumber (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->propIsNumber(index);
}

int defiRegion_propIsString (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->propIsString(index);
}

int defiRegion_hasType (const ::defiRegion* obj) {
    return ((LefDefParser::defiRegion*)obj)->hasType();
}

const char* defiRegion_type (const ::defiRegion* obj) {
    return ((const LefDefParser::defiRegion*)obj)->type();
}

int defiRegion_numRectangles (const ::defiRegion* obj) {
    return ((LefDefParser::defiRegion*)obj)->numRectangles();
}

int defiRegion_xl (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->xl(index);
}

int defiRegion_yl (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->yl(index);
}

int defiRegion_xh (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->xh(index);
}

int defiRegion_yh (const ::defiRegion* obj, int  index) {
    return ((LefDefParser::defiRegion*)obj)->yh(index);
}

void defiRegion_print (const ::defiRegion* obj, FILE*  f) {
    ((LefDefParser::defiRegion*)obj)->print(f);
}

