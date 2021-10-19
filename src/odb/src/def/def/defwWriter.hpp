// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef DEFW_WRITER_H
#define DEFW_WRITER_H

#include <stdarg.h>
#include <stdio.h>

#include "defiKRDefs.hpp"
#include "defiDefs.hpp"
#include "defiUser.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

/* Return codes for writing functions: */
#define DEFW_OK                0
#define DEFW_UNINITIALIZED     1
#define DEFW_BAD_ORDER         2
#define DEFW_BAD_DATA          3
#define DEFW_ALREADY_DEFINED   4
#define DEFW_WRONG_VERSION     5
#define DEFW_OBSOLETE          6
#define DEFW_TOO_MANY_STMS     7  // the number defined at the beginning of the
                                  // section is smaller than the actual number
                                  // of statements defined in that section 

/* orient
   0 = N
   1 = W
   2 = S
   3 = E
   4 = FN
   5 = FW
   6 = FS
   7 = FE
*/

/* This routine will write a new line */
extern int defwNewLine();

/* The DEF writer initialization.  Must be called first.
 * Either this routine or defwInitCbk should be call only.
 * Can't call both routines in one program.
 * This routine is for user who does not want to use the callback machanism.
 * Returns 0 if successful. */
extern int defwInit ( FILE* f, int vers1, int version2,
	      const char* caseSensitive,  /* NAMESCASESENSITIVE */
	      const char* dividerChar,    /* DIVIDERCHAR */
	      const char* busBitChars,    /* BUSBITCHARS */
	      const char* designName,     /* DESIGN */
	      const char* technology,     /* optional(NULL) - TECHNOLOGY */
	      const char* array,          /* optional(NULL) - ARRAYNAME */
	      const char* floorplan,      /* optional(NULL) - FLOORPLAN */
	      double units );             /* optional  (set to -1 to ignore) */

/* The DEF writer initialization.  Must be called first.
 * Either this routine or defwInit should be call only.
 * Can't call both routines in one program.
 * This routine is for user who choose to use the callback machanism.
 * If user uses the callback for the writer, they need to provide
 * callbacks for Version, NamesCaseSensitive, BusBitChars and DividerChar.
 * These sections are required by the def.  If any of these callbacks
 * are missing, defaults will be used.
 * Returns 0 if successful. */
extern int defwInitCbk (FILE* f);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwVersion (int vers1, int vers2);

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwCaseSensitive ( const char* caseSensitive );

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwBusBitChars ( const char* busBitChars );

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwDividerChar ( const char* dividerChar );

/* This routine must be called after the defwInit.
 * This routine is required.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwDesignName ( const char* name );

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwTechnology ( const char* technology );

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwArray ( const char* array );

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwFloorplan ( const char* floorplan );

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwUnits ( int units );

/* This routine must be called after the defwInit.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called 0 to many times. */
extern int defwHistory ( const char* string );

/* This routine must be called after the history routines (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwStartPropDef ( void );

/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds integer property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
extern int defwIntPropDef(
               const char* objType,   // LIBRARY | LAYER | VIA | VIARULE |
                                      // NONDEFAULTRULE | MACRO | PIN 
               const char* propName,
               double leftRange,      /* optional(0) - RANGE */
               double rightRange,     /* optional(0) */
               int    propValue);     /* optional(NULL) */
 
/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds real property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
extern int defwRealPropDef(
               const char* objType,   // LIBRARY | LAYER | VIA | VIARULE |
                                      // NONDEFAULTRULE | MACRO | PIN 
               const char* propName,
               double leftRange,      /* optional(0) - RANGE */
               double rightRange,     /* optional(0) */
               double propValue);     /* optional(NULL) */
 
/* This routine must be called after defwStartPropDef.
 * This routine can be called multiple times.
 * It adds string property definition to the statement.
 * Returns 0 if successfull.
 * The objType can be LIBRARY or VIA or MACRO or PIN. */
extern int defwStringPropDef(
               const char* objType,    // LIBRARY | LAYER | VIA | VIARULE |
                                       // NONDEFAULTRULE | MACRO | PIN
               const char* propName,
               double leftRange,       /* optional(0) - RANGE */
               double rightRange,      /* optional(0) */
               const char* propValue); /* optional(NULL) */

/* This routine must be called after all the properties have been
 * added to the file.
 * If you called defwPropertyDefinitions then this routine is NOT optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwEndPropDef ( void );

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds string property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
extern int defwStringProperty(const char* propName, const char* propValue);

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds real property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
extern int defwRealProperty(const char* propName, double propValue);

/* This routine can be called after defwRow, defwRegion, defwComponent,
 * defwPin, defwSpecialNet, defwNet, and defwGroup
 * This routine is optional, it adds int property to the statement.
 * Returns 0 if successful.
 * This routine can be called 0 to many times */
extern int defwIntProperty(const char* propName, int propValue);

/* This routine must be called after the property definitions (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwDieArea ( int xl,            /* point1 - x */
                         int yl,            /* point1 - y */
                         int xh,            /* point2 - x */
                         int yh );          /* point2 - y */

/* This routine must be called after the property definitions (if any).
 * This routine is optional.
 * This routine is the same as defwDieArea, but accept more than 2 points
 * This is a 5.6 syntax
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwDieAreaList ( int num_points, /* number of points on list */
                             int* xl,        /* all the x points */
                             int* yh);       /* all the y points */

/* This routine must be called after the Die Area (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The integer "orient" and operation of the do is explained in
 * the documentation.
 * In 5.6, the DO syntax is optional and the STEP syntax is optional in DO */
extern int defwRow ( const char* rowName, const char* rowType,
		     int x_orig, int y_orig, int orient,
		     int do_count,            /* optional (0) */
                     int do_increment,        /* optional (0) */
                     int xstep,               /* optional (0) */
                     int ystep);              /* optional (0) */

/* This routine must be called after the Die Area (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * This routine is the same as defwRow, excpet orient is a char* */
extern int defwRowStr ( const char* rowName, const char* rowType,
		        int x_orig, int y_orig, const char* orient,
		        int do_count,         /* optional (0) */
                        int do_increment,     /* optional (0) */
                        int xstep,            /* optional (0) */
                        int ystep);           /* optional (0) */

/* This routine must be called after the defwRow (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The operation of the do is explained in the documentation. */
extern int defwTracks ( const char* master,   /* X | Y */
                        int doStart,          /* start */
	                int doCount,          /* numTracks */
                        int doStep,           /* space */
                        int numLayers,        /* number of layers */
                        const char** layers,  /* list of layers */
			int mask = 0,         /* optional */
			int sameMask = 0);    /* optional */

/* This routine must be called after the defwTracks (if any).
 * This routine is optional.
 * Returns 0 if successful.
 * The operation of the do is explained in the documentation. */
extern int defwGcellGrid ( const char* master, /* X | Y */
                           int doStart,        /* start */
	                   int doCount,        /* numColumns | numRows */
                           int doStep);        /* space */

/* This routine must be called after the defwTracks (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the default capacitance section.   All of the
 * capacitances must follow.
 * The count is the number of defwDefaultCap calls to follow.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
extern int defwStartDefaultCap ( int count );

/* This routine is called once for each default cap.  The calls must
 * be preceeded by a call to defwStartDefaultCap and must be
 * terminated by a call to defwEndDefaultCap.
 * Returns 0 if successful.
 * This api is obsolete in 5.4. */
extern int defwDefaultCap ( int pins,           /* MINPINS */
                            double cap);        /* WIRECAP */

/* This routine must be called after the defwDefaultCap calls (if any).
 * Returns 0 if successful.
 * If the count in StartDefaultCap is not the same as the number of
 * calls to DefaultCap then DEFW_BAD_DATA will return returned.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
extern int defwEndDefaultCap ( void );

/* This routine must be called after the defwDefaultCap calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwCanPlace(const char* master,     /* sitename */
                        int xOrig,
                        int yOrig,
	                int orient,             /* 0 to 7 */
                        int doCnt,              /* numX */
                        int doInc,              /* numY */
                        int xStep,              /* spaceX */
                        int yStep);             /* spaceY */

/* This routine must be called after the defwDefaultCap calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwCanPlace, except orient is a char* */
extern int defwCanPlaceStr(const char* master,     /* sitename */
                           int xOrig,
                           int yOrig,
	                   const char* orient,     /* 0 to 7 */
                           int doCnt,              /* numX */
                           int doInc,              /* numY */
                           int xStep,              /* spaceX */
                           int yStep);             /* spaceY */

/* This routine must be called after the defwCanPlace calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwCannotOccupy(const char* master,  /* sitename */
                            int xOrig,
                            int yOrig,
	                    int orient,          /* 0 to 7 */
                            int doCnt,           /* numX */
                            int doInc,           /* numY */
                            int xStep,           /* spaceX */
                            int yStep);          /* spaceY */

/* This routine must be called after the defwCanPlace calls (if any).
 * The operation of the do is explained in the documentation.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwCannotOccupy, except orient is a char* */
extern int defwCannotOccupyStr(const char* master,  /* sitename */
                               int xOrig,
                               int yOrig,
	                       const char* orient,  /* 0 to 7 */
                               int doCnt,           /* numX */
                               int doInc,           /* numY */
                               int xStep,           /* spaceX */
                               int yStep);          /* spaceY */

/* This routine must be called after defwCannotOccupy (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the via section.   All of the vias must follow.
 * The count is the number of defwVia calls to follow.
 * The routine can be called only once. */
extern int defwStartVias( int count );

/* These routines enter each via into the file.
 * These routines must be called after the defwStartVias call.
 * defwViaName should be called first, follow either by defwViaPattern or
 * defwViaLayer.  At the end of each via, defwOneViaEnd should be called
 * These routines are for [- viaName [+ PATTERNNAME patternName + RECT layerName
 * pt pt]...;]...
 * Returns 0 if successful.
 * The routines can be called many times. */
extern int defwViaName(const char* name);

extern int defwViaPattern(const char* patternName);

/* This routine can be called multiple times. */
/* mask is 5.8 syntax */
extern int defwViaRect(const char* layerName,
                   int xl,         /* xl from the RECT */
                   int yl,         /* yl from the RECT */
                   int xh,         /* xh from the RECT */
                   int yh,         /* yh from the RECT */
		   int mask = 0);  /* optional */

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
/* mask is 5.8 syntax */
extern int defwViaPolygon(const char* layerName,
                          int num_polys, 
			  double* xl, 
			  double* yl, 
			  int mask = 0);

/* These routine must be called after defwViaName.
 * Either this routine or defwViaPattern can be called after each
 * defwViaName is called.
 * This is a 5.6 syntax
 * Returns 0 if successful
 * The routine can be called only once per defwViaName called. */
extern int defwViaViarule(const char* viaRuleName,
                          double xCutSize, double yCutSize,
                          const char* botMetalLayer, const char* cutLayer,
                          const char* topMetalLayer,
                          double xCutSpacing, double yCutSpacing,
                          double xBotEnc, double yBotEnc,
                          double xTopEnc, double yTopEnc);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwViaViaruleRowCol(int numCutRows, int numCutCols);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwViaViaruleOrigin(int xOffset, int yOffset);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwViaViaruleOffset(int xBotOffset, int yBotOffset,
                                int xTopOffset, int yTopOffset);

/* This routine can call only after defwViaViarule.
 * It can only be called once.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwViaViarulePattern(const char* cutPattern);

extern int defwOneViaEnd();

/* This routine must be called after the defwVia calls.
 * Returns 0 if successful.
 * If the count in StartVias is not the same as the number of
 * calls to Via or ViaPattern then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
extern int defwEndVias( void );

/* This routine must be called after via section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the region section.   All of the regions must follow.
 * The count is the number of defwRegion calls to follow.
 * The routine can be called only once. */
extern int defwStartRegions( int count );

/* This routine enter each region into the file.
 * This routine must be called after the defwStartRegions call.
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwRegionName(const char* name);

/* This routine enter the region point to the region name.
 * This routine must be called after the defwRegionName call.
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwRegionPoints(int xl, int yl, int xh, int yh);

/* This routine enter the region type, FENCE | GUIDE.
 * This routine must be called after the defwRegionName call.
 * This is a 5.4.1 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwRegionType(const char* type);  /* FENCE | GUIDE */

/* This routine must be called after the defwRegion calls.
 * Returns 0 if successful.
 * If the count in StartRegions is not the same as the number of
 * calls to Region or RegionPattern then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
extern int defwEndRegions( void );

/* This is a 5.8 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwComponentMaskShiftLayers(const char** layerNames,
	                                int          numLayerName);

/* This routine must be called after the regions section (if any).
 * This section of routines is NOT optional.
 * Returns 0 if successful.
 * The routine starts the components section. All of the components
 * must follow.
 * The count is the number of defwComponent calls to follow.
 * The routine can be called only once. */
extern int defwStartComponents( int count );

/* This routine enter each component into the file.
 * This routine must be called after the defwStartComponents call.
 * The optional fields will be ignored if they are set to zero
 * (except for weight which must be set to -1.0).
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwComponent(const char* instance,  /* compName */
              const char* master,           /* modelName */
              int   numNetName,             // optional(0) - # netNames defined
                                            
              const char** netNames,        /* optional(NULL) - list */
              const char* eeq,              /* optional(NULL) - EEQMASTER */
              const char* genName,          /* optional(NULL) - GENERATE */
              const char* genParemeters,    /* optional(NULL) - parameters */
              const char* source,           // optional(NULL) - NETLIST | DIST |
                                            // USER | TIMING 
              int numForeign,               // optional(0) - # foreigns,
                                            // foreignx, foreigny & orients
              const char** foreigns,        /* optional(NULL) - list */
              int* foreignX, int* foreignY, /* optional(0) - list foreign pts */
              int* foreignOrients,          /* optional(-1) - 0 to 7 */
              const char* status,           // optional(NULL) - FIXED | COVER |
                                            //  PLACED | UNPLACED
              int statusX, int statusY,     /* optional(0) - status pt */
              int statusOrient,             /* optional(-1) - 0 to 7 */
              double weight,                /* optional(0) */
              const char* region,           // optional(NULL) - either xl, yl,
                                            // xh, yh or region 
              int xl, int yl,               /* optional(0) - region pt1 */
              int xh, int yh);              /* optional(0) - region pt2 */


/* This routine enter each component into the file.
 * This routine must be called after the defwStartComponents call.
 * The optional fields will be ignored if they are set to zero
 * (except for weight which must be set to -1.0).
 * Returns 0 if successful.
 * The routine can be called many times. 
 * This routine is the same as defwComponent, except orient is a char** */
extern int defwComponentStr(const char* instance,  /* compName */
              const char* master,           /* modelName */
              int   numNetName,             // optional(0) - # netNames defined
                                            //
              const char** netNames,        /* optional(NULL) - list */
              const char* eeq,              /* optional(NULL) - EEQMASTER */
              const char* genName,          /* optional(NULL) - GENERATE */
              const char* genParemeters,    /* optional(NULL) - parameters */
              const char* source,           // optional(NULL) - NETLIST | DIST |
                                            // USER | TIMING 
              int numForeign,               // optional(0) - # foreigns,
                                            // foreignx, foreigny & orients
              const char** foreigns,        /* optional(NULL) - list */
              int* foreignX, int* foreignY, /* optional(0) - list foreign pts */
              const char** foreignOrients,  /* optional(NULL) */
              const char* status,           // optional(NULL) - FIXED | COVER |
                                            // PLACED | UNPLACED 
              int statusX, int statusY,     /* optional(0) - status pt */
              const char* statusOrient,     /* optional(NULL) */
              double weight,                /* optional(0) */
              const char* region,           // optional(NULL) - either xl, yl,
                                            // xh, yh or region 
              int xl, int yl,               /* optional(0) - region pt1 */
              int xh, int yh);              /* optional(0) - region pt2 */

/* This is a 5.8 syntax.
 * Returns 0 if successful.
 * The routine can be called only once. */
extern int defwComponentMaskShift(int shiftLayerMasks);

/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * Either this routine or defwComponentHaloSoft can be called, but not both
 * This routine is optional.
 * This is a 5.6 syntax.
 * Returns 0 if successful.  */
extern int defwComponentHalo(int left, int bottom, int right, int top);
 
/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * This routine is just like defwComponentHalo, except it writes the option SOFT
 * Either this routine or defwComponentHalo can be called, but not both
 * This routine is optional.
 * This is a 5.7 syntax.
 * Returns 0 if successful.  */
extern int defwComponentHaloSoft(int left, int bottom, int right, int top);
 
/* This routine must be called after either the defwComponent or
 * defwComponentStr.
 * This routine can only called once per component.
 * This routine is optional.
 * This is a 5.7 syntax.
 * Returns 0 if successful.  */
extern int defwComponentRouteHalo(int haloDist, const char* minLayer,
                                  const char* maxLayer);
 
/* This routine must be called after the defwComponent calls.
 * Returns 0 if successful.
 * If the count in StartComponents is not the same as the number of
 * calls to Component then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
extern int defwEndComponents( void );

/* This routine must be called after the components section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the pins section. All of the pins must follow.
 * The count is the number of defwPin calls to follow.
 * The routine can be called only once. */
extern int defwStartPins( int count );

/* This routine enter each pin into the file.
 * This routine must be called after the defwStartPins call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times.
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
extern int defwPin(const char* name,     /* pinName */
              const char* net,           /* netName */
              int special,               /* 0 - ignore, 1 - special */
              const char* direction,     // optional(NULL) - INPUT | OUTPUT |
                                         // INOUT | FEEDTHRU 
              const char* use,           // optional(NULL) - SIGNAL | POWER |
                                         // GROUND | CLOCK | TIEOFF | ANALOG 
              const char* status,        // optional(NULL) - FIXED | PLACED |
                                         // COVER 
              int statusX, int statusY,  /* optional(0) - status point */
              int orient,                /* optional(-1) - status orient */
              const char* layer,         /* optional(NULL) - layerName */
              int xl, int yl,            /* optional(0) - layer point1 */
              int xh, int yh);           /* optional(0) - layer point2 */

/* This routine enter each pin into the file.
 * This routine must be called after the defwStartPins call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times.
 * This routine is the same as defwPin, except orient is a char*
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
extern int defwPinStr(const char* name,     /* pinName */
              const char* net,           /* netName */
              int special,               /* 0 - ignore, 1 - special */
              const char* direction,     // optional(NULL) - INPUT | OUTPUT |
                                         // INOUT | FEEDTHRU 
              const char* use,           // optional(NULL) - SIGNAL | POWER |
                                         // GROUND | CLOCK | TIEOFF | ANALOG 
              const char* status,        // optional(NULL) - FIXED | PLACED |
                                         // COVER
              int statusX, int statusY,  /* optional(0) - status point */
              const char* orient,        /* optional(NULL) */
              const char* layer,         /* optional(NULL) - layerName */
              int xl, int yl,            /* optional(0) - layer point1 */
              int xh, int yh);           /* optional(0) - layer point2 */

/* This routine should be called if the layer has either SPACING or
 * DESIGNRULEWIDTH.  If this routine is used and the pin has only one
 * layer, the layer in defwPin or defwPinStr has to be null, otherwise
 * the layer will be written out twice.
 * This routine must be called after defwPin or defwPinStr.
 * This is a 5.6 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * This routine can be called multiple times within a pin. */
extern int defwPinLayer(const char* layerName,
              int spacing,         /* optional(0) - SPACING & DESIGNRULEWIDTH */
              int designRuleWidth, /* are mutually exclusive */
              int xl, int yl,
              int xh, int yh,
              int mask = 0);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out layer with polygon.
 * This is a 5.6 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
extern int defwPinPolygon(const char* layerName,
              int spacing,         /* optional(0) - SPACING & DESIGNRULEWIDTH */
              int designRuleWidth, /* are mutually exclusive */
              int num_polys, double* xl, double* yl,
              int mask = 0);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out layer with via.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
extern int defwPinVia(const char* viaName, int xl, int yl, int mask = 0);

/* This routine must be called after defwPin or defwPinStr.
 * This routine is to write out pin with port.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
extern int defwPinPort();

/* This routine is called after defwPinPort. 
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * This routine can be called multiple times within a pin. */
extern int defwPinPortLayer(const char* layerName,
              int spacing,         /* optional(0) - SPACING & DESIGNRULEWIDTH */
              int designRuleWidth, /* are mutually exclusive */
              int xl, int yl,
              int xh, int yh,
              int mask = 0);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
extern int defwPinPortPolygon(const char* layerName,
              int spacing,         /* optional(0) - SPACING & DESIGNRULEWIDTH */
              int designRuleWidth, /* are mutually exclusive */
              int num_polys, double* xl, double* yl,
              int mask = 0);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called multiple times within a pin. */
extern int defwPinPortVia(const char* viaName, int xl, int yl, int mask = 0);

/* This routine must be called after defwPinPort.
 * This is a 5.7 syntax.
 * This routine is optional.
 * Returns 0 if successful.
 * The routine can be called many times.
 * NOTE: Use defwPinLayer to write out layer with SPACING or DESIGNRULEWIDTH */
extern int defwPinPortLocation(
              const char* status,        /* FIXED | PLACED | COVER */
              int statusX, int statusY,  /* status point */
              const char* orient);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
extern int defwPinNetExpr(const char* pinExpr);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
extern int defwPinSupplySensitivity(const char* pinName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.6 syntax.
 * The routine can be called only once per pin. */
extern int defwPinGroundSensitivity(const char* pinName);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinPartialMetalArea(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinPartialMetalSideArea(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinPartialCutArea(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinDiffArea(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.5 syntax.
 * The oxide can be either OXIDE1, OXIDE2, OXIDE3, or OXIDE4.
 * Each oxide value can be called only once after defwPin. */
extern int defwPinAntennaModel(const char* oxide);

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinGateArea(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinMaxAreaCar(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinMaxSideAreaCar(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after defwPin.
 * Returns 0 if successful.
 * This is a 5.4 syntax.
 * The routine can be called multiple times. */
extern int defwPinAntennaPinMaxCutCar(int value,
              const char* layerName);    /* optional(NULL) */

/* This routine must be called after the defwPin calls.
 * Returns 0 if successful.
 * If the count in StartPins is not the same as the number of
 * calls to Pin then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
extern int defwEndPins( void );

/* This routine must be called after the pin section (if any).
 * This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the pinproperties section. All of the pinproperties
 * must follow.
 * The count is the number of defwPinProp calls to follow.
 * The routine can be called only once. */
extern int defwStartPinProperties( int count );

/* This routine enter each pinproperty into the file.
 * This routine must be called after the defwStartPinProperties call.
 * The optional fields will be ignored if they are set to zero.
 * Returns 0 if successful.
 * The routine can be called many times. */
extern int defwPinProperty(const char* name,     /* compName | PIN */
              const char* pinName);              /* pinName */

/* This routine must be called after the defwPinProperty calls.
 * Returns 0 if successful.
 * If the count in StartPins is not the same as the number of
 * calls to Pin then DEFW_BAD_DATA will return returned.
 * The routine can be called only once. */
extern int defwEndPinProperties( void );

/* Routines to enter a special net or nets into the file.
 * You must first call defwStartSpecialNets with the number of
 * nets. This section is required, even if you do not have any nets.
 * For each net you should call defwSpecialNet followed by
 * one or more defwSpecialNetConnection calls.
 * After the connections come the options.  Options are
 * NOT required.
 * Each net is completed by calling defwSpecialNetEndOneNet().
 * The nets section is finished by calling defwEndNets(). */
extern int defwStartSpecialNets(int count);

/* This routine must be called after the defwStartSpecialNets it is for
 * - netName */
extern int defwSpecialNet(const char* name);   /* netName */

/* This routine is for compNameRegExpr, pinName, and SYNTHESIZED */
/* It can be called multiple times */
extern int defwSpecialNetConnection(const char* inst,    /* compNameRegExpr */
                   const char* pin,       /* pinName */
                   int synthesized);     /* 0 - ignore, 1 - SYNTHESIZED  */

/* This routine is for + FIXEDBUMP
 * This is a 5.4.1 syntax */
extern int defwSpecialNetFixedbump();

/* This routine is for + VOLTAGE volts */
extern int defwSpecialNetVoltage(double v);

/* This routine is for + SPACING layerName spacing [RANGE minwidth maxwidth */
extern int defwSpecialNetSpacing(const char* layer,  /* layerName */
                    int spacing,          /* spacing */
                    double minwidth,      /* optional(0) - minwidth */
                    double maxwidth);     /* optional(0) - maxwidth */

/* This routine is for + WIDTH layerName width */
extern int defwSpecialNetWidth(const char* layer, /* layerName */
                    int width);                   /* width */

/* This routine is for + SOURCE {NETLIST | DIST | USER | TIMING} */
extern int defwSpecialNetSource(const char* name);

/* This routine is for + ORIGINAL netName */
extern int defwSpecialNetOriginal(const char* name);   /* netName */

/* This routine is for + PATTERN {STEINER | BALANCED | WIREDLOGIC | TRUNK} */
extern int defwSpecialNetPattern(const char* name);

/* This routine is for + USE {SIGNAL | POWER | GROUND | CLOCK | TIEOFF |
   ANALOG | SCAN | RESET} */
extern int defwSpecialNetUse(const char* name);

/* This routine is for + WEIGHT weight */
extern int defwSpecialNetWeight(double value);

/* This routine is for + ESTCAP wireCapacitance */
extern int defwSpecialNetEstCap(double value);

/* Paths are a special type of option.  A path must begin
 * with a defwSpecialNetPathStart and end with a defwSpecialNetPathEnd().
 * The individual parts of the path can be entered in
 * any order. */
extern int defwSpecialNetPathStart(const char* typ); // ROUTED | FIXED | COVER |
                                                     // SHIELD | NEW 
extern int defwSpecialNetShieldNetName(const char* name); /* shieldNetName */

extern int defwSpecialNetPathLayer(const char* name); /* layerName */

extern int defwSpecialNetPathWidth(int width);

/* This routine is optional.
 * This is a 5.6 syntax. */
extern int defwSpecialNetPathStyle(int styleNum);

extern int defwSpecialNetPathShape(const char* shapeType); // RING | STRIPE |
        // FOLLOWPIN | IOWIRE | COREWIRE | BLOCKWIRE | FILLWIRE | BLOCKAGEWIRE 


/* This routine is optional.
   This is a 5.8 syntax.
 * Returns 0 if successful. */
extern int defwSpecialNetPathMask(int colorMask);

/* x and y location of the path */
extern int defwSpecialNetPathPoint(int numPts,  /* number of connected points */
                   double* pointx,     /* point x list */
                   double* pointy);    /* point y list */
extern int defwSpecialNetPathVia(const char* name);   /* viaName */

/* This routine is called after defwSpecialNetPath
 * This is a 5.4.1 syntax */
extern int defwSpecialNetPathViaData(int numX, int numY, int stepX, int stepY);

/* x and y location of the path */
extern int defwSpecialNetPathPointWithWireExt(
                   int numPts,                /* number of connected points */
                   double* pointx,       /* point x list */
                   double* pointy,       /* point y list */
                   double* optValue);       /* optional(NULL) value */

extern int defwSpecialNetPathEnd();

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
extern int defwSpecialNetPolygon(const char* layerName,
                                 int num_polys, double* xl, double* yl);

/* This is a 5.6 syntax
 * This routine can be called multiple times. */
extern int defwSpecialNetRect(const char* layerName,
                              int xl, int yl, int xh, int yh);

extern int defwSpecialNetVia(const char* layerName);

extern int defwSpecialNetViaWithOrient(const char* layerName, int orient);

extern int defwSpecialNetViaPoints(int num_points, double* xl, double* yl);

/* This routine is called at the end of each net */
extern int defwSpecialNetEndOneNet();

/* 5.3 for special net */
/* Shields are a special type of option.  A shield must begin
 * with a defwSpecialNetShieldStart and end with a defwSpecialNetShieldEnd().
 * The individual parts of the shield can be entered in
 * any order. */
extern int defwSpecialNetShieldStart(const char* name);

extern int defwSpecialNetShieldLayer(const char* name); /* layerName */
extern int defwSpecialNetShieldWidth(int width);        /* width */
extern int defwSpecialNetShieldShape(const char* shapeType); // RING | STRIPE |
        // FOLLOWPIN | IOWIRE | COREWIRE | BLOCKWIRE | FILLWIRE | BLOCKAGEWIRE
 
/* x and y location of the path */
extern int defwSpecialNetShieldPoint(int numPts, /* # of connected points */
                   double* pointx,     /* point x list */
                   double* pointy);    /* point y list */
extern int defwSpecialNetShieldVia(const char* name);   /* viaName */

/* A 5.4.1 syntax */
extern int defwSpecialNetShieldViaData(int numX, int numY, int stepX, int stepY);
extern int defwSpecialNetShieldEnd();
/* end 5.3 */
 
/* This routine is called at the end of the special net section */
extern int defwEndSpecialNets();

/* Routines to enter a net or nets into the file.
 * You must first call defwNets with the number of nets.
 * This section is required, even if you do not have any nets.
 * For each net you should call defwNet followed by one or
 * more defwNetConnection calls.
 * After the connections come the options.  Options are
 * NOT required.
 * Each net is completed by calling defwNetEndOneNet().
 * The nets section is finished by calling defwEndNets(). */
extern int defwStartNets(int count);

/* This routine must be called after the defwStartNets, it is for - netName */
extern int defwNet(const char* name);

/* This routine is for { compName | PIN } pinName [+ SYNTHESIZED] */
/* It can be called multiple times */
extern int defwNetConnection(const char* inst,    /* compName */
                   const char* pin,      /* pinName */
                   int synthesized);    /* 0 - ignore, 1 - SYNTHESIZED */

/* This routine is for MUSTJOIN, compName, pinName */
extern int defwNetMustjoinConnection(const char* inst,  /* compName */
                                     const char* pin);     /* pinName */

/* This routine is for + VPIN vpinName [LAYER layerName pt pt
 * [{ PLACED | FIXED | COVER } pt orient] */
extern int defwNetVpin(const char* vpinName,
                 const char* layerName,     /* optional(NULL) */
                 int layerXl, int layerYl,  /* layer point1 */
                 int layerXh, int layerYh,  /* layer point2 */
                 const char* status,        /* optional(NULL) */
                 int statusX, int statusY,  /* optional(0) - status point */
                 int orient);               /* optional(-1) */

/* This routine is for + VPIN vpinName [LAYER layerName pt pt
 * [{ PLACED | FIXED | COVER } pt orient]
 * This routine is the same as defwNetVpin, except orient is a char* */
extern int defwNetVpinStr(const char* vpinName,
                 const char* layerName,     /* optional(NULL) */
                 int layerXl, int layerYl,  /* layer point1 */
                 int layerXh, int layerYh,  /* layer point2 */
                 const char* status,        /* optional(NULL) */
                 int statusX, int statusY,  /* optional(0) - status point */
                 const char* orient);       /* optional(NULL) */

/* This routine can be called either within net or subnet.
 * it is for NONDEFAULTRULE rulename */
extern int defwNetNondefaultRule(const char* name);

/* This routine is for + XTALK num */
extern int defwNetXtalk(int xtalk);

/* This routine is for + FIXEDBUMP
 * This is a 5.4.1 syntax */
extern int defwNetFixedbump();

/* This routine is for + FREQUENCY
 * This is a 5.4.1 syntax */
extern int defwNetFrequency(double frequency);

/* This routine is for + SOURCE {NETLIST | DIST | USER | TEST | TIMING} */
extern int defwNetSource(const char* name);

/* This routine is for + ORIGINAL netname */
extern int defwNetOriginal(const char* name);

/* This routine is for + USE {SIGNAL | POWER | GROUND | CLOCK | TIEOFF | 
 * ANALOG} */
extern int defwNetUse(const char* name);

/* This routine is for + PATTERN {STEINER | BALANCED | WIREDLOGIC} */
extern int defwNetPattern(const char* name);

/* This routine is for + ESTCAP wireCapacitance */
extern int defwNetEstCap(double value);

/* This routine is for + WEIGHT weight */
extern int defwNetWeight(double value);

/* 5.3 for net */
/* This routine is for + SHIELDNET weight */
extern int defwNetShieldnet(const char* name);

/* Noshield are a special type of option.  A noshield must begin
 * with a defwNetNoshieldStart and end with a defwNetNoshieldEnd().
 * The individual parts of the noshield can be entered in
 * any order. */
extern int defwNetNoshieldStart(const char* name);

/* x and y location of the path */
extern int defwNetNoshieldPoint(int numPts, /* number of connected points */
                   const char** pointx,     /* point x list */
                   const char** pointy);    /* point y list */
extern int defwNetNoshieldVia(const char* name);   /* viaName */
extern int defwNetNoshieldEnd();
/* end 5.3 */

/* Subnet are a special type of option. A subnet must begin
 * with a defwNetSubnetStart and end with a defwNetSubnetEnd().
 * Routines to call within the subnet are: defwNetSubnetPin,
 * defwNetNondefaultRule and defwNetPathStart... */
extern int defwNetSubnetStart(const char* name);

/* This routine is called after the defwNetSubnet, it is for
 * [({compName | PIN} pinName) | (VPIN vpinName)]... */
extern int defwNetSubnetPin(const char* compName,  /* compName | PIN | VPIN */
                 const char* pinName);    /* pinName | vpinName */

extern int defwNetSubnetEnd();

/* Paths are a special type of option.  A path must begin
 * with a defwNetPathStart and end with a defwPathEnd().
 * The individual parts of the path can be entered in
 * any order. */
extern int defwNetPathStart(const char* typ); // ROUTED | FIXED | COVER |
                                              // NOSHIELD | NEW 
extern int defwNetPathWidth(int w);           /* width */
extern int defwNetPathLayer(const char* name, /* layerName */
                 int isTaper,                 /* 0 - ignore, 1 - TAPER */
                 const char* rulename);       /* only one, isTaper or */
                                              /*rulename can be assigned */
/* This routine is optional.
 * This is a 5.6 syntax. */
extern int defwNetPathStyle(int styleNum);

/* This routine is optional.
 * This is a 5.8 syntax. */
extern int defwNetPathMask(int maskNum);

extern int defwNetPathRect(int deltaX1, int deltaY1, int deltaX2, int deltaY2);

extern int defwNetPathVirtual(int x, int y);

/* x and y location of the path */
extern int defwNetPathPoint(int numPts,       /* number of connected points */
                   double* pointx,       /* point x list */
                   double* pointy);      /* point y list */

extern int defwNetPathPointWithExt(int numPts,
				   double* pointx,
				   double* pointy,
				   double* optValue);

extern int defwNetPathVia(const char* name);  /* viaName */

extern int defwNetPathViaWithOrient(const char* name,
                                    int orient);  /* optional(-1) */

extern int defwNetPathViaWithOrientStr(const char* name,
                                       const char* orient); /* optional(Null) */
extern int defwNetPathEnd();

/* This routine is called at the end of each net */
extern int defwNetEndOneNet();

/* This routine is called at the end of the net section */
extern int defwEndNets();

/* This section of routines is optional.
 * Returns 0 if successful.
 * The routine starts the I/O Timing section. All of the iotimings options
 * must follow.
 * The count is the number of defwIOTiming calls to follow.
 * The routine can be called only once.
 * This api is obsolete in 5.4. */
extern int defwStartIOTimings(int count);

/* This routine can be called after defwStaratIOTiming
 * It is for - - {(comp pin) | (PIN name)}
 * This api is obsolete in 5.4. */
extern int defwIOTiming(const char* inst,      /* compName | PIN */
                   const char* pin);           /* pinName */

/* This routine is for + { RISE | FALL } VARIABLE min max
 * This api is obsolete in 5.4. */
extern int defwIOTimingVariable(const char* riseFall,  /* RISE | FALL */
                   int num1,                   /* min */
                   int num2);                  /* max */

/* This routine is for + { RISE | FALL } SLEWRATE min max
 * This api is obsolete in 5.4. */
extern int defwIOTimingSlewrate(const char* riseFall,  /* RISE | FALL */
                   int num1,                   /* min */
                   int num2);                  /* max */

/* This routine is for + DRIVECELL macroName [[FROMPIN pinName] TOPIN pinName]
 * [PARALLEL numDrivers]
 * This api is obsolete in 5.4. */
extern int defwIOTimingDrivecell(const char* name,   /* macroName*/
                   const char* fromPin,        /* optional(NULL) */
                   const char* toPin,          /* optional(NULL) */
                   int numDrivers);            /* optional(0) */

/* This routine is for + CAPACITANCE capacitance
 * This api is obsolete in 5.4. */
extern int defwIOTimingCapacitance(double num);

/* This api is obsolete in 5.4. */
extern int defwEndIOTimings();

/* Routines to enter scan chains.  This section is optional
 * The section must start with a defwStartScanchains() call and
 * end with a defwEndScanchain() call.
 * Each scan chain begins with a defwScanchain() call.
 * The rest of the calls follow.  */
extern int defwStartScanchains(int count);

/* This routine can be called after defwStartScanchains
 * It is for - chainName */
extern int defwScanchain(const char* name);

/* This routine is for + COMMONSCANPINS [IN pin] [OUT pin] */
extern int defwScanchainCommonscanpins(
                  const char* inst1,      /* optional(NULL) - IN | OUT*/
                  const char* pin1,       /* can't be null if inst1 is set */
	          const char* inst2,      /* optional(NULL) - IN | OUT */
                  const char* pin2);      /* can't be null if inst2 is set */


/* This routine is for + PARTITION paratitionName [MAXBITS maxBits] */
/* This is 5.4.1 syntax */
extern int defwScanchainPartition(const char* name,
                  int maxBits);           /* optional(-1) */

/* This routine is for + START {fixedInComp | PIN } [outPin] */
extern int defwScanchainStart(const char* inst,   /* fixedInComp | PIN */
                  const char* pin);               /* outPin */

/* This routine is for + STOP {fixedOutComp | PIN } [inPin] */
extern int defwScanchainStop(const char* inst,    /* fixedOutComp | PIN */
                  const char* pin);               /* inPin */

/* This routine is for + FLOATING {floatingComp [IN pin] [OUT pin]}
 * This is a 5.4.1 syntax */
extern int defwScanchainFloating(const char* name,   /* floatingComp */
                  const char* inst1,      /* optional(NULL) - IN | OUT */
                  const char* pin1,       /* can't be null if inst1 is set */
	          const char* inst2,      /* optional(NULL) - IN | OUT */
                  const char* pin2);      /* can't be null if inst2 is set */

/* This routine is for + FLOATING {floatingComp [IN pin] [OUT pin]}
 * This is a 5.4.1 syntax.
 * This routine is the same as defwScanchainFloating.  But also added
 * the option BITS. */
extern int defwScanchainFloatingBits(const char* name,   /* floatingComp */
                  const char* inst1,      /* optional(NULL) - IN | OUT */
                  const char* pin1,       /* can't be null if inst1 is set */
	          const char* inst2,      /* optional(NULL) - IN | OUT */
                  const char* pin2,       /* can't be null if inst2 is set */
                  int   bits);            /* optional (-1) */

/* This routine is for + ORDERED {fixedComp [IN pin] [OUT pin]
 * fixedComp [IN pin] [OUT pin].
 * When this routine is called for the 1st time within a scanchain, 
 * both name1 and name2 are required.  Only name1 is required is the
 * routine is called more than once. */
extern int defwScanchainOrdered(const char* name1,
                  const char* inst1,      /* optional(NULL) - IN | OUT */
                  const char* pin1,       /* can't be null if inst1 is set */
	          const char* inst2,      /* optional(NULL) - IN | OUT */
                  const char* pin2,       /* can't be null if inst2 is set */
                  const char* name2,
                  const char* inst3,      /* optional(NULL) - IN | OUT */
                  const char* pin3,       /* can't be null if inst3 is set */
	          const char* inst4,      /* optional(NULL) - IN | OUT */
                  const char* pin4);      /* can't be null if inst4 is set */

/* This routine is for + ORDERED {fixedComp [IN pin] [OUT pin]
 * fixedComp [IN pin] [OUT pin].
 * When this routine is called for the 1st time within a scanchain, 
 * both name1 and name2 are required.  Only name1 is required is the
 * routine is called more than once.
 * This is a 5.4.1 syntax.
 * This routine is the same as defwScanchainOrdered.  But also added
 * the option BITS */
extern int defwScanchainOrderedBits(const char* name1,
                  const char* inst1,      /* optional(NULL) - IN | OUT */
                  const char* pin1,       /* can't be null if inst1 is set */
	          const char* inst2,      /* optional(NULL) - IN | OUT */
                  const char* pin2,       /* can't be null if inst2 is set */
                  int   bits1,            /* optional(-1) */
                  const char* name2,
                  const char* inst3,      /* optional(NULL) - IN | OUT */
                  const char* pin3,       /* can't be null if inst3 is set */
	          const char* inst4,      /* optional(NULL) - IN | OUT */
                  const char* pin4,       /* can't be null if inst4 is set */
                  int   bits2);           /* optional(-1) */

extern int defwEndScanchain();

/* Routines to enter constraints.  This section is optional
 * The section must start with a defwStartConstrains() call and
 * end with a defwEndConstraints() call.
 * Each contraint will call the defwConstraint...().
 * This api is obsolete in 5.4. */
extern int defwStartConstraints (int count);  /* optional */

/* The following routines are for - {operand [+ RISEMAX time] [+ FALLMAX time]
 * [+ RISEMIN time] [+ FALLMIN time] | WIREDLOGIC netName MAXDIST distance };}
 * operand - NET netName | PATH comp fromPin comp toPin | SUM (operand, ...)
 * The following apis are obsolete in 5.4. */ 
extern int defwConstraintOperand();          /* begin an operand */
extern int defwConstraintOperandNet(const char* netName);  /* NET */
extern int defwConstraintOperandPath(const char* comp1,    /* PATH - comp|PIN */
                  const char* fromPin,
                  const char* comp2,
                  const char* toPin);
extern int defwConstraintOperandSum();        /* SUM */
extern int defwConstraintOperandSumEnd();     /* mark the end of SUM */
extern int defwConstraintOperandTime(const char* timeType, //  RISEMAX | FALLMAX | RISEMIN | FALLMIN
                                     int time);
extern int defwConstraintOperandEnd();        /* mark the end of operand */

/* This routine is for - WIRELOGIC netName MAXDIST distance */
extern int defwConstraintWiredlogic(const char* netName,
                  int distance);

extern int defwEndConstraints ();

/* Routines to enter groups.  This section is optional
 * The section must start with a defwStartGroups() call and
 * end with a defwEndGroups() call.
 * Each group will call the defwGroup...(). */
extern int defwStartGroups (int count);  /* optional */

/* This routine is for - groupName compNameRegExpr ... */
extern int defwGroup(const char* groupName,
                   int numExpr,
                   const char** groupExpr);

/* This routine is for + SOFT [MAXHALFPERIMETER value] [MAXX value]
 * [MAXY value] */
extern int defwGroupSoft(const char* type1, /* MAXHALFPERIMETER | MAXX | MAXY */
                   double value1,
                   const char* type2,
                   double value2,
                   const char* type3,
                   double value3);

/* This routine is for + REGION {pt pt | regionName} */
extern int defwGroupRegion(int xl, int yl,   /* either the x & y or    */
                   int xh, int yh,           /* regionName only, can't */
                   const char* regionName);  /* be both */

extern int defwEndGroups();

/* Routines to enter Blockages.  This section is optional
 * The section must start with a defwStartBlockages() call and
 * end with a defwEndBlockages() call.
 * Each blockage will call the defwBlockages...().
 * This is a 5.4 syntax. */
extern int defwStartBlockages(int count);   /* count = numBlockages */

/* This routine is for - layerName
* This routine is called per entry within a blockage for layer.
* This is a 5.4 syntax. */
extern int defwBlockagesLayer(const char* layerName);

/* This routine is for - slots 
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
extern int defwBlockagesLayerSlots();

/* This routine is for - fills
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
extern int defwBlockagesLayerFills();

/* This routine is for - pushdown
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.4 syntax. */
extern int defwBlockagesLayerPushdown();

/* This routine is for - exceptpgnet
* This routine is called per entry within a blockage layer, can't be more then one.
* This is a 5.7 syntax. */
extern int defwBlockagesLayerExceptpgnet();

/* This routine is for - component
* This routine called per entry within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
extern int defwBlockagesLayerComponent(const char* compName);

/* This routine is for - spacing
* Either this routine or defwBlockagesDesignRuleWidth is called per entry
* within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
extern int defwBlockagesLayerSpacing(int minSpacing);

/* This routine is for - designrulewidth
* Either this routine or defwBlockagesSpacing is called per entry
* within a blockage layer, can't be more than one.
* This is a 5.6 syntax. */
extern int defwBlockagesLayerDesignRuleWidth(int effectiveWidth);

/* This routine is for - mask.
* This routine called per entry within a blockage layer, can't be more than one.
* This is a 5.8 syntax. */
extern int defwBlockagesLayerMask(int maskColor);

/* This routine is for - layerName & compName
 * Either this routine, defBlockageLayerSlots, defBlockageLayerFills,
 * or defwBlockagePlacement is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockageLayer(const char* layerName,
                             const char* compName);   /* optional(NULL) */

/* This routine is for - layerName & slots 
 * Either this routine, defBlockageLayer, defBlockageLayerFills,
 * defwBlockagePlacement, or defwBlockagePushdown is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockageLayerSlots(const char* layerName);

/* This routine is for - layerName & fills
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockagePushdown is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockageLayerFills(const char* layerName);

/* This routine is for - layerName & pushdown
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockageFills is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockageLayerPushdown(const char* layerName);

/* This routine is for - exceptpgnet
 * Either this routine, defBlockageLayer, defBlockageLayerSlots,
 * defwBlockagePlacement, or defwBlockageFills is called per entry within
 * a blockage, can't be more then one.
 * This is a 5.7 syntax. */
extern int defwBlockageLayerExceptpgnet(const char* layerName);

/* This routine is for - spacing
 * Either this routine or defwBlockageDesignRuleWidth is called per entry
 * within a blockage, can't be more than one.
 * This is a 5.6 syntax. */
extern int defwBlockageSpacing(int minSpacing);

/* This routine is for - designrulewidth
 * Either this routine or defwBlockageSpacing is called per entry
 * within a blockage, can't be more than one.
 * This is a 5.6 syntax. */
extern int defwBlockageDesignRuleWidth(int effectiveWidth);

/* This routine is for - placement
 * This routine is called per entry within blockage for placement.
 * This is a 5.4 syntax.
 * 11/25/2002 - bug fix: submitted by Craig Files (cfiles@ftc.agilent.com)
 * this routine allows to call blockage without a component. */
extern int defwBlockagesPlacement();

/* This routine is for - component
* This routine is called per entry within blockage placement, can't be more then one.
* This is a 5.4 syntax. */
extern int defwBlockagesPlacementComponent(const char* compName);

/* This routine is for - Pushdown
* This routine is called per entry within blockage placement, can't be more then one.
* This is a 5.4 syntax. */
extern int defwBlockagesPlacementPushdown();

/* This routine is for - soft
* Either this routine or defwBlockagesPlacementPartial
* is called per entry within blockage placement, can't be more then one.
* This is a 5.7 syntax. */
extern int defwBlockagesPlacementSoft();

/* This routine is for - Partial
* Either this routine or defwBlockagesPlacementSoft
* is called per entry within blockage placement, can't be more then one.
* This is a 5.7 syntax. */
extern int defwBlockagesPlacementPartial(double maxDensity);

/* This routine is for rectangle.
* This routine is optional and can be called multiple time. 
* This is a 5.4 syntax. */
extern int defwBlockagesRect(int xl, int yl, int xh, int yh);

/* This routine is for polygon.
* This routine is optional and can be called multiple time. 
* This is a 5.6 syntax. */
extern int defwBlockagesPolygon(int num_polys, int* xl, int* yl);

/* This routine is for - placement
* Either this routine or defBlockageLayer
* is called per entry within blockage, can't be more then one.
* This is a 5.4 syntax.
* 11/25/2002 - bug fix: submitted by Craig Files (cfiles@ftc.agilent.com)
* this routine allows to call blockage without a component. */
extern int defwBlockagePlacement();

/* This routine is for - placement & component
 * Either this routine or defwBlockagePlacementPushdown
 * is called per entry within blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockagePlacementComponent(const char* compName);

/* This routine is for - placement & Pushdown
 * Either this routine or defwBlockagePlacementComponent
 * is called per entry within blockage, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwBlockagePlacementPushdown();

/* This routine is for - placement & soft
 * Either this routine or defwBlockagePlacementPushdown
 * is called per entry within blockage, can't be more then one.
 * This is a 5.7 syntax. */
extern int defwBlockagePlacementSoft();

/* This routine is for - placement & Partial
 * Either this routine or defwBlockagePlacementComponent
 * is called per entry within blockage, can't be more then one.
 * This is a 5.7 syntax. */
extern int defwBlockagePlacementPartial(double maxDensity);

/* This routine is optional.
 * This is a 5.8 syntax. */
extern int defwBlockageMask(int maskColor);

/* This routine is for rectangle.
 * This is a 5.4 syntax. */
extern int defwBlockageRect(int xl, int yl, int xh, int yh);

/* This routine is for polygon.
 * This routine is optinal and can be called multiple time. 
 * This is a 5.6 syntax. */
extern int defwBlockagePolygon(int num_polys, int* xl, int* yl);

/* This is a 5.4 syntax. */
extern int defwEndBlockages();

/* Routines to enter Slots.  This section is optional
 * The section must start with a defwStartSlots() call and
 * end with a defwEndSlots() call.
 * Each slots will call the defwSlots...().
 * This is a 5.4 syntax. */
extern int defwStartSlots(int count);   /* count = numSlots */

/* This routine is for - layerName & compName
 * Either this routine, defSlots, defSlotsLayerFills,
 * or defwSlotsPlacement is called per entry within
 * a slot, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwSlotLayer(const char* layerName);

/* This routine is for rectangle
 * This is a 5.4 syntax. */
extern int defwSlotRect(int xl, int yl, int xh, int yh);

/* This routine is for rectangle
 * This is a 5.6 syntax and can be called multiple time. */
extern int defwSlotPolygon(int num_polys, double* xl, double* yl);

/* This is a 5.4 syntax. */
extern int defwEndSlots();

/* Routines to enter Fills.  This section is optional
 * The section must start with a defwStartFills() call and
 * end with a defwEndFills() call.
 * Each fills will call the defwFills...().
 * This is a 5.4 syntax. */
extern int defwStartFills(int count);   /* count = numFills */

/* This routine is for - layerName & compName
 * Either this routine, defFills, defFillsLayerFills,
 * or defwFillsPlacement is called per entry within
 * a fill, can't be more then one.
 * This is a 5.4 syntax. */
extern int defwFillLayer(const char* layerName);

/* This routine is optional.
 * This is a 5.8 syntax. */
extern int defwFillLayerMask(int maskColor);

/* This routine has to be called after defwFillLayer
 * This routine is optional.
 * This is a 5.7 syntax. */
extern int defwFillLayerOPC();

/* This routine is for rectangle.
 * This is a 5.4 syntax. */
extern int defwFillRect(int xl, int yl, int xh, int yh);

/* This routine is for polygon.
 * This is a 5.6 syntax and can be called multiple time. */
extern int defwFillPolygon(int num_polys, double* xl, double* yl);

/* This routine is for via.
 * This routine is optional.
 * This is a 5.7 syntax and can be called multiple time. */
extern int defwFillVia(const char* viaName);

/* This routine is optional.
 * This is a 5.8 syntax. */
extern int defwFillViaMask(int colorMask);

/* This routine is for via OPC.
 * This routine can only be called after defwFillVia.
 * This routine is optional.
 * This is a 5.7 syntax and can be called multiple time. */
extern int defwFillViaOPC();

/* This routine is for via OPC.
 * This routine can only be called after defwFillVia.
 * This routine is required following defwFillVia.
 * This is a 5.7 syntax and can be called multiple time. */
extern int defwFillPoints(int num_points, double* xl, double* yl);

/* This is a 5.4 syntax. */
extern int defwEndFills();

/* Routines to enter NONDEFAULTRULES.  This section is required
 * The section must start with a defwStartNonDefaultRules() and
 * end with defwEndNonDefaultRules() call.
 * This is a 5.6 syntax. */
extern int defwStartNonDefaultRules(int count);

/* This routine is for Layer within the NONDEFAULTRULES
 * This routine can be called multiple times.  It is required.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwNonDefaultRule(const char* ruleName,
                              int hardSpacing);   /* optional(0) */

/* Routines to enter NONDEFAULTRULES.  This section is required
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwNonDefaultRuleLayer(const char* layerName,
                                   int width,
                                   int diagWidth,    /* optional(0) */
                                   int spacing,      /* optional(0) */
                                   int wireExt);     /* optional(0) */

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwNonDefaultRuleVia(const char* viaName);

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwNonDefaultRuleViaRule(const char* viaRuleName);

/* Routines to enter NONDEFAULTRULES.  This section is optional.
 * This routine must be called after the defwNonDefaultRule. 
 * This routine can be called multiple times.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwNonDefaultRuleMinCuts(const char* cutLayerName, int numCutS);

/* This is a 5.4 syntax. */
extern int defwEndNonDefaultRules();

/* Routines to enter STYLES.  This section is required
 * The section must start with a defwStartStyles() and
 * end with defwEndStyles() call.
 * This is a 5.6 syntax. */
extern int defwStartStyles(int count);

/* This routine is for Layer within the NONDEFAULTRULES
 * This routine can be called multiple times.  It is required.
 * This is a 5.6 syntax.
 * Returns 0 if successful. */
extern int defwStyles(int styleNums, int num_points, double* xp, double* yp);

/* This is a 5.4 syntax. */
extern int defwEndStyles();

/* This routine is called after defwInit.
 * This routine is optional and it can be called only once.
 * Returns 0 if successful. */
extern int defwStartBeginext(const char* name);

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * Returns 0 if successful. */
extern int defwBeginextCreator (const char* creatorName);

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * It gets the current system time and date.
 * Returns 0 if successful. */
extern int defwBeginextDate ();

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called only once.
 * Returns 0 if successful. */
extern int defwBeginextRevision (int vers1, int vers2);   /* vers1.vers2 */

/* This routine is called after defwBeginext.
 * This routine is optional, it can be called many times.
 * It allows user to customize their own syntax.
 * Returns 0 if successful. */
extern int defwBeginextSyntax (const char* title, const char* string);

/* This routine is called after defwInit.
 * This routine is optional and it can be called only once.
 * Returns 0 if successful. */
extern int defwEndBeginext();

/* End the DEF file.
 * This routine IS NOT OPTIONAL.
 * The routine must be called LAST. */
extern int defwEnd ( void );

/* General routines that can be called anytime after the Init is called.
 */
extern int defwCurrentLineNumber ( void );

/*
 * extern void defwError ( const char *, ... );
 * extern void defwWarning ( const char *, ... );
 * extern void defwVError ( const char *, va_list );
 * extern void defwVWarning ( const char *, va_list );
 * extern int  defwGetCurrentLineNumber (void);
 * extern const char *defwGetCurrentFileName (void);
 */
 
/* This routine will print the error message. */
extern void defwPrintError(int status);

/* This routine will allow user to write their own comemnt.  It will
 * automactically add a # infront of the line.
 */
extern void defwAddComment(const char* comment);
 
/* This routine will indent 3 blank spaces */
extern void defwAddIndent();

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
