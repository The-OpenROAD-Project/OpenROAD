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


#ifndef CDEFIGROUP_H
#define CDEFIGROUP_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Struct holds the data for one property.                                    */

EXTERN const char* defiGroup_name (const defiGroup* obj);
EXTERN const char* defiGroup_regionName (const defiGroup* obj);
EXTERN int defiGroup_hasRegionBox (const defiGroup* obj);
EXTERN int defiGroup_hasRegionName (const defiGroup* obj);
EXTERN int defiGroup_hasMaxX (const defiGroup* obj);
EXTERN int defiGroup_hasMaxY (const defiGroup* obj);
EXTERN int defiGroup_hasPerim (const defiGroup* obj);
EXTERN void defiGroup_regionRects (const defiGroup* obj, int*  size, int**  xl, int** yl, int**  xh, int**  yh);
EXTERN int defiGroup_maxX (const defiGroup* obj);
EXTERN int defiGroup_maxY (const defiGroup* obj);
EXTERN int defiGroup_perim (const defiGroup* obj);

EXTERN int defiGroup_numProps (const defiGroup* obj);
EXTERN const char* defiGroup_propName (const defiGroup* obj, int  index);
EXTERN const char* defiGroup_propValue (const defiGroup* obj, int  index);
EXTERN double defiGroup_propNumber (const defiGroup* obj, int  index);
EXTERN char defiGroup_propType (const defiGroup* obj, int  index);
EXTERN int defiGroup_propIsNumber (const defiGroup* obj, int  index);
EXTERN int defiGroup_propIsString (const defiGroup* obj, int  index);

  /* debug print                                                              */
EXTERN void defiGroup_print (const defiGroup* obj, FILE*  f);

#endif
