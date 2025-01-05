// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2019, Cadence Design Systems
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

#ifndef lefiMacro_h
#define lefiMacro_h

#include <cstdio>

#include "lefiKRDefs.hpp"
#include "lefiMisc.hpp"

BEGIN_LEF_PARSER_NAMESPACE

class lefiObstruction
{
 public:
  lefiObstruction();
  void Init();

  void Destroy();
  ~lefiObstruction();

  void clear();
  void setGeometries(lefiGeometries* g);

  lefiGeometries* geometries() const;

  void print(FILE* f) const;

 protected:
  lefiGeometries* geometries_{nullptr};
};

// 5.5
class lefiPinAntennaModel
{
 public:
  lefiPinAntennaModel();
  ~lefiPinAntennaModel();

  void Init();
  void clear();

  void setAntennaModel(int oxide);
  void addAntennaGateArea(double value, const char* layer);
  void addAntennaMaxAreaCar(double value, const char* layer);
  void addAntennaMaxSideAreaCar(double value, const char* layer);
  void addAntennaMaxCutCar(double value, const char* layer);
  void setAntennaReturnFlag(int flag);

  int hasAntennaGateArea() const;
  int hasAntennaMaxAreaCar() const;
  int hasAntennaMaxSideAreaCar() const;
  int hasAntennaMaxCutCar() const;

  char* antennaOxide() const;

  int numAntennaGateArea() const;
  double antennaGateArea(int index) const;
  const char* antennaGateAreaLayer(int index) const;

  int numAntennaMaxAreaCar() const;
  double antennaMaxAreaCar(int index) const;
  const char* antennaMaxAreaCarLayer(int index) const;

  int numAntennaMaxSideAreaCar() const;
  double antennaMaxSideAreaCar(int index) const;
  const char* antennaMaxSideAreaCarLayer(int index) const;

  int numAntennaMaxCutCar() const;
  double antennaMaxCutCar(int index) const;
  const char* antennaMaxCutCarLayer(int index) const;

  int hasReturn() const;

 protected:
  char* oxide_{nullptr};
  int hasReturn_{0};

  int numAntennaGateArea_{0};
  int antennaGateAreaAllocated_{0};
  double* antennaGateArea_{nullptr};
  char** antennaGateAreaLayer_{nullptr};

  int numAntennaMaxAreaCar_{0};
  int antennaMaxAreaCarAllocated_{0};
  double* antennaMaxAreaCar_{nullptr};
  char** antennaMaxAreaCarLayer_{nullptr};

  int numAntennaMaxSideAreaCar_{0};
  int antennaMaxSideAreaCarAllocated_{0};
  double* antennaMaxSideAreaCar_{nullptr};
  char** antennaMaxSideAreaCarLayer_{nullptr};

  int numAntennaMaxCutCar_{0};
  int antennaMaxCutCarAllocated_{0};
  double* antennaMaxCutCar_{nullptr};
  char** antennaMaxCutCarLayer_{nullptr};
};

class lefiPin
{
 public:
  lefiPin();
  void Init();

  void Destroy();
  ~lefiPin();

  void clear();
  void bump(char** array, int len, int* size);
  void setName(const char* name);
  void addPort(lefiGeometries* g);
  void addForeign(const char* name, int hasPnt, double x, double y, int orient);
  void setLEQ(const char* name);
  void setDirection(const char* name);
  void setUse(const char* name);
  void setShape(const char* name);
  void setMustjoin(const char* name);
  void setOutMargin(double high, double low);
  void setOutResistance(double high, double low);
  void setInMargin(double high, double low);
  void setPower(double power);
  void setLeakage(double current);
  void setMaxload(double capacitance);
  void setMaxdelay(double delayTime);
  void setCapacitance(double capacitance);
  void setResistance(double resistance);
  void setPulldownres(double resistance);
  void setTieoffr(double resistance);
  void setVHI(double voltage);
  void setVLO(double voltage);
  void setRiseVoltage(double voltage);
  void setFallVoltage(double voltage);
  void setRiseThresh(double capacitance);
  void setFallThresh(double capacitance);
  void setRiseSatcur(double current);
  void setFallSatcur(double current);
  void setCurrentSource(const char* name);
  void setTables(const char* highName, const char* lowName);
  void setProperty(const char* name, const char* value, const char type);
  void setNumProperty(const char* name,
                      double d,
                      const char* value,
                      const char type);
  void addAntennaModel(int oxide);  // 5.5
  void addAntennaSize(double value, const char* layer);
  void addAntennaMetalArea(double value, const char* layer);
  void addAntennaMetalLength(double value, const char* layer);
  void addAntennaPartialMetalArea(double value, const char* layer);
  void addAntennaPartialMetalSideArea(double value, const char* layer);
  void addAntennaGateArea(double value, const char* layer);
  void addAntennaDiffArea(double value, const char* layer);
  void addAntennaMaxAreaCar(double value, const char* layer);
  void addAntennaMaxSideAreaCar(double value, const char* layer);
  void addAntennaPartialCutArea(double value, const char* layer);
  void addAntennaMaxCutCar(double value, const char* layer);
  void setRiseSlewLimit(double value);
  void setFallSlewLimit(double value);
  void setTaperRule(const char* name);
  void setNetExpr(const char* name);               // 5.6
  void setSupplySensitivity(const char* pinName);  // 5.6
  void setGroundSensitivity(const char* pinName);  // 5.6
  void bumpProps();

  int hasForeign() const;
  int hasForeignOrient(int index = 0) const;
  int hasForeignPoint(int index = 0) const;
  int hasLEQ() const;
  int hasDirection() const;
  int hasUse() const;
  int hasShape() const;
  int hasMustjoin() const;
  int hasOutMargin() const;
  int hasOutResistance() const;
  int hasInMargin() const;
  int hasPower() const;
  int hasLeakage() const;
  int hasMaxload() const;
  int hasMaxdelay() const;
  int hasCapacitance() const;
  int hasResistance() const;
  int hasPulldownres() const;
  int hasTieoffr() const;
  int hasVHI() const;
  int hasVLO() const;
  int hasRiseVoltage() const;
  int hasFallVoltage() const;
  int hasRiseThresh() const;
  int hasFallThresh() const;
  int hasRiseSatcur() const;
  int hasFallSatcur() const;
  int hasCurrentSource() const;
  int hasTables() const;
  int hasAntennaSize() const;
  int hasAntennaMetalArea() const;
  int hasAntennaMetalLength() const;
  int hasAntennaPartialMetalArea() const;
  int hasAntennaPartialMetalSideArea() const;
  int hasAntennaPartialCutArea() const;
  int hasAntennaDiffArea() const;
  int hasAntennaModel() const;  // 5.5
  int hasTaperRule() const;
  int hasRiseSlewLimit() const;
  int hasFallSlewLimit() const;
  int hasNetExpr() const;            // 5.6
  int hasSupplySensitivity() const;  // 5.6
  int hasGroundSensitivity() const;  // 5.6

  const char* name() const;

  int numPorts() const;
  lefiGeometries* port(int index) const;

  int numForeigns() const;
  const char* foreignName(int index = 0) const;
  const char* taperRule() const;
  int foreignOrient(int index = 0) const;
  const char* foreignOrientStr(int index = 0) const;
  double foreignX(int index = 0) const;
  double foreignY(int index = 0) const;
  const char* LEQ() const;
  const char* direction() const;
  const char* use() const;
  const char* shape() const;
  const char* mustjoin() const;
  double outMarginHigh() const;
  double outMarginLow() const;
  double outResistanceHigh() const;
  double outResistanceLow() const;
  double inMarginHigh() const;
  double inMarginLow() const;
  double power() const;
  double leakage() const;
  double maxload() const;
  double maxdelay() const;
  double capacitance() const;
  double resistance() const;
  double pulldownres() const;
  double tieoffr() const;
  double VHI() const;
  double VLO() const;
  double riseVoltage() const;
  double fallVoltage() const;
  double riseThresh() const;
  double fallThresh() const;
  double riseSatcur() const;
  double fallSatcur() const;
  double riseSlewLimit() const;
  double fallSlewLimit() const;
  const char* currentSource() const;
  const char* tableHighName() const;
  const char* tableLowName() const;

  int numAntennaSize() const;
  double antennaSize(int index) const;
  const char* antennaSizeLayer(int index) const;

  int numAntennaMetalArea() const;
  double antennaMetalArea(int index) const;
  const char* antennaMetalAreaLayer(int index) const;

  int numAntennaMetalLength() const;
  double antennaMetalLength(int index) const;
  const char* antennaMetalLengthLayer(int index) const;

  int numAntennaPartialMetalArea() const;
  double antennaPartialMetalArea(int index) const;
  const char* antennaPartialMetalAreaLayer(int index) const;

  int numAntennaPartialMetalSideArea() const;
  double antennaPartialMetalSideArea(int index) const;
  const char* antennaPartialMetalSideAreaLayer(int index) const;

  int numAntennaPartialCutArea() const;
  double antennaPartialCutArea(int index) const;
  const char* antennaPartialCutAreaLayer(int index) const;

  int numAntennaDiffArea() const;
  double antennaDiffArea(int index) const;
  const char* antennaDiffAreaLayer(int index) const;

  // 5.6
  const char* netExpr() const;
  const char* supplySensitivity() const;
  const char* groundSensitivity() const;

  // 5.5
  int numAntennaModel() const;
  lefiPinAntennaModel* antennaModel(int index) const;

  int numProperties() const;
  const char* propName(int index) const;
  const char* propValue(int index) const;
  double propNum(int index) const;
  const char propType(int index) const;
  int propIsNumber(int index) const;
  int propIsString(int index) const;

  void print(FILE* f) const;

 protected:
  int nameSize_{0};
  char* name_{nullptr};

  char hasLEQ_{0};
  char hasDirection_{0};
  char hasUse_{0};
  char hasShape_{0};
  char hasMustjoin_{0};
  char hasOutMargin_{0};
  char hasOutResistance_{0};
  char hasInMargin_{0};
  char hasPower_{0};
  char hasLeakage_{0};
  char hasMaxload_{0};
  char hasMaxdelay_{0};
  char hasCapacitance_{0};
  char hasResistance_{0};
  char hasPulldownres_{0};
  char hasTieoffr_{0};
  char hasVHI_{0};
  char hasVLO_{0};
  char hasRiseVoltage_{0};
  char hasFallVoltage_{0};
  char hasRiseThresh_{0};
  char hasFallThresh_{0};
  char hasRiseSatcur_{0};
  char hasFallSatcur_{0};
  char hasCurrentSource_{0};
  char hasTables_{0};
  char hasAntennasize_{0};
  char hasRiseSlewLimit_{0};
  char hasFallSlewLimit_{0};

  int numForeigns_{0};
  int foreignAllocated_{0};
  int* hasForeignOrient_{nullptr};
  int* hasForeignPoint_{nullptr};
  int* foreignOrient_{nullptr};
  double* foreignX_{nullptr};
  double* foreignY_{nullptr};
  char** foreign_{nullptr};

  int LEQSize_{0};
  char* LEQ_{nullptr};
  int mustjoinSize_{0};
  char* mustjoin_{nullptr};
  double outMarginH_{0.0};
  double outMarginL_{0.0};
  double outResistanceH_{0.0};
  double outResistanceL_{0.0};
  double inMarginH_{0.0};
  double inMarginL_{0.0};
  double power_{0.0};
  double leakage_{0.0};
  double maxload_{0.0};
  double maxdelay_{0.0};
  double capacitance_{0.0};
  double resistance_{0.0};
  double pulldownres_{0.0};
  double tieoffr_{0.0};
  double VHI_{0.0};
  double VLO_{0.0};
  double riseVoltage_{0.0};
  double fallVoltage_{0.0};
  double riseThresh_{0.0};
  double fallThresh_{0.0};
  double riseSatcur_{0.0};
  double fallSatcur_{0.0};
  int lowTableSize_{0};
  char* lowTable_{nullptr};
  int highTableSize_{0};
  char* highTable_{nullptr};
  double riseSlewLimit_{0.0};
  double fallSlewLimit_{0.0};

  // 5.5 AntennaModel
  int numAntennaModel_{0};
  int antennaModelAllocated_{0};
  int curAntennaModelIndex_{0};  // save the current index of the antenna
  lefiPinAntennaModel** pinAntennaModel_{nullptr};

  int numAntennaSize_{0};
  int antennaSizeAllocated_{0};
  double* antennaSize_{nullptr};
  char** antennaSizeLayer_{nullptr};

  int numAntennaMetalArea_{0};
  int antennaMetalAreaAllocated_{0};
  double* antennaMetalArea_{nullptr};
  char** antennaMetalAreaLayer_{nullptr};

  int numAntennaMetalLength_{0};
  int antennaMetalLengthAllocated_{0};
  double* antennaMetalLength_{nullptr};
  char** antennaMetalLengthLayer_{nullptr};

  int numAntennaPartialMetalArea_{0};
  int antennaPartialMetalAreaAllocated_{0};
  double* antennaPartialMetalArea_{nullptr};
  char** antennaPartialMetalAreaLayer_{nullptr};

  int numAntennaPartialMetalSideArea_{0};
  int antennaPartialMetalSideAreaAllocated_{0};
  double* antennaPartialMetalSideArea_{nullptr};
  char** antennaPartialMetalSideAreaLayer_{nullptr};

  int numAntennaPartialCutArea_{0};
  int antennaPartialCutAreaAllocated_{0};
  double* antennaPartialCutArea_{nullptr};
  char** antennaPartialCutAreaLayer_{nullptr};

  int numAntennaDiffArea_{0};
  int antennaDiffAreaAllocated_{0};
  double* antennaDiffArea_{nullptr};
  char** antennaDiffAreaLayer_{nullptr};

  char* taperRule_{nullptr};

  char* netEpxr_{nullptr};
  char* ssPinName_{nullptr};
  char* gsPinName_{nullptr};

  char direction_[32];
  char use_[12];
  char shape_[12];
  char currentSource_[12];

  int numProperties_{0};
  int propertiesAllocated_{0};
  char** propNames_{nullptr};
  char** propValues_{nullptr};
  double* propNums_{nullptr};
  char* propTypes_{nullptr};

  int numPorts_{0};
  int portsAllocated_{0};
  lefiGeometries** ports_{nullptr};
};

// 5.6
class lefiDensity
{
 public:
  lefiDensity();
  void Init();

  void Destroy();
  ~lefiDensity();

  void clear();
  void addLayer(const char* name);
  void addRect(double x1, double y1, double x2, double y2, double value);

  int numLayer() const;
  char* layerName(int index) const;
  int numRects(int index) const;
  lefiGeomRect getRect(int index, int rectIndex) const;
  double densityValue(int index, int rectIndex) const;

  void print(FILE* f) const;

 protected:
  int numLayers_{0};
  int layersAllocated_{0};
  char** layerName_{nullptr};
  int* numRects_{nullptr};
  int* rectsAllocated_{nullptr};
  lefiGeomRect** rects_{nullptr};
  double** densityValue_{nullptr};
};

class lefiMacro
{
 public:
  lefiMacro();
  void Init();

  void Destroy();
  ~lefiMacro();

  void clear();
  void bump(char** array, int len, int* size);
  void setName(const char* name);
  void setGenerator(const char* name);
  void setGenerate(const char* name1, const char* name2);
  void setPower(double d);
  void setOrigin(double x, double y);
  void setClass(const char* name);
  void setSource(const char* name);
  void setEEQ(const char* name);
  void setLEQ(const char* name);
  void setClockType(const char* name);
  void setProperty(const char* name, const char* value, const char type);
  void setNumProperty(const char* name,
                      double d,
                      const char* value,
                      const char type);
  void bumpProps();

  // orient=-1 means no orient was specified.
  void addForeign(const char* name, int hasPnt, double x, double y, int orient);

  void setXSymmetry();
  void setYSymmetry();
  void set90Symmetry();
  void setSiteName(const char* name);
  void setSitePattern(lefiSitePattern* p);
  void setSize(double x, double y);
  void setBuffer();
  void setInverter();
  void setFixedMask(int isFixedMask = 0);

  int hasClass() const;
  int hasGenerator() const;
  int hasGenerate() const;
  int hasPower() const;
  int hasOrigin() const;
  int hasEEQ() const;
  int hasLEQ() const;
  int hasSource() const;
  int hasXSymmetry() const;
  int hasYSymmetry() const;
  int has90Symmetry() const;
  int hasSiteName() const;
  int hasSitePattern() const;
  int hasSize() const;
  int hasForeign() const;
  int hasForeignOrigin(int index = 0) const;
  int hasForeignOrient(int index = 0) const;
  int hasForeignPoint(int index = 0) const;
  int hasClockType() const;
  int isBuffer() const;
  int isInverter() const;
  int isFixedMask() const;

  int numSitePattern() const;
  int numProperties() const;
  const char* propName(int index) const;
  const char* propValue(int index) const;
  double propNum(int index) const;
  const char propType(int index) const;
  int propIsNumber(int index) const;
  int propIsString(int index) const;

  const char* name() const;
  const char* macroClass() const;
  const char* generator() const;
  const char* EEQ() const;
  const char* LEQ() const;
  const char* source() const;
  const char* clockType() const;
  double originX() const;
  double originY() const;
  double power() const;
  void generate(char** name1, char** name2) const;
  lefiSitePattern* sitePattern(int index) const;
  const char* siteName() const;
  double sizeX() const;
  double sizeY() const;
  int numForeigns() const;
  int foreignOrient(int index = 0) const;
  const char* foreignOrientStr(int index = 0) const;
  double foreignX(int index = 0) const;
  double foreignY(int index = 0) const;
  const char* foreignName(int index = 0) const;

  // Debug print
  void print(FILE* f) const;

 protected:
  int nameSize_{0};
  char* name_{nullptr};
  char macroClass_[32];
  char source_[12];

  int generatorSize_{0};
  char* generator_{nullptr};

  char hasClass_{0};
  char hasGenerator_{0};
  char hasGenerate_{0};
  char hasPower_{0};
  char hasOrigin_{0};
  char hasSource_{0};
  char hasEEQ_{0};
  char hasLEQ_{0};
  char hasSymmetry_{0};  // X=1  Y=2  R90=4  (can be combined)
  char hasSiteName_{0};
  char hasSize_{0};
  char hasClockType_{0};
  char isBuffer_{0};
  char isInverter_{0};

  char* EEQ_{nullptr};
  int EEQSize_{0};
  char* LEQ_{nullptr};
  int LEQSize_{0};
  char* gen1_{nullptr};
  int gen1Size_{0};
  char* gen2_{nullptr};
  int gen2Size_{0};
  double power_{0.0};
  double originX_{0.0};
  double originY_{0.0};
  double sizeX_{0.0};
  double sizeY_{0.0};

  int numSites_{0};
  int sitesAllocated_{0};
  lefiSitePattern** pattern_{nullptr};

  int numForeigns_{0};
  int foreignAllocated_{0};
  int* hasForeignOrigin_{nullptr};
  int* hasForeignPoint_{nullptr};
  int* foreignOrient_{nullptr};
  double* foreignX_{nullptr};
  double* foreignY_{nullptr};
  char** foreign_{nullptr};

  int siteNameSize_{0};
  char* siteName_{nullptr};

  char* clockType_{nullptr};
  int clockTypeSize_{0};

  int numProperties_{0};
  int propertiesAllocated_{0};
  char** propNames_{nullptr};
  char** propValues_{nullptr};
  double* propNums_{nullptr};
  char* propTypes_{nullptr};

  int isFixedMask_{0};
};

class lefiTiming
{
 public:
  lefiTiming();
  void Init();

  void Destroy();
  ~lefiTiming();

  void addRiseFall(const char* risefall, double one, double two);
  void addRiseFallVariable(double one, double two);
  void addRiseFallSlew(double one, double two, double three, double four);
  void addRiseFallSlew2(double one, double two, double three);
  void setRiseRS(double one, double two);
  void setFallRS(double one, double two);
  void setRiseCS(double one, double two);
  void setFallCS(double one, double two);
  void setRiseAtt1(double one, double two);
  void setFallAtt1(double one, double two);
  void setRiseTo(double one, double two);
  void setFallTo(double one, double two);
  void addUnateness(const char* typ);
  void setStable(double one, double two, const char* typ);
  void addTableEntry(double one, double two, double three);
  void addTableAxisNumber(double one);
  void addFromPin(const char* name);
  void addToPin(const char* name);
  void addDelay(const char* risefall,
                const char* unateness,
                double one,
                double two,
                double three);
  void addTransition(const char* risefall,
                     const char* unateness,
                     double one,
                     double two,
                     double three);
  // addSDF2Pins & addSDF1Pin are for 5.1
  void addSDF2Pins(const char* trigType,
                   const char* fromTrig,
                   const char* toTrig,
                   double one,
                   double two,
                   double three);
  void addSDF1Pin(const char* trigType, double one, double two, double three);
  void setSDFcondStart(const char* condStart);
  void setSDFcondEnd(const char* condEnd);
  void setSDFcond(const char* cond);
  int hasData();
  void clear();

  int numFromPins();
  const char* fromPin(int index);
  int numToPins();
  const char* toPin(int index);
  int hasTransition();
  int hasDelay();
  int hasRiseSlew();
  int hasRiseSlew2();
  int hasFallSlew();
  int hasFallSlew2();
  int hasRiseIntrinsic();
  int hasFallIntrinsic();
  int numOfAxisNumbers();
  double* axisNumbers();
  int numOfTableEntries();
  void tableEntry(int num, double* one, double* two, double* three);
  const char* delayRiseOrFall();
  const char* delayUnateness();
  double delayTableOne();
  double delayTableTwo();
  double delayTableThree();
  const char* transitionRiseOrFall();
  const char* transitionUnateness();
  double transitionTableOne();
  double transitionTableTwo();
  double transitionTableThree();
  double fallIntrinsicOne();
  double fallIntrinsicTwo();
  double fallIntrinsicThree();
  double fallIntrinsicFour();
  double riseIntrinsicOne();
  double riseIntrinsicTwo();
  double riseIntrinsicThree();
  double riseIntrinsicFour();
  double fallSlewOne();
  double fallSlewTwo();
  double fallSlewThree();
  double fallSlewFour();
  double fallSlewFive();
  double fallSlewSix();
  double fallSlewSeven();
  double riseSlewOne();
  double riseSlewTwo();
  double riseSlewThree();
  double riseSlewFour();
  double riseSlewFive();
  double riseSlewSix();
  double riseSlewSeven();
  int hasRiseRS();
  double riseRSOne();
  double riseRSTwo();
  int hasRiseCS();
  double riseCSOne();
  double riseCSTwo();
  int hasFallRS();
  double fallRSOne();
  double fallRSTwo();
  int hasFallCS();
  double fallCSOne();
  double fallCSTwo();
  int hasUnateness();
  const char* unateness();
  int hasRiseAtt1();
  double riseAtt1One();
  double riseAtt1Two();
  int hasFallAtt1();
  double fallAtt1One();
  double fallAtt1Two();
  int hasFallTo();
  double fallToOne();
  double fallToTwo();
  int hasRiseTo();
  double riseToOne();
  double riseToTwo();
  int hasStableTiming();
  double stableSetup();
  double stableHold();
  const char* stableRiseFall();
  // The following are for 5.1
  int hasSDFonePinTrigger();
  int hasSDFtwoPinTrigger();
  int hasSDFcondStart();
  int hasSDFcondEnd();
  int hasSDFcond();
  const char* SDFonePinTriggerType();
  const char* SDFtwoPinTriggerType();
  const char* SDFfromTrigger();
  const char* SDFtoTrigger();
  double SDFtriggerOne();
  double SDFtriggerTwo();
  double SDFtriggerThree();
  const char* SDFcondStart();
  const char* SDFcondEnd();
  const char* SDFcond();

 protected:
  int numFrom_{0};
  char** from_{nullptr};
  int fromAllocated_{0};
  int numTo_{0};
  char** to_{nullptr};
  int toAllocated_{0};

  int hasTransition_{0};
  int hasDelay_{0};
  int hasRiseSlew_{0};
  int hasRiseSlew2_{0};
  int hasFallSlew_{0};
  int hasFallSlew2_{0};
  int hasRiseIntrinsic_{0};
  int hasFallIntrinsic_{0};
  int hasRiseRS_{0};
  int hasRiseCS_{0};
  int hasFallRS_{0};
  int hasFallCS_{0};
  int hasUnateness_{0};
  int hasFallAtt1_{0};
  int hasRiseAtt1_{0};
  int hasFallTo_{0};
  int hasRiseTo_{0};
  int hasStableTiming_{0};
  int hasSDFonePinTrigger_{0};
  int hasSDFtwoPinTrigger_{0};
  int hasSDFcondStart_{0};
  int hasSDFcondEnd_{0};
  int hasSDFcond_{0};
  int nowRise_{0};

  int numOfAxisNumbers_{0};
  double* axisNumbers_{nullptr};
  int axisNumbersAllocated_{0};

  int numOfTableEntries_{0};
  int tableEntriesAllocated_{0};
  double* table_{nullptr};  // three numbers per entry

  char* delayRiseOrFall_{nullptr};
  char* delayUnateness_{nullptr};
  double delayTableOne_{0.0};
  double delayTableTwo_{0.0};
  double delayTableThree_{0.0};
  char* transitionRiseOrFall_{nullptr};
  char* transitionUnateness_{nullptr};
  double transitionTableOne_{0.0};
  double transitionTableTwo_{0.0};
  double transitionTableThree_{0.0};
  double riseIntrinsicOne_{0.0};
  double riseIntrinsicTwo_{0.0};
  double riseIntrinsicThree_{0.0};
  double riseIntrinsicFour_{0.0};
  double fallIntrinsicOne_{0.0};
  double fallIntrinsicTwo_{0.0};
  double fallIntrinsicThree_{0.0};
  double fallIntrinsicFour_{0.0};
  double riseSlewOne_{0.0};
  double riseSlewTwo_{0.0};
  double riseSlewThree_{0.0};
  double riseSlewFour_{0.0};
  double riseSlewFive_{0.0};
  double riseSlewSix_{0.0};
  double riseSlewSeven_{0.0};
  double fallSlewOne_{0.0};
  double fallSlewTwo_{0.0};
  double fallSlewThree_{0.0};
  double fallSlewFour_{0.0};
  double fallSlewFive_{0.0};
  double fallSlewSix_{0.0};
  double fallSlewSeven_{0.0};
  double riseRSOne_{0.0};
  double riseRSTwo_{0.0};
  double riseCSOne_{0.0};
  double riseCSTwo_{0.0};
  double fallRSOne_{0.0};
  double fallRSTwo_{0.0};
  double fallCSOne_{0.0};
  double fallCSTwo_{0.0};
  char* unateness_{nullptr};
  double riseAtt1One_{0.0};
  double riseAtt1Two_{0.0};
  double fallAtt1One_{0.0};
  double fallAtt1Two_{0.0};
  double fallToOne_{0.0};
  double fallToTwo_{0.0};
  double riseToOne_{0.0};
  double riseToTwo_{0.0};
  double stableSetup_{0.0};
  double stableHold_{0.0};
  char* stableRiseFall_{nullptr};
  char* SDFtriggerType_{nullptr};
  char* SDFfromTrigger_{nullptr};
  char* SDFtoTrigger_{nullptr};
  double SDFtriggerTableOne_{0.0};
  double SDFtriggerTableTwo_{0.0};
  double SDFtriggerTableThree_{0.0};
  char* SDFcondStart_{nullptr};
  char* SDFcondEnd_{nullptr};
  char* SDFcond_{nullptr};
};

// 5.8
class lefiMacroSite
{
 public:
  lefiMacroSite(const char* name, const lefiSitePattern* pattern);

  const char* siteName() const;
  const lefiSitePattern* sitePattern() const;

 protected:
  const char* siteName_;
  const lefiSitePattern* sitePattern_;
};

class lefiMacroForeign
{
 public:
  lefiMacroForeign(const char* name,
                   int hasPts,
                   double x,
                   double y,
                   int hasOrient,
                   int orient);

  const char* cellName() const;
  int cellHasPts() const;
  double px() const;
  double py() const;
  int cellHasOrient() const;
  int cellOrient() const;

 protected:
  const char* cellName_;
  int cellHasPts_;
  double px_;
  double py_;
  int cellHasOrient_;
  int cellOrient_;
};

END_LEF_PARSER_NAMESPACE

#endif
