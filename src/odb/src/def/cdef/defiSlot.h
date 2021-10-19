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


#ifndef CDEFISLOT_H
#define CDEFISLOT_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiSlot_hasLayer (const defiSlot* obj);
EXTERN const char* defiSlot_layerName (const defiSlot* obj);

EXTERN int defiSlot_numRectangles (const defiSlot* obj);
EXTERN int defiSlot_xl (const defiSlot* obj, int  index);
EXTERN int defiSlot_yl (const defiSlot* obj, int  index);
EXTERN int defiSlot_xh (const defiSlot* obj, int  index);
EXTERN int defiSlot_yh (const defiSlot* obj, int  index);

EXTERN int defiSlot_numPolygons (const defiSlot* obj);
EXTERN struct defiPoints defiSlot_getPolygon (const defiSlot* obj, int  index);

EXTERN void defiSlot_print (const defiSlot* obj, FILE*  f);

#endif
