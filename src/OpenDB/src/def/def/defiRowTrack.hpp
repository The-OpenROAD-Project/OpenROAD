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

#ifndef defiRowTrack_h
#define defiRowTrack_h

#include "defiKRDefs.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiRow{
public:

  defiRow(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiRow );
  DEF_ASSIGN_OPERATOR_H( defiRow );

  ~defiRow();
  void Destroy();
  void clear();

  void setup(const char* name, const char* macro,
	     double x, double y, int orient);
  void setDo(double x_num, double y_num,
	     double x_step, double y_step);
  void setHasDoStep();
  void addProperty(const char* name, const char* value, const char type);
  void addNumProperty(const char* name, const double d, 
                      const char* value, const char type);

  const char* name() const;
  const char* macro() const;
  double x() const;
  double y() const;
  int orient() const;
  const char* orientStr() const;
  int hasDo() const;               // 5.6, DO is optional
  double xNum() const;
  double yNum() const;
  int hasDoStep() const;           // 5.6, STEP is optional in DO
  double xStep() const;
  double yStep() const;

  int numProps() const;
  const char*  propName(int index) const;
  const char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int propIsNumber(int index) const;
  int propIsString(int index) const;

  void print(FILE* f) const;

protected:
  int nameLength_;
  char* name_;
  int macroLength_;
  char* macro_;
  double x_;
  double y_;
  double xNum_;
  double yNum_;
  int orient_;
  double xStep_;
  double yStep_;
  int    hasDo_;
  int    hasDoStep_;

  int numProps_;
  int propsAllocated_;
  char**  propNames_;
  char**  propValues_;
  double* propDValues_;
  char*   propTypes_;

  defrData *defData;
};



class defiTrack{
public:

  defiTrack(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiTrack );
  DEF_ASSIGN_OPERATOR_H( defiTrack );

  ~defiTrack();
  void Destroy();

  void setup(const char* macro);
  void setDo(double x, double x_num, double x_step);
  void addLayer(const char* layer);
  void addMask(int colorMask, int sameMask);

  const char* macro() const;
  double x() const;
  double xNum() const;
  double xStep() const;
  int numLayers() const;
  const char* layer(int index) const;
  int firstTrackMask() const;
  int sameMask() const;

  void print(FILE* f) const;

protected:
  int macroLength_;  // allocated size of macro_;
  char* macro_;
  double x_;
  double xNum_;
  double xStep_;
  int layersLength_;  // allocated size of layers_
  int numLayers_;  // number of places used in layers_
  char** layers_;
  int firstTrackMask_;
  int samemask_;

  defrData *defData;
};



class defiGcellGrid {
public:

  defiGcellGrid(defrData *data);
  void Init();
 
  DEF_COPY_CONSTRUCTOR_H( defiGcellGrid );
  DEF_ASSIGN_OPERATOR_H( defiGcellGrid );
 
  ~defiGcellGrid();
  void Destroy();

  void setup(const char* macro, int x, int xNum, double xStep);

  const char* macro() const;
  int x() const;
  int xNum() const;
  double xStep() const;

  void print(FILE* f) const;

protected:
  int macroLength_;
  char* macro_;
  int x_;
  int xNum_;
  double xStep_;

  defrData *defData;
};


END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
