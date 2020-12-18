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

#include "defiIOTiming.h"
#include "defiIOTiming.hpp"

// Wrappers definitions.
int defiIOTiming_hasVariableRise (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasVariableRise();
}

int defiIOTiming_hasVariableFall (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasVariableFall();
}

int defiIOTiming_hasSlewRise (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasSlewRise();
}

int defiIOTiming_hasSlewFall (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasSlewFall();
}

int defiIOTiming_hasCapacitance (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasCapacitance();
}

int defiIOTiming_hasDriveCell (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasDriveCell();
}

int defiIOTiming_hasFrom (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasFrom();
}

int defiIOTiming_hasTo (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasTo();
}

int defiIOTiming_hasParallel (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->hasParallel();
}

const char* defiIOTiming_inst (const ::defiIOTiming* obj) {
    return ((const LefDefParser::defiIOTiming*)obj)->inst();
}

const char* defiIOTiming_pin (const ::defiIOTiming* obj) {
    return ((const LefDefParser::defiIOTiming*)obj)->pin();
}

double defiIOTiming_variableFallMin (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->variableFallMin();
}

double defiIOTiming_variableRiseMin (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->variableRiseMin();
}

double defiIOTiming_variableFallMax (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->variableFallMax();
}

double defiIOTiming_variableRiseMax (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->variableRiseMax();
}

double defiIOTiming_slewFallMin (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->slewFallMin();
}

double defiIOTiming_slewRiseMin (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->slewRiseMin();
}

double defiIOTiming_slewFallMax (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->slewFallMax();
}

double defiIOTiming_slewRiseMax (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->slewRiseMax();
}

double defiIOTiming_capacitance (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->capacitance();
}

const char* defiIOTiming_driveCell (const ::defiIOTiming* obj) {
    return ((const LefDefParser::defiIOTiming*)obj)->driveCell();
}

const char* defiIOTiming_from (const ::defiIOTiming* obj) {
    return ((const LefDefParser::defiIOTiming*)obj)->from();
}

const char* defiIOTiming_to (const ::defiIOTiming* obj) {
    return ((const LefDefParser::defiIOTiming*)obj)->to();
}

double defiIOTiming_parallel (const ::defiIOTiming* obj) {
    return ((LefDefParser::defiIOTiming*)obj)->parallel();
}

void defiIOTiming_print (const ::defiIOTiming* obj, FILE*  f) {
    ((LefDefParser::defiIOTiming*)obj)->print(f);
}

