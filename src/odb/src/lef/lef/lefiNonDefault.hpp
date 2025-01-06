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
//  $Date: 2020/09/29 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifndef lefiNonDefault_h
#define lefiNonDefault_h

#include <cstdio>

#include "lefiKRDefs.hpp"
#include "lefiMisc.hpp"
#include "lefiVia.hpp"

BEGIN_LEF_PARSER_NAMESPACE

class lefiNonDefault
{
 public:
  lefiNonDefault();
  void Init();

  void Destroy();
  ~lefiNonDefault();

  void setName(const char* name);
  void addLayer(const char* name);
  void addWidth(double num);
  void addWireExtension(double num);
  void addSpacing(double num);
  void addSpacingRule(lefiSpacing* s);
  void addResistance(double num);
  void addCapacitance(double num);
  void addEdgeCap(double num);
  void addViaRule(lefiVia* v);
  void addDiagWidth(double num);  // 5.6
  void end();
  void clear();
  void addProp(const char* name, const char* value, const char type);
  void addNumProp(const char* name,
                  const double d,
                  const char* value,
                  const char type);
  void setHardspacing();                           // 5.6
  void addUseVia(const char* name);                // 5.6
  void addUseViaRule(const char* name);            // 5.6
  void addMinCuts(const char* name, int numCuts);  // 5.6

  const char* name() const;
  int hasHardspacing() const;  // 5.6

  int numProps() const;
  const char* propName(int index) const;
  const char* propValue(int index) const;
  double propNumber(int index) const;
  const char propType(int index) const;
  int propIsNumber(int index) const;
  int propIsString(int index) const;

  // A non default rule can have one or more layers.
  // The layer information is kept in an array.
  int numLayers() const;
  const char* layerName(int index) const;
  int hasLayerWidth(int index) const;
  double layerWidth(int index) const;
  int hasLayerSpacing(int index) const;
  double layerSpacing(int index) const;
  int hasLayerWireExtension(int index) const;
  double layerWireExtension(int index) const;
  int hasLayerResistance(int index) const;   // obsolete in 5.6
  double layerResistance(int index) const;   // obsolete in 5.6
  int hasLayerCapacitance(int index) const;  // obsolete in 5.6
  double layerCapacitance(int index) const;  // obsolete in 5.6
  int hasLayerEdgeCap(int index) const;      // obsolete in 5.6
  double layerEdgeCap(int index) const;      // obsolete in 5.6
  int hasLayerDiagWidth(int index) const;    // 5.6
  double layerDiagWidth(int index) const;    // 5.6

  // A non default rule can have one or more vias.
  // These routines return the via info.
  int numVias() const;
  lefiVia* viaRule(int index) const;

  // A non default rule can have one or more spacing rules.
  // These routines return the that info.
  int numSpacingRules() const;
  lefiSpacing* spacingRule(int index) const;

  int numUseVia() const;                      // 5.6
  const char* viaName(int index) const;       // 5.6
  int numUseViaRule() const;                  // 5.6
  const char* viaRuleName(int index) const;   // 5.6
  int numMinCuts() const;                     // 5.6
  const char* cutLayerName(int index) const;  // 5.6
  int numCuts(int index) const;               // 5.6

  // Debug print
  void print(FILE* f);

 protected:
  int nameSize_{0};
  char* name_{nullptr};

  // Layer information
  int numLayers_{0};
  int layersAllocated_{0};
  char** layerName_{nullptr};
  double* width_{nullptr};
  double* spacing_{nullptr};
  double* wireExtension_{nullptr};
  char* hasWidth_{nullptr};
  char* hasSpacing_{nullptr};
  char* hasWireExtension_{nullptr};

  // 5.4
  double* resistance_{nullptr};
  double* capacitance_{nullptr};
  double* edgeCap_{nullptr};
  char* hasResistance_{nullptr};
  char* hasCapacitance_{nullptr};
  char* hasEdgeCap_{nullptr};

  double* diagWidth_{nullptr};   // 5.6
  char* hasDiagWidth_{nullptr};  // 5.6

  int numVias_{0};
  int allocatedVias_{0};
  lefiVia** viaRules_{nullptr};

  int numSpacing_{0};
  int allocatedSpacing_{0};
  lefiSpacing** spacingRules_{nullptr};

  int hardSpacing_{0};              // 5.6
  int numUseVias_{0};               // 5.6
  int allocatedUseVias_{0};         // 5.6
  char** useViaName_{nullptr};      // 5.6
  int numUseViaRules_{0};           // 5.6
  int allocatedUseViaRules_{0};     // 5.6
  char** useViaRuleName_{nullptr};  // 5.6
  int numMinCuts_{0};               // 5.6
  int allocatedMinCuts_{0};         // 5.6
  char** cutLayerName_{nullptr};    // 5.6
  int* numCuts_{nullptr};           // 5.6

  int numProps_{0};
  int propsAllocated_{0};
  char** names_{nullptr};
  char** values_{nullptr};
  double* dvalues_{nullptr};
  char* types_{nullptr};
};

END_LEF_PARSER_NAMESPACE

#endif
