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

#include "defiAssertion.h"
#include "defiAssertion.hpp"

// Wrappers definitions.
int defiAssertion_isAssertion (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isAssertion();
}

int defiAssertion_isConstraint (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isConstraint();
}

int defiAssertion_isWiredlogic (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isWiredlogic();
}

int defiAssertion_isDelay (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isDelay();
}

int defiAssertion_isSum (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isSum();
}

int defiAssertion_isDiff (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->isDiff();
}

int defiAssertion_hasRiseMin (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->hasRiseMin();
}

int defiAssertion_hasRiseMax (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->hasRiseMax();
}

int defiAssertion_hasFallMin (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->hasFallMin();
}

int defiAssertion_hasFallMax (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->hasFallMax();
}

double defiAssertion_riseMin (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->riseMin();
}

double defiAssertion_riseMax (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->riseMax();
}

double defiAssertion_fallMin (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->fallMin();
}

double defiAssertion_fallMax (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->fallMax();
}

const char* defiAssertion_netName (const ::defiAssertion* obj) {
    return ((const LefDefParser::defiAssertion*)obj)->netName();
}

double defiAssertion_distance (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->distance();
}

int defiAssertion_numItems (const ::defiAssertion* obj) {
    return ((LefDefParser::defiAssertion*)obj)->numItems();
}

int defiAssertion_isPath (const ::defiAssertion* obj, int  index) {
    return ((LefDefParser::defiAssertion*)obj)->isPath(index);
}

int defiAssertion_isNet (const ::defiAssertion* obj, int  index) {
    return ((LefDefParser::defiAssertion*)obj)->isNet(index);
}

void defiAssertion_path (const ::defiAssertion* obj, int  index, char**  fromInst, char**  fromPin, char**  toInst, char**  toPin) {
    ((LefDefParser::defiAssertion*)obj)->path(index, fromInst, fromPin, toInst, toPin);
}

void defiAssertion_net (const ::defiAssertion* obj, int  index, char**  netName) {
    ((LefDefParser::defiAssertion*)obj)->net(index, netName);
}

void defiAssertion_print (const ::defiAssertion* obj, FILE*  f) {
    ((LefDefParser::defiAssertion*)obj)->print(f);
}

