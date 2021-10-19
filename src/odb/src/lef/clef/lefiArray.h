/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2013, Cadence Design Systems                              */
/*                                                                            */
/* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source             */
/* Distribution,  Product Version 5.8.                                        */
/*                                                                            */
/* Licensed under the Apache License, Version 2.0 (the "License");            */
/*    you may not use this file except in compliance with the License.        */
/*    You may obtain a copy of the License at                                 */
/*                                                                            */
/*        http://www.apache.org/licenses/LICENSE-2.0                          */
/*                                                                            */
/*    Unless required by applicable law or agreed to in writing, software     */
/*    distributed under the License is distributed on an "AS IS" BASIS,       */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or         */
/*    implied. See the License for the specific language governing            */
/*    permissions and limitations under the License.                          */
/*                                                                            */
/* For updates, support, or to become part of the LEF/DEF Community,          */
/* check www.openeda.org for details.                                         */
/*                                                                            */
/*  $Author: dell $                                                                  */
/*  $Revision: #1 $                                                                */
/*  $Date: 2017/06/06 $                                                                    */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CLEFIARRAY_H
#define CLEFIARRAY_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN int lefiArrayFloorPlan_numPatterns (const lefiArrayFloorPlan* obj);
EXTERN const lefiSitePattern* lefiArrayFloorPlan_pattern (const lefiArrayFloorPlan* obj, int  index);
EXTERN char* lefiArrayFloorPlan_typ (const lefiArrayFloorPlan* obj, int  index);
EXTERN const char* lefiArrayFloorPlan_name (const lefiArrayFloorPlan* obj);

EXTERN int lefiArray_numSitePattern (const lefiArray* obj);
EXTERN int lefiArray_numCanPlace (const lefiArray* obj);
EXTERN int lefiArray_numCannotOccupy (const lefiArray* obj);
EXTERN int lefiArray_numTrack (const lefiArray* obj);
EXTERN int lefiArray_numGcell (const lefiArray* obj);
EXTERN int lefiArray_hasDefaultCap (const lefiArray* obj);

EXTERN const char* lefiArray_name (const lefiArray* obj);
EXTERN const lefiSitePattern* lefiArray_sitePattern (const lefiArray* obj, int  index);
EXTERN const lefiSitePattern* lefiArray_canPlace (const lefiArray* obj, int  index);
EXTERN const lefiSitePattern* lefiArray_cannotOccupy (const lefiArray* obj, int  index);
EXTERN const lefiTrackPattern* lefiArray_track (const lefiArray* obj, int  index);
EXTERN const lefiGcellPattern* lefiArray_gcell (const lefiArray* obj, int  index);

EXTERN int lefiArray_tableSize (const lefiArray* obj);
EXTERN int lefiArray_numDefaultCaps (const lefiArray* obj);
EXTERN int lefiArray_defaultCapMinPins (const lefiArray* obj, int  index);
EXTERN double lefiArray_defaultCap (const lefiArray* obj, int  index);

EXTERN int lefiArray_numFloorPlans (const lefiArray* obj);
EXTERN const char* lefiArray_floorPlanName (const lefiArray* obj, int  index);
EXTERN int lefiArray_numSites (const lefiArray* obj, int  index);
EXTERN const char* lefiArray_siteType (const lefiArray* obj, int  floorIndex, int  siteIndex);
EXTERN const lefiSitePattern* lefiArray_site (const lefiArray* obj, int  floorIndex, int  siteIndex);

  /* Debug print                                                              */
EXTERN void lefiArray_print (const lefiArray* obj, FILE*  f);

#endif
