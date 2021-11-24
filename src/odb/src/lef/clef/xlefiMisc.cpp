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

#include "lefiMisc.h"
#include "lefiMisc.hpp"

// Wrappers definitions.
int lefiGeometries_numItems (const ::lefiGeometries* obj) {
    return ((LefDefParser::lefiGeometries*)obj)->numItems();
}

enum ::lefiGeomEnum lefiGeometries_itemType (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomEnum) ((LefDefParser::lefiGeometries*)obj)->itemType(index);
}

struct ::lefiGeomRect* lefiGeometries_getRect (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomRect*) ((LefDefParser::lefiGeometries*)obj)->getRect(index);
}

struct ::lefiGeomRectIter* lefiGeometries_getRectIter (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomRectIter*) ((LefDefParser::lefiGeometries*)obj)->getRectIter(index);
}

struct ::lefiGeomPath* lefiGeometries_getPath (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomPath*) ((LefDefParser::lefiGeometries*)obj)->getPath(index);
}

struct ::lefiGeomPathIter* lefiGeometries_getPathIter (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomPathIter*) ((LefDefParser::lefiGeometries*)obj)->getPathIter(index);
}

int lefiGeometries_hasLayerExceptPgNet (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->hasLayerExceptPgNet(index);
}

char* lefiGeometries_getLayer (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->getLayer(index);
}

double lefiGeometries_getLayerMinSpacing (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->getLayerMinSpacing(index);
}

double lefiGeometries_getLayerRuleWidth (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->getLayerRuleWidth(index);
}

double lefiGeometries_getWidth (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->getWidth(index);
}

struct ::lefiGeomPolygon* lefiGeometries_getPolygon (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomPolygon*) ((LefDefParser::lefiGeometries*)obj)->getPolygon(index);
}

struct ::lefiGeomPolygonIter* lefiGeometries_getPolygonIter (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomPolygonIter*) ((LefDefParser::lefiGeometries*)obj)->getPolygonIter(index);
}

char* lefiGeometries_getClass (const ::lefiGeometries* obj, int  index) {
    return ((LefDefParser::lefiGeometries*)obj)->getClass(index);
}

struct ::lefiGeomVia* lefiGeometries_getVia (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomVia*) ((LefDefParser::lefiGeometries*)obj)->getVia(index);
}

struct ::lefiGeomViaIter* lefiGeometries_getViaIter (const ::lefiGeometries* obj, int  index) {
    return (::lefiGeomViaIter*) ((LefDefParser::lefiGeometries*)obj)->getViaIter(index);
}

void lefiGeometries_print (const ::lefiGeometries* obj, FILE*  f) {
    ((LefDefParser::lefiGeometries*)obj)->print(f);
}

int lefiSpacing_hasStack (const ::lefiSpacing* obj) {
    return ((LefDefParser::lefiSpacing*)obj)->hasStack();
}

const char* lefiSpacing_name1 (const ::lefiSpacing* obj) {
    return ((const LefDefParser::lefiSpacing*)obj)->name1();
}

const char* lefiSpacing_name2 (const ::lefiSpacing* obj) {
    return ((const LefDefParser::lefiSpacing*)obj)->name2();
}

double lefiSpacing_distance (const ::lefiSpacing* obj) {
    return ((LefDefParser::lefiSpacing*)obj)->distance();
}

void lefiSpacing_print (const ::lefiSpacing* obj, FILE*  f) {
    ((LefDefParser::lefiSpacing*)obj)->print(f);
}

const char* lefiIRDrop_name (const ::lefiIRDrop* obj) {
    return ((const LefDefParser::lefiIRDrop*)obj)->name();
}

double lefiIRDrop_value1 (const ::lefiIRDrop* obj, int  index) {
    return ((LefDefParser::lefiIRDrop*)obj)->value1(index);
}

double lefiIRDrop_value2 (const ::lefiIRDrop* obj, int  index) {
    return ((LefDefParser::lefiIRDrop*)obj)->value2(index);
}

int lefiIRDrop_numValues (const ::lefiIRDrop* obj) {
    return ((LefDefParser::lefiIRDrop*)obj)->numValues();
}

void lefiIRDrop_print (const ::lefiIRDrop* obj, FILE*  f) {
    ((LefDefParser::lefiIRDrop*)obj)->print(f);
}

double lefiMinFeature_one (const ::lefiMinFeature* obj) {
    return ((LefDefParser::lefiMinFeature*)obj)->one();
}

double lefiMinFeature_two (const ::lefiMinFeature* obj) {
    return ((LefDefParser::lefiMinFeature*)obj)->two();
}

void lefiMinFeature_print (const ::lefiMinFeature* obj, FILE*  f) {
    ((LefDefParser::lefiMinFeature*)obj)->print(f);
}

const char* lefiSite_name (const ::lefiSite* obj) {
    return ((const LefDefParser::lefiSite*)obj)->name();
}

int lefiSite_hasClass (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->hasClass();
}

const char* lefiSite_siteClass (const ::lefiSite* obj) {
    return ((const LefDefParser::lefiSite*)obj)->siteClass();
}

double lefiSite_sizeX (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->sizeX();
}

double lefiSite_sizeY (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->sizeY();
}

int lefiSite_hasSize (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->hasSize();
}

int lefiSite_hasXSymmetry (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->hasXSymmetry();
}

int lefiSite_hasYSymmetry (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->hasYSymmetry();
}

int lefiSite_has90Symmetry (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->has90Symmetry();
}

int lefiSite_hasRowPattern (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->hasRowPattern();
}

int lefiSite_numSites (const ::lefiSite* obj) {
    return ((LefDefParser::lefiSite*)obj)->numSites();
}

char* lefiSite_siteName (const ::lefiSite* obj, int  index) {
    return ((LefDefParser::lefiSite*)obj)->siteName(index);
}

int lefiSite_siteOrient (const ::lefiSite* obj, int  index) {
    return ((LefDefParser::lefiSite*)obj)->siteOrient(index);
}

char* lefiSite_siteOrientStr (const ::lefiSite* obj, int  index) {
    return ((LefDefParser::lefiSite*)obj)->siteOrientStr(index);
}

void lefiSite_print (const ::lefiSite* obj, FILE*  f) {
    ((LefDefParser::lefiSite*)obj)->print(f);
}

const char* lefiSitePattern_name (const ::lefiSitePattern* obj) {
    return ((const LefDefParser::lefiSitePattern*)obj)->name();
}

int lefiSitePattern_orient (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->orient();
}

const char* lefiSitePattern_orientStr (const ::lefiSitePattern* obj) {
    return ((const LefDefParser::lefiSitePattern*)obj)->orientStr();
}

double lefiSitePattern_x (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->x();
}

double lefiSitePattern_y (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->y();
}

int lefiSitePattern_hasStepPattern (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->hasStepPattern();
}

double lefiSitePattern_xStart (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->xStart();
}

double lefiSitePattern_yStart (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->yStart();
}

double lefiSitePattern_xStep (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->xStep();
}

double lefiSitePattern_yStep (const ::lefiSitePattern* obj) {
    return ((LefDefParser::lefiSitePattern*)obj)->yStep();
}

void lefiSitePattern_print (const ::lefiSitePattern* obj, FILE*  f) {
    ((LefDefParser::lefiSitePattern*)obj)->print(f);
}

const char* lefiTrackPattern_name (const ::lefiTrackPattern* obj) {
    return ((const LefDefParser::lefiTrackPattern*)obj)->name();
}

double lefiTrackPattern_start (const ::lefiTrackPattern* obj) {
    return ((LefDefParser::lefiTrackPattern*)obj)->start();
}

int lefiTrackPattern_numTracks (const ::lefiTrackPattern* obj) {
    return ((LefDefParser::lefiTrackPattern*)obj)->numTracks();
}

double lefiTrackPattern_space (const ::lefiTrackPattern* obj) {
    return ((LefDefParser::lefiTrackPattern*)obj)->space();
}

int lefiTrackPattern_numLayers (const ::lefiTrackPattern* obj) {
    return ((LefDefParser::lefiTrackPattern*)obj)->numLayers();
}

const char* lefiTrackPattern_layerName (const ::lefiTrackPattern* obj, int  index) {
    return ((const LefDefParser::lefiTrackPattern*)obj)->layerName(index);
}

void lefiTrackPattern_print (const ::lefiTrackPattern* obj, FILE*  f) {
    ((LefDefParser::lefiTrackPattern*)obj)->print(f);
}

const char* lefiGcellPattern_name (const ::lefiGcellPattern* obj) {
    return ((const LefDefParser::lefiGcellPattern*)obj)->name();
}

double lefiGcellPattern_start (const ::lefiGcellPattern* obj) {
    return ((LefDefParser::lefiGcellPattern*)obj)->start();
}

int lefiGcellPattern_numCRs (const ::lefiGcellPattern* obj) {
    return ((LefDefParser::lefiGcellPattern*)obj)->numCRs();
}

double lefiGcellPattern_space (const ::lefiGcellPattern* obj) {
    return ((LefDefParser::lefiGcellPattern*)obj)->space();
}

void lefiGcellPattern_print (const ::lefiGcellPattern* obj, FILE*  f) {
    ((LefDefParser::lefiGcellPattern*)obj)->print(f);
}

const char* lefiUseMinSpacing_name (const ::lefiUseMinSpacing* obj) {
    return ((const LefDefParser::lefiUseMinSpacing*)obj)->name();
}

int lefiUseMinSpacing_value (const ::lefiUseMinSpacing* obj) {
    return ((LefDefParser::lefiUseMinSpacing*)obj)->value();
}

void lefiUseMinSpacing_print (const ::lefiUseMinSpacing* obj, FILE*  f) {
    ((LefDefParser::lefiUseMinSpacing*)obj)->print(f);
}

int lefiMaxStackVia_maxStackVia (const ::lefiMaxStackVia* obj) {
    return ((LefDefParser::lefiMaxStackVia*)obj)->maxStackVia();
}

int lefiMaxStackVia_hasMaxStackViaRange (const ::lefiMaxStackVia* obj) {
    return ((LefDefParser::lefiMaxStackVia*)obj)->hasMaxStackViaRange();
}

const char* lefiMaxStackVia_maxStackViaBottomLayer (const ::lefiMaxStackVia* obj) {
    return ((const LefDefParser::lefiMaxStackVia*)obj)->maxStackViaBottomLayer();
}

const char* lefiMaxStackVia_maxStackViaTopLayer (const ::lefiMaxStackVia* obj) {
    return ((const LefDefParser::lefiMaxStackVia*)obj)->maxStackViaTopLayer();
}

void lefiMaxStackVia_print (const ::lefiMaxStackVia* obj, FILE*  f) {
    ((LefDefParser::lefiMaxStackVia*)obj)->print(f);
}

