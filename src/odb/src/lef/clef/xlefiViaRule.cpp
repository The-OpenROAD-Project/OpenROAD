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

#include "lefiViaRule.h"
#include "lefiViaRule.hpp"

// Wrappers definitions.
int lefiViaRuleLayer_hasDirection (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasDirection();
}

int lefiViaRuleLayer_hasEnclosure (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasEnclosure();
}

int lefiViaRuleLayer_hasWidth (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasWidth();
}

int lefiViaRuleLayer_hasResistance (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasResistance();
}

int lefiViaRuleLayer_hasOverhang (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasOverhang();
}

int lefiViaRuleLayer_hasMetalOverhang (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasMetalOverhang();
}

int lefiViaRuleLayer_hasSpacing (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasSpacing();
}

int lefiViaRuleLayer_hasRect (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->hasRect();
}

char* lefiViaRuleLayer_name (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->name();
}

int lefiViaRuleLayer_isHorizontal (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->isHorizontal();
}

int lefiViaRuleLayer_isVertical (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->isVertical();
}

double lefiViaRuleLayer_enclosureOverhang1 (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->enclosureOverhang1();
}

double lefiViaRuleLayer_enclosureOverhang2 (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->enclosureOverhang2();
}

double lefiViaRuleLayer_widthMin (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->widthMin();
}

double lefiViaRuleLayer_widthMax (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->widthMax();
}

double lefiViaRuleLayer_overhang (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->overhang();
}

double lefiViaRuleLayer_metalOverhang (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->metalOverhang();
}

double lefiViaRuleLayer_resistance (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->resistance();
}

double lefiViaRuleLayer_spacingStepX (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->spacingStepX();
}

double lefiViaRuleLayer_spacingStepY (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->spacingStepY();
}

double lefiViaRuleLayer_xl (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->xl();
}

double lefiViaRuleLayer_yl (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->yl();
}

double lefiViaRuleLayer_xh (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->xh();
}

double lefiViaRuleLayer_yh (const ::lefiViaRuleLayer* obj) {
    return ((LefDefParser::lefiViaRuleLayer*)obj)->yh();
}

void lefiViaRuleLayer_print (const ::lefiViaRuleLayer* obj, FILE*  f) {
    ((LefDefParser::lefiViaRuleLayer*)obj)->print(f);
}

int lefiViaRule_hasGenerate (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->hasGenerate();
}

int lefiViaRule_hasDefault (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->hasDefault();
}

char* lefiViaRule_name (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->name();
}

int lefiViaRule_numLayers (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->numLayers();
}

const ::lefiViaRuleLayer* lefiViaRule_layer (const ::lefiViaRule* obj, int  index) {
    return (const ::lefiViaRuleLayer*) ((LefDefParser::lefiViaRule*)obj)->layer(index);
}

int lefiViaRule_numVias (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->numVias();
}

char* lefiViaRule_viaName (const ::lefiViaRule* obj, int  index) {
    return ((LefDefParser::lefiViaRule*)obj)->viaName(index);
}

int lefiViaRule_numProps (const ::lefiViaRule* obj) {
    return ((LefDefParser::lefiViaRule*)obj)->numProps();
}

const char* lefiViaRule_propName (const ::lefiViaRule* obj, int  index) {
    return ((const LefDefParser::lefiViaRule*)obj)->propName(index);
}

const char* lefiViaRule_propValue (const ::lefiViaRule* obj, int  index) {
    return ((const LefDefParser::lefiViaRule*)obj)->propValue(index);
}

double lefiViaRule_propNumber (const ::lefiViaRule* obj, int  index) {
    return ((LefDefParser::lefiViaRule*)obj)->propNumber(index);
}

const char lefiViaRule_propType (const ::lefiViaRule* obj, int  index) {
    return ((const LefDefParser::lefiViaRule*)obj)->propType(index);
}

int lefiViaRule_propIsNumber (const ::lefiViaRule* obj, int  index) {
    return ((LefDefParser::lefiViaRule*)obj)->propIsNumber(index);
}

int lefiViaRule_propIsString (const ::lefiViaRule* obj, int  index) {
    return ((LefDefParser::lefiViaRule*)obj)->propIsString(index);
}

void lefiViaRule_print (const ::lefiViaRule* obj, FILE*  f) {
    ((LefDefParser::lefiViaRule*)obj)->print(f);
}

