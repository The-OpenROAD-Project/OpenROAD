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


#ifndef CDEFIREGION_H
#define CDEFIREGION_H

#include <stdio.h>
#include "defiTypedefs.h"

/* Struct holds the data for one property.                                    */

EXTERN const char* defiRegion_name (const defiRegion* obj);

EXTERN int defiRegion_numProps (const defiRegion* obj);
EXTERN const char* defiRegion_propName (const defiRegion* obj, int  index);
EXTERN const char* defiRegion_propValue (const defiRegion* obj, int  index);
EXTERN double defiRegion_propNumber (const defiRegion* obj, int  index);
EXTERN char defiRegion_propType (const defiRegion* obj, int  index);
EXTERN int defiRegion_propIsNumber (const defiRegion* obj, int  index);
EXTERN int defiRegion_propIsString (const defiRegion* obj, int  index);

EXTERN int defiRegion_hasType (const defiRegion* obj);
EXTERN const char* defiRegion_type (const defiRegion* obj);

EXTERN int defiRegion_numRectangles (const defiRegion* obj);
EXTERN int defiRegion_xl (const defiRegion* obj, int  index);
EXTERN int defiRegion_yl (const defiRegion* obj, int  index);
EXTERN int defiRegion_xh (const defiRegion* obj, int  index);
EXTERN int defiRegion_yh (const defiRegion* obj, int  index);

EXTERN void defiRegion_print (const defiRegion* obj, FILE*  f);

#endif
