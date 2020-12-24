// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2016, Cadence Design Systems
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

#include <stdlib.h>
#include <string.h>
#include "defiComponent.hpp"
#include "defiDebug.hpp"
#include "lex.h"
#include "defiUtil.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

/*********************************************************
* class defiComponentMaskShiftLayer
**********************************************************/
defiComponentMaskShiftLayer::defiComponentMaskShiftLayer(defrData *data)
 : defData(data)
{
    Init();
}

defiComponentMaskShiftLayer::defiComponentMaskShiftLayer() {
    defData = NULL;
    layersAllocated_ = 0;
    numLayers_ = 0;
    layers_ = 0;
}

DEF_COPY_CONSTRUCTOR_C( defiComponentMaskShiftLayer ) {
    layersAllocated_ = 0;
    numLayers_ = 0;
    layers_ = 0;
    DEF_COPY_FUNC( layersAllocated_ );
    DEF_COPY_FUNC( numLayers_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( layers_ , numLayers_  );
    
}

DEF_ASSIGN_OPERATOR_C( defiComponentMaskShiftLayer ) {
    CHECK_SELF_ASSIGN
    layersAllocated_ = 0;
    numLayers_ = 0;
    layers_ = 0;
    DEF_COPY_FUNC( layersAllocated_ );
    DEF_COPY_FUNC( numLayers_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( layers_ , numLayers_ );
    return *this;
}

defiComponentMaskShiftLayer::~defiComponentMaskShiftLayer() {
    Destroy();
}

void defiComponentMaskShiftLayer::Init() {
    layersAllocated_ = 0;
    numLayers_ = 0;
    layers_ = 0;

    bumpLayers(16);
}

void defiComponentMaskShiftLayer::Destroy() {
    if (numLayers_) {
        for (int i = 0; i < numLayers_; i++) {
            if (layers_[i]) {
                free(layers_[i]);
            }
        }
        free((char*)(layers_));
    } else {
        if (layersAllocated_) {
            free((char*)(layers_));
        }
    }
    layersAllocated_ = 0;
    numLayers_ = 0;
    layers_ = 0;
}

void defiComponentMaskShiftLayer::addMaskShiftLayer(const char* layer) {
    int len = strlen(layer) + 1;
    if (numLayers_ == layersAllocated_)
        bumpLayers(numLayers_ * 2);
    layers_[numLayers_] = (char*)malloc(len);
    strcpy(layers_[numLayers_], defData->DEFCASE(layer));
    (numLayers_)++;
}

void defiComponentMaskShiftLayer::bumpLayers(int size) {
    int i;
    char** newLayers = (char**)malloc(sizeof(char*)* size);
    for (i = 0; i < numLayers_; i++) {
        newLayers[i] = layers_[i];
    }
    if (layers_) {
        free((char*)(layers_));
    }
    layers_ = newLayers;
    layersAllocated_ = size;
}

void defiComponentMaskShiftLayer::clear() {
    for (int i = 0; i < numLayers_; i++) {
        free(layers_[i]);
    }
    numLayers_ = 0;
}

int defiComponentMaskShiftLayer::numMaskShiftLayers() const {
    return numLayers_;
}

const char* defiComponentMaskShiftLayer::maskShiftLayer(int index) const {
    if (index >= 0 && index < numLayers_) {
        return layers_[index];
    }

    return 0;
}


/*********************************************************
* class defiComponent
**********************************************************/
defiComponent::defiComponent(defrData *data) 
 : defData(data)
{
  Init();
}


void defiComponent::Init() {
  id_ = 0;
  name_ = 0;
  regionName_ = 0;
  foreignName_ = 0;
  Fori_ = 0;
  EEQ_ = 0;
  generateName_ = 0;
  macroName_ = 0;
  generateNameSize_ = 0;
  maskShiftSize_ = 0;
  maskShift_ = 0;
  macroNameSize_ = 0;
  minLayerSize_ = 0;
  maxLayerSize_ = 0;
  minLayer_ = 0;
  maxLayer_ = 0;
  nets_ = 0;
  source_ = 0;
  numNets_ = 0;
  bumpName(16);
  bumpId(16);
  bumpRegionName(16);
  bumpEEQ(16);
  bumpNets(16);
  bumpForeignName(16);
  bumpMinLayer(16);
  bumpMaxLayer(16);
  numProps_ = 0;
  propsAllocated_ = 2;
  orient_ = 0;
  names_ = (char**)malloc(sizeof(char*)* 2);
  values_ = (char**)malloc(sizeof(char*)* 2);
  dvalues_ = (double*)malloc(sizeof(double)* 2);
  types_ = (char*)malloc(sizeof(char)* 2);
  clear();

  numRects_ = 0;
  rectsAllocated_ = 1;
  rectXl_ = (int*)malloc(sizeof(int)*1);
  rectYl_ = (int*)malloc(sizeof(int)*1);
  rectXh_ = (int*)malloc(sizeof(int)*1);
  rectYh_ = (int*)malloc(sizeof(int)*1);
}

DEF_COPY_CONSTRUCTOR_C( defiComponent ) 
{
    defData = NULL;
    this->Init();

    DEF_COPY_FUNC( idSize_ );
    DEF_COPY_FUNC( nameSize_ );
//    printf("nameSize_:  %d\n", nameSize_);
//    fflush(stdout);
    DEF_MALLOC_FUNC( id_, char, sizeof(char) * (strlen(prev.id_)+1));
    DEF_MALLOC_FUNC( name_, char, sizeof(char) * (strlen(prev.name_)+1));
    DEF_COPY_FUNC( ForiSize_ );
    DEF_COPY_FUNC( status_ );
    DEF_COPY_FUNC( hasRegionName_ );
    DEF_COPY_FUNC( hasEEQ_ );
    DEF_COPY_FUNC( hasGenerate_ );
    DEF_COPY_FUNC( hasWeight_ );
    DEF_COPY_FUNC( hasFori_ );
    DEF_COPY_FUNC( orient_ );
    DEF_COPY_FUNC( x_ );
    DEF_COPY_FUNC( y_ );
    DEF_COPY_FUNC( numRects_ );
    DEF_COPY_FUNC( rectsAllocated_ );
    DEF_MALLOC_FUNC( rectXl_, int, sizeof(int) * numRects_);
    DEF_MALLOC_FUNC( rectYl_, int, sizeof(int) * numRects_);
    DEF_MALLOC_FUNC( rectXh_, int, sizeof(int) * numRects_);
    DEF_MALLOC_FUNC( rectYh_, int, sizeof(int) * numRects_);
    
    DEF_COPY_FUNC( regionNameSize_ );
    DEF_MALLOC_FUNC( regionName_, char, sizeof(char) * (regionNameSize_));
    DEF_COPY_FUNC( EEQSize_ );
    DEF_MALLOC_FUNC( EEQ_, char, sizeof(char) * (EEQSize_));
    DEF_COPY_FUNC( numNets_ );
    DEF_COPY_FUNC( netsAllocated_ );

    DEF_MALLOC_FUNC_FOR_2D_STR( nets_, numNets_ );

    DEF_COPY_FUNC( weight_ );
    DEF_COPY_FUNC( maskShiftSize_ );
    DEF_MALLOC_FUNC( maskShift_, int, sizeof(int) * maskShiftSize_);

    DEF_MALLOC_FUNC( source_, char, sizeof(char) * (strlen(prev.source_) +1));
    DEF_COPY_FUNC( hasForeignName_ );
    DEF_COPY_FUNC( foreignNameSize_ );
    DEF_MALLOC_FUNC( foreignName_, char, sizeof(char) * foreignNameSize_);
    DEF_COPY_FUNC( Fx_ );
    DEF_COPY_FUNC( Fy_ );
    DEF_COPY_FUNC( Fori_ );
    DEF_COPY_FUNC( generateNameSize_ );
    DEF_MALLOC_FUNC( generateName_, char, sizeof(char) * (generateNameSize_));
    DEF_COPY_FUNC( macroNameSize_ );
    DEF_MALLOC_FUNC( macroName_, char, sizeof(char) * (macroNameSize_));
    DEF_COPY_FUNC( hasHalo_ );
    DEF_COPY_FUNC( hasHaloSoft_ );
    DEF_COPY_FUNC( leftHalo_ );
    DEF_COPY_FUNC( bottomHalo_ );
    DEF_COPY_FUNC( rightHalo_ );
    DEF_COPY_FUNC( topHalo_ );
    DEF_COPY_FUNC( haloDist_ );
    DEF_COPY_FUNC( minLayerSize_ );
    DEF_MALLOC_FUNC( minLayer_, char, sizeof(char) * (minLayerSize_));
    DEF_COPY_FUNC( maxLayerSize_ );
    DEF_MALLOC_FUNC( maxLayer_, char, sizeof(char) * (maxLayerSize_));
    DEF_COPY_FUNC( numProps_ );
    DEF_COPY_FUNC( propsAllocated_ );
    DEF_MALLOC_FUNC_FOR_2D_STR( names_, numProps_);
    DEF_MALLOC_FUNC_FOR_2D_STR( values_, numProps_);
    DEF_MALLOC_FUNC( dvalues_, double, sizeof(double) * numProps_);
    DEF_MALLOC_FUNC( types_, char, sizeof(char) * numProps_);
}

void defiComponent::Destroy() {
  free(name_);
  free(regionName_);
  free(id_);
  free(EEQ_);
  free(minLayer_);
  free(maxLayer_);
  free((char*)(nets_));
  netsAllocated_ = 0;      // avoid freeing again later
  if (source_) free(source_);
  if (foreignName_) free(foreignName_);
  if (generateName_) free(generateName_);
  if (macroName_) free(macroName_);
  if (netsAllocated_) free((char*)(nets_));
  free((char*)(maskShift_));
  free((char*)(names_));
  free((char*)(values_));
  free((char*)(dvalues_));
  free((char*)(types_));
  free((char*)(rectXl_));
  free((char*)(rectYl_));
  free((char*)(rectXh_));
  free((char*)(rectYh_));
}


defiComponent::~defiComponent() {
  Destroy();
}


void defiComponent::IdAndName(const char* id, const char* name) {
  int len;

  clear();

  if ((len = strlen(id)+1) > idSize_)
    bumpId(len);
  strcpy(id_, defData->DEFCASE(id));

  if ((len = strlen(name)+1) > nameSize_)
    bumpName(len);
  strcpy(name_, defData->DEFCASE(name));
}


const char* defiComponent::source() const {
  return source_;
}


int defiComponent::weight() const {
  return weight_;
}


void defiComponent::setWeight(int w) {
  weight_ = w;
  hasWeight_ = 1;
}

int defiComponent::maskShift(int index) const {
    if (index < 0 || index >= maskShiftSize_) {
        defiError(1, 0, "bad index for component maskShift", defData);
        return 0;
    }

    return maskShift_[index];
}

void defiComponent::setMaskShift(const char *shiftMask) {
    int shiftMaskLength = strlen(shiftMask);

    maskShift_ = (int*)malloc(sizeof(int)* shiftMaskLength);
    maskShiftSize_ = shiftMaskLength;

    for (int i = 0; i < shiftMaskLength; i++) {
        int curShift = shiftMask[i] - '0';

        // Strip possible error data.
        if (curShift > 9 || curShift < 0) {
            curShift = 0;
        }

        maskShift_[shiftMaskLength - i - 1] = curShift;
    } 
}

void defiComponent::setGenerate(const char* newName, const char* macroName) {
  int len = strlen(newName) + 1;

  if (generateNameSize_ < len) {
    if (generateName_) free(generateName_);
    generateName_ = (char*)malloc(len);
    generateNameSize_ = len;
  }
  strcpy(generateName_, defData->DEFCASE(newName));

  len = strlen(macroName) + 1;
  if (macroNameSize_ < len) {
    if (macroName_) free(macroName_);
    macroName_ = (char*)malloc(len);
    macroNameSize_ = len;
  }
  strcpy(macroName_, defData->DEFCASE(macroName));

  hasGenerate_ = 1;  // Ying Tan fix at 20010918
}


void defiComponent::setSource(const char* name) {
  int len = strlen(name) + 1;
  source_ = (char*)malloc(len);
  strcpy(source_, defData->DEFCASE(name));
}


void defiComponent::setRegionName(const char* name) {
  int len;

  if ((len = strlen(name)+1) > regionNameSize_)
    bumpRegionName(len);
  strcpy(regionName_, defData->DEFCASE(name));
  hasRegionName_ = 1;
}


void defiComponent::setEEQ(const char* name) {
  int len;

  if ((len = strlen(name)+1) > EEQSize_)
    bumpEEQ(len);
  strcpy(EEQ_, defData->DEFCASE(name));
  hasEEQ_ = 1;
}


void defiComponent::setPlacementStatus(int n) {
  status_= n;
}


void defiComponent::setPlacementLocation(int x, int y, int orient) {
  x_ = x;
  y_ = y;

  if( orient != -1 ) { // mgwoo
      orient_ = orient;
  }
}


void defiComponent::setRegionBounds(int xl, int yl, int xh, int yh) {
  int i;
  i = numRects_;
  if (i == rectsAllocated_) {
    int max = rectsAllocated_ * 2;
    int* nxl = (int*)malloc(sizeof(int)*max);
    int* nyl = (int*)malloc(sizeof(int)*max);
    int* nxh = (int*)malloc(sizeof(int)*max);
    int* nyh = (int*)malloc(sizeof(int)*max);
    for (i = 0; i < numRects_; i++) {
      nxl[i] = rectXl_[i];
      nyl[i] = rectYl_[i];
      nxh[i] = rectXh_[i];
      nyh[i] = rectYh_[i];
    }
    free((char*)(rectXl_));
    free((char*)(rectYl_));
    free((char*)(rectXh_));
    free((char*)(rectYh_));
    rectXl_ = nxl;
    rectYl_ = nyl;
    rectXh_ = nxh;
    rectYh_ = nyh;
    rectsAllocated_ = max;
  }
  rectXl_[i] = xl;
  rectYl_[i] = yl;
  rectXh_[i] = xh;
  rectYh_[i] = yh;
  numRects_ += 1;
}


// 5.6
void defiComponent::setHalo(int left, int bottom, int right, int top) {
  hasHalo_ = 1;
  leftHalo_ = left; 
  bottomHalo_ = bottom; 
  rightHalo_ = right; 
  topHalo_ = top; 
}

// 5.7
void defiComponent::setHaloSoft() {
  hasHaloSoft_ = 1;
}

// 5.7
void defiComponent::setRouteHalo(int haloDist, const char* minLayer,
                                 const char* maxLayer) {
  int len;

  haloDist_ = haloDist;
  if ((len = strlen(minLayer)+1) > minLayerSize_)
    bumpMinLayer(len);
  strcpy(minLayer_, defData->DEFCASE(minLayer));
  if ((len = strlen(maxLayer)+1) > maxLayerSize_)
    bumpMaxLayer(len);
  strcpy(maxLayer_, defData->DEFCASE(maxLayer));
}

void defiComponent::changeIdAndName(const char* id, const char* name) {
  int len;

  if ((len = strlen(id)+1) > idSize_)
    bumpId(len);
  strcpy(id_, defData->DEFCASE(id));

  if ((len = strlen(name)+1) > nameSize_)
    bumpName(len);
  strcpy(name_, defData->DEFCASE(name));
}


const char* defiComponent::id() const {
  return id_;
}


const char* defiComponent::name() const {
  return name_;
}


int defiComponent::placementStatus() const {
  return status_;
}


int defiComponent::placementX() const {
  return x_;
}


int defiComponent::placementY() const {
  return y_;
}


int defiComponent::placementOrient() const {
  return orient_;
}


const char* defiComponent::placementOrientStr() const {
  return (defiOrientStr(orient_));
}


const char* defiComponent::regionName() const {
  return regionName_;
}


const char* defiComponent::EEQ() const {
  return EEQ_;
}


const char* defiComponent::generateName() const {
  return generateName_;
}


const char* defiComponent::macroName() const {
  return macroName_;
}


void defiComponent::regionBounds(int* size,
	  int** xl, int** yl, int** xh, int** yh) const {
  *size = numRects_;
  *xl = rectXl_;
  *yl = rectYl_;
  *xh = rectXh_;
  *yh = rectYh_;
}


void defiComponent::bumpId(int size) {
  if (id_) free(id_);
  id_ = (char*)malloc(size);
  idSize_ = size;
  *(id_) = '\0';
}


void defiComponent::bumpName(int size) {
  if (name_) free(name_);
  name_ = (char*)malloc(size);
  nameSize_ = size;
  *(name_) = '\0';
}


void defiComponent::bumpRegionName(int size) {
  if (regionName_) free(regionName_);
  regionName_ = (char*)malloc(size);
  regionNameSize_ = size;
  *(regionName_) = '\0';
}


void defiComponent::bumpEEQ(int size) {
  if (EEQ_) free(EEQ_);
  EEQ_ = (char*)malloc(size);
  EEQSize_ = size;
  *(EEQ_) = '\0';
}


void defiComponent::bumpMinLayer(int size) {
  if (minLayer_) free(minLayer_);
  minLayer_ = (char*)malloc(size);
  minLayerSize_ = size;
  *(minLayer_) = '\0';
}


void defiComponent::bumpMaxLayer(int size) {
  if (maxLayer_) free(maxLayer_);
  maxLayer_ = (char*)malloc(size);
  maxLayerSize_ = size;
  *(maxLayer_) = '\0';
}

void defiComponent::clear() {
  int i;

  if (id_)
     *(id_) = '\0';
  if (name_)
     *(name_) = '\0';
  if (regionName_)
     *(regionName_) = '\0';
  if (foreignName_)
     *(foreignName_) = '\0';
  if (EEQ_)
     *(EEQ_) = '\0';
  if (minLayer_)
     *(minLayer_) = '\0';
  if (maxLayer_)
     *(maxLayer_) = '\0';
  Fori_ = 0;
  status_ = 0;
  hasRegionName_ = 0;
  hasForeignName_ = 0;
  hasFori_ = 0;
  hasEEQ_ = 0;
  hasWeight_ = 0;
  hasGenerate_ = 0;
  if (maskShiftSize_) {
     free((int*)(maskShift_));
  }
  maskShift_ = 0;
  maskShiftSize_ = 0;
  weight_ = 0;
  if (source_) free(source_);
  for (i = 0; i < numNets_; i++) {
    free(nets_[i]);
  }
  numNets_ = 0;
  source_ = 0;
  hasHalo_ = 0;
  hasHaloSoft_ = 0;
  haloDist_ = 0;
  leftHalo_ = 0;
  bottomHalo_ = 0;
  rightHalo_ = 0;
  topHalo_ = 0;
  for (i = 0; i < numProps_; i++) {
    free(names_[i]);
    free(values_[i]);
    dvalues_[i] = 0;
  }
  numProps_ = 0;
  numRects_ = 0;
}


int defiComponent::isUnplaced() const {
  return status_ == DEFI_COMPONENT_UNPLACED ? 1 : 0 ;
}


int defiComponent::isPlaced() const {
  return status_ == DEFI_COMPONENT_PLACED ? 1 : 0 ;
}


int defiComponent::isFixed() const {
  return status_ == DEFI_COMPONENT_FIXED ? 1 : 0 ;
}


int defiComponent::isCover() const {
  return status_ == DEFI_COMPONENT_COVER ? 1 : 0 ;
}


void defiComponent::print(FILE* fout) const {
  fprintf(fout, "Component id '%s' name '%s'",
      id(),
      name());
  if (isPlaced()) {
    fprintf(fout, " Placed at %d,%d orient %s",
    placementX(),
    placementY(),
    placementOrientStr());
  }
  if (isFixed()) {
    fprintf(fout, " Fixed at %d,%d orient %s",
    placementX(),
    placementY(),
    placementOrientStr());
  }
  if (isCover()) {
    fprintf(fout, " Cover at %d,%d orient %s",
    placementX(),
    placementY(),
    placementOrientStr());
  }
  fprintf(fout, "\n");

  if (hasGenerate()) {
    fprintf(fout, "  generate %s %s\n", generateName(),
    macroName());
  }
  if (hasWeight()) {
    fprintf(fout, "  weight %d\n", weight());
  }
  if (maskShiftSize()) {
      fprintf(fout, "  maskShift ");

      for (int i = 0; i < maskShiftSize(); i++) {
        fprintf(fout, " %d", maskShift(i));
      }
      fprintf(fout, "\n");
  }
  if (hasSource()) {
    fprintf(fout, "  source '%s'\n", source());
  }
  if (hasEEQ()) {
    fprintf(fout, "  EEQ '%s'\n", EEQ());
  }

  if (hasRegionName()) {
    fprintf(fout, "  Region '%s'\n", regionName());
  }
  if (hasRegionBounds()) {
    int size;
    int *xl, *yl, *xh, *yh;
    int j;
    regionBounds(&size, &xl, &yl, &xh, &yh);
    for (j = 0; j < size; j++)
      fprintf(fout, "  Region bounds %d,%d %d,%d\n", xl[j], yl[j], xh[j], yh[j]);
  }
  if (hasNets()) {
    int i;
    fprintf(fout, " Net connections:\n");
    for (i = 0; i < numNets(); i++) {
      fprintf(fout, "  '%s'\n", net(i));
    }
  }
}


int defiComponent::hasRegionName() const {
  return (int)(hasRegionName_);
}


int defiComponent::hasGenerate() const {
  return (int)(hasGenerate_);
}


int defiComponent::hasWeight() const {
  return (int)(hasWeight_);
}

int defiComponent::maskShiftSize() const {
    return maskShiftSize_;
}

int defiComponent::hasSource() const {
  return source_ ? 1 : 0;
}


int defiComponent::hasRegionBounds() const {
  return numRects_ ? 1 : 0 ;
}


int defiComponent::hasEEQ() const {
  return (int)(hasEEQ_);
}


int defiComponent::hasNets() const {
  return numNets_ ? 1 : 0;
}


int defiComponent::numNets() const {
  return numNets_;
}


// 5.6
int defiComponent::hasHalo() const {
  return hasHalo_;
}


// 5.7
int defiComponent::hasHaloSoft() const {
  return hasHaloSoft_;
}


// 5.7
int defiComponent::hasRouteHalo() const {
  return haloDist_;
}

// 5.7
int defiComponent::haloDist() const {
  return haloDist_;
}

// 5.7
const char* defiComponent::minLayer() const {
  return minLayer_;
}

// 5.7
const char* defiComponent::maxLayer() const {
  return maxLayer_;
}

void defiComponent::haloEdges(int* left, int* bottom, int* right, int* top) {
  *left = leftHalo_;
  *bottom = bottomHalo_;
  *right = rightHalo_;
  *top = topHalo_;
}

void defiComponent::reverseNetOrder() {
  // Reverse the order of the items in the nets array.
  int one = 0;
  int two = numNets_ - 1;
  char* t;
  while (one < two) {
    t = nets_[one];
    nets_[one] = nets_[two];
    nets_[two] = t;
    one++;
    two--;
  }
}


char* defiComponent::propName(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return names_[index];
}


char* defiComponent::propValue(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return values_[index];
}


double defiComponent::propNumber(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return dvalues_[index];
}


char defiComponent::propType(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return types_[index];
}


int defiComponent::propIsNumber(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return dvalues_[index] ? 1 : 0;
}

int defiComponent::propIsString(int index) const {
  if (index < 0 || index >= numProps_) {
    defiError(1, 0, "bad index for component property", defData);
    return 0;
  }
  return dvalues_[index] ? 0 : 1;
}

int defiComponent::numProps() const {
  return numProps_;
}


void defiComponent::addProperty(const char* name, const char* value,
                                const char type) {
  int len = strlen(name) + 1;
  if (numProps_ == propsAllocated_) {
    int i;
    char**  nn;
    char**  nv;
    double* nd;
    char*   nt;

    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = names_[i];
      nv[i] = values_[i];
      nd[i] = dvalues_[i];
      nt[i] = types_[i];
    }
    free((char*)(names_));
    free((char*)(values_));
    free((char*)(dvalues_));
    free((char*)(types_));
    names_ = nn;
    values_ = nv;
    dvalues_ = nd;
    types_ = nt;
  }
  names_[numProps_] = (char*)malloc(len);
  strcpy(names_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  values_[numProps_] = (char*)malloc(len);
  strcpy(values_[numProps_], defData->DEFCASE(value));
  dvalues_[numProps_] = 0;
  types_[numProps_] = type;
  numProps_ += 1;
}


void defiComponent::addNumProperty(const char* name, const double d,
                                   const char* value, const char type) {
  int len = strlen(name) + 1;
  if (numProps_ == propsAllocated_) {
    int i;
    char**  nn;
    char**  nv;
    double* nd;
    char*   nt;

    propsAllocated_ *= 2;
    nn = (char**)malloc(sizeof(char*)*propsAllocated_);
    nv = (char**)malloc(sizeof(char*)*propsAllocated_);
    nd = (double*)malloc(sizeof(double)*propsAllocated_);
    nt = (char*)malloc(sizeof(char)*propsAllocated_);
    for (i = 0; i < numProps_; i++) {
      nn[i] = names_[i];
      nv[i] = values_[i];
      nd[i] = dvalues_[i];
      nt[i] = types_[i];
    }
    free((char*)(names_));
    free((char*)(values_));
    free((char*)(dvalues_));
    free((char*)(types_));
    names_ = nn;
    values_ = nv;
    dvalues_ = nd;
    types_ = nt;
  }
  names_[numProps_] = (char*)malloc(len);
  strcpy(names_[numProps_], defData->DEFCASE(name));
  len = strlen(value) + 1;
  values_[numProps_] = (char*)malloc(len);
  strcpy(values_[numProps_], defData->DEFCASE(value));
  dvalues_[numProps_] = d;
  types_[numProps_] = type;
  numProps_ += 1;
}


void defiComponent::addNet(const char* net) {
  int len = strlen(net) + 1;
  if (numNets_ == netsAllocated_)
    bumpNets(numNets_ * 2);
  nets_[numNets_] = (char*)malloc(len);
  strcpy(nets_[numNets_], defData->DEFCASE(net));
  (numNets_)++;
}


void defiComponent::bumpNets(int size) {
  int i;
  char** newNets = (char**)malloc(sizeof(char*)* size);
  for (i = 0; i < numNets_; i++) {
    newNets[i] = nets_[i];
  }
  free((char*)(nets_));
  nets_ = newNets;
  netsAllocated_ = size;
}


const char* defiComponent::net(int index) const {
  if (index >= 0 && index < numNets_) {
    return nets_[index];
  }
  return 0;
}


void defiComponent::bumpForeignName(int size) {
  if (foreignName_) free(foreignName_);
  foreignName_ = (char*)malloc(sizeof(char) * size);
  foreignNameSize_ = size;
  *(foreignName_) = '\0';
}


void defiComponent::setForeignName(const char* name) {
  int len;

  if (hasForeignName())
      defiError(1, 0,
      "Multiple define of '+ FOREIGN' in COMPONENT is not supported.\n", defData);
  if ((len = strlen(name)+1) > foreignNameSize_)
    bumpForeignName(len);
  strcpy(foreignName_, defData->DEFCASE(name));
  hasForeignName_ = 1;
}


void defiComponent::setForeignLocation(int x, int y, int orient) {
  Fx_ = x;
  Fy_ = y;
  Fori_ = orient;
  hasFori_ = 1;
}


int defiComponent::hasForeignName() const {
  return (int)(hasForeignName_);
}


const char* defiComponent::foreignName() const {
  return foreignName_;
}


int defiComponent::foreignX() const {
  return Fx_;
}


int defiComponent::foreignY() const {
  return Fy_;
}


int defiComponent::hasFori() const {
  return (int)(hasFori_);
}

const char* defiComponent::foreignOri() const {
  switch (Fori_) {
    case 0: return ("N");
    case 1: return ("W");
    case 2: return ("S");
    case 3: return ("E");
    case 4: return ("FN");
    case 5: return ("FW");
    case 6: return ("FS");
    case 7: return ("FE");
  }
  return 0;
}

int defiComponent::foreignOrient() const {
  return Fori_;
}
END_LEFDEF_PARSER_NAMESPACE

