/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2013, Cadence Design Systems                              */
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


#ifndef CLEFIMACRO_H
#define CLEFIMACRO_H

#include <stdio.h>
#include "lefiTypedefs.h"

EXTERN const lefiGeometries* lefiObstruction_geometries (const lefiObstruction* obj);

EXTERN void lefiObstruction_print (const lefiObstruction* obj, FILE*  f);

/* 5.5                                                                        */

EXTERN int lefiPinAntennaModel_hasAntennaGateArea (const lefiPinAntennaModel* obj);
EXTERN int lefiPinAntennaModel_hasAntennaMaxAreaCar (const lefiPinAntennaModel* obj);
EXTERN int lefiPinAntennaModel_hasAntennaMaxSideAreaCar (const lefiPinAntennaModel* obj);
EXTERN int lefiPinAntennaModel_hasAntennaMaxCutCar (const lefiPinAntennaModel* obj);

EXTERN char* lefiPinAntennaModel_antennaOxide (const lefiPinAntennaModel* obj);

EXTERN int lefiPinAntennaModel_numAntennaGateArea (const lefiPinAntennaModel* obj);
EXTERN double lefiPinAntennaModel_antennaGateArea (const lefiPinAntennaModel* obj, int  index);
EXTERN const char* lefiPinAntennaModel_antennaGateAreaLayer (const lefiPinAntennaModel* obj, int  index);

EXTERN int lefiPinAntennaModel_numAntennaMaxAreaCar (const lefiPinAntennaModel* obj);
EXTERN double lefiPinAntennaModel_antennaMaxAreaCar (const lefiPinAntennaModel* obj, int  index);
EXTERN const char* lefiPinAntennaModel_antennaMaxAreaCarLayer (const lefiPinAntennaModel* obj, int  index);

EXTERN int lefiPinAntennaModel_numAntennaMaxSideAreaCar (const lefiPinAntennaModel* obj);
EXTERN double lefiPinAntennaModel_antennaMaxSideAreaCar (const lefiPinAntennaModel* obj, int  index);
EXTERN const char* lefiPinAntennaModel_antennaMaxSideAreaCarLayer (const lefiPinAntennaModel* obj, int  index);

EXTERN int lefiPinAntennaModel_numAntennaMaxCutCar (const lefiPinAntennaModel* obj);
EXTERN double lefiPinAntennaModel_antennaMaxCutCar (const lefiPinAntennaModel* obj, int  index);
EXTERN const char* lefiPinAntennaModel_antennaMaxCutCarLayer (const lefiPinAntennaModel* obj, int  index);

EXTERN int lefiPinAntennaModel_hasReturn (const lefiPinAntennaModel* obj);

EXTERN int lefiPin_hasForeign (const lefiPin* obj);
EXTERN int lefiPin_hasForeignOrient (const lefiPin* obj, int  index);
EXTERN int lefiPin_hasForeignPoint (const lefiPin* obj, int  index);
EXTERN int lefiPin_hasLEQ (const lefiPin* obj);
EXTERN int lefiPin_hasDirection (const lefiPin* obj);
EXTERN int lefiPin_hasUse (const lefiPin* obj);
EXTERN int lefiPin_hasShape (const lefiPin* obj);
EXTERN int lefiPin_hasMustjoin (const lefiPin* obj);
EXTERN int lefiPin_hasOutMargin (const lefiPin* obj);
EXTERN int lefiPin_hasOutResistance (const lefiPin* obj);
EXTERN int lefiPin_hasInMargin (const lefiPin* obj);
EXTERN int lefiPin_hasPower (const lefiPin* obj);
EXTERN int lefiPin_hasLeakage (const lefiPin* obj);
EXTERN int lefiPin_hasMaxload (const lefiPin* obj);
EXTERN int lefiPin_hasMaxdelay (const lefiPin* obj);
EXTERN int lefiPin_hasCapacitance (const lefiPin* obj);
EXTERN int lefiPin_hasResistance (const lefiPin* obj);
EXTERN int lefiPin_hasPulldownres (const lefiPin* obj);
EXTERN int lefiPin_hasTieoffr (const lefiPin* obj);
EXTERN int lefiPin_hasVHI (const lefiPin* obj);
EXTERN int lefiPin_hasVLO (const lefiPin* obj);
EXTERN int lefiPin_hasRiseVoltage (const lefiPin* obj);
EXTERN int lefiPin_hasFallVoltage (const lefiPin* obj);
EXTERN int lefiPin_hasRiseThresh (const lefiPin* obj);
EXTERN int lefiPin_hasFallThresh (const lefiPin* obj);
EXTERN int lefiPin_hasRiseSatcur (const lefiPin* obj);
EXTERN int lefiPin_hasFallSatcur (const lefiPin* obj);
EXTERN int lefiPin_hasCurrentSource (const lefiPin* obj);
EXTERN int lefiPin_hasTables (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaSize (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaMetalArea (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaMetalLength (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaPartialMetalArea (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaPartialMetalSideArea (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaPartialCutArea (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaDiffArea (const lefiPin* obj);
EXTERN int lefiPin_hasAntennaModel (const lefiPin* obj);
EXTERN int lefiPin_hasTaperRule (const lefiPin* obj);
EXTERN int lefiPin_hasRiseSlewLimit (const lefiPin* obj);
EXTERN int lefiPin_hasFallSlewLimit (const lefiPin* obj);
EXTERN int lefiPin_hasNetExpr (const lefiPin* obj);
EXTERN int lefiPin_hasSupplySensitivity (const lefiPin* obj);
EXTERN int lefiPin_hasGroundSensitivity (const lefiPin* obj);

EXTERN const char* lefiPin_name (const lefiPin* obj);

EXTERN int lefiPin_numPorts (const lefiPin* obj);
EXTERN const lefiGeometries* lefiPin_port (const lefiPin* obj, int  index);

EXTERN int lefiPin_numForeigns (const lefiPin* obj);
EXTERN const char* lefiPin_foreignName (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_taperRule (const lefiPin* obj);
EXTERN int lefiPin_foreignOrient (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_foreignOrientStr (const lefiPin* obj, int  index);
EXTERN double lefiPin_foreignX (const lefiPin* obj, int  index);
EXTERN double lefiPin_foreignY (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_LEQ (const lefiPin* obj);
EXTERN const char* lefiPin_direction (const lefiPin* obj);
EXTERN const char* lefiPin_use (const lefiPin* obj);
EXTERN const char* lefiPin_shape (const lefiPin* obj);
EXTERN const char* lefiPin_mustjoin (const lefiPin* obj);
EXTERN double lefiPin_outMarginHigh (const lefiPin* obj);
EXTERN double lefiPin_outMarginLow (const lefiPin* obj);
EXTERN double lefiPin_outResistanceHigh (const lefiPin* obj);
EXTERN double lefiPin_outResistanceLow (const lefiPin* obj);
EXTERN double lefiPin_inMarginHigh (const lefiPin* obj);
EXTERN double lefiPin_inMarginLow (const lefiPin* obj);
EXTERN double lefiPin_power (const lefiPin* obj);
EXTERN double lefiPin_leakage (const lefiPin* obj);
EXTERN double lefiPin_maxload (const lefiPin* obj);
EXTERN double lefiPin_maxdelay (const lefiPin* obj);
EXTERN double lefiPin_capacitance (const lefiPin* obj);
EXTERN double lefiPin_resistance (const lefiPin* obj);
EXTERN double lefiPin_pulldownres (const lefiPin* obj);
EXTERN double lefiPin_tieoffr (const lefiPin* obj);
EXTERN double lefiPin_VHI (const lefiPin* obj);
EXTERN double lefiPin_VLO (const lefiPin* obj);
EXTERN double lefiPin_riseVoltage (const lefiPin* obj);
EXTERN double lefiPin_fallVoltage (const lefiPin* obj);
EXTERN double lefiPin_riseThresh (const lefiPin* obj);
EXTERN double lefiPin_fallThresh (const lefiPin* obj);
EXTERN double lefiPin_riseSatcur (const lefiPin* obj);
EXTERN double lefiPin_fallSatcur (const lefiPin* obj);
EXTERN double lefiPin_riseSlewLimit (const lefiPin* obj);
EXTERN double lefiPin_fallSlewLimit (const lefiPin* obj);
EXTERN const char* lefiPin_currentSource (const lefiPin* obj);
EXTERN const char* lefiPin_tableHighName (const lefiPin* obj);
EXTERN const char* lefiPin_tableLowName (const lefiPin* obj);

EXTERN int lefiPin_numAntennaSize (const lefiPin* obj);
EXTERN double lefiPin_antennaSize (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaSizeLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaMetalArea (const lefiPin* obj);
EXTERN double lefiPin_antennaMetalArea (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaMetalAreaLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaMetalLength (const lefiPin* obj);
EXTERN double lefiPin_antennaMetalLength (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaMetalLengthLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaPartialMetalArea (const lefiPin* obj);
EXTERN double lefiPin_antennaPartialMetalArea (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaPartialMetalAreaLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaPartialMetalSideArea (const lefiPin* obj);
EXTERN double lefiPin_antennaPartialMetalSideArea (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaPartialMetalSideAreaLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaPartialCutArea (const lefiPin* obj);
EXTERN double lefiPin_antennaPartialCutArea (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaPartialCutAreaLayer (const lefiPin* obj, int  index);

EXTERN int lefiPin_numAntennaDiffArea (const lefiPin* obj);
EXTERN double lefiPin_antennaDiffArea (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_antennaDiffAreaLayer (const lefiPin* obj, int  index);

  /* 5.6                                                                      */
EXTERN const char* lefiPin_netExpr (const lefiPin* obj);
EXTERN const char* lefiPin_supplySensitivity (const lefiPin* obj);
EXTERN const char* lefiPin_groundSensitivity (const lefiPin* obj);

  /* 5.5                                                                      */
EXTERN int lefiPin_numAntennaModel (const lefiPin* obj);
EXTERN const lefiPinAntennaModel* lefiPin_antennaModel (const lefiPin* obj, int  index);

EXTERN int lefiPin_numProperties (const lefiPin* obj);
EXTERN const char* lefiPin_propName (const lefiPin* obj, int  index);
EXTERN const char* lefiPin_propValue (const lefiPin* obj, int  index);
EXTERN double lefiPin_propNum (const lefiPin* obj, int  index);
EXTERN const char lefiPin_propType (const lefiPin* obj, int  index);
EXTERN int lefiPin_propIsNumber (const lefiPin* obj, int  index);
EXTERN int lefiPin_propIsString (const lefiPin* obj, int  index);

EXTERN void lefiPin_print (const lefiPin* obj, FILE*  f);

  /* 5.5 AntennaModel                                                         */

/* 5.6                                                                        */

EXTERN int lefiDensity_numLayer (const lefiDensity* obj);
EXTERN char* lefiDensity_layerName (const lefiDensity* obj, int  index);
EXTERN int lefiDensity_numRects (const lefiDensity* obj, int  index);
EXTERN struct lefiGeomRect lefiDensity_getRect (const lefiDensity* obj, int  index, int  rectIndex);
EXTERN double lefiDensity_densityValue (const lefiDensity* obj, int  index, int  rectIndex);

EXTERN void lefiDensity_print (const lefiDensity* obj, FILE*  f);

  /* orient=-1 means no orient was specified.                                 */

EXTERN int lefiMacro_hasClass (const lefiMacro* obj);
EXTERN int lefiMacro_hasGenerator (const lefiMacro* obj);
EXTERN int lefiMacro_hasGenerate (const lefiMacro* obj);
EXTERN int lefiMacro_hasPower (const lefiMacro* obj);
EXTERN int lefiMacro_hasOrigin (const lefiMacro* obj);
EXTERN int lefiMacro_hasEEQ (const lefiMacro* obj);
EXTERN int lefiMacro_hasLEQ (const lefiMacro* obj);
EXTERN int lefiMacro_hasSource (const lefiMacro* obj);
EXTERN int lefiMacro_hasXSymmetry (const lefiMacro* obj);
EXTERN int lefiMacro_hasYSymmetry (const lefiMacro* obj);
EXTERN int lefiMacro_has90Symmetry (const lefiMacro* obj);
EXTERN int lefiMacro_hasSiteName (const lefiMacro* obj);
EXTERN int lefiMacro_hasSitePattern (const lefiMacro* obj);
EXTERN int lefiMacro_hasSize (const lefiMacro* obj);
EXTERN int lefiMacro_hasForeign (const lefiMacro* obj);
EXTERN int lefiMacro_hasForeignOrigin (const lefiMacro* obj, int  index);
EXTERN int lefiMacro_hasForeignOrient (const lefiMacro* obj, int  index);
EXTERN int lefiMacro_hasForeignPoint (const lefiMacro* obj, int  index);
EXTERN int lefiMacro_hasClockType (const lefiMacro* obj);
EXTERN int lefiMacro_isBuffer (const lefiMacro* obj);
EXTERN int lefiMacro_isInverter (const lefiMacro* obj);
EXTERN int lefiMacro_isFixedMask (const lefiMacro* obj);

EXTERN int lefiMacro_numSitePattern (const lefiMacro* obj);
EXTERN int lefiMacro_numProperties (const lefiMacro* obj);
EXTERN const char* lefiMacro_propName (const lefiMacro* obj, int  index);
EXTERN const char* lefiMacro_propValue (const lefiMacro* obj, int  index);
EXTERN double lefiMacro_propNum (const lefiMacro* obj, int  index);
EXTERN const char lefiMacro_propType (const lefiMacro* obj, int  index);
EXTERN int lefiMacro_propIsNumber (const lefiMacro* obj, int  index);
EXTERN int lefiMacro_propIsString (const lefiMacro* obj, int  index);

EXTERN const char* lefiMacro_name (const lefiMacro* obj);
EXTERN const char* lefiMacro_macroClass (const lefiMacro* obj);
EXTERN const char* lefiMacro_generator (const lefiMacro* obj);
EXTERN const char* lefiMacro_EEQ (const lefiMacro* obj);
EXTERN const char* lefiMacro_LEQ (const lefiMacro* obj);
EXTERN const char* lefiMacro_source (const lefiMacro* obj);
EXTERN const char* lefiMacro_clockType (const lefiMacro* obj);
EXTERN double lefiMacro_originX (const lefiMacro* obj);
EXTERN double lefiMacro_originY (const lefiMacro* obj);
EXTERN double lefiMacro_power (const lefiMacro* obj);
EXTERN void lefiMacro_generate (const lefiMacro* obj, char**  name1, char**  name2);
EXTERN const lefiSitePattern* lefiMacro_sitePattern (const lefiMacro* obj, int  index);
EXTERN const char* lefiMacro_siteName (const lefiMacro* obj);
EXTERN double lefiMacro_sizeX (const lefiMacro* obj);
EXTERN double lefiMacro_sizeY (const lefiMacro* obj);
EXTERN int lefiMacro_numForeigns (const lefiMacro* obj);
EXTERN int lefiMacro_foreignOrient (const lefiMacro* obj, int  index);
EXTERN const char* lefiMacro_foreignOrientStr (const lefiMacro* obj, int  index);
EXTERN double lefiMacro_foreignX (const lefiMacro* obj, int  index);
EXTERN double lefiMacro_foreignY (const lefiMacro* obj, int  index);
EXTERN const char* lefiMacro_foreignName (const lefiMacro* obj, int  index);

  /* Debug print                                                              */
EXTERN void lefiMacro_print (const lefiMacro* obj, FILE*  f);

  /* addSDF2Pins & addSDF1Pin are for 5.1                                     */

  /* The following are for 5.1                                                */

#endif
