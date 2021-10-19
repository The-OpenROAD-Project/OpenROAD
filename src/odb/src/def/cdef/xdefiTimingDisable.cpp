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

#include "defiTimingDisable.h"
#include "defiTimingDisable.hpp"

// Wrappers definitions.
int defiTimingDisable_hasMacroThru (const ::defiTimingDisable* obj) {
    return ((LefDefParser::defiTimingDisable*)obj)->hasMacroThru();
}

int defiTimingDisable_hasMacroFromTo (const ::defiTimingDisable* obj) {
    return ((LefDefParser::defiTimingDisable*)obj)->hasMacroFromTo();
}

int defiTimingDisable_hasThru (const ::defiTimingDisable* obj) {
    return ((LefDefParser::defiTimingDisable*)obj)->hasThru();
}

int defiTimingDisable_hasFromTo (const ::defiTimingDisable* obj) {
    return ((LefDefParser::defiTimingDisable*)obj)->hasFromTo();
}

int defiTimingDisable_hasReentrantPathsFlag (const ::defiTimingDisable* obj) {
    return ((LefDefParser::defiTimingDisable*)obj)->hasReentrantPathsFlag();
}

const char* defiTimingDisable_fromPin (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->fromPin();
}

const char* defiTimingDisable_toPin (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->toPin();
}

const char* defiTimingDisable_fromInst (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->fromInst();
}

const char* defiTimingDisable_toInst (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->toInst();
}

const char* defiTimingDisable_macroName (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->macroName();
}

const char* defiTimingDisable_thruPin (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->thruPin();
}

const char* defiTimingDisable_thruInst (const ::defiTimingDisable* obj) {
    return ((const LefDefParser::defiTimingDisable*)obj)->thruInst();
}

void defiTimingDisable_print (const ::defiTimingDisable* obj, FILE*  f) {
    ((LefDefParser::defiTimingDisable*)obj)->print(f);
}

