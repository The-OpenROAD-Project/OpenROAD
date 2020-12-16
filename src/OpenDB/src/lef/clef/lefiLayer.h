/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2015, Cadence Design Systems                              */
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


#ifndef CLEFILAYER_H
#define CLEFILAYER_H

#include <stdio.h>
#include "lefiTypedefs.h"

typedef enum lefiAntennaEnum {
  lefiAntennaAR = 1,
  lefiAntennaDAR = 2,
  lefiAntennaCAR = 3,
  lefiAntennaCDAR = 4,
  lefiAntennaAF = 5,
  lefiAntennaSAR = 6,
  lefiAntennaDSAR = 7,
  lefiAntennaCSAR = 8,
  lefiAntennaCDSAR = 9,
  lefiAntennaSAF = 10,
  lefiAntennaO = 11,
  lefiAntennaADR = 12
} lefiAntennaEnum;

EXTERN int lefiAntennaPWL_numPWL (const lefiAntennaPWL* obj);

EXTERN char* lefiLayerDensity_type (const lefiLayerDensity* obj);
EXTERN int lefiLayerDensity_hasOneEntry (const lefiLayerDensity* obj);
EXTERN double lefiLayerDensity_oneEntry (const lefiLayerDensity* obj);
EXTERN int lefiLayerDensity_numFrequency (const lefiLayerDensity* obj);
EXTERN double lefiLayerDensity_frequency (const lefiLayerDensity* obj, int  index);
EXTERN int lefiLayerDensity_numWidths (const lefiLayerDensity* obj);
EXTERN double lefiLayerDensity_width (const lefiLayerDensity* obj, int  index);
EXTERN int lefiLayerDensity_numTableEntries (const lefiLayerDensity* obj);
EXTERN double lefiLayerDensity_tableEntry (const lefiLayerDensity* obj, int  index);
EXTERN int lefiLayerDensity_numCutareas (const lefiLayerDensity* obj);
EXTERN double lefiLayerDensity_cutArea (const lefiLayerDensity* obj, int  index);

/* 5.5                                                                        */

EXTERN int lefiParallel_numLength (const lefiParallel* obj);
EXTERN int lefiParallel_numWidth (const lefiParallel* obj);
EXTERN double lefiParallel_length (const lefiParallel* obj, int  iLength);
EXTERN double lefiParallel_width (const lefiParallel* obj, int  iWidth);
EXTERN double lefiParallel_widthSpacing (const lefiParallel* obj, int  iWidth, int  iWidthSpacing);

/* 5.5                                                                        */

EXTERN int lefiInfluence_numInfluenceEntry (const lefiInfluence* obj);
EXTERN double lefiInfluence_width (const lefiInfluence* obj, int  index);
EXTERN double lefiInfluence_distance (const lefiInfluence* obj, int  index);
EXTERN double lefiInfluence_spacing (const lefiInfluence* obj, int  index);

/* 5.7                                                                        */

EXTERN int lefiTwoWidths_numWidth (const lefiTwoWidths* obj);
EXTERN double lefiTwoWidths_width (const lefiTwoWidths* obj, int  iWidth);
EXTERN int lefiTwoWidths_hasWidthPRL (const lefiTwoWidths* obj, int  iWidth);
EXTERN double lefiTwoWidths_widthPRL (const lefiTwoWidths* obj, int  iWidth);
EXTERN int lefiTwoWidths_numWidthSpacing (const lefiTwoWidths* obj, int  iWidth);
EXTERN double lefiTwoWidths_widthSpacing (const lefiTwoWidths* obj, int  iWidth, int  iWidthSpacing);

/* 5.5                                                                        */

EXTERN int lefiSpacingTable_isInfluence (const lefiSpacingTable* obj);
EXTERN const lefiInfluence* lefiSpacingTable_influence (const lefiSpacingTable* obj);
EXTERN int lefiSpacingTable_isParallel (const lefiSpacingTable* obj);
EXTERN const lefiParallel* lefiSpacingTable_parallel (const lefiSpacingTable* obj);
EXTERN const lefiTwoWidths* lefiSpacingTable_twoWidths (const lefiSpacingTable* obj);

/* 5.7                                                                        */

EXTERN int lefiOrthogonal_numOrthogonal (const lefiOrthogonal* obj);
EXTERN double lefiOrthogonal_cutWithin (const lefiOrthogonal* obj, int  index);
EXTERN double lefiOrthogonal_orthoSpacing (const lefiOrthogonal* obj, int  index);

/* 5.5                                                                        */

EXTERN int lefiAntennaModel_hasAntennaAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaDiffAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaDiffAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumDiffAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumDiffAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaAreaFactor (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaAreaFactorDUO (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaSideAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaDiffSideAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaDiffSideAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumSideAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumDiffSideAreaRatio (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumDiffSideAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaSideAreaFactor (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaSideAreaFactorDUO (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaCumRoutingPlusCut (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaGatePlusDiff (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaAreaMinusDiff (const lefiAntennaModel* obj);
EXTERN int lefiAntennaModel_hasAntennaAreaDiffReducePWL (const lefiAntennaModel* obj);

EXTERN char* lefiAntennaModel_antennaOxide (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaAreaRatio (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaDiffAreaRatio (const lefiAntennaModel* obj);
EXTERN const lefiAntennaPWL* lefiAntennaModel_antennaDiffAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaCumAreaRatio (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaCumDiffAreaRatio (const lefiAntennaModel* obj);
EXTERN const lefiAntennaPWL* lefiAntennaModel_antennaCumDiffAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaAreaFactor (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaSideAreaRatio (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaDiffSideAreaRatio (const lefiAntennaModel* obj);
EXTERN const lefiAntennaPWL* lefiAntennaModel_antennaDiffSideAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaCumSideAreaRatio (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaCumDiffSideAreaRatio (const lefiAntennaModel* obj);
EXTERN const lefiAntennaPWL* lefiAntennaModel_antennaCumDiffSideAreaRatioPWL (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaSideAreaFactor (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaGatePlusDiff (const lefiAntennaModel* obj);
EXTERN double lefiAntennaModel_antennaAreaMinusDiff (const lefiAntennaModel* obj);
EXTERN const lefiAntennaPWL* lefiAntennaModel_antennaAreaDiffReducePWL (const lefiAntennaModel* obj);

  /* 5.6 - minstep switch to multiple and added more options                  */

  /* 5.5 SPACINGTABLE                                                         */

  /* 5.6                                                                      */

  /* 5.8                                                                      */
  /* POLYROUTING, MIMCAP, TSV, PASSIVATION, NWELL                             */

EXTERN int lefiLayer_hasType (const lefiLayer* obj);
EXTERN int lefiLayer_hasLayerType (const lefiLayer* obj);
                                     /*  ROUTING can be POLYROUTING or MIMCAP */
                                     /*  CUT can be TSV or PASSIVATION        */
                                     /*  MASTERSLICE can be NWELL             */
EXTERN int lefiLayer_hasMask (const lefiLayer* obj);
EXTERN int lefiLayer_hasPitch (const lefiLayer* obj);
EXTERN int lefiLayer_hasXYPitch (const lefiLayer* obj);
EXTERN int lefiLayer_hasOffset (const lefiLayer* obj);
EXTERN int lefiLayer_hasXYOffset (const lefiLayer* obj);
EXTERN int lefiLayer_hasWidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasArea (const lefiLayer* obj);
EXTERN int lefiLayer_hasDiagPitch (const lefiLayer* obj);
EXTERN int lefiLayer_hasXYDiagPitch (const lefiLayer* obj);
EXTERN int lefiLayer_hasDiagWidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasDiagSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasSpacingNumber (const lefiLayer* obj);
EXTERN int lefiLayer_hasSpacingName (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingLayerStack (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingAdjacent (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingCenterToCenter (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingRange (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingRangeUseLengthThreshold (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingRangeInfluence (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingRangeInfluenceRange (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingRangeRange (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingLengthThreshold (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingLengthThresholdRange (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingParallelOverlap (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingArea (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingEndOfLine (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingParellelEdge (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingTwoEdges (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingAdjacentExcept (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingSamenet (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingSamenetPGonly (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingNotchLength (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingEndOfNotchWidth (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasDirection (const lefiLayer* obj);
EXTERN int lefiLayer_hasResistance (const lefiLayer* obj);
EXTERN int lefiLayer_hasResistanceArray (const lefiLayer* obj);
EXTERN int lefiLayer_hasCapacitance (const lefiLayer* obj);
EXTERN int lefiLayer_hasCapacitanceArray (const lefiLayer* obj);
EXTERN int lefiLayer_hasHeight (const lefiLayer* obj);
EXTERN int lefiLayer_hasThickness (const lefiLayer* obj);
EXTERN int lefiLayer_hasWireExtension (const lefiLayer* obj);
EXTERN int lefiLayer_hasShrinkage (const lefiLayer* obj);
EXTERN int lefiLayer_hasCapMultiplier (const lefiLayer* obj);
EXTERN int lefiLayer_hasEdgeCap (const lefiLayer* obj);
EXTERN int lefiLayer_hasAntennaLength (const lefiLayer* obj);
EXTERN int lefiLayer_hasAntennaArea (const lefiLayer* obj);
EXTERN int lefiLayer_hasCurrentDensityPoint (const lefiLayer* obj);
EXTERN int lefiLayer_hasCurrentDensityArray (const lefiLayer* obj);
EXTERN int lefiLayer_hasAccurrentDensity (const lefiLayer* obj);
EXTERN int lefiLayer_hasDccurrentDensity (const lefiLayer* obj);

EXTERN int lefiLayer_numProps (const lefiLayer* obj);
EXTERN const char* lefiLayer_propName (const lefiLayer* obj, int  index);
EXTERN const char* lefiLayer_propValue (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_propNumber (const lefiLayer* obj, int  index);
EXTERN const char lefiLayer_propType (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_propIsNumber (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_propIsString (const lefiLayer* obj, int  index);

EXTERN int lefiLayer_numSpacing (const lefiLayer* obj);

EXTERN char* lefiLayer_name (const lefiLayer* obj);
EXTERN const char* lefiLayer_type (const lefiLayer* obj);
EXTERN const char* lefiLayer_layerType (const lefiLayer* obj);
EXTERN double lefiLayer_pitch (const lefiLayer* obj);
EXTERN int lefiLayer_mask (const lefiLayer* obj);
EXTERN double lefiLayer_pitchX (const lefiLayer* obj);
EXTERN double lefiLayer_pitchY (const lefiLayer* obj);
EXTERN double lefiLayer_offset (const lefiLayer* obj);
EXTERN double lefiLayer_offsetX (const lefiLayer* obj);
EXTERN double lefiLayer_offsetY (const lefiLayer* obj);
EXTERN double lefiLayer_width (const lefiLayer* obj);
EXTERN double lefiLayer_area (const lefiLayer* obj);
EXTERN double lefiLayer_diagPitch (const lefiLayer* obj);
EXTERN double lefiLayer_diagPitchX (const lefiLayer* obj);
EXTERN double lefiLayer_diagPitchY (const lefiLayer* obj);
EXTERN double lefiLayer_diagWidth (const lefiLayer* obj);
EXTERN double lefiLayer_diagSpacing (const lefiLayer* obj);
EXTERN double lefiLayer_spacing (const lefiLayer* obj, int  index);
EXTERN char* lefiLayer_spacingName (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_spacingAdjacentCuts (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingAdjacentWithin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingArea (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeMin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeMax (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeInfluence (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeInfluenceMin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeInfluenceMax (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeRangeMin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingRangeRangeMax (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingLengthThreshold (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingLengthThresholdRangeMin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingLengthThresholdRangeMax (const lefiLayer* obj, int  index);

  /* 5.7 Spacing endofline                                                    */
EXTERN double lefiLayer_spacingEolWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingEolWithin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingParSpace (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingParWithin (const lefiLayer* obj, int  index);

  /* 5.7 Spacing Notch                                                        */
EXTERN double lefiLayer_spacingNotchLength (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingEndOfNotchWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingEndOfNotchSpacing (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_spacingEndOfNotchLength (const lefiLayer* obj, int  index);

  /* 5.5 Minimum cut rules                                                    */
EXTERN int lefiLayer_numMinimumcut (const lefiLayer* obj);
EXTERN int lefiLayer_minimumcut (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minimumcutWidth (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinimumcutWithin (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minimumcutWithin (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinimumcutConnection (const lefiLayer* obj, int  index);
EXTERN const char* lefiLayer_minimumcutConnection (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinimumcutNumCuts (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minimumcutLength (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minimumcutDistance (const lefiLayer* obj, int  index);

EXTERN const char* lefiLayer_direction (const lefiLayer* obj);
EXTERN double lefiLayer_resistance (const lefiLayer* obj);
EXTERN double lefiLayer_capacitance (const lefiLayer* obj);
EXTERN double lefiLayer_height (const lefiLayer* obj);
EXTERN double lefiLayer_wireExtension (const lefiLayer* obj);
EXTERN double lefiLayer_thickness (const lefiLayer* obj);
EXTERN double lefiLayer_shrinkage (const lefiLayer* obj);
EXTERN double lefiLayer_capMultiplier (const lefiLayer* obj);
EXTERN double lefiLayer_edgeCap (const lefiLayer* obj);
EXTERN double lefiLayer_antennaLength (const lefiLayer* obj);
EXTERN double lefiLayer_antennaArea (const lefiLayer* obj);
EXTERN double lefiLayer_currentDensityPoint (const lefiLayer* obj);
EXTERN void lefiLayer_currentDensityArray (const lefiLayer* obj, int*  numPoints, double**  widths, double**  current);
EXTERN void lefiLayer_capacitanceArray (const lefiLayer* obj, int*  numPoints, double**  widths, double**  resValues);
EXTERN void lefiLayer_resistanceArray (const lefiLayer* obj, int*  numPoints, double**  widths, double**  capValues);

EXTERN int lefiLayer_numAccurrentDensity (const lefiLayer* obj);
EXTERN const lefiLayerDensity* lefiLayer_accurrent (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_numDccurrentDensity (const lefiLayer* obj);
EXTERN const lefiLayerDensity* lefiLayer_dccurrent (const lefiLayer* obj, int  index);

  /* 3/23/2000 - Wanda da Rosa.  The following are for 5.4 Antenna.           */
  /*             Only 5.4 or 5.3 are allowed in a lef file, but not both      */

  /* 5.5                                                                      */
EXTERN int lefiLayer_numAntennaModel (const lefiLayer* obj);
EXTERN const lefiAntennaModel* lefiLayer_antennaModel (const lefiLayer* obj, int  index);

  /* The following is 8/21/01 5.4 enhancements                                */

EXTERN int lefiLayer_hasSlotWireWidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasSlotWireLength (const lefiLayer* obj);
EXTERN int lefiLayer_hasSlotWidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasSlotLength (const lefiLayer* obj);
EXTERN int lefiLayer_hasMaxAdjacentSlotSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasMaxCoaxialSlotSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasMaxEdgeSlotSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasSplitWireWidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasMinimumDensity (const lefiLayer* obj);
EXTERN int lefiLayer_hasMaximumDensity (const lefiLayer* obj);
EXTERN int lefiLayer_hasDensityCheckWindow (const lefiLayer* obj);
EXTERN int lefiLayer_hasDensityCheckStep (const lefiLayer* obj);
EXTERN int lefiLayer_hasFillActiveSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasMaxwidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasMinwidth (const lefiLayer* obj);
EXTERN int lefiLayer_hasMinstep (const lefiLayer* obj);
EXTERN int lefiLayer_hasProtrusion (const lefiLayer* obj);

EXTERN double lefiLayer_slotWireWidth (const lefiLayer* obj);
EXTERN double lefiLayer_slotWireLength (const lefiLayer* obj);
EXTERN double lefiLayer_slotWidth (const lefiLayer* obj);
EXTERN double lefiLayer_slotLength (const lefiLayer* obj);
EXTERN double lefiLayer_maxAdjacentSlotSpacing (const lefiLayer* obj);
EXTERN double lefiLayer_maxCoaxialSlotSpacing (const lefiLayer* obj);
EXTERN double lefiLayer_maxEdgeSlotSpacing (const lefiLayer* obj);
EXTERN double lefiLayer_splitWireWidth (const lefiLayer* obj);
EXTERN double lefiLayer_minimumDensity (const lefiLayer* obj);
EXTERN double lefiLayer_maximumDensity (const lefiLayer* obj);
EXTERN double lefiLayer_densityCheckWindowLength (const lefiLayer* obj);
EXTERN double lefiLayer_densityCheckWindowWidth (const lefiLayer* obj);
EXTERN double lefiLayer_densityCheckStep (const lefiLayer* obj);
EXTERN double lefiLayer_fillActiveSpacing (const lefiLayer* obj);
EXTERN double lefiLayer_maxwidth (const lefiLayer* obj);
EXTERN double lefiLayer_minwidth (const lefiLayer* obj);
EXTERN double lefiLayer_protrusionWidth1 (const lefiLayer* obj);
EXTERN double lefiLayer_protrusionLength (const lefiLayer* obj);
EXTERN double lefiLayer_protrusionWidth2 (const lefiLayer* obj);

EXTERN int lefiLayer_numMinstep (const lefiLayer* obj);
EXTERN double lefiLayer_minstep (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepType (const lefiLayer* obj, int  index);
EXTERN char* lefiLayer_minstepType (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepLengthsum (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minstepLengthsum (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepMaxedges (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_minstepMaxedges (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepMinAdjLength (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minstepMinAdjLength (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepMinBetLength (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minstepMinBetLength (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinstepXSameCorners (const lefiLayer* obj, int  index);

  /* 5.5 MINENCLOSEDAREA                                                      */
EXTERN int lefiLayer_numMinenclosedarea (const lefiLayer* obj);
EXTERN double lefiLayer_minenclosedarea (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasMinenclosedareaWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minenclosedareaWidth (const lefiLayer* obj, int  index);

  /* 5.5 SPACINGTABLE FOR LAYER ROUTING                                       */

  /* 5.6 ENCLOSURE, PREFERENCLOSURE, RESISTANCEPERCUT & DIAGMINEDGELENGTH     */
EXTERN int lefiLayer_numEnclosure (const lefiLayer* obj);
EXTERN int lefiLayer_hasEnclosureRule (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_enclosureOverhang1 (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_enclosureOverhang2 (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasEnclosureWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_enclosureMinWidth (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasEnclosureExceptExtraCut (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_enclosureExceptExtraCut (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasEnclosureMinLength (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_enclosureMinLength (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_numPreferEnclosure (const lefiLayer* obj);
EXTERN int lefiLayer_hasPreferEnclosureRule (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_preferEnclosureOverhang1 (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_preferEnclosureOverhang2 (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasPreferEnclosureWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_preferEnclosureMinWidth (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasResistancePerCut (const lefiLayer* obj);
EXTERN double lefiLayer_resistancePerCut (const lefiLayer* obj);
EXTERN int lefiLayer_hasDiagMinEdgeLength (const lefiLayer* obj);
EXTERN double lefiLayer_diagMinEdgeLength (const lefiLayer* obj);
EXTERN int lefiLayer_numMinSize (const lefiLayer* obj);
EXTERN double lefiLayer_minSizeWidth (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_minSizeLength (const lefiLayer* obj, int  index);

  /* 5.7                                                                      */
EXTERN int lefiLayer_hasMaxFloatingArea (const lefiLayer* obj);
EXTERN double lefiLayer_maxFloatingArea (const lefiLayer* obj);
EXTERN int lefiLayer_hasArraySpacing (const lefiLayer* obj);
EXTERN int lefiLayer_hasLongArray (const lefiLayer* obj);
EXTERN int lefiLayer_hasViaWidth (const lefiLayer* obj);
EXTERN double lefiLayer_viaWidth (const lefiLayer* obj);
EXTERN double lefiLayer_cutSpacing (const lefiLayer* obj);
EXTERN int lefiLayer_numArrayCuts (const lefiLayer* obj);
EXTERN int lefiLayer_arrayCuts (const lefiLayer* obj, int  index);
EXTERN double lefiLayer_arraySpacing (const lefiLayer* obj, int  index);
EXTERN int lefiLayer_hasSpacingTableOrtho (const lefiLayer* obj);
EXTERN const lefiOrthogonal* lefiLayer_orthogonal (const lefiLayer* obj);

EXTERN double lefiLayer_lef58WidthTableOrtho (const lefiLayer* obj, int  idx);
EXTERN int lefiLayer_lef58WidthTableOrthoValues (const lefiLayer* obj);
EXTERN double lefiLayer_lef58WidthTableWrongDir (const lefiLayer* obj, int  idx);
EXTERN int lefiLayer_lef58WidthTableWrongDirValues (const lefiLayer* obj);

EXTERN int lefiLayer_need58PropsProcessing (const lefiLayer* obj);

  /* Debug print                                                              */
EXTERN void lefiLayer_print (const lefiLayer* obj, FILE*  f);

  /* 5.5                                                                      */

                                      /* Q: quotedstring                      */

  /* 3/23/2000 - Wanda da Rosa.  The following is for 5.4 ANTENNA.            */
  /*             Either 5.4 or 5.3 are allowed, not both                      */

  /* 5.5 AntennaModel                                                         */

  /* 8/29/2001 - Wanda da Rosa.  The following is for 5.4 enhancements.       */

  /* 5.5 SPACINGTABLE                                                         */

  /* 5.6                                                                      */

  /* 5.7                                                                      */

#endif
