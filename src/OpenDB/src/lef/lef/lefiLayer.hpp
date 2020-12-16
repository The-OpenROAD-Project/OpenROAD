// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2015, Cadence Design Systems
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

#ifndef lefiLayer_h
#define lefiLayer_h

#include <stdio.h>
#include "lefiKRDefs.hpp"
#include "lefiMisc.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

typedef enum lefiAntennaEnum {
  lefiAntennaAR,
  lefiAntennaDAR,
  lefiAntennaCAR,
  lefiAntennaCDAR,
  lefiAntennaAF,
  lefiAntennaSAR,
  lefiAntennaDSAR,
  lefiAntennaCSAR,
  lefiAntennaCDSAR,
  lefiAntennaSAF,
  lefiAntennaO,
  lefiAntennaADR
} lefiAntennaEnum;

class lefiAntennaPWL {
public:
  lefiAntennaPWL();
  ~lefiAntennaPWL();

  lefiAntennaPWL(const lefiAntennaPWL& prev);
  static lefiAntennaPWL* create();
  void Init();
  void clear();
  void Destroy();

  void addAntennaPWL(double d, double r);

  int  numPWL() const;
  double PWLdiffusion(int index); 
  double PWLratio(int index); 

protected:
  int numAlloc_;
  int numPWL_;
  double* d_;
  double* r_;
};

class lefiLayerDensity {
public:
  lefiLayerDensity();
  ~lefiLayerDensity();

  lefiLayerDensity(const lefiLayerDensity& prev);
  void Init(const char* type);
  void Destroy();
  void clear();
  

  void setOneEntry(double entry);
  void addFrequency(int num, double* frequency);
  void addWidth(int num, double* width);
  void addTableEntry(int num, double* entry);
  void addCutarea(int num, double* cutarea);

  char* type() const;
  int hasOneEntry() const;
  double oneEntry() const;
  int numFrequency() const;
  double frequency(int index) const;
  int numWidths() const;
  double width(int index) const;
  int numTableEntries() const;
  double tableEntry(int index) const;
  int numCutareas() const;
  double cutArea(int index) const;

protected:
  char* type_;
  double oneEntry_;
  int numFrequency_;
  double* frequency_;
  int numWidths_;
  double* widths_;
  int numTableEntries_;
  double* tableEntries_;
  int numCutareas_;
  double* cutareas_;
};

// 5.5
class lefiParallel {
public:
  lefiParallel();
  ~lefiParallel();

  LEF_COPY_CONSTRUCTOR_H(lefiParallel);
  LEF_ASSIGN_OPERATOR_H(lefiParallel);
  void Init();
  void clear();
  void Destroy();

  void addParallelLength(int numLength, double* lengths);
  void addParallelWidth(double width);
  void addParallelWidthSpacing(int numSpacing, double* spacings);

  int  numLength() const;
  int  numWidth() const;
  double length(int iLength) const;
  double width(int iWidth) const;
  double widthSpacing(int iWidth, int iWidthSpacing) const;

protected:
  int numLength_;
  int numWidth_;
  int numWidthAllocated_;
  double* length_;
  double* width_;
  double* widthSpacing_;
};

// 5.5
class lefiInfluence {
public:
  lefiInfluence();
  ~lefiInfluence();

  LEF_COPY_CONSTRUCTOR_H (lefiInfluence);
  LEF_ASSIGN_OPERATOR_H (lefiInfluence);
  void Init();
  void clear();
  void Destroy();

  void addInfluence(double width, double distance, double spacing);

  int  numInfluenceEntry() const;
  double width(int index) const;
  double distance(int index) const;
  double spacing(int index) const;

protected:
  int numAllocated_;
  int numWidth_;
  int numDistance_;
  int numSpacing_;
  double* width_;
  double* distance_;
  double* spacing_;
};

// 5.7
class lefiTwoWidths {
public:
  lefiTwoWidths();
  ~lefiTwoWidths();

  lefiTwoWidths(const lefiTwoWidths& prev);
  void Init();
  void clear();
  void Destroy();

  void addTwoWidths(double width, double runLength, int numSpacing,
                    double* spacings, int hasPRL = 0);

  int    numWidth() const;
  double width(int iWidth) const;
  int    hasWidthPRL(int iWidth) const;
  double widthPRL(int iWidth) const;
  int    numWidthSpacing(int iWidth) const;
  double widthSpacing(int iWidth, int iWidthSpacing) const;

protected:
  int numWidth_;
  int numWidthAllocated_;
  double* width_;
  double* prl_;
  int*    hasPRL_;
  int*    numWidthSpacing_;   // each slot contains number of spacing of slot
  double* widthSpacing_;
  int*    atNsp_;             // accumulate total number of spacing
};

// 5.5
class lefiSpacingTable {
public:
  lefiSpacingTable();
  ~lefiSpacingTable();

  LEF_COPY_CONSTRUCTOR_H(lefiSpacingTable);
  LEF_ASSIGN_OPERATOR_H(lefiSpacingTable);
  void Init();
  void clear();
  void Destroy();

  void addParallelLength(int numLength, double* lengths);
  void addParallelWidth(double width);
  void addParallelWidthSpacing(int numSpacing, double* spacing);
  void setInfluence();
  void addInfluence(double width, double distance, double spacing);
  void addTwoWidths(double width, double runLength, int numSpacing,
                    double* spacing, int hasPRL = 0);          // 5.7
  
  int isInfluence() const;
  lefiInfluence* influence() const;
  int isParallel() const;
  lefiParallel*  parallel() const;
  lefiTwoWidths* twoWidths() const;           // 5.7

protected:
  int hasInfluence_;
  lefiInfluence*   influence_;
  lefiParallel*    parallel_;
  lefiTwoWidths*   twoWidths_;               // 5.7
};

// 5.7
class lefiOrthogonal {
public:
  lefiOrthogonal();
  ~lefiOrthogonal();

  LEF_COPY_CONSTRUCTOR_H( lefiOrthogonal );
  LEF_ASSIGN_OPERATOR_H( lefiOrthogonal );
  void Init();
  void Destroy();

  void addOrthogonal(double cutWithin, double ortho);

  int  numOrthogonal() const;
  double cutWithin(int index) const;
  double orthoSpacing(int index) const;

protected:
  int numAllocated_;
  int numCutOrtho_;
  double* cutWithin_;
  double* ortho_;
};

// 5.5
class lefiAntennaModel {
public:
  lefiAntennaModel();
  ~lefiAntennaModel();

  lefiAntennaModel( const lefiAntennaModel& prev);
  void Init();
  void Destroy();

  void setAntennaModel(int oxide);
  void setAntennaAreaRatio(double value);
  void setAntennaCumAreaRatio(double value);
  void setAntennaAreaFactor(double value);
  void setAntennaSideAreaRatio(double value);
  void setAntennaCumSideAreaRatio(double value);
  void setAntennaSideAreaFactor(double value);
  void setAntennaValue(lefiAntennaEnum antennaType, double value);
  void setAntennaDUO(lefiAntennaEnum antennaType);
  void setAntennaPWL(lefiAntennaEnum antennaType, lefiAntennaPWL* pwl);
  void setAntennaCumRoutingPlusCut();             // 5.7
  void setAntennaGatePlusDiff(double value);      // 5.7
  void setAntennaAreaMinusDiff(double value);     // 5.7

  int hasAntennaAreaRatio() const;
  int hasAntennaDiffAreaRatio() const;
  int hasAntennaDiffAreaRatioPWL() const;
  int hasAntennaCumAreaRatio() const;
  int hasAntennaCumDiffAreaRatio() const;
  int hasAntennaCumDiffAreaRatioPWL() const;
  int hasAntennaAreaFactor() const;
  int hasAntennaAreaFactorDUO() const;
  int hasAntennaSideAreaRatio() const;
  int hasAntennaDiffSideAreaRatio() const;
  int hasAntennaDiffSideAreaRatioPWL() const;
  int hasAntennaCumSideAreaRatio() const;
  int hasAntennaCumDiffSideAreaRatio() const;
  int hasAntennaCumDiffSideAreaRatioPWL() const;
  int hasAntennaSideAreaFactor() const;
  int hasAntennaSideAreaFactorDUO() const;
  int hasAntennaCumRoutingPlusCut() const;        // 5.7
  int hasAntennaGatePlusDiff() const;             // 5.7
  int hasAntennaAreaMinusDiff() const;            // 5.7
  int hasAntennaAreaDiffReducePWL() const;        // 5.7

  char*  antennaOxide() const;
  double antennaAreaRatio() const;
  double antennaDiffAreaRatio() const;
  lefiAntennaPWL* antennaDiffAreaRatioPWL() const;
  double antennaCumAreaRatio() const;
  double antennaCumDiffAreaRatio() const;
  lefiAntennaPWL* antennaCumDiffAreaRatioPWL() const;
  double antennaAreaFactor() const;
  double antennaSideAreaRatio() const;
  double antennaDiffSideAreaRatio() const;
  lefiAntennaPWL* antennaDiffSideAreaRatioPWL() const;
  double antennaCumSideAreaRatio() const;
  double antennaCumDiffSideAreaRatio() const;
  lefiAntennaPWL* antennaCumDiffSideAreaRatioPWL() const;
  double antennaSideAreaFactor() const;
  double antennaGatePlusDiff() const;                  // 5.7
  double antennaAreaMinusDiff() const;                 // 5.7
  lefiAntennaPWL* antennaAreaDiffReducePWL() const;    // 5.7

protected:
  int hasAntennaAreaRatio_;
  int hasAntennaDiffAreaRatio_;
  int hasAntennaDiffAreaRatioPWL_;
  int hasAntennaCumAreaRatio_;
  int hasAntennaCumDiffAreaRatio_;
  int hasAntennaCumDiffAreaRatioPWL_;
  int hasAntennaAreaFactor_;
  int hasAntennaAreaFactorDUO_;
  int hasAntennaSideAreaRatio_;
  int hasAntennaDiffSideAreaRatio_;
  int hasAntennaDiffSideAreaRatioPWL_;
  int hasAntennaCumSideAreaRatio_;
  int hasAntennaCumDiffSideAreaRatio_;
  int hasAntennaCumDiffSideAreaRatioPWL_;
  int hasAntennaSideAreaFactor_;
  int hasAntennaSideAreaFactorDUO_;
  int hasAntennaCumRoutingPlusCut_;        // 5.7
  int hasAntennaGatePlusDiff_;             // 5.7
  int hasAntennaAreaMinusDiff_;            // 5.7

  char*  oxide_;
  double antennaAreaRatio_;
  double antennaDiffAreaRatio_;
  lefiAntennaPWL* antennaDiffAreaRatioPWL_;
  double antennaCumAreaRatio_;
  double antennaCumDiffAreaRatio_;
  lefiAntennaPWL* antennaCumDiffAreaRatioPWL_;
  double antennaAreaFactor_;
  double antennaSideAreaRatio_;
  double antennaDiffSideAreaRatio_;
  lefiAntennaPWL* antennaDiffSideAreaRatioPWL_;
  double antennaCumSideAreaRatio_;
  double antennaCumDiffSideAreaRatio_;
  lefiAntennaPWL* antennaCumDiffSideAreaRatioPWL_;
  double antennaSideAreaFactor_;
  double antennaGatePlusDiff_;                  // 5.7
  double antennaAreaMinusDiff_;                 // 5.7
  lefiAntennaPWL* antennaAreaDiffReducePWL_;    // 5.7
};

class lefiLayer {
public:
  lefiLayer();
  void Init();

  lefiLayer(const lefiLayer& prev);
  void Destroy();
  ~lefiLayer();

  void clear();
  void setName(const char* name); // calls clear to init
  void setType(const char* typ);
  void setPitch(double num);
  void setMask(int num);                           // 5.8
  void setPitchXY(double xdist, double ydist);     // 5.6
  void setOffset(double num);
  void setOffsetXY(double xdist, double ydist);    // 5.6
  void setWidth(double num);
  void setArea(double num);
  void setDiagPitch(double dist);                  // 5.6
  void setDiagPitchXY(double xdist, double ydist); // 5.6
  void setDiagWidth(double width);                 // 5.6
  void setDiagSpacing(double spacing);             // 5.6
  void setSpacingMin(double dist);
  void setSpacingName(const char* spacingName);    // for CUT layer
  void setSpacingLayerStack();                     // 5.7 for CUT layer
  void setSpacingAdjacent(int numCuts, double distance);  // 5.5for CUT layer
  void setSpacingRange(double left, double right);
  void setSpacingRangeUseLength();
  void setSpacingRangeInfluence(double infLength);
  void setSpacingRangeInfluenceRange(double min, double max);
  void setSpacingRangeRange(double min, double max);
  void setSpacingLength(double num);
  void setSpacingLengthRange(double min, double max);
  void setSpacingCenterToCenter();
  void setSpacingParallelOverlap();                            // 5.7
  void setSpacingArea(double cutArea);                         // 5.7
  void setSpacingEol(double width, double within);             // 5.7
  void setSpacingParSW(double space, double within);           // 5.7
  void setSpacingParTwoEdges();                                // 5.7
  void setSpacingAdjacentExcept();                             // 5.7
  void setSpacingSamenet();                                    // 5.7
  void setSpacingSamenetPGonly();                              // 5.7
  void setSpacingTableOrtho();                                 // 5.7
  void addSpacingTableOrthoWithin(double cutWithin, double ortho);  // 5.7
  void setMaxFloatingArea(double num);                         // 5.7
  void setArraySpacingLongArray();                             // 5.7
  void setArraySpacingWidth(double viaWidth);                  // 5.7
  void setArraySpacingCut(double cutSpacing);                  // 5.7
  void addArraySpacingArray(int aCuts, double aSpacing);       // 5.7
  void setSpacingNotchLength(double minNotchLength);           // 5.7
  void setSpacingEndOfNotchWidth (double endOfNotchWidth,
           double minNotchSpacing, double minNotchLength);     // 5.7
  void setDirection(const char* dir);
  void setResistance(double num);
  void setCapacitance(double num);
  void setHeight(double num);
  void setWireExtension(double num);
  void setThickness(double num);
  void setShrinkage(double num);
  void setCapMultiplier(double num);
  void setEdgeCap(double num);
  void setAntennaArea(double num);
  void setAntennaLength(double num);
  void setCurrentDensity(double num);
  void setCurrentPoint(double width, double current);
  void setResistancePoint(double width, double res);
  void setCapacitancePoint(double width, double cap);
  void addProp(const char* name, const char* value, const char type);
  void addNumProp(const char* name, const double d,
                  const char* value, const char type);
  void addAccurrentDensity(const char* type);
  void setAcOneEntry(double num);
  void addAcFrequency();
  void addAcCutarea();
  void addAcTableEntry();
  void addAcWidth();
  void addDccurrentDensity(const char* type);
  void setDcOneEntry(double num);
  void addDcTableEntry();
  void addDcWidth();
  void addDcCutarea();
  void addNumber(double num);
  void setMaxwidth(double width);                                   // 5.5
  void setMinwidth(double width);                                   // 5.5
  void addMinenclosedarea(double area);                             // 5.5
  void addMinenclosedareaWidth(double width);                       // 5.5
  void addMinimumcut(int cuts, double width);                       // 5.5
  void addMinimumcutWithin(double cutDistance);                     // 5.7
  void addMinimumcutConnect(const char* direction);                 // 5.5
  void addMinimumcutLengDis(double length, double distance);        // 5.5
  void addParellelLength(double length);                            // 5.5
  void addParellelSpacing(double width, double spacing);            // 5.5
  void addParellelWidth(double width);                              // 5.5
  void setProtrusion(double width, double length, double width2);   // 5.5

  // 5.6 - minstep switch to multiple and added more options
  void addMinstep(double distance);                                 // 5.5
  void addMinstepType(char* type);                                  // 5.6
  void addMinstepLengthsum(double maxLength);                       // 5.6
  void addMinstepMaxedges(int maxEdges);                            // 5.7
  void addMinstepMinAdjLength(double minAdjLength);                 // 5.7
  void addMinstepMinBetLength(double minBetLength);                 // 5.7
  void addMinstepXSameCorners();                                    // 5.7

  int  getNumber();     // this is for the parser internal use only

  // 5.5 SPACINGTABLE
  void addSpacingTable();
  void addSpParallelLength();
  void addSpParallelWidth(double width);
  void addSpParallelWidthSpacing();
  void setInfluence();
  void setSpTwoWidthsHasPRL(int hasPRL);
  void addSpInfluence(double width, double distance, double spacing);
  void addSpTwoWidths(double width, double runLength);              // 5.7
  

  // 5.6
  void addEnclosure(char* enclRule, double overhang1, double overhang2);
  void addEnclosureWidth(double minWidth); 
  void addEnclosureExceptEC(double cutWithin);       // 5.7
  void addEnclosureLength(double minLength);         // 5.7
  void addEnclosureExtraCut();                       // 5.7+
  void addPreferEnclosure(char* enclRule, double overhang1, double overhang2);
  void addPreferEnclosureWidth(double minWidth); 
  void setResPerCut(double value);
  void setDiagMinEdgeLength(double value);
  void setMinSize(lefiGeometries* geom);

  // 5.8
  // POLYROUTING, MIMCAP, TSV, PASSIVATION, NWELL
  void setLayerType(const char* lType) ; 

  int hasType() const ;
  int hasLayerType() const ;         // 5.8 - Some layers can be another types
                                     //  ROUTING can be POLYROUTING or MIMCAP
                                     //  CUT can be TSV or PASSIVATION
                                     //  MASTERSLICE can be NWELL
  int hasMask() const ;              // 5.8
  int hasPitch() const ;
  int hasXYPitch() const ;           // 5.6
  int hasOffset() const ;
  int hasXYOffset() const ;          // 5.6
  int hasWidth() const ;
  int hasArea() const ;
  int hasDiagPitch() const;          // 5.6
  int hasXYDiagPitch() const;        // 5.6
  int hasDiagWidth() const;          // 5.6
  int hasDiagSpacing() const;        // 5.6 
  int hasSpacingNumber() const ;
  int hasSpacingName(int index) const ;
  int hasSpacingLayerStack(int index) const ;            // 5.7
  int hasSpacingAdjacent(int index) const ;
  int hasSpacingCenterToCenter(int index) const ;
  int hasSpacingRange(int index) const ;
  int hasSpacingRangeUseLengthThreshold(int index) const;
  int hasSpacingRangeInfluence(int index) const;
  int hasSpacingRangeInfluenceRange(int index) const;
  int hasSpacingRangeRange(int index) const;
  int hasSpacingLengthThreshold(int index) const;
  int hasSpacingLengthThresholdRange(int index) const;
  int hasSpacingParallelOverlap(int index) const;        // 5.7
  int hasSpacingArea(int index) const;                   // 5.7
  int hasSpacingEndOfLine(int index) const;              // 5.7
  int hasSpacingParellelEdge(int index) const;           // 5.7
  int hasSpacingTwoEdges(int index) const;               // 5.7
  int hasSpacingAdjacentExcept(int index) const;         // 5.7
  int hasSpacingSamenet(int index) const;                // 5.7
  int hasSpacingSamenetPGonly(int index) const;          // 5.7
  int hasSpacingNotchLength(int index) const;            // 5.7
  int hasSpacingEndOfNotchWidth(int index) const;        // 5.7
  int hasDirection() const ;
  int hasResistance() const ;
  int hasResistanceArray() const ;
  int hasCapacitance() const ;
  int hasCapacitanceArray() const ;
  int hasHeight() const ;
  int hasThickness() const ;
  int hasWireExtension() const ;
  int hasShrinkage() const ;
  int hasCapMultiplier() const ;
  int hasEdgeCap() const ;
  int hasAntennaLength() const ;
  int hasAntennaArea() const ;
  int hasCurrentDensityPoint() const ;
  int hasCurrentDensityArray() const ;
  int hasAccurrentDensity() const;
  int hasDccurrentDensity() const;

  int numProps() const;
  const char*  propName(int index) const;
  const char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int    propIsNumber(int index) const;
  int    propIsString(int index) const;

  int numSpacing() const;

  char* name() const ;
  const char* type() const ;
  const char* layerType() const ;           // 5.8
  double pitch() const ;
  int    mask() const;                      // 5.8
  double pitchX() const ;                   // 5.6
  double pitchY() const ;                   // 5.6
  double offset() const ;
  double offsetX() const ;                  // 5.6
  double offsetY() const ;                  // 5.6
  double width() const ;
  double area() const ;
  double diagPitch() const ;                // 5.6
  double diagPitchX() const ;               // 5.6
  double diagPitchY() const ;               // 5.6
  double diagWidth() const ;                // 5.6 
  double diagSpacing() const ;              // 5.6 
  double spacing(int index) const ;
  char*  spacingName(int index) const ;     // for CUT layer
  int    spacingAdjacentCuts(int index) const ;   // 5.5 - for CUT layer
  double spacingAdjacentWithin(int index) const ; // 5.5 - for CUT layer
  double spacingArea(int index) const;            // 5.7 - for CUT layer
  double spacingRangeMin(int index) const ;
  double spacingRangeMax(int index) const ;
  double spacingRangeInfluence(int index) const ;
  double spacingRangeInfluenceMin(int index) const ;
  double spacingRangeInfluenceMax(int index) const ;
  double spacingRangeRangeMin(int index) const ;
  double spacingRangeRangeMax(int index) const ;
  double spacingLengthThreshold(int index) const;
  double spacingLengthThresholdRangeMin(int index) const;
  double spacingLengthThresholdRangeMax(int index) const;

  // 5.7 Spacing endofline
  double spacingEolWidth(int index) const;
  double spacingEolWithin(int index) const;
  double spacingParSpace(int index) const;
  double spacingParWithin(int index) const;

  // 5.7 Spacing Notch
  double spacingNotchLength(int index) const;
  double spacingEndOfNotchWidth(int index) const;
  double spacingEndOfNotchSpacing(int index) const;
  double spacingEndOfNotchLength(int index) const;

  // 5.5 Minimum cut rules
  int    numMinimumcut() const;
  int    minimumcut(int index) const;
  double minimumcutWidth(int index) const;
  int    hasMinimumcutWithin(int index) const;          // 5.7
  double minimumcutWithin(int index) const;             // 5.7
  int    hasMinimumcutConnection(int index) const;      // FROMABOVE|FROMBELOW
  const  char* minimumcutConnection(int index) const;   // FROMABOVE|FROMBELOW
  int    hasMinimumcutNumCuts(int index) const;
  double minimumcutLength(int index) const;
  double minimumcutDistance(int index) const;

  const char* direction() const ;
  double resistance() const ;
  double capacitance() const ;
  double height() const ;
  double wireExtension() const ;
  double thickness() const ;
  double shrinkage() const ;
  double capMultiplier() const ;
  double edgeCap() const ;
  double antennaLength() const ;
  double antennaArea() const ;
  double currentDensityPoint() const ;
  void currentDensityArray(int* numPoints,
       double** widths, double** current) const ;
  void capacitanceArray(int* numPoints,
       double** widths, double** resValues) const ;
  void resistanceArray(int* numPoints,
       double** widths, double** capValues) const ;

  int numAccurrentDensity() const;
  lefiLayerDensity* accurrent(int index) const;
  int numDccurrentDensity() const;
  lefiLayerDensity* dccurrent(int index) const;

  // 3/23/2000 - Wanda da Rosa.  The following are for 5.4 Antenna.
  //             Only 5.4 or 5.3 are allowed in a lef file, but not both
  void setAntennaAreaRatio(double value);
  void setAntennaCumAreaRatio(double value);
  void setAntennaAreaFactor(double value);
  void setAntennaSideAreaRatio(double value);
  void setAntennaCumSideAreaRatio(double value);
  void setAntennaSideAreaFactor(double value);
  void setAntennaValue(lefiAntennaEnum antennaType, double value);
  void setAntennaDUO(lefiAntennaEnum antennaType);
  void setAntennaPWL(lefiAntennaEnum antennaType, lefiAntennaPWL* pwl);
  void setAntennaCumRoutingPlusCut();             // 5.7
  void setAntennaGatePlusDiff(double value);      // 5.7
  void setAntennaAreaMinusDiff(double value);     // 5.7
  void addAntennaModel (int aOxide);  // 5.5

  // 5.5
  int numAntennaModel() const;
  lefiAntennaModel* antennaModel(int index) const;

  // The following is 8/21/01 5.4 enhancements 
  void setSlotWireWidth(double num);
  void setSlotWireLength(double num);
  void setSlotWidth(double num);
  void setSlotLength(double num);
  void setMaxAdjacentSlotSpacing(double num);
  void setMaxCoaxialSlotSpacing(double num);
  void setMaxEdgeSlotSpacing(double num);
  void setSplitWireWidth(double num);
  void setMinimumDensity(double num);
  void setMaximumDensity(double num);
  void setDensityCheckWindow(double length, double width);
  void setDensityCheckStep(double num);
  void setFillActiveSpacing(double num);

  int hasSlotWireWidth() const;
  int hasSlotWireLength() const;
  int hasSlotWidth() const;
  int hasSlotLength() const;
  int hasMaxAdjacentSlotSpacing() const;
  int hasMaxCoaxialSlotSpacing() const;
  int hasMaxEdgeSlotSpacing() const;
  int hasSplitWireWidth() const;
  int hasMinimumDensity() const;
  int hasMaximumDensity() const;
  int hasDensityCheckWindow() const;
  int hasDensityCheckStep() const;
  int hasFillActiveSpacing() const;
  int hasMaxwidth() const;                     // 5.5
  int hasMinwidth() const;                     // 5.5
  int hasMinstep() const;                      // 5.5
  int hasProtrusion() const;                   // 5.5

  double slotWireWidth() const;
  double slotWireLength() const;
  double slotWidth() const;
  double slotLength() const;
  double maxAdjacentSlotSpacing() const;
  double maxCoaxialSlotSpacing() const;
  double maxEdgeSlotSpacing() const;
  double splitWireWidth() const;
  double minimumDensity() const;
  double maximumDensity() const;
  double densityCheckWindowLength() const;
  double densityCheckWindowWidth() const;
  double densityCheckStep() const;
  double fillActiveSpacing() const;
  double maxwidth() const;                     // 5.5
  double minwidth() const;                     // 5.5
  double protrusionWidth1() const;             // 5.5
  double protrusionLength() const;             // 5.5
  double protrusionWidth2() const;             // 5.5

  int    numMinstep() const;                   // 5.6
  double minstep(int index) const;             // 5.5, 5.6 switch to multiple
  int    hasMinstepType(int index) const;      // 5.6
  char*  minstepType(int index) const;         // 5.6
  int    hasMinstepLengthsum(int index) const; // 5.6
  double minstepLengthsum(int index) const;    // 5.6
  int    hasMinstepMaxedges(int index) const;  // 5.7
  int    minstepMaxedges(int index) const;     // 5.7
  int    hasMinstepMinAdjLength(int index) const;  // 5.7
  double minstepMinAdjLength(int index) const;     // 5.7
  int    hasMinstepMinBetLength(int index) const;  // 5.7
  double minstepMinBetLength(int index) const;     // 5.7
  int    hasMinstepXSameCorners(int index) const;  // 5.7

  // 5.5 MINENCLOSEDAREA
  int    numMinenclosedarea() const;
  double minenclosedarea(int index) const;
  int    hasMinenclosedareaWidth(int index) const;
  double minenclosedareaWidth(int index) const;

  // 5.5 SPACINGTABLE FOR LAYER ROUTING
  int               numSpacingTable();
  lefiSpacingTable* spacingTable(int index);

  // 5.6 ENCLOSURE, PREFERENCLOSURE, RESISTANCEPERCUT & DIAGMINEDGELENGTH
  int    numEnclosure() const;
  int    hasEnclosureRule(int index) const;
  char*  enclosureRule(int index) ;
  double enclosureOverhang1(int index) const;
  double enclosureOverhang2(int index) const;
  int    hasEnclosureWidth(int index) const;
  double enclosureMinWidth(int index) const;
  int    hasEnclosureExceptExtraCut(int index) const;    // 5.7
  double enclosureExceptExtraCut(int index) const;       // 5.7
  int    hasEnclosureMinLength(int index) const;         // 5.7
  double enclosureMinLength(int index) const;            // 5.7
  int    numPreferEnclosure() const;
  int    hasPreferEnclosureRule(int index) const;
  char*  preferEnclosureRule(int index) ;
  double preferEnclosureOverhang1(int index) const;
  double preferEnclosureOverhang2(int index) const;
  int    hasPreferEnclosureWidth(int index) const;
  double preferEnclosureMinWidth(int index) const;
  int    hasResistancePerCut() const;
  double resistancePerCut() const;
  int    hasDiagMinEdgeLength() const;
  double diagMinEdgeLength() const;
  int    numMinSize() const;
  double minSizeWidth(int index) const;
  double minSizeLength(int index) const;

  // 5.7
  int    hasMaxFloatingArea() const; 
  double maxFloatingArea() const;
  int    hasArraySpacing() const;
  int    hasLongArray() const;
  int    hasViaWidth() const;
  double viaWidth() const;
  double cutSpacing() const;
  int    numArrayCuts() const;
  int    arrayCuts(int index) const;
  double arraySpacing(int index) const;
  int    hasSpacingTableOrtho() const; // SPACINGTABLE ORTHOGONAL FOR LAYER CUT
  lefiOrthogonal *orthogonal() const;

  void parse65nmRules();                  // 5.7
  void parseLEF58Layer();                 // 5.8
  int  need58PropsProcessing() const;     // 5.8

  // Debug print
  void print(FILE* f) const;

private:
  void parseSpacing(int index);
  void parseMaxFloating(int index);
  void parseArraySpacing(int index);
  void parseMinstep(int index);
  void parseAntennaCumRouting(int index);
  void parseAntennaGatePlus(int index);
  void parseAntennaAreaMinus(int index);
  void parseAntennaAreaDiff(int index);

  void parseLayerType(int index);         // 5.8
  void parseLayerEnclosure(int index);    // 5.8
  void parseLayerWidthTable(int indxe);   // 5.8

protected:
  char* name_;
  int nameSize_;
  char* type_;
  int typeSize_;
  char* layerType_;   // 5.8 - POLYROUTING, MIMCAP, TSV, PASSIVATION, NWELL

  int hasPitch_;
  int hasMask_;                       // 5.8 native
  int hasOffset_;
  int hasWidth_;            
  int hasArea_;
  int hasSpacing_;
  int hasDiagPitch_;                  // 5.6
  int hasDiagWidth_;                  // 5.6
  int hasDiagSpacing_;                // 5.6
  int* hasSpacingName_;               // 5.5
  int* hasSpacingLayerStack_;         // 5.7
  int* hasSpacingAdjacent_;           // 5.5
  int* hasSpacingRange_;              // pcr 409334
  int* hasSpacingUseLengthThreshold_; // pcr 282799, due to mult. spacing allow
  int* hasSpacingLengthThreshold_;    // pcr 409334
  int* hasSpacingCenterToCenter_;     // 5.6
  int* hasSpacingParallelOverlap_;    // 5.7
  int* hasSpacingCutArea_;            // 5.7
  int* hasSpacingEndOfLine_;          // 5.7
  int* hasSpacingParellelEdge_;       // 5.7
  int* hasSpacingTwoEdges_;           // 5.7
  int* hasSpacingAdjacentExcept_;     // 5.7
  int* hasSpacingSamenet_;            // 5.7
  int* hasSpacingSamenetPGonly_;      // 5.7
  int hasArraySpacing_;               // 5.7
  int hasDirection_;
  int hasResistance_;
  int hasCapacitance_;
  int hasHeight_;
  int hasWireExtension_;
  int hasThickness_;
  int hasShrinkage_;
  int hasCapMultiplier_;
  int hasEdgeCap_;
  int hasAntennaArea_;
  int hasAntennaLength_;
  int hasCurrentDensityPoint_;

  double currentDensity_;
  double pitchX_;                     // 5.6
  double pitchY_;                     // 5.6
  double offsetX_;                    // 5.6
  double offsetY_;                    // 5.6
  double diagPitchX_;                 // 5.6
  double diagPitchY_;                 // 5.6
  double diagWidth_;                  // 5.6
  double diagSpacing_;                // 5.6
  double width_;
  double area_;
  double wireExtension_;
  int numSpacings_;
  int spacingsAllocated_;
  int maskNumber_;                     // 5.8
  double* spacing_;          // for Cut & routing Layer, spacing is multiple
  char**  spacingName_;
  int*    spacingAdjacentCuts_;    // 5.5
  double* spacingAdjacentWithin_;  // 5.5
  double* spacingCutArea_;         // 5.7
  double* rangeMin_;         // pcr 282799 & 408930, due to mult spacing allow
  double* rangeMax_;         // pcr 282799 & 408930, due to mult spacing allow
  double* rangeInfluence_;   // pcr 282799 & 408930, due to mult spacing allow
  double* rangeInfluenceRangeMin_;          // pcr 388183 & 408930
  double* rangeInfluenceRangeMax_;          // pcr 388183 & 408930
  double* rangeRangeMin_;                   // pcr 408930
  double* rangeRangeMax_;                   // pcr 408930
  double* lengthThreshold_;                 // pcr 408930
  double* lengthThresholdRangeMin_;         // pcr 408930
  double* lengthThresholdRangeMax_;         // pcr 408930

  // 5.5
  int     numMinimumcut_;
  int     minimumcutAllocated_;
  int*    minimumcut_;                       // pcr 409334
  double* minimumcutWidth_;                  // pcr 409334
  int*    hasMinimumcutWithin_;              // 5.7
  double* minimumcutWithin_;                 // 5.7
  int*    hasMinimumcutConnection_;
  char**  minimumcutConnection_;
  int*    hasMinimumcutNumCuts_;
  double* minimumcutLength_;
  double* minimumcutDistance_;

  double  maxwidth_;                          // 5.5
  double  minwidth_;                          // 5.5
  int     numMinenclosedarea_;                // 5.5
  int     minenclosedareaAllocated_;          // 5.5
  double* minenclosedarea_;                   // 5.5
  double* minenclosedareaWidth_;              // 5.5
  double  protrusionWidth1_;                  // 5.5
  double  protrusionLength_;                  // 5.5
  double  protrusionWidth2_;                  // 5.5

  int     numMinstep_;                        // 5.6
  int     numMinstepAlloc_;                   // 5.6
  double* minstep_;                           // 5.6, switch to multiple
  char**  minstepType_;                       // INSIDECORNER|OUTSIDECORNER|STEP
  double* minstepLengthsum_; 
  int*    minstepMaxEdges_;                   // 5.7
  double* minstepMinAdjLength_;               // 5.7
  double* minstepMinBetLength_;               // 5.7
  int*    minstepXSameCorners_;               // 5.7

  char*  direction_;
  double resistance_;
  double capacitance_;
  double height_;
  double thickness_;
  double shrinkage_;
  double capMultiplier_;
  double edgeCap_;
  double antennaArea_;
  double antennaLength_;

  int numCurrentPoints_;
  int currentPointsAllocated_;
  double* currentWidths_;
  double* current_;

  int numCapacitancePoints_;
  int capacitancePointsAllocated_;
  double* capacitanceWidths_;
  double* capacitances_;

  int numResistancePoints_;
  int resistancePointsAllocated_;
  double* resistanceWidths_;
  double* resistances_;

  int numProps_;
  int propsAllocated_;
  char**  names_;
  char**  values_;
  double* dvalues_;
  char*   types_;                     // I: integer, R: real, S:string
                                      // Q: quotedstring
  int numAccurrents_;                 // number of ACCURRENTDENSITY
  int accurrentAllocated_;
  lefiLayerDensity** accurrents_;
  int numDccurrents_;                 // number of DCCURRENTDENSITY
  int dccurrentAllocated_;
  lefiLayerDensity** dccurrents_;
  int numNums_;
  int numAllocated_;
  double* nums_;

  // 3/23/2000 - Wanda da Rosa.  The following is for 5.4 ANTENNA.
  //             Either 5.4 or 5.3 are allowed, not both
  int hasAntennaAreaRatio_;
  int hasAntennaDiffAreaRatio_;
  int hasAntennaDiffAreaRatioPWL_;
  int hasAntennaCumAreaRatio_;
  int hasAntennaCumDiffAreaRatio_;
  int hasAntennaCumDiffAreaRatioPWL_;
  int hasAntennaAreaFactor_;
  int hasAntennaAreaFactorDUO_;
  int hasAntennaSideAreaRatio_;
  int hasAntennaDiffSideAreaRatio_;
  int hasAntennaDiffSideAreaRatioPWL_;
  int hasAntennaCumSideAreaRatio_;
  int hasAntennaCumDiffSideAreaRatio_;
  int hasAntennaCumDiffSideAreaRatioPWL_;
  int hasAntennaSideAreaFactor_;
  int hasAntennaSideAreaFactorDUO_;

  // 5.5 AntennaModel
  lefiAntennaModel* currentAntennaModel_;
  int numAntennaModel_;
  int antennaModelAllocated_;
  lefiAntennaModel** antennaModel_;

  // 8/29/2001 - Wanda da Rosa.  The following is for 5.4 enhancements.
  int hasSlotWireWidth_;
  int hasSlotWireLength_;
  int hasSlotWidth_;
  int hasSlotLength_;
  int hasMaxAdjacentSlotSpacing_;
  int hasMaxCoaxialSlotSpacing_;
  int hasMaxEdgeSlotSpacing_;
  int hasSplitWireWidth_;
  int hasMinimumDensity_;
  int hasMaximumDensity_;
  int hasDensityCheckWindow_;
  int hasDensityCheckStep_;
  int hasFillActiveSpacing_;
  int hasTwoWidthPRL_;

  double slotWireWidth_; 
  double slotWireLength_; 
  double slotWidth_; 
  double slotLength_; 
  double maxAdjacentSlotSpacing_; 
  double maxCoaxialSlotSpacing_; 
  double maxEdgeSlotSpacing_; 
  double splitWireWidth_; 
  double minimumDensity_; 
  double maximumDensity_; 
  double densityCheckWindowLength_; 
  double densityCheckWindowWidth_; 
  double densityCheckStep_; 
  double fillActiveSpacing_; 

  // 5.5 SPACINGTABLE
  int numSpacingTable_;
  int spacingTableAllocated_;
  lefiSpacingTable** spacingTable_;

  // 5.6
  int numEnclosure_;
  int enclosureAllocated_;
  char** enclosureRules_;
  double* overhang1_;
  double* overhang2_;
  double* encminWidth_;
  double* cutWithin_;
  double* minLength_;
  int numPreferEnclosure_;
  int preferEnclosureAllocated_;
  char** preferEnclosureRules_;
  double* preferOverhang1_;
  double* preferOverhang2_;
  double* preferMinWidth_;
  double  resPerCut_;
  double  diagMinEdgeLength_;
  int numMinSize_;
  double* minSizeWidth_;
  double* minSizeLength_;

  // 5.7
  double* eolWidth_;
  double* eolWithin_;
  double* parSpace_;
  double* parWithin_;
  double  maxArea_;
  int     hasLongArray_;
  double  viaWidth_;
  double  cutSpacing_;
  int     numArrayCuts_;
  int     arrayCutsAllocated_;
  int*    arrayCuts_;
  double* arraySpacings_;
  int     hasSpacingTableOrtho_;
  lefiOrthogonal* spacingTableOrtho_;
  double* notchLength_;
  double* endOfNotchWidth_;
  double* minNotchSpacing_;
  double* eonotchLength_;

  int     lef58WidthTableOrthoValues_;
  int     lef58WidthTableWrongDirValues_;
  double* lef58WidthTableOrtho_;
  double* lef58WidthTableWrongDir_;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
