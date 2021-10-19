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


#ifndef CDEFIBLOCKAGE_H
#define CDEFIBLOCKAGE_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiBlockage_hasLayer (const defiBlockage* obj);
EXTERN int defiBlockage_hasPlacement (const defiBlockage* obj);
EXTERN int defiBlockage_hasComponent (const defiBlockage* obj);
EXTERN int defiBlockage_hasSlots (const defiBlockage* obj);
EXTERN int defiBlockage_hasFills (const defiBlockage* obj);
EXTERN int defiBlockage_hasPushdown (const defiBlockage* obj);
EXTERN int defiBlockage_hasExceptpgnet (const defiBlockage* obj);
EXTERN int defiBlockage_hasSoft (const defiBlockage* obj);
EXTERN int defiBlockage_hasPartial (const defiBlockage* obj);
EXTERN int defiBlockage_hasSpacing (const defiBlockage* obj);
EXTERN int defiBlockage_hasDesignRuleWidth (const defiBlockage* obj);
EXTERN int defiBlockage_hasMask (const defiBlockage* obj);
EXTERN int defiBlockage_mask (const defiBlockage* obj);
EXTERN int defiBlockage_minSpacing (const defiBlockage* obj);
EXTERN int defiBlockage_designRuleWidth (const defiBlockage* obj);
EXTERN double defiBlockage_placementMaxDensity (const defiBlockage* obj);
EXTERN const char* defiBlockage_layerName (const defiBlockage* obj);
EXTERN const char* defiBlockage_layerComponentName (const defiBlockage* obj);
EXTERN const char* defiBlockage_placementComponentName (const defiBlockage* obj);

EXTERN int defiBlockage_numRectangles (const defiBlockage* obj);
EXTERN int defiBlockage_xl (const defiBlockage* obj, int  index);
EXTERN int defiBlockage_yl (const defiBlockage* obj, int  index);
EXTERN int defiBlockage_xh (const defiBlockage* obj, int  index);
EXTERN int defiBlockage_yh (const defiBlockage* obj, int  index);

EXTERN int defiBlockage_numPolygons (const defiBlockage* obj);
EXTERN struct defiPoints defiBlockage_getPolygon (const defiBlockage* obj, int  index);

EXTERN void defiBlockage_print (const defiBlockage* obj, FILE*  f);

#endif
