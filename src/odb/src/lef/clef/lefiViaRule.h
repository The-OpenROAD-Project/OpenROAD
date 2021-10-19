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


#ifndef CLEFIVIARULE_H
#define CLEFIVIARULE_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN int lefiViaRuleLayer_hasDirection (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasEnclosure (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasWidth (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasResistance (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasOverhang (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasMetalOverhang (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasSpacing (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_hasRect (const lefiViaRuleLayer* obj);

EXTERN char* lefiViaRuleLayer_name (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_isHorizontal (const lefiViaRuleLayer* obj);
EXTERN int lefiViaRuleLayer_isVertical (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_enclosureOverhang1 (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_enclosureOverhang2 (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_widthMin (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_widthMax (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_overhang (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_metalOverhang (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_resistance (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_spacingStepX (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_spacingStepY (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_xl (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_yl (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_xh (const lefiViaRuleLayer* obj);
EXTERN double lefiViaRuleLayer_yh (const lefiViaRuleLayer* obj);

  /* Debug print                                                              */
EXTERN void lefiViaRuleLayer_print (const lefiViaRuleLayer* obj, FILE*  f);

  /* This should clear out all the old stuff.                                 */

  /* Add one of possibly many via names                                       */

  /* These routines set a part of the active layer.                           */

  /* This routine sets and creates the active layer.                          */

EXTERN int lefiViaRule_hasGenerate (const lefiViaRule* obj);
EXTERN int lefiViaRule_hasDefault (const lefiViaRule* obj);
EXTERN char* lefiViaRule_name (const lefiViaRule* obj);

  /* There are 2 or 3 layers in a rule.                                       */
  /* numLayers() tells how many.                                              */
  /* If a third layer exists then it is the cut layer.                        */
EXTERN int lefiViaRule_numLayers (const lefiViaRule* obj);
EXTERN const lefiViaRuleLayer* lefiViaRule_layer (const lefiViaRule* obj, int  index);

EXTERN int lefiViaRule_numVias (const lefiViaRule* obj);
EXTERN char* lefiViaRule_viaName (const lefiViaRule* obj, int  index);

EXTERN int lefiViaRule_numProps (const lefiViaRule* obj);
EXTERN const char* lefiViaRule_propName (const lefiViaRule* obj, int  index);
EXTERN const char* lefiViaRule_propValue (const lefiViaRule* obj, int  index);
EXTERN double lefiViaRule_propNumber (const lefiViaRule* obj, int  index);
EXTERN const char lefiViaRule_propType (const lefiViaRule* obj, int  index);
EXTERN int lefiViaRule_propIsNumber (const lefiViaRule* obj, int  index);
EXTERN int lefiViaRule_propIsString (const lefiViaRule* obj, int  index);

  /* Debug print                                                              */
EXTERN void lefiViaRule_print (const lefiViaRule* obj, FILE*  f);

#endif
