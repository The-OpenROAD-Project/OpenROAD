/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2013-2014, Cadence Design Systems                                */
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
/*  $Author: dell $                                                       */
/*  $Revision: #1 $                                                           */
/*  $Date: 2017/06/06 $                                                       */
/*  $State:  $                                                                */
/* ************************************************************************** */
/* ************************************************************************** */


#ifndef CDEFIPATH_H
#define CDEFIPATH_H

#include <stdio.h>
#include "defiTypedefs.h"

/* TX_DIR:TRANSLATION ON                                                      */

/* 5.4.1 1-D & 2-D Arrays of Vias in SPECIALNET Section                       */

/* value returned by the next() routine.                                      */
enum defiPath_e {
  DEFIPATH_DONE = 0,
  DEFIPATH_LAYER = 1,
  DEFIPATH_VIA = 2,
  DEFIPATH_VIAROTATION = 3,
  DEFIPATH_WIDTH = 4,
  DEFIPATH_POINT = 5,
  DEFIPATH_FLUSHPOINT = 6,
  DEFIPATH_TAPER = 7,
  DEFIPATH_SHAPE = 8,
  DEFIPATH_STYLE = 9,
  DEFIPATH_TAPERRULE = 10,
  DEFIPATH_VIADATA = 11,
  DEFIPATH_RECT = 12,
  DEFIPATH_VIRTUALPOINT = 13,
  DEFIPATH_MASK = 14,
  DEFIPATH_VIAMASK = 15
  } ;

  /* This is 'data ownership transfer' constructor.                           */

  /* To traverse the path and get the parts.                                  */
EXTERN void defiPath_initTraverse (const defiPath* obj);
EXTERN void defiPath_initTraverseBackwards (const defiPath* obj);
EXTERN int defiPath_next (const defiPath* obj);
EXTERN int defiPath_prev (const defiPath* obj);
EXTERN const char* defiPath_getLayer (const defiPath* obj);
EXTERN const char* defiPath_getTaperRule (const defiPath* obj);
EXTERN const char* defiPath_getVia (const defiPath* obj);
EXTERN const char* defiPath_getShape (const defiPath* obj);
EXTERN int defiPath_getTaper (const defiPath* obj);
EXTERN int defiPath_getStyle (const defiPath* obj);
EXTERN int defiPath_getViaRotation (const defiPath* obj);
EXTERN void defiPath_getViaRect (const defiPath* obj, int*  deltaX1, int*  deltaY1, int*  deltaX2, int*  deltaY2);
EXTERN const char* defiPath_getViaRotationStr (const defiPath* obj);
EXTERN void defiPath_getViaData (const defiPath* obj, int*  numX, int*  numY, int*  stepX, int*  stepY);
EXTERN int defiPath_getWidth (const defiPath* obj);
EXTERN void defiPath_getPoint (const defiPath* obj, int*  x, int*  y);
EXTERN void defiPath_getFlushPoint (const defiPath* obj, int*  x, int*  y, int*  ext);
EXTERN void defiPath_getVirtualPoint (const defiPath* obj, int*  x, int*  y);
EXTERN int defiPath_getMask (const defiPath* obj);
EXTERN int defiPath_getViaTopMask (const defiPath* obj);
EXTERN int defiPath_getViaCutMask (const defiPath* obj);
EXTERN int defiPath_getViaBottomMask (const defiPath* obj);
EXTERN int defiPath_getRectMask (const defiPath* obj);

  /* These routines are called by the parser to fill the path.                */

  /* debug printing                                                           */
EXTERN void defiPath_print (const defiPath* obj, FILE*  fout);

                        /* as iterator in const traversal functions.          */

#endif
