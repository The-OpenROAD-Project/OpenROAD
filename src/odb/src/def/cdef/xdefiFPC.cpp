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

#include "defiFPC.h"
#include "defiFPC.hpp"

// Wrappers definitions.
const char* defiFPC_name (const ::defiFPC* obj) {
    return ((const LefDefParser::defiFPC*)obj)->name();
}

int defiFPC_isVertical (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->isVertical();
}

int defiFPC_isHorizontal (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->isHorizontal();
}

int defiFPC_hasAlign (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->hasAlign();
}

int defiFPC_hasMax (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->hasMax();
}

int defiFPC_hasMin (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->hasMin();
}

int defiFPC_hasEqual (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->hasEqual();
}

double defiFPC_alignMax (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->alignMax();
}

double defiFPC_alignMin (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->alignMin();
}

double defiFPC_equal (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->equal();
}

int defiFPC_numParts (const ::defiFPC* obj) {
    return ((LefDefParser::defiFPC*)obj)->numParts();
}

void defiFPC_getPart (const ::defiFPC* obj, int  index, int*  corner, int*  typ, char**  name) {
    ((LefDefParser::defiFPC*)obj)->getPart(index, corner, typ, name);
}

void defiFPC_print (const ::defiFPC* obj, FILE*  f) {
    ((LefDefParser::defiFPC*)obj)->print(f);
}

