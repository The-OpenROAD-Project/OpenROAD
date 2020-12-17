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

#include "defiPath.h"
#include "defiPath.hpp"

// Wrappers definitions.
void defiPath_initTraverse (const ::defiPath* obj) {
    ((LefDefParser::defiPath*)obj)->initTraverse();
}

void defiPath_initTraverseBackwards (const ::defiPath* obj) {
    ((LefDefParser::defiPath*)obj)->initTraverseBackwards();
}

int defiPath_next (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->next();
}

int defiPath_prev (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->prev();
}

const char* defiPath_getLayer (const ::defiPath* obj) {
    return ((const LefDefParser::defiPath*)obj)->getLayer();
}

const char* defiPath_getTaperRule (const ::defiPath* obj) {
    return ((const LefDefParser::defiPath*)obj)->getTaperRule();
}

const char* defiPath_getVia (const ::defiPath* obj) {
    return ((const LefDefParser::defiPath*)obj)->getVia();
}

const char* defiPath_getShape (const ::defiPath* obj) {
    return ((const LefDefParser::defiPath*)obj)->getShape();
}

int defiPath_getTaper (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getTaper();
}

int defiPath_getStyle (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getStyle();
}

int defiPath_getViaRotation (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getViaRotation();
}

void defiPath_getViaRect (const ::defiPath* obj, int*  deltaX1, int*  deltaY1, int*  deltaX2, int*  deltaY2) {
    ((LefDefParser::defiPath*)obj)->getViaRect(deltaX1, deltaY1, deltaX2, deltaY2);
}

const char* defiPath_getViaRotationStr (const ::defiPath* obj) {
    return ((const LefDefParser::defiPath*)obj)->getViaRotationStr();
}

void defiPath_getViaData (const ::defiPath* obj, int*  numX, int*  numY, int*  stepX, int*  stepY) {
    ((LefDefParser::defiPath*)obj)->getViaData(numX, numY, stepX, stepY);
}

int defiPath_getWidth (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getWidth();
}

void defiPath_getPoint (const ::defiPath* obj, int*  x, int*  y) {
    ((LefDefParser::defiPath*)obj)->getPoint(x, y);
}

void defiPath_getFlushPoint (const ::defiPath* obj, int*  x, int*  y, int*  ext) {
    ((LefDefParser::defiPath*)obj)->getFlushPoint(x, y, ext);
}

void defiPath_getVirtualPoint (const ::defiPath* obj, int*  x, int*  y) {
    ((LefDefParser::defiPath*)obj)->getVirtualPoint(x, y);
}

int defiPath_getMask (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getMask();
}

int defiPath_getViaTopMask (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getViaTopMask();
}

int defiPath_getViaCutMask (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getViaCutMask();
}

int defiPath_getViaBottomMask (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getViaBottomMask();
}

int defiPath_getRectMask (const ::defiPath* obj) {
    return ((LefDefParser::defiPath*)obj)->getRectMask();
}

void defiPath_print (const ::defiPath* obj, FILE*  fout) {
    ((LefDefParser::defiPath*)obj)->print(fout);
}

