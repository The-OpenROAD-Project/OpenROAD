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

#ifndef defiFILL_h
#define defiFILL_h

#include <stdio.h>
#include "defiKRDefs.hpp"
#include "defiMisc.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiFill {
public:
  defiFill(defrData *data);
  void Init();

  void Destroy();
  ~defiFill();

  void clear();
  void clearPoly();
  void clearPts();

  void setLayer(const char* name);
  void setLayerOpc();                             // 5.7
  void addRect(int xl, int yl, int xh, int yh);
  void addPolygon(defiGeometries* geom);
  void setVia(const char* name);                  // 5.7
  void setViaOpc();                               // 5.7
  void addPts(defiGeometries* geom);              // 5.7

  int hasLayer() const;
  const char* layerName() const;
  int hasLayerOpc() const;                        // 5.7

  void setMask(int colorMask);			  // 5.8
  int layerMask() const;                          // 5.8
  int viaTopMask() const;			  // 5.8
  int viaCutMask() const;			  // 5.8
  int viaBottomMask() const;                      // 5.8

  int numRectangles() const;
  int xl(int index) const;
  int yl(int index) const;
  int xh(int index) const;
  int yh(int index) const;

  int numPolygons() const;                        // 5.6
  struct defiPoints getPolygon(int index) const;  // 5.6

  int hasVia() const;                             // 5.7
  const char* viaName() const;                    // 5.7
  int hasViaOpc() const;                          // 5.7

  int numViaPts() const;                          // 5.7
  struct defiPoints getViaPts(int index) const;   // 5.7

  void print(FILE* f) const;

protected:
  int   hasLayer_;
  char* layerName_;
  int   layerNameLength_;
  int   layerOpc_;                  // 5.7
  int   numRectangles_;
  int   rectsAllocated_;
  int*  xl_;
  int*  yl_;
  int*  xh_;
  int*  yh_;
  int   numPolys_;                  // 5.6
  int   polysAllocated_;            // 5.6
  struct defiPoints** polygons_;    // 5.6
  int   hasVia_;                    // 5.7
  char* viaName_;                   // 5.7
  int   viaNameLength_;             // 5.7
  int   viaOpc_;                    // 5.7
  int   numPts_;                    // 5.7
  int   ptsAllocated_;              // 5.7
  int   mask_;                      // 5.8
  struct defiPoints** viaPts_;      // 5.7

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
