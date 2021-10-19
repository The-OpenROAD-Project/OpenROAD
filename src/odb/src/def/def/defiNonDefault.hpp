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

#ifndef defiNonDefault_h
#define defiNonDefault_h

#include <stdio.h>
#include "defiKRDefs.hpp"
#include "defiMisc.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defiNonDefault {
public:
  defiNonDefault(defrData *data);
  void Init();

  void Destroy();
  ~defiNonDefault();

  void clear();
  void setName(const char* name);
  void setHardspacing();
  void addLayer(const char* name);
  void addWidth(double num);
  void addDiagWidth(double num);
  void addSpacing(double num);
  void addWireExt(double num);
  void addVia(const char* name);
  void addViaRule(const char* name);
  void addMinCuts(const char* name, int numCuts);
  void addProperty(const char* name, const char* value, const char type);
  void addNumProperty(const char* name, const double d,
                      const char* value, const char type);
  void end();

  const char* name() const;
  int   hasHardspacing() const;

  int         numProps() const;
  const char* propName(int index) const;
  const char* propValue(int index) const;
  double      propNumber(int index) const;
  char        propType(int index) const;
  int         propIsNumber(int index) const;
  int         propIsString(int index) const;
  
  // A non default rule can have one or more layers.
  // The layer information is kept in an array.
  int         numLayers() const ;
  const char* layerName(int index) const ;
  double      layerWidth(int index) const ;       // Will be obsoleted in 5.7
  int         layerWidthVal(int index) const ;
  int         hasLayerDiagWidth(int index) const;
  double      layerDiagWidth(int index) const;    // Will be obsoleted in 5.7
  int         layerDiagWidthVal(int index) const;
  int         hasLayerSpacing(int index) const ;
  double      layerSpacing(int index) const ;     // Will be obsoleted in 5.7
  int         layerSpacingVal(int index) const ;
  int         hasLayerWireExt(int index) const ;
  double      layerWireExt(int index) const ;     // Will be obsoleted in 5.7
  int         layerWireExtVal(int index) const ;
  int         numVias() const ;
  const char* viaName(int index) const ;
  int         numViaRules() const ;
  const char* viaRuleName(int index) const ;
  int         numMinCuts() const;
  const char* cutLayerName(int index) const;
  int         numCuts(int index) const;

  // Debug print
  void print(FILE* f) const;

protected:
  char*   name_;
  char    hardSpacing_;

  // Layer information
  int     numLayers_;
  int     layersAllocated_;
  char**  layerName_;
  double* width_;
  char*   hasDiagWidth_;
  double* diagWidth_;
  char*   hasSpacing_;
  double* spacing_;
  char*   hasWireExt_;
  double* wireExt_;

  int     numVias_;
  int     viasAllocated_;
  char**  viaNames_;
  int     numViaRules_;
  int     viaRulesAllocated_;
  char**  viaRuleNames_;
  int     numMinCuts_;
  int     minCutsAllocated_;
  char**  cutLayerName_;
  int*    numCuts_;

  int     numProps_;
  int     propsAllocated_;
  char**  names_;
  char**  values_;
  double* dvalues_;
  char*   types_;

  defrData *defData;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
