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

#include "lefiUnits.h"
#include "lefiUnits.hpp"

// Wrappers definitions.
int lefiUnits_hasDatabase (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasDatabase();
}

int lefiUnits_hasCapacitance (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasCapacitance();
}

int lefiUnits_hasResistance (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasResistance();
}

int lefiUnits_hasTime (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasTime();
}

int lefiUnits_hasPower (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasPower();
}

int lefiUnits_hasCurrent (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasCurrent();
}

int lefiUnits_hasVoltage (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasVoltage();
}

int lefiUnits_hasFrequency (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->hasFrequency();
}

const char* lefiUnits_databaseName (const ::lefiUnits* obj) {
    return ((const LefDefParser::lefiUnits*)obj)->databaseName();
}

double lefiUnits_databaseNumber (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->databaseNumber();
}

double lefiUnits_capacitance (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->capacitance();
}

double lefiUnits_resistance (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->resistance();
}

double lefiUnits_time (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->time();
}

double lefiUnits_power (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->power();
}

double lefiUnits_current (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->current();
}

double lefiUnits_voltage (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->voltage();
}

double lefiUnits_frequency (const ::lefiUnits* obj) {
    return ((LefDefParser::lefiUnits*)obj)->frequency();
}

void lefiUnits_print (const ::lefiUnits* obj, FILE*  f) {
    ((LefDefParser::lefiUnits*)obj)->print(f);
}

