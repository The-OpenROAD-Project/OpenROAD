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


#ifndef CDEFIROWTRACK_H
#define CDEFIROWTRACK_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN const char* defiRow_name (const defiRow* obj);
EXTERN const char* defiRow_macro (const defiRow* obj);
EXTERN double defiRow_x (const defiRow* obj);
EXTERN double defiRow_y (const defiRow* obj);
EXTERN int defiRow_orient (const defiRow* obj);
EXTERN const char* defiRow_orientStr (const defiRow* obj);
EXTERN int defiRow_hasDo (const defiRow* obj);
EXTERN double defiRow_xNum (const defiRow* obj);
EXTERN double defiRow_yNum (const defiRow* obj);
EXTERN int defiRow_hasDoStep (const defiRow* obj);
EXTERN double defiRow_xStep (const defiRow* obj);
EXTERN double defiRow_yStep (const defiRow* obj);

EXTERN int defiRow_numProps (const defiRow* obj);
EXTERN const char* defiRow_propName (const defiRow* obj, int  index);
EXTERN const char* defiRow_propValue (const defiRow* obj, int  index);
EXTERN double defiRow_propNumber (const defiRow* obj, int  index);
EXTERN char defiRow_propType (const defiRow* obj, int  index);
EXTERN int defiRow_propIsNumber (const defiRow* obj, int  index);
EXTERN int defiRow_propIsString (const defiRow* obj, int  index);

EXTERN void defiRow_print (const defiRow* obj, FILE*  f);

EXTERN const char* defiTrack_macro (const defiTrack* obj);
EXTERN double defiTrack_x (const defiTrack* obj);
EXTERN double defiTrack_xNum (const defiTrack* obj);
EXTERN double defiTrack_xStep (const defiTrack* obj);
EXTERN int defiTrack_numLayers (const defiTrack* obj);
EXTERN const char* defiTrack_layer (const defiTrack* obj, int  index);
EXTERN int defiTrack_firstTrackMask (const defiTrack* obj);
EXTERN int defiTrack_sameMask (const defiTrack* obj);

EXTERN void defiTrack_print (const defiTrack* obj, FILE*  f);

EXTERN const char* defiGcellGrid_macro (const defiGcellGrid* obj);
EXTERN int defiGcellGrid_x (const defiGcellGrid* obj);
EXTERN int defiGcellGrid_xNum (const defiGcellGrid* obj);
EXTERN double defiGcellGrid_xStep (const defiGcellGrid* obj);

EXTERN void defiGcellGrid_print (const defiGcellGrid* obj, FILE*  f);

#endif
