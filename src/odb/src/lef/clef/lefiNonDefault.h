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


#ifndef CLEFINONDEFAULT_H
#define CLEFINONDEFAULT_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN const char* lefiNonDefault_name (const lefiNonDefault* obj);
EXTERN int lefiNonDefault_hasHardspacing (const lefiNonDefault* obj);

EXTERN int lefiNonDefault_numProps (const lefiNonDefault* obj);
EXTERN const char* lefiNonDefault_propName (const lefiNonDefault* obj, int  index);
EXTERN const char* lefiNonDefault_propValue (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_propNumber (const lefiNonDefault* obj, int  index);
EXTERN const char lefiNonDefault_propType (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_propIsNumber (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_propIsString (const lefiNonDefault* obj, int  index);

  /* A non default rule can have one or more layers.                          */
  /* The layer information is kept in an array.                               */
EXTERN int lefiNonDefault_numLayers (const lefiNonDefault* obj);
EXTERN const char* lefiNonDefault_layerName (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerWidth (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerWidth (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerSpacing (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerSpacing (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerWireExtension (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerWireExtension (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerResistance (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerResistance (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerCapacitance (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerCapacitance (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerEdgeCap (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerEdgeCap (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_hasLayerDiagWidth (const lefiNonDefault* obj, int  index);
EXTERN double lefiNonDefault_layerDiagWidth (const lefiNonDefault* obj, int  index);

  /* A non default rule can have one or more vias.                            */
  /* These routines return the via info.                                      */
EXTERN int lefiNonDefault_numVias (const lefiNonDefault* obj);
EXTERN const lefiVia* lefiNonDefault_viaRule (const lefiNonDefault* obj, int  index);

  /* A non default rule can have one or more spacing rules.                   */
  /* These routines return the that info.                                     */
EXTERN int lefiNonDefault_numSpacingRules (const lefiNonDefault* obj);
EXTERN const lefiSpacing* lefiNonDefault_spacingRule (const lefiNonDefault* obj, int  index);

EXTERN int lefiNonDefault_numUseVia (const lefiNonDefault* obj);
EXTERN const char* lefiNonDefault_viaName (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_numUseViaRule (const lefiNonDefault* obj);
EXTERN const char* lefiNonDefault_viaRuleName (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_numMinCuts (const lefiNonDefault* obj);
EXTERN const char* lefiNonDefault_cutLayerName (const lefiNonDefault* obj, int  index);
EXTERN int lefiNonDefault_numCuts (const lefiNonDefault* obj, int  index);

  /* Debug print                                                              */

  /* Layer information                                                        */

  /* 5.4                                                                      */

#endif
