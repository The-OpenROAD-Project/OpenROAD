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

#include "defiScanchain.h"
#include "defiScanchain.hpp"

// Wrappers definitions.
int defiOrdered_num (const ::defiOrdered* obj) {
    return ((LefDefParser::defiOrdered*)obj)->num();
}

char** defiOrdered_inst (const ::defiOrdered* obj) {
    return ((LefDefParser::defiOrdered*)obj)->inst();
}

char** defiOrdered_in (const ::defiOrdered* obj) {
    return ((LefDefParser::defiOrdered*)obj)->in();
}

char** defiOrdered_out (const ::defiOrdered* obj) {
    return ((LefDefParser::defiOrdered*)obj)->out();
}

int* defiOrdered_bits (const ::defiOrdered* obj) {
    return ((LefDefParser::defiOrdered*)obj)->bits();
}

const char* defiScanchain_name (const ::defiScanchain* obj) {
    return ((const LefDefParser::defiScanchain*)obj)->name();
}

int defiScanchain_hasStart (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasStart();
}

int defiScanchain_hasStop (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasStop();
}

int defiScanchain_hasFloating (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasFloating();
}

int defiScanchain_hasOrdered (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasOrdered();
}

int defiScanchain_hasCommonInPin (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasCommonInPin();
}

int defiScanchain_hasCommonOutPin (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasCommonOutPin();
}

int defiScanchain_hasPartition (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasPartition();
}

int defiScanchain_hasPartitionMaxBits (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->hasPartitionMaxBits();
}

void defiScanchain_start (const ::defiScanchain* obj, char**  inst, char**  pin) {
    ((LefDefParser::defiScanchain*)obj)->start(inst, pin);
}

void defiScanchain_stop (const ::defiScanchain* obj, char**  inst, char**  pin) {
    ((LefDefParser::defiScanchain*)obj)->stop(inst, pin);
}

int defiScanchain_numOrderedLists (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->numOrderedLists();
}

void defiScanchain_ordered (const ::defiScanchain* obj, int  index, int*  size, char***  inst, char***  inPin, char***  outPin, int**  bits) {
    ((LefDefParser::defiScanchain*)obj)->ordered(index, size, inst, inPin, outPin, bits);
}

void defiScanchain_floating (const ::defiScanchain* obj, int*  size, char***  inst, char***  inPin, char***  outPin, int**  bits) {
    ((LefDefParser::defiScanchain*)obj)->floating(size, inst, inPin, outPin, bits);
}

const char* defiScanchain_commonInPin (const ::defiScanchain* obj) {
    return ((const LefDefParser::defiScanchain*)obj)->commonInPin();
}

const char* defiScanchain_commonOutPin (const ::defiScanchain* obj) {
    return ((const LefDefParser::defiScanchain*)obj)->commonOutPin();
}

const char* defiScanchain_partitionName (const ::defiScanchain* obj) {
    return ((const LefDefParser::defiScanchain*)obj)->partitionName();
}

int defiScanchain_partitionMaxBits (const ::defiScanchain* obj) {
    return ((LefDefParser::defiScanchain*)obj)->partitionMaxBits();
}

void defiScanchain_print (const ::defiScanchain* obj, FILE*  f) {
    ((LefDefParser::defiScanchain*)obj)->print(f);
}

