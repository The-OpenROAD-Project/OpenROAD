 /* ***************************************************************************** */
 /* ***************************************************************************** */
 /* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT! */
 /* ***************************************************************************** */
 /* ***************************************************************************** */
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
 /* ***************************************************************************** */
 /* ***************************************************************************** */

#ifndef CLEFITYPEDEFS_H
#define CLEFITYPEDEFS_H

#ifndef EXTERN
#define EXTERN extern
#endif

#define bool int
#define defiUserData void *
#define defiUserDataHandle void **

/* Typedefs */

/* Pointers to C++ classes */
typedef void *defiPinPort;
typedef void *defiTimingDisable;
typedef void *defiPartition;
typedef void *defiAssertion;
typedef void *defiPinAntennaModel;
typedef void *defiIOTiming;
typedef void *defiRegion;
typedef void *defiSubnet;
typedef void *defiTrack;
typedef void *defiProp;
typedef void *defiRow;
typedef void *defiFPC;
typedef void *defiShield;
typedef void *defiVia;
typedef void *defiNonDefault;
typedef void *defiBox;
typedef void *defiWire;
typedef void *defiOrdered;
typedef void *defiPropType;
typedef void *defiAlias_itr;
typedef void *defiScanchain;
typedef void *defiComponent;
typedef void *defiFill;
typedef void *defiSite;
typedef void *defiPin;
typedef void *defiPinProp;
typedef void *defiStyles;
typedef void *defiBlockage;
typedef void *defiGeometries;
typedef void *defiVpin;
typedef void *defiNet;
typedef void *defiSlot;
typedef void *defiGcellGrid;
typedef void *defiPath;
typedef void *defiGroup;
typedef void *defiPinCap;
typedef void *defiComponentMaskShiftLayer;
typedef void *defrData;

/* Data structures definitions */
struct defiPoints {
  int numPoints;
  int* x;
  int* y;
};

struct defiPnt {
  int x;
  int y;
  int ext;
};

struct defiViaData {
  int numX;
  int numY;
  int stepX;
  int stepY;
};

struct defiViaRect {
  int deltaX1;
  int deltaY1;
  int deltaX2;
  int deltaY2;
};


#endif
