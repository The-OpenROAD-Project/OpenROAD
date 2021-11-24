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

#include "lefiCrossTalk.h"
#include "lefiCrossTalk.hpp"

// Wrappers definitions.
double lefiNoiseVictim_length (const ::lefiNoiseVictim* obj) {
    return ((LefDefParser::lefiNoiseVictim*)obj)->length();
}

int lefiNoiseVictim_numNoises (const ::lefiNoiseVictim* obj) {
    return ((LefDefParser::lefiNoiseVictim*)obj)->numNoises();
}

double lefiNoiseVictim_noise (const ::lefiNoiseVictim* obj, int  index) {
    return ((LefDefParser::lefiNoiseVictim*)obj)->noise(index);
}

int lefiNoiseResistance_numNums (const ::lefiNoiseResistance* obj) {
    return ((LefDefParser::lefiNoiseResistance*)obj)->numNums();
}

double lefiNoiseResistance_num (const ::lefiNoiseResistance* obj, int  index) {
    return ((LefDefParser::lefiNoiseResistance*)obj)->num(index);
}

int lefiNoiseResistance_numVictims (const ::lefiNoiseResistance* obj) {
    return ((LefDefParser::lefiNoiseResistance*)obj)->numVictims();
}

const ::lefiNoiseVictim* lefiNoiseResistance_victim (const ::lefiNoiseResistance* obj, int  index) {
    return (const ::lefiNoiseVictim*) ((LefDefParser::lefiNoiseResistance*)obj)->victim(index);
}

