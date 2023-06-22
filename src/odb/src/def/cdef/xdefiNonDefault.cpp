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

#include "defiNonDefault.h"
#include "defiNonDefault.hpp"

// Wrappers definitions.
const char* defiNonDefault_name (const ::defiNonDefault* obj) {
    return ((const LefDefParser::defiNonDefault*)obj)->name();
}

int defiNonDefault_hasHardspacing (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->hasHardspacing();
}

int defiNonDefault_numProps (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->numProps();
}

const char* defiNonDefault_propName (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->propName(index);
}

const char* defiNonDefault_propValue (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->propValue(index);
}

double defiNonDefault_propNumber (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->propNumber(index);
}

const char defiNonDefault_propType (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->propType(index);
}

int defiNonDefault_propIsNumber (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->propIsNumber(index);
}

int defiNonDefault_propIsString (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->propIsString(index);
}

int defiNonDefault_numLayers (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->numLayers();
}

const char* defiNonDefault_layerName (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->layerName(index);
}

double defiNonDefault_layerWidth (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerWidth(index);
}

int defiNonDefault_layerWidthVal (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerWidthVal(index);
}

int defiNonDefault_hasLayerDiagWidth (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->hasLayerDiagWidth(index);
}

double defiNonDefault_layerDiagWidth (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerDiagWidth(index);
}

int defiNonDefault_layerDiagWidthVal (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerDiagWidthVal(index);
}

int defiNonDefault_hasLayerSpacing (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->hasLayerSpacing(index);
}

double defiNonDefault_layerSpacing (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerSpacing(index);
}

int defiNonDefault_layerSpacingVal (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerSpacingVal(index);
}

int defiNonDefault_hasLayerWireExt (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->hasLayerWireExt(index);
}

double defiNonDefault_layerWireExt (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerWireExt(index);
}

int defiNonDefault_layerWireExtVal (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->layerWireExtVal(index);
}

int defiNonDefault_numVias (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->numVias();
}

const char* defiNonDefault_viaName (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->viaName(index);
}

int defiNonDefault_numViaRules (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->numViaRules();
}

const char* defiNonDefault_viaRuleName (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->viaRuleName(index);
}

int defiNonDefault_numMinCuts (const ::defiNonDefault* obj) {
    return ((LefDefParser::defiNonDefault*)obj)->numMinCuts();
}

const char* defiNonDefault_cutLayerName (const ::defiNonDefault* obj, int  index) {
    return ((const LefDefParser::defiNonDefault*)obj)->cutLayerName(index);
}

int defiNonDefault_numCuts (const ::defiNonDefault* obj, int  index) {
    return ((LefDefParser::defiNonDefault*)obj)->numCuts(index);
}

void defiNonDefault_print (const ::defiNonDefault* obj, FILE*  f) {
    ((LefDefParser::defiNonDefault*)obj)->print(f);
}

