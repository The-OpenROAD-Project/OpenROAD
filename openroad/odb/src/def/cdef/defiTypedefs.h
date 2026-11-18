/* *****************************************************************************
 */
/* *****************************************************************************
 */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT! */
/* *****************************************************************************
 */
/* *****************************************************************************
 */
/* Copyright 2012, Cadence Design Systems */
/*  */
/* This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source */
/* Distribution,  Product Version 5.8.  */
/*  */
/* Licensed under the Apache License, Version 2.0 (the \"License\"); */
/*    you may not use this file except in compliance with the License. */
/*    You may obtain a copy of the License at */
/*  */
/*        http://www.apache.org/licenses/LICENSE-2.0 */
/*  */
/*    Unless required by applicable law or agreed to in writing, software */
/*    distributed under the License is distributed on an \"AS IS\" BASIS, */
/*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or */
/*    implied. See the License for the specific language governing */
/*    permissions and limitations under the License. */
/*  */
/*  */
/* For updates, support, or to become part of the LEF/DEF Community, */
/* check www.openeda.org for details. */
/*  */
/*  $Author: xxx $ */
/*  $Revision: xxx $ */
/*  $Date: xxx $ */
/*  $State: xxx $ */
/* *****************************************************************************
 */
/* *****************************************************************************
 */

#ifndef CLEFITYPEDEFS_H
#define CLEFITYPEDEFS_H

#ifndef EXTERN
#define EXTERN extern
#endif

#define bool int
#define defiUserData void*
#define defiUserDataHandle void**

/* Typedefs */

/* Pointers to C++ classes */
using defiPinPort = void*;
using defiTimingDisable = void*;
using defiPartition = void*;
using defiAssertion = void*;
using defiPinAntennaModel = void*;
using defiIOTiming = void*;
using defiRegion = void*;
using defiSubnet = void*;
using defiTrack = void*;
using defiProp = void*;
using defiRow = void*;
using defiFPC = void*;
using defiShield = void*;
using defiVia = void*;
using defiNonDefault = void*;
using defiBox = void*;
using defiWire = void*;
using defiOrdered = void*;
using defiPropType = void*;
using defiAlias_itr = void*;
using defiScanchain = void*;
using defiComponent = void*;
using defiFill = void*;
using defiSite = void*;
using defiPin = void*;
using defiPinProp = void*;
using defiStyles = void*;
using defiBlockage = void*;
using defiGeometries = void*;
using defiVpin = void*;
using defiNet = void*;
using defiSlot = void*;
using defiGcellGrid = void*;
using defiPath = void*;
using defiGroup = void*;
using defiPinCap = void*;
using defiComponentMaskShiftLayer = void*;
using defrData = void*;

/* Data structures definitions */
struct defiPoints
{
  int numPoints;
  int* x;
  int* y;
};

struct defiPnt
{
  int x;
  int y;
  int ext;
};

struct defiViaData
{
  int numX;
  int numY;
  int stepX;
  int stepY;
};

struct defiViaRect
{
  int deltaX1;
  int deltaY1;
  int deltaX2;
  int deltaY2;
};

#endif
