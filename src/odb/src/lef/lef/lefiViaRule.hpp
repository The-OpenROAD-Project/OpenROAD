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

#ifndef lefiViaRule_h
#define lefiViaRule_h

#include <stdio.h>
#include "lefiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class lefiViaRuleLayer {
public:
  lefiViaRuleLayer();
  void Init();

  void Destroy();
  ~lefiViaRuleLayer();
  void clearLayerOverhang();

  void setName(const char* name);
  void setHorizontal();
  void setVertical();
  void setEnclosure(double overhang1, double overhang2);     // 5.5
  void setWidth(double minW, double maxW);
  void setOverhang(double d);
  void setOverhangToEnclosure(double d);                     // 5.6
  void setMetalOverhang(double d);
  void setResistance(double d);
  void setSpacing(double x, double y);
  void setRect(double xl, double yl, double xh, double yh);

  int hasDirection() const ;
  int hasEnclosure() const ;                                 // 5.5
  int hasWidth() const ;
  int hasResistance() const ;
  int hasOverhang() const ;
  int hasMetalOverhang() const ;
  int hasSpacing() const ;
  int hasRect() const ;

  char* name() const ;
  int isHorizontal() const ;
  int isVertical() const ;
  double enclosureOverhang1() const;                        // 5.5
  double enclosureOverhang2() const;                        // 5.5
  double widthMin() const ;
  double widthMax() const ;
  double overhang() const ;
  double metalOverhang() const ;
  double resistance() const ;
  double spacingStepX() const ;
  double spacingStepY() const ;
  double xl() const ;
  double yl() const ;
  double xh() const ;
  double yh() const ;

  // Debug print
  void print(FILE* f) const;

protected:
  char* name_;
  char direction_;
  double overhang1_;                                       // 5.5
  double overhang2_;                                       // 5.5
  int hasWidth_;
  int hasResistance_;
  int hasOverhang_;
  int hasMetalOverhang_;
  int hasSpacing_;
  int hasRect_;
  double widthMin_;
  double widthMax_;
  double overhang_;
  double metalOverhang_;
  double resistance_;
  double spacingStepX_;
  double spacingStepY_;
  double xl_, yl_, xh_, yh_;
};

class lefiViaRule {
public:
  lefiViaRule();
  void Init();

  void clear();
  void clearLayerOverhang();

  void Destroy();
  ~lefiViaRule();

  void setGenerate();
  void setDefault();

  // This should clear out all the old stuff.
  void setName(const char* name);

  // Add one of possibly many via names
  void addViaName(const char* name);

  // These routines set a part of the active layer.
  void setRect(double xl, double yl, double xh, double yh);
  void setSpacing(double x, double y);
  void setWidth(double x, double y);
  void setResistance(double d);
  void setOverhang(double d);
  void setOverhangToEnclosure(double d);            // 5.6
  void setMetalOverhang(double d);
  void setVertical();
  void setHorizontal();
  void setEnclosure(double overhang1, double overhang2);
  void addProp(const char* name, const char* value, const char type);
  void addNumProp(const char* name, const double d,
                  const char* value, const char type);

  // This routine sets and creates the active layer.
  void setLayer(const char* name);

  int hasGenerate() const ;
  int hasDefault() const ;
  char* name() const ;

  // There are 2 or 3 layers in a rule.
  // numLayers() tells how many.
  // If a third layer exists then it is the cut layer.
  int numLayers() const ;
  lefiViaRuleLayer* layer(int index) const;

  int numVias() const ;
  char* viaName(int index) const ;

  int numProps() const;
  const char*  propName(int index) const;
  const char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int    propIsNumber(int index) const;
  int    propIsString(int index) const;

  // Debug print
  void print(FILE* f) const;

protected:
  char* name_;
  int nameSize_;
 
  int hasGenerate_;
  int hasDefault_;

  int numLayers_;
  lefiViaRuleLayer layers_[3];

  int numVias_;
  int viasAllocated_;
  char** vias_;

  int numProps_;
  int propsAllocated_;
  char**  names_;
  char**  values_;
  double* dvalues_;
  char*   types_;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
