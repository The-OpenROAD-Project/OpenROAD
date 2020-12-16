// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2013, Cadence Design Systems
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

#ifndef lefiVia_h
#define lefiVia_h

#include <stdio.h>
#include "lefiKRDefs.hpp"
#include "lefiMisc.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class lefiViaLayer {
public:
  lefiViaLayer();
  void Init();

  LEF_COPY_CONSTRUCTOR_H(lefiViaLayer);
  LEF_ASSIGN_OPERATOR_H(lefiViaLayer);

  void Destroy();
  ~lefiViaLayer();

  void setName(const char* name);
  void addRect(int mask,
               double xl, double yl, 
               double xh, double yh);
  void addPoly(int mask,
               lefiGeometries* geom);

  lefiViaLayer* clone();

  int numRects();
  char* name();
  double xl(int index);
  double yl(int index);
  double xh(int index);
  double yh(int index);
  int rectColorMask(int index);
  int polyColorMask(int index);

  int numPolygons();                                     // 5.6
  lefiGeomPolygon* getPolygon(int index) const;   // 5.6

protected:
  char* name_;
  int* rectColorMask_;
  int* polyColorMask_;
  int numRects_;
  int rectsAllocated_;
  double* xl_;
  double* yl_;
  double* xh_;
  double* yh_;

  int numPolys_;
  int polysAllocated_;
  lefiGeomPolygon** polygons_;

};

class lefiVia {
public:
  lefiVia();
  void Init();

  LEF_COPY_CONSTRUCTOR_H(lefiVia);
  void Destroy();
  ~lefiVia();

  void clear();

  // setName calls clear to init
  // deflt=0 no default specified
  // deflt=1 default specified in lef file
  void setName(const char* name, int viaType);

  void setResistance(double num);
  void addProp(const char* name, const char* value, const char type);
  void addNumProp(const char* name, double d, const char* value,
                  const char type);

  // orient=-1 means no orient was specified.
  void setForeign(const char* name, int hasPnt,
     double x, double y, int orient);
  void setTopOfStack();

  void addLayer(const char* name);
  void addRectToLayer(int    mask, 
                      double xl, double yl, 
                      double xh, double yh);
  void addPolyToLayer(int    mask,
                      lefiGeometries* geom);
  void bumpProps();

  void setViaRule(const char* viaRuleName, double xSize, double ySize,
                  const char* botLayer, const char* cutLayer,
                  const char* topLayer, double xCut, double yCut,
                  double xBotEnc, double yBotEnc, double xTopEnc,
                  double yTopEnc);                          // 5.6
  void setRowCol(int numRows, int numCols);                 // 5.6
  void setOrigin(double xOffset, double yOffset);           // 5.6
  void setOffset(double xBot, double yBot, double xTop, double yTop);   // 5.6
  void setPattern(const char* cutPattern);                  // 5.6

  // make a new one 
  lefiVia* clone();

  int hasDefault() const ;
  int hasGenerated() const ; // 5.6, this no longer in 5.6, should be removed
  int hasForeign() const ;
  int hasForeignPnt() const ;
  int hasForeignOrient() const ;
  int hasProperties() const ;
  int hasResistance() const ;
  int hasTopOfStack() const ;

  int numLayers() const;
  char* layerName(int layerNum) const;
  int numRects(int layerNum) const;
  double xl(int layerNum, int rectNum) const;
  double yl(int layerNum, int rectNum) const;
  double xh(int layerNum, int rectNum) const;
  double yh(int layerNum, int rectNum) const;
  int rectColorMask(int layerNum, int rectNum) const;
  int polyColorMask(int layerNum, int polyNum) const;
  int numPolygons(int layerNum) const;                                 // 5.6
  lefiGeomPolygon getPolygon(int layerNum, int polyNum) const;  // 5.6

  char* name() const ;
  double resistance() const ;

  // Given an index from 0 to numProperties()-1 return
  // information about that property.
  int numProperties() const ;
  char* propName(int index) const;
  char* propValue(int index) const;
  double propNumber(int index) const;
  char  propType(int index) const;
  int propIsNumber(int index) const;
  int propIsString(int index) const;
  char* foreign() const;
  double foreignX() const;
  double foreignY() const;
  int foreignOrient() const;
  char* foreignOrientStr() const;

  // 5.6 VIARULE inside a VIA
  int hasViaRule() const;
  const char* viaRuleName() const;
  double xCutSize() const;
  double yCutSize() const;
  const char* botMetalLayer() const;
  const char* cutLayer() const;
  const char* topMetalLayer() const;
  double xCutSpacing() const;
  double yCutSpacing() const;
  double xBotEnc() const;
  double yBotEnc() const;
  double xTopEnc() const;
  double yTopEnc() const;
  int hasRowCol() const;
  int numCutRows() const; 
  int numCutCols() const; 
  int hasOrigin() const;
  double xOffset() const; 
  double yOffset() const; 
  int hasOffset() const;
  double xBotOffset() const; 
  double yBotOffset() const; 
  double xTopOffset() const; 
  double yTopOffset() const; 
  int hasCutPattern() const;
  const char* cutPattern() const;

  // Debug print
  void print(FILE* f) const;

protected:
  char* name_;
  int nameSize_;

  int hasDefault_;
  int hasGenerated_;
  int hasResistance_;
  int hasForeignPnt_;
  int hasTopOfStack_;

  int numProps_;
  int propsAllocated_;
  char** propName_;
  // The prop value is stored in the propValue_ or the propDValue_.
  // If it is a string it is in propValue_.  If it is a number,
  // then propValue_ is NULL and it is stored in propDValue_;
  char** propValue_;
  double* propDValue_;
  char*   propType_;

  int numLayers_;
  int layersAllocated_;
  lefiViaLayer** layers_;

  double resistance_;

  char* foreign_;
  double foreignX_;
  double foreignY_;
  int foreignOrient_;

  char* viaRuleName_;          // 5.6
  double xSize_;               // 5.6
  double ySize_;               // 5.6
  char* botLayer_;             // 5.6
  char* cutLayer_;             // 5.6
  char* topLayer_;             // 5.6
  double xSpacing_;            // 5.6
  double ySpacing_;            // 5.6
  double xBotEnc_;             // 5.6
  double yBotEnc_;             // 5.6
  double xTopEnc_;             // 5.6
  double yTopEnc_;             // 5.6
  int numRows_;                // 5.6
  int numCols_;                // 5.6
  double xOffset_;             // 5.6
  double yOffset_;             // 5.6
  double xBotOs_;              // 5.6
  double yBotOs_;              // 5.6
  double xTopOs_;              // 5.6
  double yTopOs_;              // 5.6
  char* cutPattern_;           // 5.6
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
