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

#ifndef defiPinCap_h
#define defiPinCap_h


#include "defiKRDefs.hpp"
#include "defiMisc.hpp"
#include <stdio.h>

BEGIN_LEFDEF_PARSER_NAMESPACE

class defrData;

class defiPinCap {
public:

  void setPin(int p);
  void setCap(double d);

  int pin() const;
  double cap() const;

  void print(FILE* f) const;

protected:
  int pin_;        // pin num
  double cap_;     // capacitance
};


// 5.5
class defiPinAntennaModel {
public:
  defiPinAntennaModel(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H(defiPinAntennaModel);
  DEF_ASSIGN_OPERATOR_H(defiPinAntennaModel);

  ~defiPinAntennaModel();
  void clear();
  void Destroy();

  void setAntennaModel(int oxide);
  void addAPinGateArea(int value, const char* layer);
  void addAPinMaxAreaCar(int value, const char* layer);
  void addAPinMaxSideAreaCar(int value, const char* layer);
  void addAPinMaxCutCar(int value, const char* layer);

  char* antennaOxide() const;

  int hasAPinGateArea() const;               // ANTENNAPINGATEAREA
  int numAPinGateArea() const;
  int APinGateArea(int index) const;
  int hasAPinGateAreaLayer(int index) const;
  const char* APinGateAreaLayer(int index) const;

  int hasAPinMaxAreaCar() const;             // ANTENNAPINMAXAREACAR
  int numAPinMaxAreaCar() const;
  int APinMaxAreaCar(int index) const;
  int hasAPinMaxAreaCarLayer(int index) const;
  const char* APinMaxAreaCarLayer(int index) const;

  int hasAPinMaxSideAreaCar() const;         // ANTENNAPINMAXSIDEAREACAR
  int numAPinMaxSideAreaCar() const;
  int APinMaxSideAreaCar(int index) const;
  int hasAPinMaxSideAreaCarLayer(int index) const;
  const char* APinMaxSideAreaCarLayer(int index) const;

  int hasAPinMaxCutCar() const;              // ANTENNAPINMAXCUTCAR
  int numAPinMaxCutCar() const;
  int APinMaxCutCar(int index) const;
  int hasAPinMaxCutCarLayer(int index) const;
  const char* APinMaxCutCarLayer(int index) const;

protected:
  char* oxide_;

  int numAPinGateArea_;                  // 5.4
  int APinGateAreaAllocated_;
  int* APinGateArea_;                    // 5.4 AntennaPinGateArea
  char** APinGateAreaLayer_;             // 5.4 Layer

  int numAPinMaxAreaCar_;                // 5.4
  int APinMaxAreaCarAllocated_;
  int* APinMaxAreaCar_;                  // 5.4 AntennaPinMaxAreaCar
  char** APinMaxAreaCarLayer_;           // 5.4 Layer

  int numAPinMaxSideAreaCar_;            // 5.4
  int APinMaxSideAreaCarAllocated_;
  int* APinMaxSideAreaCar_;              // 5.4 AntennaPinMaxSideAreaCar
  char** APinMaxSideAreaCarLayer_;       // 5.4 Layer

  int numAPinMaxCutCar_;                 // 5.4
  int APinMaxCutCarAllocated_;
  int* APinMaxCutCar_;                   // 5.4 AntennaPinMaxCutCar
  char** APinMaxCutCarLayer_;            // 5.4 Layer

  defrData *defData;
};

class defiPinPort {                      // 5.7
public:
  defiPinPort(defrData *data);
  void Init();
  
  DEF_COPY_CONSTRUCTOR_H(defiPinPort);
  DEF_ASSIGN_OPERATOR_H(defiPinPort);
  ~defiPinPort();

  void clear();

  void addLayer(const char* layer);
  void addLayerSpacing(int minSpacing);
  void addLayerMask(int mask);
  void addLayerDesignRuleWidth(int effectiveWidth);
  void addLayerPts(int xl, int yl, int xh, int yh);
  void addPolygon(const char* layerName);
  void addPolySpacing(int minSpacing);
  void addPolyMask(int mask);
  void addPolyDesignRuleWidth(int effectiveWidth);
  void addPolygonPts(defiGeometries* geom);
  void addVia(const char* via, int viaX, int viaY, int color = 0);
  void setPlacement(int typ, int x, int y, int orient);

  int   numLayer() const;
  const char* layer(int index) const;
  void  bounds(int index, int* xl, int* yl, int* xh, int* yh) const;
  int   hasLayerSpacing(int index) const;
  int   hasLayerDesignRuleWidth(int index) const;
  int   layerSpacing(int index) const;
  int   layerMask(int index) const;
  int   layerDesignRuleWidth(int index) const;
  int   numPolygons() const;
  const char* polygonName(int index) const;
  defiPoints getPolygon(int index) const;
  int   hasPolygonSpacing(int index) const;
  int   hasPolygonDesignRuleWidth(int index) const;
  int   polygonSpacing(int index) const;
  int   polygonDesignRuleWidth(int index) const;
  int   polygonMask(int index) const;
  int   numVias() const;
  const char* viaName(int index) const;
  int   viaPtX (int index) const;
  int   viaPtY (int index) const;
  int   viaTopMask (int index) const;
  int   viaCutMask (int index) const;
  int   viaBottomMask (int index) const;
  int   hasPlacement() const;
  int   isPlaced() const;
  int   isCover() const;
  int   isFixed() const;
  int   placementX() const;
  int   placementY() const;
  int   orient() const;
  const char* orientStr() const;

protected:
  int    layersAllocated_;
  int    numLayers_;
  char** layers_;
  int    *layerMinSpacing_;
  int    *layerEffectiveWidth_;
  int    *xl_, *yl_, *xh_, *yh_;
  int    *layerMask_;
  int    polysAllocated_;
  int    numPolys_;
  char** polygonNames_;
  int    *polyMinSpacing_;
  int    *polyMask_;
  int    *polyEffectiveWidth_;
  defiPoints** polygons_;
  int    viasAllocated_;
  int    numVias_;
  char** viaNames_;
  int    *viaX_;
  int    *viaY_;
  int    *viaMask_;
  char   placeType_;
  int    x_, y_;
  char   orient_;

  defrData *defData;
};

class defiPin {
public:
  defiPin(defrData *data);
  void Init();

  DEF_COPY_CONSTRUCTOR_H( defiPin );
  DEF_ASSIGN_OPERATOR_H( defiPin );

  ~defiPin();
  void Destroy();

  void Setup(const char* pinName, const char* netName);
  void setDirection(const char* dir);
  void setUse(const char* use);
  // 5.6 setLayer is changed to addLayer due to multiple LAYER are allowed
  // in 5.6
  void addLayer(const char* layer);
  void addLayerMask(int mask);                                    // 5.8
  void addLayerSpacing(int minSpacing);                           // 5.6
  void addLayerDesignRuleWidth(int effectiveWidth);               // 5.6
  void addLayerPts(int xl, int yl, int xh, int yh);
  void addPolygon(const char* layerName);                         // 5.6
  void addPolyMask(int mask);                                     // 5.8
  void addPolySpacing(int minSpacing);                            // 5.6
  void addPolyDesignRuleWidth(int effectiveWidth);                // 5.6
  void addPolygonPts(defiGeometries* geom);                       // 5.6
  void setNetExpr(const char* netExpr);                           // 5.6
  void setSupplySens(const char* pinName);                        // 5.6
  void setGroundSens(const char* pinName);                        // 5.6
  void setPlacement(int typ, int x, int y, int orient);
  void setSpecial();
  void addAntennaModel(int oxide);                                // 5.5
  void addAPinPartialMetalArea(int value, const char* layer);
  void addAPinPartialMetalSideArea(int value, const char* layer);
  void addAPinGateArea(int value, const char* layer);
  void addAPinDiffArea(int value, const char* layer);
  void addAPinMaxAreaCar(int value, const char* layer);
  void addAPinMaxSideAreaCar(int value, const char* layer);
  void addAPinPartialCutArea(int value, const char* layer);
  void addAPinMaxCutCar(int value, const char* layer);
  void addVia(const char* via, int viaX, int viaY, int color = 0);               // 5.7
  // 5.7 port statements, which may have LAYER, POLYGON, &| VIA
  void addPort();                                                 // 5.7
  void addPortLayer(const char* layer);                           // 5.7
  void addPortLayerSpacing(int minSpacing);                       // 5.7
  void addPortLayerDesignRuleWidth(int effectiveWidth);           // 5.7
  void addPortLayerPts(int xl, int yl, int xh, int yh);           // 5.7
  void addPortLayerMask(int color);                               // 5.8
  void addPortPolygon(const char* layerName);                     // 5.7
  void addPortPolySpacing(int minSpacing);                        // 5.7
  void addPortPolyDesignRuleWidth(int effectiveWidth);            // 5.7
  void addPortPolygonPts(defiGeometries* geom);                   // 5.7
  void addPortPolyMask(int color);                                // 5.8
  void addPortVia(const char* via, int viaX, int viaY, int color = 0);           // 5.7
  void setPortPlacement(int typ, int x, int y, int orient);       // 5.7 - 5.8

  void clear();

  void changePinName(const char* pinName);       // For OA to modify the pinName

  const char* pinName() const;
  const char* netName() const;
  // optional parts
  int hasDirection() const;
  int hasUse() const;
  int hasLayer() const;
  int hasPlacement() const;
  int isUnplaced() const;
  int isPlaced() const;
  int isCover() const;
  int isFixed() const;
  int placementX() const;
  int placementY() const;
  const char* direction() const;
  const char* use() const;
  int numLayer() const;
  const char* layer(int index) const;
  void bounds(int index, int* xl, int* yl, int* xh, int* yh) const;
  int layerMask(int index) const;                     // 5.8
  int hasLayerSpacing(int index) const;               // 5.6
  int hasLayerDesignRuleWidth(int index) const;       // 5.6
  int layerSpacing(int index) const;                  // 5.6
  int layerDesignRuleWidth(int index) const;          // 5.6
  int  numPolygons() const;                           // 5.6
  const  char* polygonName(int index) const;          // 5.6
  defiPoints getPolygon(int index) const;      // 5.6
  int polygonMask(int index) const;                      // 5.8
  int hasPolygonSpacing(int index) const;             // 5.6
  int hasPolygonDesignRuleWidth(int index) const;     // 5.6
  int polygonSpacing(int index) const;                // 5.6
  int polygonDesignRuleWidth(int index) const;        // 5.6
  int  hasNetExpr() const;                            // 5.6
  int  hasSupplySensitivity() const;                  // 5.6
  int  hasGroundSensitivity() const;                  // 5.6
  const char* netExpr() const;                        // 5.6
  const char* supplySensitivity() const;              // 5.6
  const char* groundSensitivity() const;              // 5.6
  int orient() const;
  const char* orientStr() const;
  int hasSpecial() const;
  int numVias() const;                                // 5.7
  const char* viaName(int index) const;               // 5.7
  int viaTopMask(int index) const;                    // 5.8
  int viaCutMask(int index) const;                    // 5.8
  int viaBottomMask(int index) const;                 // 5.8
  int viaPtX (int index) const;                       // 5.7
  int viaPtY (int index) const;                       // 5.7

  // 5.4
  int hasAPinPartialMetalArea() const;       // ANTENNAPINPARTIALMETALAREA
  int numAPinPartialMetalArea() const;
  int APinPartialMetalArea(int index) const;
  int hasAPinPartialMetalAreaLayer(int index) const;
  const char* APinPartialMetalAreaLayer(int index) const;

  int hasAPinPartialMetalSideArea() const;   // ANTENNAPINPARTIALMETALSIDEAREA
  int numAPinPartialMetalSideArea() const;
  int APinPartialMetalSideArea(int index) const;
  int hasAPinPartialMetalSideAreaLayer(int index) const;
  const char* APinPartialMetalSideAreaLayer(int index) const;

  int hasAPinDiffArea() const;               // ANTENNAPINDIFFAREA
  int numAPinDiffArea() const;
  int APinDiffArea(int index) const;
  int hasAPinDiffAreaLayer(int index) const;
  const char* APinDiffAreaLayer(int index) const;

  int hasAPinPartialCutArea() const;         // ANTENNAPINPARTIALCUTAREA
  int numAPinPartialCutArea() const;
  int APinPartialCutArea(int index) const;
  int hasAPinPartialCutAreaLayer(int index) const;
  const char* APinPartialCutAreaLayer(int index) const;

  // 5.5
  int numAntennaModel() const;
  defiPinAntennaModel* antennaModel(int index) const;

  // 5.7
  int  hasPort() const;
  int  numPorts() const;
  defiPinPort* pinPort(int index) const;
  void print(FILE* f) const;

protected:
  int pinNameLength_;    // allocated size of pin name
  char* pinName_;
  int netNameLength_;    // allocated size of net name
  char* netName_;
  char hasDirection_;
  char hasUse_;
  char placeType_;
  char orient_;          // orient 0-7
  int useLength_;        // allocated size of length
  char* use_;
  int directionLength_;  // allocated size of direction
  char* direction_;
  char** layers_;                   // 5.6, changed to array
  int *xl_, *yl_, *xh_, *yh_;       // 5.6, changed to arrays
  int *layerMinSpacing_;            // 5.6, SPACING in LAYER
  int *layerEffectiveWidth_;        // 5.6, DESIGNRULEWIDTH in LAYER
  int layersAllocated_;             // 5.6
  int numLayers_;                   // 5.6
  int *layerMask_;                  // 5.8
  char** polygonNames_;             // 5.6 layerName for POLYGON
  int *polyMinSpacing_;             // 5.6, SPACING in POLYGON
  int *polyEffectiveWidth_;         // 5.6, DESIGNRULEWIDTH in POLYGON
  int *polyMask_;                   // 5.8
  int numPolys_;                    // 5.6
  int polysAllocated_;              // 5.6
  defiPoints** polygons_;    // 5.6 
  int x_, y_;            // placement
  int hasSpecial_;
  int numVias_;                     // 5.7
  int viasAllocated_;               // 5.7
  char** viaNames_;                 // 5.7
  int *viaX_;                       // 5.7
  int *viaY_;                       // 5.7
  int *viaMask_;                    // 5.8
  int numPorts_;                    // 5.7
  int portsAllocated_;              // 5.7
  defiPinPort ** pinPort_;          // 5.7

  // 5.5 AntennaModel
  int numAntennaModel_;
  int antennaModelAllocated_;
  defiPinAntennaModel** antennaModel_;

  int numAPinPartialMetalArea_;          // 5.4
  int APinPartialMetalAreaAllocated_;
  int* APinPartialMetalArea_;            // 5.4 AntennaPinPartialMetalArea
  char** APinPartialMetalAreaLayer_;     // 5.4 Layer

  int numAPinPartialMetalSideArea_;      // 5.4
  int APinPartialMetalSideAreaAllocated_;
  int* APinPartialMetalSideArea_;        // 5.4 AntennaPinPartialMetalSideArea
  char** APinPartialMetalSideAreaLayer_; // 5.4 Layer

  int numAPinDiffArea_;                  // 5.4
  int APinDiffAreaAllocated_;
  int* APinDiffArea_;                    // 5.4 AntennaPinDiffArea
  char** APinDiffAreaLayer_;             // 5.4 Layer

  int numAPinPartialCutArea_;            // 5.4
  int APinPartialCutAreaAllocated_;
  int* APinPartialCutArea_;              // 5.4 AntennaPinPartialCutArea
  char** APinPartialCutAreaLayer_;       // 5.4 Layer

  int netExprLength_;                    // 5.6
  char hasNetExpr_;                      // 5.6
  char* netExpr_;                        // 5.6
  int supplySensLength_;                 // 5.6
  char hasSupplySens_;                   // 5.6
  char* supplySens_;                     // 5.6
  int groundSensLength_;                 // 5.6
  char hasGroundSens_;                   // 5.6
  char* groundSens_;                     // 5.6

  defrData *defData;
};

END_LEFDEF_PARSER_NAMESPACE

USE_LEFDEF_PARSER_NAMESPACE

#endif
