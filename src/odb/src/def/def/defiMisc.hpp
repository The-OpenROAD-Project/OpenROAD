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

#ifndef defiMisc_h
#define defiMisc_h

#include <stdio.h>
#include "defiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

struct defiPoints {
  int numPoints;
  int* x;
  int* y;

  defiPoints();
  DEF_COPY_CONSTRUCTOR_H( defiPoints );
  DEF_ASSIGN_OPERATOR_H( defiPoints );
};

class defiGeometries {
public:
  defiGeometries(defrData *data);
  void Init();
  void Reset();

  void Destroy();
  ~defiGeometries();

  void startList(int x, int y);
  void addToList(int x, int y);

  int  numPoints() const;
  void points(int index, int* x, int* y) const;

protected:
  int numPoints_;
  int pointsAllocated_;
  int* x_;
  int* y_;

  defrData *defData;
};

class defiStyles {
public:
  defiStyles();
  void Init();

  void Destroy();
  ~defiStyles();

  void clear();

  void setStyle(int styleNum);
  void setPolygon(defiGeometries* geom);

  int style() const;
  struct defiPoints getPolygon() const;

  protected:
    int    styleNum_;
    struct defiPoints* polygon_;
    int    numPointAlloc_;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
