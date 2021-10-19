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


#ifndef CLEFIVIA_H
#define CLEFIVIA_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN struct lefiGeomPolygon* lefiViaLayer_getPolygon (const lefiViaLayer* obj, int  index);

  /* setName calls clear to init                                              */
  /* deflt=0 no default specified                                             */
  /* deflt=1 default specified in lef file                                    */

  /* orient=-1 means no orient was specified.                                 */

  /* make a new one                                                           */

EXTERN int lefiVia_hasDefault (const lefiVia* obj);
EXTERN int lefiVia_hasGenerated (const lefiVia* obj);
EXTERN int lefiVia_hasForeign (const lefiVia* obj);
EXTERN int lefiVia_hasForeignPnt (const lefiVia* obj);
EXTERN int lefiVia_hasForeignOrient (const lefiVia* obj);
EXTERN int lefiVia_hasProperties (const lefiVia* obj);
EXTERN int lefiVia_hasResistance (const lefiVia* obj);
EXTERN int lefiVia_hasTopOfStack (const lefiVia* obj);

EXTERN int lefiVia_numLayers (const lefiVia* obj);
EXTERN char* lefiVia_layerName (const lefiVia* obj, int  layerNum);
EXTERN int lefiVia_numRects (const lefiVia* obj, int  layerNum);
EXTERN double lefiVia_xl (const lefiVia* obj, int  layerNum, int  rectNum);
EXTERN double lefiVia_yl (const lefiVia* obj, int  layerNum, int  rectNum);
EXTERN double lefiVia_xh (const lefiVia* obj, int  layerNum, int  rectNum);
EXTERN double lefiVia_yh (const lefiVia* obj, int  layerNum, int  rectNum);
EXTERN int lefiVia_rectColorMask (const lefiVia* obj, int  layerNum, int  rectNum);
EXTERN int lefiVia_polyColorMask (const lefiVia* obj, int  layerNum, int  polyNum);
EXTERN int lefiVia_numPolygons (const lefiVia* obj, int  layerNum);
EXTERN struct lefiGeomPolygon lefiVia_getPolygon (const lefiVia* obj, int  layerNum, int  polyNum);

EXTERN char* lefiVia_name (const lefiVia* obj);
EXTERN double lefiVia_resistance (const lefiVia* obj);

  /* Given an index from 0 to numProperties()-1 return                        */
  /* information about that property.                                         */
EXTERN int lefiVia_numProperties (const lefiVia* obj);
EXTERN char* lefiVia_propName (const lefiVia* obj, int  index);
EXTERN char* lefiVia_propValue (const lefiVia* obj, int  index);
EXTERN double lefiVia_propNumber (const lefiVia* obj, int  index);
EXTERN char lefiVia_propType (const lefiVia* obj, int  index);
EXTERN int lefiVia_propIsNumber (const lefiVia* obj, int  index);
EXTERN int lefiVia_propIsString (const lefiVia* obj, int  index);
EXTERN char* lefiVia_foreign (const lefiVia* obj);
EXTERN double lefiVia_foreignX (const lefiVia* obj);
EXTERN double lefiVia_foreignY (const lefiVia* obj);
EXTERN int lefiVia_foreignOrient (const lefiVia* obj);
EXTERN char* lefiVia_foreignOrientStr (const lefiVia* obj);

  /* 5.6 VIARULE inside a VIA                                                 */
EXTERN int lefiVia_hasViaRule (const lefiVia* obj);
EXTERN const char* lefiVia_viaRuleName (const lefiVia* obj);
EXTERN double lefiVia_xCutSize (const lefiVia* obj);
EXTERN double lefiVia_yCutSize (const lefiVia* obj);
EXTERN const char* lefiVia_botMetalLayer (const lefiVia* obj);
EXTERN const char* lefiVia_cutLayer (const lefiVia* obj);
EXTERN const char* lefiVia_topMetalLayer (const lefiVia* obj);
EXTERN double lefiVia_xCutSpacing (const lefiVia* obj);
EXTERN double lefiVia_yCutSpacing (const lefiVia* obj);
EXTERN double lefiVia_xBotEnc (const lefiVia* obj);
EXTERN double lefiVia_yBotEnc (const lefiVia* obj);
EXTERN double lefiVia_xTopEnc (const lefiVia* obj);
EXTERN double lefiVia_yTopEnc (const lefiVia* obj);
EXTERN int lefiVia_hasRowCol (const lefiVia* obj);
EXTERN int lefiVia_numCutRows (const lefiVia* obj);
EXTERN int lefiVia_numCutCols (const lefiVia* obj);
EXTERN int lefiVia_hasOrigin (const lefiVia* obj);
EXTERN double lefiVia_xOffset (const lefiVia* obj);
EXTERN double lefiVia_yOffset (const lefiVia* obj);
EXTERN int lefiVia_hasOffset (const lefiVia* obj);
EXTERN double lefiVia_xBotOffset (const lefiVia* obj);
EXTERN double lefiVia_yBotOffset (const lefiVia* obj);
EXTERN double lefiVia_xTopOffset (const lefiVia* obj);
EXTERN double lefiVia_yTopOffset (const lefiVia* obj);
EXTERN int lefiVia_hasCutPattern (const lefiVia* obj);
EXTERN const char* lefiVia_cutPattern (const lefiVia* obj);

  /* Debug print                                                              */
EXTERN void lefiVia_print (const lefiVia* obj, FILE*  f);

  /* The prop value is stored in the propValue_ or the propDValue_.           */
  /* If it is a string it is in propValue_.  If it is a number,               */
  /* then propValue_ is NULL and it is stored in propDValue_;                 */

#endif
