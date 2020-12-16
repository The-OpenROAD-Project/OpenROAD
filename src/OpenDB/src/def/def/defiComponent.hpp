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

#ifndef defiComponent_h
#define defiComponent_h

#include <stdio.h>
#include "defiKRDefs.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

// Placement status for the component.
// Default is 0
#define DEFI_COMPONENT_UNPLACED 1
#define DEFI_COMPONENT_PLACED 2
#define DEFI_COMPONENT_FIXED 3
#define DEFI_COMPONENT_COVER 4


// Struct holds the data for componentMaskShiftLayers.
class defiComponentMaskShiftLayer {
public:
                defiComponentMaskShiftLayer();
                defiComponentMaskShiftLayer(defrData *data);
                ~defiComponentMaskShiftLayer();
    DEF_COPY_CONSTRUCTOR_H( defiComponentMaskShiftLayer );
    DEF_ASSIGN_OPERATOR_H( defiComponentMaskShiftLayer );
    void         Init();
    void         Destroy();
    void         addMaskShiftLayer(const char* layer);
    int          numMaskShiftLayers() const;
    void         bumpLayers(int size);
    void         clear();
    const  char* maskShiftLayer(int index) const;

protected:
    int          layersAllocated_;  // allocated size of layers_
    int          numLayers_;        // number of places used in layers_
    char**       layers_;

    defrData    *defData;
};


// Struct holds the data for one component.
class defiComponent {
public:
  defiComponent(defrData *defData);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiComponent );
  void Destroy();
  ~defiComponent();

  void IdAndName(const char* id, const char* name);
  void setGenerate(const char* genName, const char* macroName);
  void setPlacementStatus(int n);
  void setPlacementLocation(int x, int y, int orient = -1); // changed by Mgwoo
  void setRegionName(const char* name);
  void setRegionBounds(int xl, int yl, int xh, int yh);
  void setEEQ(const char* name);
  void addNet(const char* netName);
  void addProperty(const char* name, const char* value, const char type);
  void addNumProperty(const char* name, const double d,
                      const char* value, const char type);
  void reverseNetOrder();
  void setWeight(int w);
  void setMaskShift(const char* color);
  void setSource(const char* name);
  void setForeignName(const char* name);
  void setFori(const char* name);
  void setForeignLocation(int x, int y, int orient);
  void setHalo(int left, int bottom, int right, int top);   // 5.6
  void setHaloSoft();                                       // 5.7
  void setRouteHalo(int haloDist, const char* minLayer, const char* maxLayer);
                                                            // 5.7
  void clear();

  // For OA to modify the Id & Name
  void changeIdAndName(const char* id, const char* name);

  const char* id() const;
  const char* name() const;
  int placementStatus() const;
  int isUnplaced() const;
  int isPlaced() const;
  int isFixed() const;
  int isCover() const;
  int placementX() const;
  int placementY() const;
  int placementOrient() const;
  const char* placementOrientStr() const;
  int hasRegionName() const;
  int hasRegionBounds() const;
  int hasEEQ() const;
  int hasGenerate() const;
  int hasSource() const;
  int hasWeight() const;
  int weight() const;
  int maskShiftSize() const;
  int maskShift(int index) const;
  int hasNets() const;
  int numNets() const;
  const char* net(int index) const;
  const char* regionName() const;
  const char* source() const;
  const char* EEQ() const;
  const char* generateName() const;
  const char* macroName() const;
  int hasHalo() const;                     // 5.6
  int hasHaloSoft() const;                 // 5.7
  void haloEdges(int* left, int* bottom, int* right, int* top);  // 5.6 
  int hasRouteHalo() const;                // 5.7
  int haloDist() const;                    // 5.7
  const char* minLayer() const;            // 5.7
  const char* maxLayer() const;            // 5.7

  // Returns arrays for the ll and ur of the rectangles in the region.
  // The number of items in the arrays is given in size.
  void regionBounds(int*size, int** xl, int** yl, int** xh, int** yh) const;

  int hasForeignName() const;
  const char* foreignName() const;
  int foreignX() const;
  int foreignY() const;
  const char* foreignOri() const; // return the string value of the orient
  int foreignOrient() const;      // return the enum value of the orient
  int hasFori() const;

  int    numProps() const;
  char*  propName(int index) const;
  char*  propValue(int index) const;
  double propNumber(int index) const;
  char   propType(int index) const;
  int    propIsNumber(int index) const;
  int    propIsString(int index) const;


  // Debug printing
  void print(FILE* fout) const;

  void bumpId(int size);
  void bumpName(int size);
  void bumpRegionName(int size);
  void bumpEEQ(int size);
  void bumpNets(int size);
  void bumpForeignName(int size);
  void bumpMinLayer(int size);
  void bumpMaxLayer(int size);
  void bumpFori(int size);

protected:
  char* id_;            // instance id
  char* name_;          // name.
  int nameSize_;        // allocated size of name.
  int idSize_;          // allocated size of id.
  int ForiSize_;        // allocate size of foreign ori
  int status_;          // placement status
  char hasRegionName_;  // the file supplied a region name for this comp
  char hasEEQ_;         // the file supplied an eeq
  char hasGenerate_;    // the file supplied an generate name and macro name
  char hasWeight_;      // the file supplied a weight
  char hasFori_;        // the file supplied a foreign orig name
  int orient_;          // orientation
  int x_, y_;           // placement loc

  int numRects_;
  int rectsAllocated_;
  int* rectXl_;       // region points
  int* rectYl_;
  int* rectXh_;
  int* rectYh_;

  char* regionName_;    // name.
  int regionNameSize_;  // allocated size of region name

  char* EEQ_;
  int EEQSize_;         // allocated size of eeq

  int numNets_;         // number of net connections
  int netsAllocated_;   // allocated size of nets array
  char** nets_;         // net connections

  int weight_;
  int* maskShift_; 
  int maskShiftSize_;
  char* source_;
  char hasForeignName_; // the file supplied a foreign name
  char* foreignName_;   // name
  int foreignNameSize_; // allocate size of foreign name
  int Fx_, Fy_;         // foreign loc
  int Fori_;            // foreign ori
  int generateNameSize_;
  char* generateName_;
  int macroNameSize_;
  char* macroName_;

  int hasHalo_;
  int hasHaloSoft_;           // 5.7
  int leftHalo_;
  int bottomHalo_;
  int rightHalo_;
  int topHalo_;
  int haloDist_;              // 5.7
  int minLayerSize_;          // 5.7
  char* minLayer_;            // 5.7
  int maxLayerSize_;          // 5.7
  char* maxLayer_;            // 5.7

  int numProps_;
  int propsAllocated_;
  char**  names_;
  char**  values_;
  double* dvalues_;
  char*   types_;

  defrData *defData;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
