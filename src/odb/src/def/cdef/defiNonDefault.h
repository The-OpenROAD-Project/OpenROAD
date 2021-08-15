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


#ifndef CDEFINONDEFAULT_H
#define CDEFINONDEFAULT_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN const char* defiNonDefault_name (const defiNonDefault* obj);
EXTERN int defiNonDefault_hasHardspacing (const defiNonDefault* obj);

EXTERN int defiNonDefault_numProps (const defiNonDefault* obj);
EXTERN const char* defiNonDefault_propName (const defiNonDefault* obj, int  index);
EXTERN const char* defiNonDefault_propValue (const defiNonDefault* obj, int  index);
EXTERN double defiNonDefault_propNumber (const defiNonDefault* obj, int  index);
EXTERN char defiNonDefault_propType (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_propIsNumber (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_propIsString (const defiNonDefault* obj, int  index);

  /* A non default rule can have one or more layers.                          */
  /* The layer information is kept in an array.                               */
EXTERN int defiNonDefault_numLayers (const defiNonDefault* obj);
EXTERN const char* defiNonDefault_layerName (const defiNonDefault* obj, int  index);
EXTERN double defiNonDefault_layerWidth (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_layerWidthVal (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_hasLayerDiagWidth (const defiNonDefault* obj, int  index);
EXTERN double defiNonDefault_layerDiagWidth (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_layerDiagWidthVal (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_hasLayerSpacing (const defiNonDefault* obj, int  index);
EXTERN double defiNonDefault_layerSpacing (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_layerSpacingVal (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_hasLayerWireExt (const defiNonDefault* obj, int  index);
EXTERN double defiNonDefault_layerWireExt (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_layerWireExtVal (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_numVias (const defiNonDefault* obj);
EXTERN const char* defiNonDefault_viaName (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_numViaRules (const defiNonDefault* obj);
EXTERN const char* defiNonDefault_viaRuleName (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_numMinCuts (const defiNonDefault* obj);
EXTERN const char* defiNonDefault_cutLayerName (const defiNonDefault* obj, int  index);
EXTERN int defiNonDefault_numCuts (const defiNonDefault* obj, int  index);

  /* Debug print                                                              */
EXTERN void defiNonDefault_print (const defiNonDefault* obj, FILE*  f);

  /* Layer information                                                        */

#endif
