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

#include "lefiArray.h"
#include "lefiArray.hpp"

// Wrappers definitions.
int lefiArrayFloorPlan_numPatterns (const ::lefiArrayFloorPlan* obj) {
    return ((LefDefParser::lefiArrayFloorPlan*)obj)->numPatterns();
}

const ::lefiSitePattern* lefiArrayFloorPlan_pattern (const ::lefiArrayFloorPlan* obj, int  index) {
    return (const ::lefiSitePattern*) ((LefDefParser::lefiArrayFloorPlan*)obj)->pattern(index);
}

char* lefiArrayFloorPlan_typ (const ::lefiArrayFloorPlan* obj, int  index) {
    return ((LefDefParser::lefiArrayFloorPlan*)obj)->typ(index);
}

const char* lefiArrayFloorPlan_name (const ::lefiArrayFloorPlan* obj) {
    return ((const LefDefParser::lefiArrayFloorPlan*)obj)->name();
}

int lefiArray_numSitePattern (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numSitePattern();
}

int lefiArray_numCanPlace (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numCanPlace();
}

int lefiArray_numCannotOccupy (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numCannotOccupy();
}

int lefiArray_numTrack (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numTrack();
}

int lefiArray_numGcell (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numGcell();
}

int lefiArray_hasDefaultCap (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->hasDefaultCap();
}

const char* lefiArray_name (const ::lefiArray* obj) {
    return ((const LefDefParser::lefiArray*)obj)->name();
}

const ::lefiSitePattern* lefiArray_sitePattern (const ::lefiArray* obj, int  index) {
    return (const ::lefiSitePattern*) ((LefDefParser::lefiArray*)obj)->sitePattern(index);
}

const ::lefiSitePattern* lefiArray_canPlace (const ::lefiArray* obj, int  index) {
    return (const ::lefiSitePattern*) ((LefDefParser::lefiArray*)obj)->canPlace(index);
}

const ::lefiSitePattern* lefiArray_cannotOccupy (const ::lefiArray* obj, int  index) {
    return (const ::lefiSitePattern*) ((LefDefParser::lefiArray*)obj)->cannotOccupy(index);
}

const ::lefiTrackPattern* lefiArray_track (const ::lefiArray* obj, int  index) {
    return (const ::lefiTrackPattern*) ((LefDefParser::lefiArray*)obj)->track(index);
}

const ::lefiGcellPattern* lefiArray_gcell (const ::lefiArray* obj, int  index) {
    return (const ::lefiGcellPattern*) ((LefDefParser::lefiArray*)obj)->gcell(index);
}

int lefiArray_tableSize (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->tableSize();
}

int lefiArray_numDefaultCaps (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numDefaultCaps();
}

int lefiArray_defaultCapMinPins (const ::lefiArray* obj, int  index) {
    return ((LefDefParser::lefiArray*)obj)->defaultCapMinPins(index);
}

double lefiArray_defaultCap (const ::lefiArray* obj, int  index) {
    return ((LefDefParser::lefiArray*)obj)->defaultCap(index);
}

int lefiArray_numFloorPlans (const ::lefiArray* obj) {
    return ((LefDefParser::lefiArray*)obj)->numFloorPlans();
}

const char* lefiArray_floorPlanName (const ::lefiArray* obj, int  index) {
    return ((const LefDefParser::lefiArray*)obj)->floorPlanName(index);
}

int lefiArray_numSites (const ::lefiArray* obj, int  index) {
    return ((LefDefParser::lefiArray*)obj)->numSites(index);
}

const char* lefiArray_siteType (const ::lefiArray* obj, int  floorIndex, int  siteIndex) {
    return ((const LefDefParser::lefiArray*)obj)->siteType(floorIndex, siteIndex);
}

const ::lefiSitePattern* lefiArray_site (const ::lefiArray* obj, int  floorIndex, int  siteIndex) {
    return (const ::lefiSitePattern*) ((LefDefParser::lefiArray*)obj)->site(floorIndex, siteIndex);
}

void lefiArray_print (const ::lefiArray* obj, FILE*  f) {
    ((LefDefParser::lefiArray*)obj)->print(f);
}

