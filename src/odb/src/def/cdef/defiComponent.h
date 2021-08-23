/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013, Cadence Design Systems                                     */
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


#ifndef CDEFICOMPONENT_H
#define CDEFICOMPONENT_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Placement status for the component.                                        */
/* Default is 0                                                               */
#define DEFI_COMPONENT_UNPLACED 1
#define DEFI_COMPONENT_PLACED 2
#define DEFI_COMPONENT_FIXED 3
#define DEFI_COMPONENT_COVER 4

/* Struct holds the data for componentMaskShiftLayers.                        */

EXTERN int defiComponentMaskShiftLayer_numMaskShiftLayers (const defiComponentMaskShiftLayer* obj);
EXTERN const char* defiComponentMaskShiftLayer_maskShiftLayer (const defiComponentMaskShiftLayer* obj, int  index);

/* Struct holds the data for one component.                                   */

                                                            /* 5.7            */

  /* For OA to modify the Id & Name                                           */

EXTERN const char* defiComponent_id (const defiComponent* obj);
EXTERN const char* defiComponent_name (const defiComponent* obj);
EXTERN int defiComponent_placementStatus (const defiComponent* obj);
EXTERN int defiComponent_isUnplaced (const defiComponent* obj);
EXTERN int defiComponent_isPlaced (const defiComponent* obj);
EXTERN int defiComponent_isFixed (const defiComponent* obj);
EXTERN int defiComponent_isCover (const defiComponent* obj);
EXTERN int defiComponent_placementX (const defiComponent* obj);
EXTERN int defiComponent_placementY (const defiComponent* obj);
EXTERN int defiComponent_placementOrient (const defiComponent* obj);
EXTERN const char* defiComponent_placementOrientStr (const defiComponent* obj);
EXTERN int defiComponent_hasRegionName (const defiComponent* obj);
EXTERN int defiComponent_hasRegionBounds (const defiComponent* obj);
EXTERN int defiComponent_hasEEQ (const defiComponent* obj);
EXTERN int defiComponent_hasGenerate (const defiComponent* obj);
EXTERN int defiComponent_hasSource (const defiComponent* obj);
EXTERN int defiComponent_hasWeight (const defiComponent* obj);
EXTERN int defiComponent_weight (const defiComponent* obj);
EXTERN int defiComponent_maskShiftSize (const defiComponent* obj);
EXTERN int defiComponent_maskShift (const defiComponent* obj, int  index);
EXTERN int defiComponent_hasNets (const defiComponent* obj);
EXTERN int defiComponent_numNets (const defiComponent* obj);
EXTERN const char* defiComponent_net (const defiComponent* obj, int  index);
EXTERN const char* defiComponent_regionName (const defiComponent* obj);
EXTERN const char* defiComponent_source (const defiComponent* obj);
EXTERN const char* defiComponent_EEQ (const defiComponent* obj);
EXTERN const char* defiComponent_generateName (const defiComponent* obj);
EXTERN const char* defiComponent_macroName (const defiComponent* obj);
EXTERN int defiComponent_hasHalo (const defiComponent* obj);
EXTERN int defiComponent_hasHaloSoft (const defiComponent* obj);
EXTERN int defiComponent_hasRouteHalo (const defiComponent* obj);
EXTERN int defiComponent_haloDist (const defiComponent* obj);
EXTERN const char* defiComponent_minLayer (const defiComponent* obj);
EXTERN const char* defiComponent_maxLayer (const defiComponent* obj);

  /* Returns arrays for the ll and ur of the rectangles in the region.        */
  /* The number of items in the arrays is given in size.                      */
EXTERN void defiComponent_regionBounds (const defiComponent* obj, int* size, int**  xl, int**  yl, int**  xh, int**  yh);

EXTERN int defiComponent_hasForeignName (const defiComponent* obj);
EXTERN const char* defiComponent_foreignName (const defiComponent* obj);
EXTERN int defiComponent_foreignX (const defiComponent* obj);
EXTERN int defiComponent_foreignY (const defiComponent* obj);
EXTERN const char* defiComponent_foreignOri (const defiComponent* obj);
EXTERN int defiComponent_foreignOrient (const defiComponent* obj);
EXTERN int defiComponent_hasFori (const defiComponent* obj);

EXTERN int defiComponent_numProps (const defiComponent* obj);
EXTERN char* defiComponent_propName (const defiComponent* obj, int  index);
EXTERN char* defiComponent_propValue (const defiComponent* obj, int  index);
EXTERN double defiComponent_propNumber (const defiComponent* obj, int  index);
EXTERN char defiComponent_propType (const defiComponent* obj, int  index);
EXTERN int defiComponent_propIsNumber (const defiComponent* obj, int  index);
EXTERN int defiComponent_propIsString (const defiComponent* obj, int  index);

  /* Debug printing                                                           */
EXTERN void defiComponent_print (const defiComponent* obj, FILE*  fout);

#endif
