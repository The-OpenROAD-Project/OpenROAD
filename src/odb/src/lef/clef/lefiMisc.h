/* ************************************************************************** */
/* ************************************************************************** */
/* ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               */
/* ************************************************************************** */
/* ************************************************************************** */
/* Copyright 2012 - 2014, Cadence Design Systems                              */
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


#ifndef CLEFIMISC_H
#define CLEFIMISC_H

#include <stdio.h>
#include "lefiTypedefs.h"

/* The different types of items in a geometry list.                           */

enum lefiGeomEnum {
  lefiGeomUnknown = 0,
  lefiGeomLayerE = 1,
  lefiGeomLayerExceptPgNetE = 2,
  lefiGeomLayerMinSpacingE = 3,
  lefiGeomLayerRuleWidthE = 4,
  lefiGeomWidthE = 5,
  lefiGeomPathE = 6,
  lefiGeomPathIterE = 7,
  lefiGeomRectE = 8,
  lefiGeomRectIterE = 9,
  lefiGeomPolygonE = 10,
  lefiGeomPolygonIterE = 11,
  lefiGeomViaE = 12,
  lefiGeomViaIterE = 13,
  lefiGeomClassE = 14,
  lefiGeomEnd = 15
};

/*  pcr 481783 & 560504
*/

EXTERN int lefiGeometries_numItems (const lefiGeometries* obj);
EXTERN enum lefiGeomEnum lefiGeometries_itemType (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomRect* lefiGeometries_getRect (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomRectIter* lefiGeometries_getRectIter (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomPath* lefiGeometries_getPath (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomPathIter* lefiGeometries_getPathIter (const lefiGeometries* obj, int  index);
EXTERN int lefiGeometries_hasLayerExceptPgNet (const lefiGeometries* obj, int  index);
EXTERN char* lefiGeometries_getLayer (const lefiGeometries* obj, int  index);
EXTERN double lefiGeometries_getLayerMinSpacing (const lefiGeometries* obj, int  index);
EXTERN double lefiGeometries_getLayerRuleWidth (const lefiGeometries* obj, int  index);
EXTERN double lefiGeometries_getWidth (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomPolygon* lefiGeometries_getPolygon (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomPolygonIter* lefiGeometries_getPolygonIter (const lefiGeometries* obj, int  index);
EXTERN char* lefiGeometries_getClass (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomVia* lefiGeometries_getVia (const lefiGeometries* obj, int  index);
EXTERN struct lefiGeomViaIter* lefiGeometries_getViaIter (const lefiGeometries* obj, int  index);

EXTERN void lefiGeometries_print (const lefiGeometries* obj, FILE*  f);

EXTERN int lefiSpacing_hasStack (const lefiSpacing* obj);

EXTERN const char* lefiSpacing_name1 (const lefiSpacing* obj);
EXTERN const char* lefiSpacing_name2 (const lefiSpacing* obj);
EXTERN double lefiSpacing_distance (const lefiSpacing* obj);

  /* Debug print                                                              */
EXTERN void lefiSpacing_print (const lefiSpacing* obj, FILE*  f);

EXTERN const char* lefiIRDrop_name (const lefiIRDrop* obj);
EXTERN double lefiIRDrop_value1 (const lefiIRDrop* obj, int  index);
EXTERN double lefiIRDrop_value2 (const lefiIRDrop* obj, int  index);

EXTERN int lefiIRDrop_numValues (const lefiIRDrop* obj);

  /* Debug print                                                              */
EXTERN void lefiIRDrop_print (const lefiIRDrop* obj, FILE*  f);

EXTERN double lefiMinFeature_one (const lefiMinFeature* obj);
EXTERN double lefiMinFeature_two (const lefiMinFeature* obj);

  /* Debug print                                                              */
EXTERN void lefiMinFeature_print (const lefiMinFeature* obj, FILE*  f);

EXTERN const char* lefiSite_name (const lefiSite* obj);
EXTERN int lefiSite_hasClass (const lefiSite* obj);
EXTERN const char* lefiSite_siteClass (const lefiSite* obj);
EXTERN double lefiSite_sizeX (const lefiSite* obj);
EXTERN double lefiSite_sizeY (const lefiSite* obj);
EXTERN int lefiSite_hasSize (const lefiSite* obj);
EXTERN int lefiSite_hasXSymmetry (const lefiSite* obj);
EXTERN int lefiSite_hasYSymmetry (const lefiSite* obj);
EXTERN int lefiSite_has90Symmetry (const lefiSite* obj);
EXTERN int lefiSite_hasRowPattern (const lefiSite* obj);
EXTERN int lefiSite_numSites (const lefiSite* obj);
EXTERN char* lefiSite_siteName (const lefiSite* obj, int  index);
EXTERN int lefiSite_siteOrient (const lefiSite* obj, int  index);
EXTERN char* lefiSite_siteOrientStr (const lefiSite* obj, int  index);

  /* Debug print                                                              */
EXTERN void lefiSite_print (const lefiSite* obj, FILE*  f);

EXTERN const char* lefiSitePattern_name (const lefiSitePattern* obj);
EXTERN int lefiSitePattern_orient (const lefiSitePattern* obj);
EXTERN const char* lefiSitePattern_orientStr (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_x (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_y (const lefiSitePattern* obj);
EXTERN int lefiSitePattern_hasStepPattern (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_xStart (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_yStart (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_xStep (const lefiSitePattern* obj);
EXTERN double lefiSitePattern_yStep (const lefiSitePattern* obj);

  /* Debug print                                                              */
EXTERN void lefiSitePattern_print (const lefiSitePattern* obj, FILE*  f);

EXTERN const char* lefiTrackPattern_name (const lefiTrackPattern* obj);
EXTERN double lefiTrackPattern_start (const lefiTrackPattern* obj);
EXTERN int lefiTrackPattern_numTracks (const lefiTrackPattern* obj);
EXTERN double lefiTrackPattern_space (const lefiTrackPattern* obj);

EXTERN int lefiTrackPattern_numLayers (const lefiTrackPattern* obj);
EXTERN const char* lefiTrackPattern_layerName (const lefiTrackPattern* obj, int  index);

  /* Debug print                                                              */
EXTERN void lefiTrackPattern_print (const lefiTrackPattern* obj, FILE*  f);

EXTERN const char* lefiGcellPattern_name (const lefiGcellPattern* obj);
EXTERN double lefiGcellPattern_start (const lefiGcellPattern* obj);
EXTERN int lefiGcellPattern_numCRs (const lefiGcellPattern* obj);
EXTERN double lefiGcellPattern_space (const lefiGcellPattern* obj);

  /* Debug print                                                              */
EXTERN void lefiGcellPattern_print (const lefiGcellPattern* obj, FILE*  f);

EXTERN const char* lefiUseMinSpacing_name (const lefiUseMinSpacing* obj);
EXTERN int lefiUseMinSpacing_value (const lefiUseMinSpacing* obj);

  /* Debug print                                                              */
EXTERN void lefiUseMinSpacing_print (const lefiUseMinSpacing* obj, FILE*  f);

/* 5.5 for Maximum Stacked-via rule                                           */

EXTERN int lefiMaxStackVia_maxStackVia (const lefiMaxStackVia* obj);
EXTERN int lefiMaxStackVia_hasMaxStackViaRange (const lefiMaxStackVia* obj);
EXTERN const char* lefiMaxStackVia_maxStackViaBottomLayer (const lefiMaxStackVia* obj);
EXTERN const char* lefiMaxStackVia_maxStackViaTopLayer (const lefiMaxStackVia* obj);

  /* Debug print                                                              */
EXTERN void lefiMaxStackVia_print (const lefiMaxStackVia* obj, FILE*  f);

#endif
