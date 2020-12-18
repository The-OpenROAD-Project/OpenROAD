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


#ifndef CDEFIVIA_H
#define CDEFIVIA_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Struct holds the data for one property.                                    */

  /* 5.6                                                                      */

EXTERN const char* defiVia_name (const defiVia* obj);
EXTERN const char* defiVia_pattern (const defiVia* obj);
EXTERN int defiVia_hasPattern (const defiVia* obj);
EXTERN int defiVia_numLayers (const defiVia* obj);
EXTERN void defiVia_layer (const defiVia* obj, int  index, char**  layer, int*  xl, int*  yl, int*  xh, int*  yh);
EXTERN int defiVia_numPolygons (const defiVia* obj);
EXTERN const char* defiVia_polygonName (const defiVia* obj, int  index);
EXTERN struct defiPoints defiVia_getPolygon (const defiVia* obj, int  index);
EXTERN int defiVia_hasViaRule (const defiVia* obj);
EXTERN void defiVia_viaRule (const defiVia* obj, char**  viaRuleName, int*  xSize, int*  ySize, char**  botLayer, char**  cutLayer, char**  topLayer, int*  xCutSpacing, int*  yCutSpacing, int*  xBotEnc, int*  yBotEnc, int*  xTopEnc, int*  yTopEnc);
EXTERN int defiVia_hasRowCol (const defiVia* obj);
EXTERN void defiVia_rowCol (const defiVia* obj, int*  numCutRows, int*  numCutCols);
EXTERN int defiVia_hasOrigin (const defiVia* obj);
EXTERN void defiVia_origin (const defiVia* obj, int*  xOffset, int*  yOffset);
EXTERN int defiVia_hasOffset (const defiVia* obj);
EXTERN void defiVia_offset (const defiVia* obj, int*  xBotOffset, int*  yBotOffset, int*  xTopOffset, int*  yTopOffset);
EXTERN int defiVia_hasCutPattern (const defiVia* obj);
EXTERN const char* defiVia_cutPattern (const defiVia* obj);
EXTERN int defiVia_hasRectMask (const defiVia* obj, int  index);
EXTERN int defiVia_rectMask (const defiVia* obj, int  index);
EXTERN int defiVia_hasPolyMask (const defiVia* obj, int  index);
EXTERN int defiVia_polyMask (const defiVia* obj, int  index);

EXTERN void defiVia_print (const defiVia* obj, FILE*  f);

#endif
