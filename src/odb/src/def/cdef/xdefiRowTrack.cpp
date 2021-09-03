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

#include "defiRowTrack.h"
#include "defiRowTrack.hpp"

// Wrappers definitions.
const char* defiRow_name (const ::defiRow* obj) {
    return ((const LefDefParser::defiRow*)obj)->name();
}

const char* defiRow_macro (const ::defiRow* obj) {
    return ((const LefDefParser::defiRow*)obj)->macro();
}

double defiRow_x (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->x();
}

double defiRow_y (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->y();
}

int defiRow_orient (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->orient();
}

const char* defiRow_orientStr (const ::defiRow* obj) {
    return ((const LefDefParser::defiRow*)obj)->orientStr();
}

int defiRow_hasDo (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->hasDo();
}

double defiRow_xNum (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->xNum();
}

double defiRow_yNum (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->yNum();
}

int defiRow_hasDoStep (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->hasDoStep();
}

double defiRow_xStep (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->xStep();
}

double defiRow_yStep (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->yStep();
}

int defiRow_numProps (const ::defiRow* obj) {
    return ((LefDefParser::defiRow*)obj)->numProps();
}

const char* defiRow_propName (const ::defiRow* obj, int  index) {
    return ((const LefDefParser::defiRow*)obj)->propName(index);
}

const char* defiRow_propValue (const ::defiRow* obj, int  index) {
    return ((const LefDefParser::defiRow*)obj)->propValue(index);
}

double defiRow_propNumber (const ::defiRow* obj, int  index) {
    return ((LefDefParser::defiRow*)obj)->propNumber(index);
}

char defiRow_propType (const ::defiRow* obj, int  index) {
    return ((const LefDefParser::defiRow*)obj)->propType(index);
}

int defiRow_propIsNumber (const ::defiRow* obj, int  index) {
    return ((LefDefParser::defiRow*)obj)->propIsNumber(index);
}

int defiRow_propIsString (const ::defiRow* obj, int  index) {
    return ((LefDefParser::defiRow*)obj)->propIsString(index);
}

void defiRow_print (const ::defiRow* obj, FILE*  f) {
    ((LefDefParser::defiRow*)obj)->print(f);
}

const char* defiTrack_macro (const ::defiTrack* obj) {
    return ((const LefDefParser::defiTrack*)obj)->macro();
}

double defiTrack_x (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->x();
}

double defiTrack_xNum (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->xNum();
}

double defiTrack_xStep (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->xStep();
}

int defiTrack_numLayers (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->numLayers();
}

const char* defiTrack_layer (const ::defiTrack* obj, int  index) {
    return ((const LefDefParser::defiTrack*)obj)->layer(index);
}

int defiTrack_firstTrackMask (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->firstTrackMask();
}

int defiTrack_sameMask (const ::defiTrack* obj) {
    return ((LefDefParser::defiTrack*)obj)->sameMask();
}

void defiTrack_print (const ::defiTrack* obj, FILE*  f) {
    ((LefDefParser::defiTrack*)obj)->print(f);
}

const char* defiGcellGrid_macro (const ::defiGcellGrid* obj) {
    return ((const LefDefParser::defiGcellGrid*)obj)->macro();
}

int defiGcellGrid_x (const ::defiGcellGrid* obj) {
    return ((LefDefParser::defiGcellGrid*)obj)->x();
}

int defiGcellGrid_xNum (const ::defiGcellGrid* obj) {
    return ((LefDefParser::defiGcellGrid*)obj)->xNum();
}

double defiGcellGrid_xStep (const ::defiGcellGrid* obj) {
    return ((LefDefParser::defiGcellGrid*)obj)->xStep();
}

void defiGcellGrid_print (const ::defiGcellGrid* obj, FILE*  f) {
    ((LefDefParser::defiGcellGrid*)obj)->print(f);
}

