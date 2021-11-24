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


#ifndef CDEFIPINCAP_H
#define CDEFIPINCAP_H

#include <stdio.h>
#include "defiTypedefs.h"

EXTERN int defiPinCap_pin (const defiPinCap* obj);
EXTERN double defiPinCap_cap (const defiPinCap* obj);

EXTERN void defiPinCap_print (const defiPinCap* obj, FILE*  f);

/* 5.5                                                                        */

EXTERN char* defiPinAntennaModel_antennaOxide (const defiPinAntennaModel* obj);

EXTERN int defiPinAntennaModel_hasAPinGateArea (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_numAPinGateArea (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_APinGateArea (const defiPinAntennaModel* obj, int  index);
EXTERN int defiPinAntennaModel_hasAPinGateAreaLayer (const defiPinAntennaModel* obj, int  index);
EXTERN const char* defiPinAntennaModel_APinGateAreaLayer (const defiPinAntennaModel* obj, int  index);

EXTERN int defiPinAntennaModel_hasAPinMaxAreaCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_numAPinMaxAreaCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_APinMaxAreaCar (const defiPinAntennaModel* obj, int  index);
EXTERN int defiPinAntennaModel_hasAPinMaxAreaCarLayer (const defiPinAntennaModel* obj, int  index);
EXTERN const char* defiPinAntennaModel_APinMaxAreaCarLayer (const defiPinAntennaModel* obj, int  index);

EXTERN int defiPinAntennaModel_hasAPinMaxSideAreaCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_numAPinMaxSideAreaCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_APinMaxSideAreaCar (const defiPinAntennaModel* obj, int  index);
EXTERN int defiPinAntennaModel_hasAPinMaxSideAreaCarLayer (const defiPinAntennaModel* obj, int  index);
EXTERN const char* defiPinAntennaModel_APinMaxSideAreaCarLayer (const defiPinAntennaModel* obj, int  index);

EXTERN int defiPinAntennaModel_hasAPinMaxCutCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_numAPinMaxCutCar (const defiPinAntennaModel* obj);
EXTERN int defiPinAntennaModel_APinMaxCutCar (const defiPinAntennaModel* obj, int  index);
EXTERN int defiPinAntennaModel_hasAPinMaxCutCarLayer (const defiPinAntennaModel* obj, int  index);
EXTERN const char* defiPinAntennaModel_APinMaxCutCarLayer (const defiPinAntennaModel* obj, int  index);

EXTERN int defiPinPort_numLayer (const defiPinPort* obj);
EXTERN const char* defiPinPort_layer (const defiPinPort* obj, int  index);
EXTERN void defiPinPort_bounds (const defiPinPort* obj, int  index, int*  xl, int*  yl, int*  xh, int*  yh);
EXTERN int defiPinPort_hasLayerSpacing (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_hasLayerDesignRuleWidth (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_layerSpacing (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_layerMask (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_layerDesignRuleWidth (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_numPolygons (const defiPinPort* obj);
EXTERN const char* defiPinPort_polygonName (const defiPinPort* obj, int  index);
EXTERN struct defiPoints defiPinPort_getPolygon (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_hasPolygonSpacing (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_hasPolygonDesignRuleWidth (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_polygonSpacing (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_polygonDesignRuleWidth (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_polygonMask (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_numVias (const defiPinPort* obj);
EXTERN const char* defiPinPort_viaName (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_viaPtX (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_viaPtY (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_viaTopMask (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_viaCutMask (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_viaBottomMask (const defiPinPort* obj, int  index);
EXTERN int defiPinPort_hasPlacement (const defiPinPort* obj);
EXTERN int defiPinPort_isPlaced (const defiPinPort* obj);
EXTERN int defiPinPort_isCover (const defiPinPort* obj);
EXTERN int defiPinPort_isFixed (const defiPinPort* obj);
EXTERN int defiPinPort_placementX (const defiPinPort* obj);
EXTERN int defiPinPort_placementY (const defiPinPort* obj);
EXTERN int defiPinPort_orient (const defiPinPort* obj);
EXTERN const char* defiPinPort_orientStr (const defiPinPort* obj);

  /* 5.6 setLayer is changed to addLayer due to multiple LAYER are allowed    */
  /* in 5.6                                                                   */
  /* 5.7 port statements, which may have LAYER, POLYGON, &| VIA               */

EXTERN const char* defiPin_pinName (const defiPin* obj);
EXTERN const char* defiPin_netName (const defiPin* obj);
  /* optional parts                                                           */
EXTERN int defiPin_hasDirection (const defiPin* obj);
EXTERN int defiPin_hasUse (const defiPin* obj);
EXTERN int defiPin_hasLayer (const defiPin* obj);
EXTERN int defiPin_hasPlacement (const defiPin* obj);
EXTERN int defiPin_isUnplaced (const defiPin* obj);
EXTERN int defiPin_isPlaced (const defiPin* obj);
EXTERN int defiPin_isCover (const defiPin* obj);
EXTERN int defiPin_isFixed (const defiPin* obj);
EXTERN int defiPin_placementX (const defiPin* obj);
EXTERN int defiPin_placementY (const defiPin* obj);
EXTERN const char* defiPin_direction (const defiPin* obj);
EXTERN const char* defiPin_use (const defiPin* obj);
EXTERN int defiPin_numLayer (const defiPin* obj);
EXTERN const char* defiPin_layer (const defiPin* obj, int  index);
EXTERN void defiPin_bounds (const defiPin* obj, int  index, int*  xl, int*  yl, int*  xh, int*  yh);
EXTERN int defiPin_layerMask (const defiPin* obj, int  index);
EXTERN int defiPin_hasLayerSpacing (const defiPin* obj, int  index);
EXTERN int defiPin_hasLayerDesignRuleWidth (const defiPin* obj, int  index);
EXTERN int defiPin_layerSpacing (const defiPin* obj, int  index);
EXTERN int defiPin_layerDesignRuleWidth (const defiPin* obj, int  index);
EXTERN int defiPin_numPolygons (const defiPin* obj);
EXTERN const char* defiPin_polygonName (const defiPin* obj, int  index);
EXTERN struct defiPoints defiPin_getPolygon (const defiPin* obj, int  index);
EXTERN int defiPin_polygonMask (const defiPin* obj, int  index);
EXTERN int defiPin_hasPolygonSpacing (const defiPin* obj, int  index);
EXTERN int defiPin_hasPolygonDesignRuleWidth (const defiPin* obj, int  index);
EXTERN int defiPin_polygonSpacing (const defiPin* obj, int  index);
EXTERN int defiPin_polygonDesignRuleWidth (const defiPin* obj, int  index);
EXTERN int defiPin_hasNetExpr (const defiPin* obj);
EXTERN int defiPin_hasSupplySensitivity (const defiPin* obj);
EXTERN int defiPin_hasGroundSensitivity (const defiPin* obj);
EXTERN const char* defiPin_netExpr (const defiPin* obj);
EXTERN const char* defiPin_supplySensitivity (const defiPin* obj);
EXTERN const char* defiPin_groundSensitivity (const defiPin* obj);
EXTERN int defiPin_orient (const defiPin* obj);
EXTERN const char* defiPin_orientStr (const defiPin* obj);
EXTERN int defiPin_hasSpecial (const defiPin* obj);
EXTERN int defiPin_numVias (const defiPin* obj);
EXTERN const char* defiPin_viaName (const defiPin* obj, int  index);
EXTERN int defiPin_viaTopMask (const defiPin* obj, int  index);
EXTERN int defiPin_viaCutMask (const defiPin* obj, int  index);
EXTERN int defiPin_viaBottomMask (const defiPin* obj, int  index);
EXTERN int defiPin_viaPtX (const defiPin* obj, int  index);
EXTERN int defiPin_viaPtY (const defiPin* obj, int  index);

  /* 5.4                                                                      */
EXTERN int defiPin_hasAPinPartialMetalArea (const defiPin* obj);
EXTERN int defiPin_numAPinPartialMetalArea (const defiPin* obj);
EXTERN int defiPin_APinPartialMetalArea (const defiPin* obj, int  index);
EXTERN int defiPin_hasAPinPartialMetalAreaLayer (const defiPin* obj, int  index);
EXTERN const char* defiPin_APinPartialMetalAreaLayer (const defiPin* obj, int  index);

EXTERN int defiPin_hasAPinPartialMetalSideArea (const defiPin* obj);
EXTERN int defiPin_numAPinPartialMetalSideArea (const defiPin* obj);
EXTERN int defiPin_APinPartialMetalSideArea (const defiPin* obj, int  index);
EXTERN int defiPin_hasAPinPartialMetalSideAreaLayer (const defiPin* obj, int  index);
EXTERN const char* defiPin_APinPartialMetalSideAreaLayer (const defiPin* obj, int  index);

EXTERN int defiPin_hasAPinDiffArea (const defiPin* obj);
EXTERN int defiPin_numAPinDiffArea (const defiPin* obj);
EXTERN int defiPin_APinDiffArea (const defiPin* obj, int  index);
EXTERN int defiPin_hasAPinDiffAreaLayer (const defiPin* obj, int  index);
EXTERN const char* defiPin_APinDiffAreaLayer (const defiPin* obj, int  index);

EXTERN int defiPin_hasAPinPartialCutArea (const defiPin* obj);
EXTERN int defiPin_numAPinPartialCutArea (const defiPin* obj);
EXTERN int defiPin_APinPartialCutArea (const defiPin* obj, int  index);
EXTERN int defiPin_hasAPinPartialCutAreaLayer (const defiPin* obj, int  index);
EXTERN const char* defiPin_APinPartialCutAreaLayer (const defiPin* obj, int  index);

  /* 5.5                                                                      */
EXTERN int defiPin_numAntennaModel (const defiPin* obj);
EXTERN const defiPinAntennaModel* defiPin_antennaModel (const defiPin* obj, int  index);

  /* 5.7                                                                      */
EXTERN int defiPin_hasPort (const defiPin* obj);
EXTERN int defiPin_numPorts (const defiPin* obj);
EXTERN const defiPinPort* defiPin_pinPort (const defiPin* obj, int  index);
EXTERN void defiPin_print (const defiPin* obj, FILE*  f);

  /* 5.5 AntennaModel                                                         */

#endif
