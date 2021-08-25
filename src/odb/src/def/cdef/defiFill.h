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


#ifndef CDEFIFILL_H
#define CDEFIFILL_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiFill_hasLayer (const defiFill* obj);
EXTERN const char* defiFill_layerName (const defiFill* obj);
EXTERN int defiFill_hasLayerOpc (const defiFill* obj);

EXTERN int defiFill_layerMask (const defiFill* obj);
EXTERN int defiFill_viaTopMask (const defiFill* obj);
EXTERN int defiFill_viaCutMask (const defiFill* obj);
EXTERN int defiFill_viaBottomMask (const defiFill* obj);

EXTERN int defiFill_numRectangles (const defiFill* obj);
EXTERN int defiFill_xl (const defiFill* obj, int  index);
EXTERN int defiFill_yl (const defiFill* obj, int  index);
EXTERN int defiFill_xh (const defiFill* obj, int  index);
EXTERN int defiFill_yh (const defiFill* obj, int  index);

EXTERN int defiFill_numPolygons (const defiFill* obj);
EXTERN struct defiPoints defiFill_getPolygon (const defiFill* obj, int  index);

EXTERN int defiFill_hasVia (const defiFill* obj);
EXTERN const char* defiFill_viaName (const defiFill* obj);
EXTERN int defiFill_hasViaOpc (const defiFill* obj);

EXTERN int defiFill_numViaPts (const defiFill* obj);
EXTERN struct defiPoints defiFill_getViaPts (const defiFill* obj, int  index);

EXTERN void defiFill_print (const defiFill* obj, FILE*  f);

#endif
