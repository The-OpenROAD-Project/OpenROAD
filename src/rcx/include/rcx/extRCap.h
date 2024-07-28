///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <map>

#include "ext2dBox.h"
#include "extprocess.h"
#include "odb/db.h"
#include "odb/dbExtControl.h"
#include "odb/dbShape.h"
#include "odb/odb.h"
#include "odb/util.h"
#include "rcx/dbUtil.h"
#include "rcx/gseq.h"

namespace utl {
class Logger;
}

namespace rcx {

class extMeasure;

using odb::Ath__array1D;
using odb::AthPool;
using odb::uint;
using utl::Logger;

class extSpef;
class Ath__gridTable;

// CoupleOptions seriously needs to be rewriten to use a class with named
// members. -cherry 05/09/2021
using CoupleOptions = std::array<int, 21>;
using CoupleAndCompute = void (*)(CoupleOptions&, void*);

class extDistRC
{
 public:
  void setLogger(Logger* logger) { logger_ = logger; }
  void printDebug(const char*,
                  const char*,
                  uint,
                  uint,
                  extDistRC* rcUnit = nullptr);
  void printDebugRC_values(const char* msg);
  void printDebugRC(const char*, Logger* logger);
  void printDebugRC(int met,
                    int overMet,
                    int underMet,
                    int width,
                    int dist,
                    int dbUnit,
                    Logger* logger);
  void printDebugRC_sum(int len, int dbUnit, Logger* logger);
  void printDebugRC_diag(int met,
                         int overMet,
                         int underMet,
                         int width,
                         int dist,
                         int dbUnit,
                         Logger* logger);
  double GetDBcoords(int x, int db_factor);
  void set(uint d, double cc, double fr, double a, double r);
  void readRC(Ath__parser* parser, double dbFactor = 1.0);
  void readRC_res2(Ath__parser* parser, double dbFactor = 1.0);
  double getFringe();
  double getCoupling();
  double getDiag();
  double getRes();
  double getTotalCap() { return _coupling + _fringe + _diag; };
  void addRC(extDistRC* rcUnit, uint len, bool addCC);
  void writeRC(FILE* fp, bool bin);
  void writeRC();
  void interpolate(uint d, extDistRC* rc1, extDistRC* rc2);
  double interpolate_res(uint d, extDistRC* rc2);

 private:
  int _sep;
  double _coupling;
  double _fringe;
  double _diag;
  double _res;
  Logger* logger_;

  friend class extDistRCTable;
  friend class extDistWidthRCTable;
  friend class extMeasure;
  friend class extMain;
};

class extDistRCTable
{
 public:
  extDistRCTable(uint distCnt);
  ~extDistRCTable();

  extDistRC* getRC_99();
  void ScaleRes(double SUB_MULT_RES, Ath__array1D<extDistRC*>* table);

  uint addMeasureRC(extDistRC* rc);
  void makeComputeTable(uint maxDist, uint distUnit);
  extDistRC* getLastRC();
  extDistRC* getRC_index(int n);
  extDistRC* getComputeRC(double dist);
  extDistRC* getComputeRC(uint dist);
  extDistRC* getRC(uint s, bool compute);
  extDistRC* getComputeRC_res(uint dist1, uint dist2);
  extDistRC* findIndexed_res(uint dist1, uint dist2);
  int getComputeRC_maxDist();
  uint writeRules(FILE* fp,
                  Ath__array1D<extDistRC*>* table,
                  double w,
                  bool bin);
  uint writeRules(FILE* fp, double w, bool compute, bool bin);
  uint writeDiagRules(FILE* fp,
                      Ath__array1D<extDistRC*>* table,
                      double w1,
                      double w2,
                      double s,
                      bool bin);
  uint writeDiagRules(FILE* fp,
                      double w1,
                      double w2,
                      double s,
                      bool compute,
                      bool bin);
  uint readRules(Ath__parser* parser,
                 AthPool<extDistRC>* rcPool,
                 bool compute,
                 bool bin,
                 bool ignore,
                 double dbFactor = 1.0);
  uint readRules_res2(Ath__parser* parser,
                      AthPool<extDistRC>* rcPool,
                      bool compute,
                      bool bin,
                      bool ignore,
                      double dbFactor = 1.0);
  uint interpolate(uint distUnit, int maxDist, AthPool<extDistRC>* rcPool);
  uint mapInterpolate(extDistRC* rc1,
                      extDistRC* rc2,
                      uint distUnit,
                      int maxDist,
                      AthPool<extDistRC>* rcPool);
  uint mapExtrapolate(uint loDist,
                      extDistRC* rc2,
                      uint distUnit,
                      AthPool<extDistRC>* rcPool);

 private:
  void makeCapTableOver();
  void makeCapTableUnder();

  Ath__array1D<extDistRC*>* _measureTable;
  Ath__array1D<extDistRC*>* _computeTable;
  Ath__array1D<extDistRC*>* _measureTableR[16];
  Ath__array1D<extDistRC*>* _computeTableR[16];  // OPTIMIZE
  int _maxDist;
  uint _distCnt;
  uint _unit;
  Logger* logger_;
};

class extDistWidthRCTable
{
 public:
  extDistWidthRCTable(bool dummy, uint met, uint layerCnt, uint width);
  extDistWidthRCTable(bool over,
                      uint met,
                      uint layerCnt,
                      uint metCnt,
                      Ath__array1D<double>* widthTable,
                      AthPool<extDistRC>* rcPool,
                      double dbFactor = 1.0);
  extDistWidthRCTable(bool over,
                      uint met,
                      uint layerCnt,
                      uint metCnt,
                      Ath__array1D<double>* widthTable,
                      int diagWidthCnt,
                      int diagDistCnt,
                      AthPool<extDistRC>* rcPool,
                      double dbFactor = 1.0);
  extDistWidthRCTable(bool over,
                      uint met,
                      uint layerCnt,
                      uint metCnt,
                      uint maxWidthCnt,
                      AthPool<extDistRC>* rcPool);
  void addRCw(uint n, uint w, extDistRC* rc);
  void createWidthMap();
  void makeWSmapping();

  ~extDistWidthRCTable();
  void setDiagUnderTables(uint met,
                          Ath__array1D<double>* diagWidthTable,
                          Ath__array1D<double>* diagDistTable,
                          double dbFactor = 1.0);
  uint getWidthIndex(uint w);
  uint getDiagWidthIndex(uint m, uint w);
  uint getDiagDistIndex(uint m, uint s);
  uint addCapOver(uint met, uint metUnder, extDistRC* rc);
  extDistRC* getCapOver(uint met, uint metUnder);
  uint writeWidthTable(FILE* fp, bool bin);
  uint writeDiagWidthTable(FILE* fp, uint met, bool bin);
  uint writeDiagDistTable(FILE* fp, uint met, bool bin);
  void writeDiagTablesCnt(FILE* fp, uint met, bool bin);
  uint writeRulesOver(FILE* fp, bool bin);
  uint writeRulesOver_res(FILE* fp, bool bin);
  uint writeRulesUnder(FILE* fp, bool bin);
  uint writeRulesDiagUnder(FILE* fp, bool bin);
  uint writeRulesDiagUnder2(FILE* fp, bool bin);
  uint writeRulesOverUnder(FILE* fp, bool bin);
  uint getMetIndexUnder(uint mOver);
  uint readRulesOver(Ath__parser* parser,
                     uint widthCnt,
                     bool bin,
                     bool ignore,
                     const char* OVER,
                     double dbFactor = 1.0);
  uint readRulesUnder(Ath__parser* parser,
                      uint widthCnt,
                      bool bin,
                      bool ignore,
                      double dbFactor = 1.0);
  uint readRulesDiagUnder(Ath__parser* parser,
                          uint widthCnt,
                          uint diagWidthCnt,
                          uint diagDistCnt,
                          bool bin,
                          bool ignore,
                          double dbFactor = 1.0);
  uint readRulesDiagUnder(Ath__parser* parser,
                          uint widthCnt,
                          bool bin,
                          bool ignore,
                          double dbFactor = 1.0);
  uint readRulesOverUnder(Ath__parser* parser,
                          uint widthCnt,
                          bool bin,
                          bool ignore,
                          double dbFactor = 1.0);
  uint readMetalHeader(Ath__parser* parser,
                       uint& met,
                       const char* keyword,
                       bool bin,
                       bool ignore);

  extDistRC* getRes(uint mou, uint w, int dist1, int dist2);
  extDistRC* getRC(uint mou, uint w, uint s);
  extDistRC* getRC(uint mou, uint w, uint dw, uint ds, uint s);
  extDistRC* getFringeRC(uint mou, uint w, int index_dist = -1);

  extDistRC* getLastWidthFringeRC(uint mou);
  extDistRC* getRC_99(uint mou, uint w, uint dw, uint ds);
  extDistRCTable* getRuleTable(uint mou, uint w);

 public:
  bool _ouReadReverse;
  bool _over;
  uint _layerCnt;
  uint _met;

  Ath__array1D<int>* _widthTable;
  Ath__array1D<uint>* _widthMapTable;
  Ath__array1D<int>* _diagWidthTable[32];
  Ath__array1D<int>* _diagDistTable[32];
  Ath__array1D<uint>* _diagWidthMapTable[32];
  Ath__array1D<uint>* _diagDistMapTable[32];

  uint _modulo;
  int _firstWidth = 0;
  int _lastWidth = -1;
  Ath__array1D<int>* _firstDiagWidth;
  Ath__array1D<int>* _lastDiagWidth;
  Ath__array1D<int>* _firstDiagDist;
  Ath__array1D<int>* _lastDiagDist;
  bool _widthTableAllocFlag;  // if false widthtable is pointer

  extDistRCTable*** _rcDistTable;  // per over/under metal, per width
  extDistRCTable***** _rcDiagDistTable;
  uint _metCnt;  // if _over==false _metCnt???

  AthPool<extDistRC>* _rcPoolPtr;
  extDistRC* _rc31;
};

class extMetRCTable
{
 public:
  extMetRCTable(uint layerCnt, AthPool<extDistRC>* rcPool, Logger* logger_);
  ~extMetRCTable();
  void allocateInitialTables(uint layerCnt,
                             uint widthCnt,
                             bool over,
                             bool under,
                             bool diag = false);
  void allocOverTable(uint met,
                      Ath__array1D<double>* wTable,
                      double dbFactor = 1.0);
  void allocDiagUnderTable(uint met,
                           Ath__array1D<double>* wTable,
                           int diagWidthCnt,
                           int diagDistCnt,
                           double dbFactor = 1.0);
  void allocDiagUnderTable(uint met,
                           Ath__array1D<double>* wTable,
                           double dbFactor = 1.0);
  void allocUnderTable(uint met,
                       Ath__array1D<double>* wTable,
                       double dbFactor = 1.0);
  void allocOverUnderTable(uint met,
                           Ath__array1D<double>* wTable,
                           double dbFactor = 1.0);
  void setDiagUnderTables(uint met,
                          uint overMet,
                          Ath__array1D<double>* diagWTable,
                          Ath__array1D<double>* diagSTable,
                          double dbFactor = 1.0);
  void addRCw(extMeasure* m);
  uint readRCstats(Ath__parser* parser);
  void mkWidthAndSpaceMappings();

  uint addCapOver(uint met, uint metUnder, extDistRC* rc);
  uint addCapUnder(uint met, uint metOver, extDistRC* rc);
  extDistRC* getCapOver(uint met, uint metUnder);
  extDistRC* getCapUnder(uint met, uint metOver);
  extDistRC* getOverFringeRC(extMeasure* m, int index_dist = -1);
  extDistRC* getOverFringeRC_last(int met, int width);
  AthPool<extDistRC>* getRCPool();

 public:
  uint _layerCnt;
  char _name[128];
  extDistWidthRCTable** _resOver;
  extDistWidthRCTable** _capOver;
  extDistWidthRCTable** _capUnder;
  extDistWidthRCTable** _capOverUnder;
  extDistWidthRCTable** _capDiagUnder;

  AthPool<extDistRC>* _rcPoolPtr;
  double _rate;
  Logger* logger_;
};

class extRCTable
{
 public:
  extRCTable(bool over, uint layerCnt);
  uint addCapOver(uint met, uint metUnder, extDistRC* rc);
  extDistRC* getCapOver(uint met, uint metUnder);

 private:
  void makeCapTableOver();
  void makeCapTableUnder();

  bool _over;
  uint _maxCnt1;
  Ath__array1D<extDistRC*>*** _inTable;  // per metal per width
  Ath__array1D<extDistRC*>*** _table;
};

class extMain;
class extMeasure;
class extMainOptions;

class extRCModel
{
 public:
  extMetRCTable* getMetRCTable(uint ii) { return _modelTable[ii]; };

  int getModelCnt() { return _modelCnt; };
  int getLayerCnt() { return _layerCnt; };
  void setLayerCnt(uint n) { _layerCnt = n; };
  int getDiagModel() { return _diagModel; };
  bool getVerticalDiagFlag() { return _verticalDiag; };
  void setDiagModel(uint i) { _diagModel = i; }
  extRCModel(uint layerCnt, const char* name, Logger* logger);
  extRCModel(const char* name, Logger* logger);
  extProcess* getProcess();
  uint findBiggestDatarateIndex(double d);
  ~extRCModel();
  void setExtMain(extMain* x);
  void createModelTable(uint n, uint layerCnt);

  uint addLefTotRC(uint met, uint underMet, double fr, double r);
  uint addCapOver(uint met,
                  uint underMet,
                  uint d,
                  double cc,
                  double fr,
                  double a,
                  double r);
  double getTotCapOverSub(uint met);
  double getRes(uint met);

  uint benchWithVar_density(extMainOptions* opt, extMeasure* measure);
  uint benchWithVar_lists(extMainOptions* opt, extMeasure* measure);

  uint runWiresSolver(uint netId, int shapeId);

  void setProcess(extProcess* p);
  void setDataRateTable(uint met);
  uint singleLineOver(uint widthCnt);
  uint twoLineOver(uint widthCnt, uint spaceCnt);
  uint threeLineOver(uint widthCnt, uint spaceCnt);
  uint getCapValues(uint lastNode,
                    double& cc1,
                    double& cc2,
                    double& fr,
                    double& tot,
                    extMeasure* m);
  uint getCapMatrixValues(uint lastNode, extMeasure* m);
  uint readCapacitance(uint wireNum,
                       double& cc1,
                       double& cc2,
                       double& fr,
                       double& tot,
                       bool readCapLog,
                       extMeasure* m = nullptr);
  uint readCapacitanceBench(bool readCapLog, extMeasure* m);
  uint readCapacitanceBenchDiag(bool readCapLog, extMeasure* m);
  extDistRC* readCap(uint wireCnt, double w, double s, double r);
  uint readCap(uint wireCnt, double cc1, double cc2, double fr, double tot);
  FILE* openFile(const char* topDir,
                 const char* name,
                 const char* suffix,
                 const char* permissions);
  FILE* openSolverFile();
  void mkNet_prefix(extMeasure* m, const char* wiresNameSuffix);
  void mkFileNames(extMeasure* m, char* wiresNameSuffix);
  void writeWires2(FILE* fp, extMeasure* measure, uint wireCnt);
  int writeBenchWires(FILE* fp, extMeasure* measure);
  void setOptions(const char* topDir,
                  const char* pattern,
                  bool writeFiles,
                  bool readFiles,
                  bool runSolver);
  void setOptions(const char* topDir,
                  const char* pattern,
                  bool writeFiles,
                  bool readFiles,
                  bool runSolver,
                  bool keepFile);
  void runSolver(const char* solverOption);
  bool solverStep(extMeasure* m);
  void cleanFiles();

  extDistRC* measurePattern(uint met,
                            int underMet,
                            int overMet,
                            double width,
                            double space,
                            uint wireCnt,
                            char* ou,
                            bool readCapLog);
  bool measurePatternVar(extMeasure* m,
                         double top_width,
                         double bot_width,
                         double thickness,
                         uint wireCnt,
                         char* wiresNameSuffix,
                         double res = 0.0);
  double measureResistance(extMeasure* m,
                           double ro,
                           double top_widthR,
                           double bot_widthR,
                           double thicknessR);

  uint linesOverBench(extMainOptions* opt);
  uint linesUnderBench(extMainOptions* opt);
  uint linesDiagUnderBench(extMainOptions* opt);
  uint linesOverUnderBench(extMainOptions* opt);

  uint benchWithVar(extMeasure* measure);
  void addRC(extMeasure* m);
  int getOverUnderIndex(extMeasure* m, uint maxCnt);
  uint getUnderIndex(extMeasure* m);
  extDistWidthRCTable* getWidthDistRCtable(uint met,
                                           int mUnder,
                                           int mOver,
                                           int& n,
                                           double dRate);

  void printCommentLine(char commentChar, extMeasure* m);

  void allocOverTable(extMeasure* measure);
  void allocDiagUnderTable(extMeasure* measure);
  void allocUnderTable(extMeasure* measure);
  void allocOverUnderTable(extMeasure* measure);
  void computeTables(extMeasure* m,
                     uint wireCnt,
                     uint widthCnt,
                     uint spaceCnt,
                     uint dCnt);

  void getDiagTables(extMeasure* m, uint widthCnt, uint spaceCnt);
  void setDiagUnderTables(extMeasure* m);
  void getRCtable(uint met, int mUnder, int OverMet, double w, double dRate);
  void getRCtable(Ath__array1D<int>* sTable,
                  Ath__array1D<double>* rcTable,
                  uint valType,
                  uint met,
                  int mUnder,
                  int mOver,
                  int width,
                  double dRate);

  void writeRules(char* name, bool binary);
  bool readRules(char* name,
                 bool binary,
                 bool over,
                 bool under,
                 bool overUnder,
                 bool diag,
                 uint cornerCnt = 0,
                 const uint* cornerTable = nullptr,
                 double dbFactor = 1.0);
  uint readMetalHeader(Ath__parser* parser,
                       uint& met,
                       const char* keyword,
                       bool bin,
                       bool ignore);
  Ath__array1D<double>* readHeaderAndWidth(Ath__parser* parser,
                                           uint& met,
                                           const char* ouKey,
                                           const char* wKey,
                                           bool bin,
                                           bool ignore);
  uint readRules(Ath__parser* parser,
                 uint m,
                 uint ii,
                 const char* ouKey,
                 const char* wKey,
                 bool over,
                 bool under,
                 bool bin,
                 bool diag,
                 bool ignore,
                 double dbFactor = 1.0);

  extDistRC* getOverFringeRC(uint met, uint underMet, uint width);
  double getFringeOver(uint met, uint mUnder, uint w, uint s);
  double getCouplingOver(uint met, uint mUnder, uint w, uint s);
  extDistRC* getOverRC(extMeasure* m);
  extDistRC* getUnderRC(extMeasure* m);
  extDistRC* getOverUnderRC(extMeasure* m);

  extDistRC* getOverFringeRC(extMeasure* m);
  extDistRC* getOverUnderFringeRC(extMeasure* m);
  extDistRC* getUnderFringeRC(extMeasure* m);

  int findDatarateIndex(double d);

  FILE* mkPatternFile();
  bool openCapLogFile();
  void closeCapLogFile();
  void closeFiles();

  void setRuleFileName(char* name) { _ruleFileName = name; }
  char* getRuleFileName() { return _ruleFileName; }
  uint getMaxCnt(int met)
  {
    return _modelTable[_tmpDataRate]->_capOverUnder[met]->_metCnt;
  }
  uint benchDB_WS(extMainOptions* opt, extMeasure* measure);
  int writeBenchWires_DB(extMeasure* measure);
  int writeBenchWires_DB_res(extMeasure* measure);

  int writeBenchWires_DB_diag(extMeasure* measure);
  extMetRCTable* initCapTables(uint layerCnt, uint widthCnt);

  extDistRC* getMinRC(int met, int width);
  extDistRC* getMaxRC(int met, int width, int dist);

 private:
  bool _ouReadReverse;
  uint _layerCnt;
  char _name[128];

  int _noVariationIndex;
  uint _modelCnt;
  Ath__array1D<double>* _dataRateTable;
  Ath__array1D<int>* _dataRateTableMap;
  extMetRCTable** _modelTable;
  uint _tmpDataRate;
  bool _diag;
  bool _verticalDiag;
  bool _maxMinFlag;
  uint _diagModel;
  bool _keepFile;
  uint _metLevel;

  extRCTable* _resOver;
  extRCTable* _capOver;
  extRCTable* _capDiagUnder;
  extRCTable* _capUnder;
  AthPool<extDistRC>* _rcPoolPtr;
  extProcess* _process;
  char* _ruleFileName;
  char* _wireFileName;
  char* _wireDirName;
  char* _topDir;
  char* _patternName;
  Ath__parser* _parser;
  char* _solverFileName;

  FILE* _capLogFP;
  FILE* _logFP;

  bool _writeFiles;
  bool _readSolver;
  bool _runSolver;

  bool _readCapLog;
  char _commentLine[10000];
  bool _commentFlag;

  uint* _singlePlaneLayerMap;

  extMain* _extMain;

  Logger* logger_;
};

class extLenOU  // assume cross-section on the z-direction
{
 public:
  void reset();
  void addOverOrUnderLen(int met, bool over, uint len);
  void addOULen(int underMet, int overMet, uint len);

 public:
  int _met;
  int _underMet;
  int _overMet;
  uint _len;
  bool _over;
  bool _overUnder;
  bool _under;
};

class extMeasure
{
 public:
  extMeasure(utl::Logger* logger);
  ~extMeasure();

  void rcNetInfo();
  bool rcSegInfo();
  bool ouCovered_debug(int covered);
  void segInfo(const char* msg, uint netId, int rsegId);
  bool isVia(uint rsegId);
  bool ouRCvalues(const char* msg, uint jj);
  bool OverSubDebug(extDistRC* rc, int lenOverSub, int lenOverSub_res);
  bool OverSubDebug(extDistRC* rc,
                    int lenOverSub,
                    int lenOverSub_res,
                    double res,
                    double cap,
                    const char* openDist);
  bool Debug_DiagValues(double res, double cap, const char* openDist);
  bool IsDebugNet();
  bool DebugStart(bool allNets = false);
  bool DebugDiagCoords(int met,
                       int targetMet,
                       int len1,
                       int diagDist,
                       int ll[2],
                       int ur[2]);
  double GetDBcoords(uint coord);
  double GetDBcoords(int coord);
  void printNetCaps();

  void printTraceNetInfo(const char* msg, uint netId, int rsegId);
  bool printTraceNet(const char* msg,
                     bool init,
                     odb::dbCCSeg* cc = nullptr,
                     uint overSub = 0,
                     uint covered = 0);

  extDistRC* areaCapOverSub(uint modelNum, extMetRCTable* rcModel);

  extDistRC* getUnderLastWidthDistRC(extMetRCTable* rcModel, uint overMet);
  void createCap(int rsegId1, uint rsegId2, double* capTable);
  void areaCap(int rsegId1, uint rsegId2, uint len, uint tgtMet);
  bool verticalCap(int rsegId1,
                   uint rsegId2,
                   uint len,
                   uint tgtWidth,
                   uint diagDist,
                   uint tgtMet);
  extDistRC* getVerticalUnderRC(extMetRCTable* rcModel,
                                uint diagDist,
                                uint tgtWidth,
                                uint overMet);

  odb::dbRSeg* getRseg(const char* netname,
                       const char* capMsg,
                       const char* tableEntryName);
  bool getFirstShape(odb::dbNet* net, odb::dbShape& s);

  void swap_coords(SEQ* s);
  uint swap_coords(uint initCnt, uint endCnt, Ath__array1D<SEQ*>* resTable);
  uint getOverlapSeq(uint met, SEQ* s, Ath__array1D<SEQ*>* resTable);
  uint getOverlapSeq(uint met, int* ll, int* ur, Ath__array1D<SEQ*>* resTable);

  bool isConnectedToBterm(odb::dbRSeg* rseg1);
  uint defineBox(CoupleOptions& options);
  void printCoords(FILE* fp);
  void printNet(odb::dbRSeg* rseg, uint netId);
  void updateBox(uint w_layout, uint s_layout, int dir = -1);
  void printBox(FILE* fp);
  uint initWS_box(extMainOptions* opt, uint gridCnt);
  odb::dbRSeg* getFirstDbRseg(uint netId);
  uint createNetSingleWire(char* dirName,
                           uint idCnt,
                           uint w_layout,
                           uint s_layout,
                           int dir = -1);
  uint createDiagNetSingleWire(char* dirName,
                               uint idCnt,
                               int begin,
                               int w_layout,
                               int s_layout,
                               int dir = -1);
  uint createNetSingleWire_cntx(int met,
                                char* dirName,
                                uint idCnt,
                                int d,
                                int ll[2],
                                int ur[2],
                                int s_layout = -1);
  uint createContextNets(char* dirName,
                         const int bboxLL[2],
                         const int bboxUR[2],
                         int met,
                         double pitchMult);
  uint getPatternExtend();

  uint createContextObstruction(const char* dirName,
                                int x1,
                                int y1,
                                int bboxUR[2],
                                int met,
                                double pitchMult);
  uint createContextGrid(char* dirName,
                         const int bboxLL[2],
                         const int bboxUR[2],
                         int met,
                         int s_layout = -1);
  uint createContextGrid_dir(char* dirName,
                             const int bboxLL[2],
                             const int bboxUR[2],
                             int met);

  double getCCfringe(uint lastNode, uint n, uint start, uint end);

  void updateForBench(extMainOptions* opt, extMain* extMain);
  uint measureOverUnderCap();
  uint measureOverUnderCap_orig(gs* pixelTable, uint** ouPixelTableIndexMap);
  uint getSeqOverOrUnder(Ath__array1D<SEQ*>* seqTable,
                         gs* pixelTable,
                         uint met,
                         Ath__array1D<SEQ*>* resTable);
  uint computeOverOrUnderSeq(Ath__array1D<SEQ*>* seqTable,
                             uint met,
                             Ath__array1D<SEQ*>* resTable,
                             bool over);
  uint computeOverUnder(int* ll, int* ur, Ath__array1D<SEQ*>* resTable);

  void release(Ath__array1D<SEQ*>* seqTable, gs* pixelTable = nullptr);
  void addSeq(Ath__array1D<SEQ*>* seqTable, gs* pixelTable);
  void addSeq(const int* ll,
              const int* ur,
              Ath__array1D<SEQ*>* seqTable,
              gs* pixelTable = nullptr);
  SEQ* addSeq(const int* ll, const int* ur);
  void copySeq(SEQ* t, Ath__array1D<SEQ*>* seqTable, gs* pixelTable);
  void tableCopyP(Ath__array1D<SEQ*>* src, Ath__array1D<SEQ*>* dst);
  void tableCopy(Ath__array1D<SEQ*>* src,
                 Ath__array1D<SEQ*>* dst,
                 gs* pixelTable);

  uint measureDiagFullOU();
  uint ouFlowStep(Ath__array1D<SEQ*>* overTable);
  int underFlowStep(Ath__array1D<SEQ*>* srcTable,
                    Ath__array1D<SEQ*>* overTable);

  void measureRC(CoupleOptions& options);
  int computeAndStoreRC(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, int srcCovered);

  double ScaleResbyTrack(bool openEnded, double& dist_track);
  void OverSubRC(odb::dbRSeg* rseg1,
                 odb::dbRSeg* rseg2,
                 int ouCovered,
                 int diagCovered,
                 int srcCovered);
  void OverSubRC_dist(odb::dbRSeg* rseg1,
                      odb::dbRSeg* rseg2,
                      int ouCovered,
                      int diagCovered,
                      int srcCovered);

  void copySeqUsingPool(SEQ* t, Ath__array1D<SEQ*>* seqTable);
  void seq_release(Ath__array1D<SEQ*>* table);
  void calcOU(uint len);
  void calcRC(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, uint totLenCovered);
  int getMaxDist(int tgtMet, uint modelIndex);
  void calcRes(int rsegId1, uint len, int dist1, int dist2, int tgtMet);
  void calcRes0(double* deltaRes,
                uint tgtMet,
                uint len,
                int dist1 = 0,
                int dist2 = 0);
  uint computeRes(SEQ* s,
                  uint targetMet,
                  uint dir,
                  uint planeIndex,
                  uint trackn,
                  Ath__array1D<SEQ*>* residueSeq);
  int computeResDist(SEQ* s,
                     uint trackMin,
                     uint trackMax,
                     uint targetMet,
                     Ath__array1D<SEQ*>* diagTable);
  uint computeDiag(SEQ* s,
                   uint targetMet,
                   uint dir,
                   uint planeIndex,
                   uint trackn,
                   Ath__array1D<SEQ*>* residueSeq);

  odb::dbCCSeg* makeCcap(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, double ccCap);
  void addCCcap(odb::dbCCSeg* ccap, double v, uint model);
  void addFringe(odb::dbRSeg* rseg1,
                 odb::dbRSeg* rseg2,
                 double frCap,
                 uint model);
  void calcDiagRC(int rsegid1,
                  uint rsegid2,
                  uint len,
                  uint diagWidth,
                  uint diagDist,
                  uint tgtMet);
  void calcDiagRC(int rsegid1, uint rsegid2, uint len, uint dist, uint tgtMet);
  int calcDist(const int* ll, const int* ur);

  void ccReportProgress();
  uint getOverUnderIndex();

  uint getLength(SEQ* s, int dir);
  uint blackCount(uint start, Ath__array1D<SEQ*>* resTable);
  extDistRC* computeR(uint len, double* valTable);
  extDistRC* computeOverFringe(uint overMet,
                               uint overWidth,
                               uint len,
                               uint dist);
  extDistRC* computeUnderFringe(uint underMet,
                                uint underWidth,
                                uint len,
                                uint dist);

  double getDiagUnderCC(extMetRCTable* rcModel, uint dist, uint overMet);
  double getDiagUnderCC(extMetRCTable* rcModel,
                        uint diagWidth,
                        uint diagDist,
                        uint overMet);
  extDistRC* getDiagUnderCC2(extMetRCTable* rcModel,
                             uint diagWidth,
                             uint diagDist,
                             uint overMet);
  extDistRC* computeOverUnderRC(uint len);
  extDistRC* computeOverRC(uint len);
  extDistRC* computeUnderRC(uint len);
  extDistRC* getOverUnderFringeRC(extMetRCTable* rcModel);
  extDistRC* getOverUnderRC(extMetRCTable* rcModel);
  extDistRC* getOverRC(extMetRCTable* rcModel);
  uint getUnderIndex();
  uint getUnderIndex(uint overMet);
  extDistRC* getUnderRC(extMetRCTable* rcModel);

  extDistRC* getFringe(uint len, double* valTable);

  void tableCopyP(Ath__array1D<int>* src, Ath__array1D<int>* dst);
  void getMinWidth(odb::dbTech* tech);
  uint measureOverUnderCapCJ();
  uint computeOverUnder(int xy1, int xy2, Ath__array1D<int>* resTable);
  uint computeOUwith2planes(int* ll, int* ur, Ath__array1D<SEQ*>* resTable);
  uint intersectContextArray(int pmin,
                             int pmax,
                             uint met1,
                             uint met2,
                             Ath__array1D<int>* tgtContext);
  uint computeOverOrUnderSeq(Ath__array1D<int>* seqTable,
                             uint met,
                             Ath__array1D<int>* resTable,
                             bool over);
  bool updateLengthAndExit(int& remainder, int& totCovered, int len);
  int compute_Diag_Over_Under(Ath__array1D<SEQ*>* seqTable,
                              Ath__array1D<SEQ*>* resTable);
  int compute_Diag_OverOrUnder(Ath__array1D<SEQ*>* seqTable,
                               bool over,
                               uint met,
                               Ath__array1D<SEQ*>* resTable);
  uint measureUnderOnly(bool diagFlag);
  uint measureOverOnly(bool diagFlag);
  uint measureDiagOU(uint ouLevelLimit, uint diagLevelLimit);

  uint mergeContextArray(Ath__array1D<int>* srcContext,
                         int minS,
                         Ath__array1D<int>* tgtContext);
  uint mergeContextArray(Ath__array1D<int>* srcContext,
                         int minS,
                         int pmin,
                         int pmax,
                         Ath__array1D<int>* tgtContext);
  uint intersectContextArray(Ath__array1D<int>* s1Context,
                             Ath__array1D<int>* s2Context,
                             int minS1,
                             int minS2,
                             Ath__array1D<int>* tgtContext);

  extDistRC* addRC(extDistRC* rcUnit, uint len, uint jj);

  void setMets(int m, int u, int o);
  void setTargetParams(double w,
                       double s,
                       double r,
                       double t,
                       double h,
                       double w2 = 0.0,
                       double s2 = 0.0);
  void setEffParams(double wTop, double wBot, double teff);
  void addCap();
  void printStats(FILE* fp);
  void printMets(FILE* fp);

  ext2dBox* addNew2dBox(odb::dbNet* net, int* ll, int* ur, uint m, bool cntx);
  void clean2dBoxTable(int met, bool cntx);
  void writeRaphaelPointXY(FILE* fp, double X, double Y);
  void getBox(int met, bool cntx, int& xlo, int& ylo, int& xhi, int& yhi);

  int getDgPlaneAndTrackIndex(uint tgt_met,
                              int trackDist,
                              int& loTrack,
                              int& hiTrack);
  int computeDiagOU(SEQ* s, uint targetMet, Ath__array1D<SEQ*>* residueSeq);
  int computeDiagOU(SEQ* s,
                    uint trackMin,
                    uint trackMax,
                    uint targetMet,
                    Ath__array1D<SEQ*>* diagTable);
  void printDgContext();
  void initTargetSeq();
  void getDgOverlap(CoupleOptions& options);
  void getDgOverlap(SEQ* sseq,
                    uint dir,
                    Ath__array1D<SEQ*>* dgContext,
                    Ath__array1D<SEQ*>* overlapSeq,
                    Ath__array1D<SEQ*>* residueSeq);
  void getDgOverlap_res(SEQ* sseq,
                        uint dir,
                        Ath__array1D<SEQ*>* dgContext,
                        Ath__array1D<SEQ*>* overlapSeq,
                        Ath__array1D<SEQ*>* residueSeq);

  uint getRSeg(odb::dbNet* net, uint shapeId);

  void allocOUpool();
  int get_nm(double n) { return 1000 * (n / _dbunit); };

  bool _skip_delims;
  bool _no_debug;

  int _met;
  int _underMet;
  int _overMet;
  uint _wireCnt;

  int _minSpaceTable[32];

  int _minWidth;
  int _minSpace;
  int _pitch;

  double _w_m;
  int _w_nm;
  double _s_m;
  int _s_nm;
  double _w2_m;
  int _w2_nm;
  double _s2_m;
  int _s2_nm;

  double _r;
  double _t;
  double _h;

  uint _wIndex;
  uint _sIndex;
  uint _dwIndex;
  uint _dsIndex;
  uint _rIndex;
  uint _pIndex;

  double _topWidth;
  double _botWidth;
  double _teff;
  double _heff;
  double _seff;

  bool _varFlag;
  bool _over;
  bool _res;
  bool _overUnder;
  bool _diag;
  bool _verticalDiag;
  bool _plate;
  bool _thickVarFlag;
  bool _metExtFlag;
  uint _diagModel;

  extDistRC* _rc[20];
  extDistRC* _tmpRC;
  extRCTable* _capTable;
  Ath__array1D<double> _widthTable;
  Ath__array1D<double> _spaceTable;
  Ath__array1D<double> _dataTable;
  Ath__array1D<double> _pTable;
  Ath__array1D<double> _widthTable0;
  Ath__array1D<double> _spaceTable0;
  Ath__array1D<double> _diagSpaceTable0;
  Ath__array1D<double> _diagWidthTable0;

  Ath__array1D<SEQ*>*** _dgContextArray;  // array
  uint* _dgContextDepth;                  // not array
  uint* _dgContextPlanes;                 // not array
  uint* _dgContextTracks;                 // not array
  uint* _dgContextBaseLvl;                // not array
  int* _dgContextLowLvl;                  // not array
  int* _dgContextHiLvl;                   // not array
  uint* _dgContextBaseTrack;              // array
  int* _dgContextLowTrack;                // array
  int* _dgContextHiTrack;                 // array
  int** _dgContextTrackBase;              // array
  FILE* _dgContextFile;
  uint _dgContextCnt;

  Ath__array1D<int>** _ccContextArray;

  Ath__array1D<ext2dBox*>
      _2dBoxTable[2][20];  // assume 20 layers; 0=main net; 1=context
  AthPool<ext2dBox>* _2dBoxPool;
  Ath__array1D<int>** _ccMergedContextArray;

  int _ll[2];
  int _ur[2];

  uint _len;
  int _dist;
  uint _width;
  uint _dir;
  uint _layerCnt;
  odb::dbBlock* _block;
  odb::dbTech* _tech;
  double _capMatrix[100][100];
  uint _idTable[10000];
  uint _mapTable[10000];
  uint _maxCapNodeCnt;

  extMain* _extMain;
  extRCModel* _currentModel;
  Ath__array1D<extMetRCTable*> _metRCTable;
  uint _minModelIndex;
  uint _maxModelIndex;

  uint _totCCcnt;
  uint _totSmallCCcnt;
  uint _totBigCCcnt;
  uint _totSignalSegCnt;
  uint _totSegCnt;

  double _resFactor;
  bool _resModify;
  double _ccFactor;
  bool _ccModify;
  double _gndcFactor;
  bool _gndcModify;

  gs* _pixelTable;

  Ath__array1D<SEQ*>* _diagTable;
  Ath__array1D<SEQ*>* _tmpSrcTable;
  Ath__array1D<SEQ*>* _tmpDstTable;
  Ath__array1D<SEQ*>* _tmpTable;
  Ath__array1D<SEQ*>* _underTable;
  Ath__array1D<SEQ*>* _ouTable;
  Ath__array1D<SEQ*>* _overTable;

  int _diagLen;
  uint _netId;
  int _rsegSrcId;
  int _rsegTgtId;
  int _netSrcId;
  int _netTgtId;
  FILE* _debugFP;

  AthPool<SEQ>* _seqPool;

  AthPool<extLenOU>* _lenOUPool;
  Ath__array1D<extLenOU*>* _lenOUtable;

  bool _diagFlow;
  bool _toHi;
  bool _sameNetFlag;

  bool _rotatedGs;

  dbCreateNetUtil _create_net_util;
  int _dbunit;

 private:
  Logger* logger_;
};

class extMainOptions
{
 public:
  extMainOptions();

  uint _overDist;
  uint _underDist;
  int _met_cnt;
  int _met;
  int _underMet;
  int _overMet;
  uint _wireCnt;
  const char* _topDir;
  const char* _name;
  const char* _wTable;
  const char* _sTable;
  const char* _thTable;
  const char* _dTable;

  bool _write_to_solver;
  bool _read_from_solver;
  bool _run_solver;

  bool _listsFlag;
  bool _thListFlag;
  bool _wsListFlag;
  bool _default_lef_rules;
  bool _nondefault_lef_rules;

  bool _multiple_widths;

  bool _varFlag;
  bool _over;
  bool _overUnder;
  int _diag;
  bool _db_only;
  bool _gen_def_patterns;

  bool _res_patterns;

  odb::dbTech* _tech;
  odb::dbBlock* _block;

  Ath__array1D<double> _widthTable;
  Ath__array1D<double> _spaceTable;
  Ath__array1D<double> _densityTable;
  Ath__array1D<double> _thicknessTable;
  Ath__array1D<double> _gridTable;

  int _ll[2];
  int _ur[2];
  uint _len;
  int _dist;
  uint _width;
  uint _dir;
  extRCModel* _rcModel;
  uint _layerCnt;
};

class extCorner
{
 public:
  extCorner();

  char* _name;
  int _model;
  int _dbIndex;
  int _scaledCornerIdx;
  float _resFactor;
  float _ccFactor;
  float _gndFactor;
  extCorner* _extCornerPtr;
};

class extMain
{
 public:
  void init(odb::dbDatabase* db, Logger* logger);
  double getTotalCouplingCap(odb::dbNet* net,
                             const char* filterNet,
                             uint corner);

  uint calcMinMaxRC();
  void resetMinMaxRC(uint ii, uint jj);
  uint getExtStats(odb::dbNet* net,
                   uint corner,
                   int& wlen,
                   double& min_cap,
                   double& max_cap,
                   double& min_res,
                   double& max_res,
                   double& via_res,
                   uint& via_cnt);

  extMain();
  ~extMain();

  void set_debug_nets(const char* nets)
  {
    _debug_net_id = 0;
    if (nets != nullptr) {
      _debug_net_id = atoi(nets);
    }
  }

  static void createShapeProperty(odb::dbNet* net, int id, int id_val);
  static int getShapeProperty(odb::dbNet* net, int id);
  static int getShapeProperty_rc(odb::dbNet* net, int rc_id);

  uint getDir(int x1, int y1, int x2, int y2);
  uint getDir(odb::Rect& r);

  uint initSearchForNets(int* X1,
                         int* Y1,
                         uint* pitchTable,
                         uint* widthTable,
                         uint* dirTable,
                         odb::Rect& extRect,
                         bool skipBaseCalc);
  uint addNetSBoxes(odb::dbNet* net,
                    uint dir,
                    int* bb_ll,
                    int* bb_ur,
                    uint wtype,
                    dbCreateNetUtil* netUtil = nullptr);
  uint addNetSBoxes2(odb::dbNet* net,
                     uint dir,
                     int* bb_ll,
                     int* bb_ur,
                     uint wtype,
                     uint step = 0);
  uint addPowerNets(uint dir,
                    int* bb_ll,
                    int* bb_ur,
                    uint wtype,
                    dbCreateNetUtil* netUtil = nullptr);
  uint addNetShapesOnSearch(odb::dbNet* net,
                            uint dir,
                            int* bb_ll,
                            int* bb_ur,
                            uint wtype,
                            FILE* fp,
                            dbCreateNetUtil* netUtil = nullptr);
  int GetDBcoords2(int coord);
  void GetDBcoords2(odb::Rect& r);
  double GetDBcoords1(int coord);
  uint addViaBoxes(odb::dbShape& sVia,
                   odb::dbNet* net,
                   uint shapeId,
                   uint wtype);
  void getViaCapacitance(odb::dbShape svia, odb::dbNet* net);

  uint addSignalNets(uint dir,
                     int* bb_ll,
                     int* bb_ur,
                     uint wtype,
                     dbCreateNetUtil* createDbNet = nullptr);
  uint addNets(uint dir,
               int* bb_ll,
               int* bb_ur,
               uint wtype,
               uint ptype,
               Ath__array1D<uint>* sdbSignalTable);
  uint addNetOnTable(uint netId,
                     uint dir,
                     odb::Rect* maxRect,
                     uint* nm_step,
                     int* bb_ll,
                     int* bb_ur,
                     Ath__array1D<uint>*** wireTable);
  void getNetShapes(odb::dbNet* net,
                    odb::Rect** maxRectSdb,
                    odb::Rect& maxRectGs,
                    bool* hasSdbWires,
                    bool& hasGsWires);
  void getNetSboxes(odb::dbNet* net,
                    odb::Rect** maxRectSdb,
                    odb::Rect& maxRectGs,
                    bool* hasSdbWires,
                    bool& hasGsWires);
  uint addNetShapesGs(odb::dbNet* net,
                      bool gsRotated,
                      bool swap_coords,
                      int dir);
  uint addNetSboxesGs(odb::dbNet* net,
                      bool gsRotated,
                      bool swap_coords,
                      int dir);

  uint getBucketNum(int base, int max, uint step, int xy);
  int getXY_gs(int base, int XY, uint minRes);
  uint couplingFlow(odb::Rect& extRect,
                    uint ccFlag,
                    extMeasure* m,
                    CoupleAndCompute coupleAndCompute);
  uint initPlanes(uint dir,
                  int* wLL,
                  int* wUR,
                  uint layerCnt,
                  uint* pitchTable,
                  uint* widthTable,
                  const uint* dirTable,
                  int* bb_ll);

  bool isIncluded(odb::Rect& r, uint dir, const int* ll, const int* ur);
  bool matchDir(uint dir, odb::Rect& r);
  bool isIncludedInsearch(odb::Rect& r,
                          uint dir,
                          const int* bb_ll,
                          const int* bb_ur);

  uint makeTree(uint netId);
  uint runSolver(extMainOptions* opt, uint netId, int shapeId);

  void resetSumRCtable();
  void addToSumRCtable();
  void copyToSumRCtable();
  uint getResCapTable();
  double getLoCoupling();
  void ccReportProgress();
  void measureRC(CoupleOptions& options);
  void updateTotalRes(odb::dbRSeg* rseg1,
                      odb::dbRSeg* rseg2,
                      extMeasure* m,
                      const double* delta,
                      uint modelCnt);
  void updateTotalCap(odb::dbRSeg* rseg,
                      double frCap,
                      double ccCap,
                      double deltaFr,
                      uint modelIndex);
  void updateTotalCap(odb::dbRSeg* rseg,
                      extMeasure* m,
                      const double* deltaFr,
                      uint modelCnt,
                      bool includeCoupling,
                      bool includeDiag = false);
  void updateCCCap(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, double ccCap);
  double measureOverUnderCap(extMeasure* m, int x1, int y1, int x2, int y2);

  int setMinTypMax(bool min,
                   bool typ,
                   bool max,
                   int setMin,
                   int setTyp,
                   int setMax,
                   uint extDbCnt);

  extRCModel* getRCmodel(uint n);

  void calcRes0(double* deltaRes,
                uint tgtMet,
                uint width,
                uint len,
                int dist1 = 0,
                int dist2 = 0);
  double getLefResistance(uint level, uint width, uint length, uint model);
  double getResistance(uint level, uint width, uint len, uint model);
  double getFringe(uint met, uint width, uint modelIndex, double& areaCap);
  void printNet(odb::dbNet* net, uint netId);
  double calcFringe(extDistRC* rc, double deltaFr, bool includeCoupling);
  double updateTotalCap(odb::dbRSeg* rseg, double cap, uint modelIndex);
  bool updateCoupCap(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, int jj, double v);
  double updateRes(odb::dbRSeg* rseg, double res, uint model);

  uint getExtBbox(int* x1, int* y1, int* x2, int* y2);

  void setupMapping(uint itermCnt);
  uint getMultiples(uint cnt, uint base);
  uint getExtLayerCnt(odb::dbTech* tech);

  void setBlockFromChip();
  void setBlock(odb::dbBlock* block);
  odb::dbBlock* getBlock() { return _block; }
  odb::dbTech* getTech() { return _tech; }
  extRCModel* getRCModel() { return _modelTable->get(0); }

  void print_RC(odb::dbRSeg* rc);
  void resetMapping(odb::dbBTerm* term, odb::dbITerm* iterm, uint junction);
  uint resetMapNodes(odb::dbNet* net);
  void setResCapFromLef(odb::dbRSeg* rc,
                        uint targetCapId,
                        odb::dbShape& s,
                        uint len);
  bool isTermPathEnded(odb::dbBTerm* bterm, odb::dbITerm* iterm);
  uint getCapNodeId(odb::dbWirePath& path, odb::dbNet* net, bool branch);
  uint getCapNodeId(odb::dbNet* net,
                    odb::dbBTerm* bterm,
                    odb::dbITerm* iterm,
                    uint junction,
                    bool branch = false);
  void unlinkExt(std::vector<odb::dbNet*>& nets);
  void unlinkCC(std::vector<odb::dbNet*>& nets);
  void unlinkRSeg(std::vector<odb::dbNet*>& nets);
  void unlinkCapNode(std::vector<odb::dbNet*>& nets);
  void removeExt(std::vector<odb::dbNet*>& nets);
  void removeRSeg(std::vector<odb::dbNet*>& nets);
  void removeCapNode(std::vector<odb::dbNet*>& nets);
  void adjustRC(double resFactor, double ccFactor, double gndcFactor);
  void updatePrevControl();
  void getPrevControl();

  void makeBlockRCsegs(const char* netNames,
                       uint cc_up,
                       uint ccFlag,
                       double resBound,
                       bool mergeViaRes,
                       double ccThres,
                       int contextDepth,
                       const char* extRules);

  uint getShortSrcJid(uint jid);
  void make1stRSeg(odb::dbNet* net,
                   odb::dbWirePath& path,
                   uint cnid,
                   bool skipStartWarning);
  uint makeNetRCsegs_old(odb::dbNet* net, double resBound, uint debug = 0);
  uint makeNetRCsegs(odb::dbNet* net, bool skipStartWarning = false);
  double getViaResistance(odb::dbTechVia* tvia);
  double getViaResistance_b(odb::dbVia* via, odb::dbNet* net = nullptr);

  void getShapeRC(odb::dbNet* net,
                  const odb::dbShape& s,
                  const odb::Point& prevPoint,
                  const odb::dbWirePathShape& pshape);
  void setResAndCap(odb::dbRSeg* rc,
                    const double* restbl,
                    const double* captbl);
  odb::dbRSeg* addRSeg(odb::dbNet* net,
                       std::vector<uint>& rsegJid,
                       uint& srcId,
                       odb::Point& prevPoint,
                       const odb::dbWirePath& path,
                       const odb::dbWirePathShape& pshape,
                       bool isBranch,
                       const double* restbl,
                       const double* captbl);
  uint print_shape(const odb::dbShape& shape, uint j1, uint j2);
  uint getNodeId(odb::dbWirePath& path, bool branch, uint* nodeType);
  uint getNodeId(odb::dbWirePathShape& pshape, uint* nodeType);
  uint computePathDir(const odb::Point& p1, const odb::Point& p2, uint* length);
  uint openSpefFile(char* filename, uint mode);

  //-------------------------------------------------------------- SPEF
  uint calibrate(char* filename,
                 bool m_map,
                 float upperLimit,
                 float lowerLimit,
                 const char* dbCornerName,
                 int corner,
                 int spefCorner);
  uint readSPEF(char* filename,
                char* netNames,
                bool force,
                bool rConn,
                char* nodeCoord,
                bool rCap,
                bool rOnlyCCcap,
                bool rRes,
                float cc_thres,
                float cc_gnd_factor,
                float length_unit,
                bool m_map,
                bool noCapNumCollapse,
                char* capNodeMapFile,
                bool log,
                int corner,
                double low,
                double up,
                char* excludeSubWord,
                char* subWord,
                char* statsFile,
                const char* dbCornerName,
                const char* calibrateBaseCorner,
                int spefCorner,
                int fixLoop,
                bool keepLoadedCorner,
                bool stampWire = false,
                uint testParsing = 0,
                bool moreToRead = false,
                bool diff = false,
                bool calib = false,
                int app_print_limit = 0);
  uint readSPEFincr(char* filename);
  void writeSPEF(bool stop);
  uint writeSPEF(uint netId,
                 bool single_pi,
                 uint debug,
                 int corner,
                 const char* corner_name,
                 const char* spef_version);
  void writeSPEF(char* filename,
                 char* netNames,
                 bool noNameMap,
                 char* nodeCoord,
                 bool termJxy,
                 const char* capUnit,
                 const char* resUnit,
                 bool gzFlag,
                 bool stopAfterMap,
                 bool wClock,
                 bool wConn,
                 bool wCap,
                 bool wOnlyCCcap,
                 bool wRes,
                 bool noCnum,
                 bool initOnly,
                 bool single_pi,
                 bool noBackSlash,
                 int corner,
                 const char* corner_name,
                 const char* spef_version,
                 bool parallel);
  uint writeNetSPEF(odb::dbNet* net, double resBound, uint debug);
  uint makeITermCapNode(uint id, odb::dbNet* net);
  uint makeBTermCapNode(uint id, odb::dbNet* net);

  double getTotalNetCap(uint netId, uint cornerNum);
  void initContextArray();
  void initDgContextArray();
  void removeDgContextArray();

  // ruLESgeN
  bool getFirstShape(odb::dbNet* net, odb::dbShape& shape);
  uint writeRules(const char* name,
                  const char* topDir,
                  const char* rulesFile,
                  int pattern);
  uint benchWires(extMainOptions* options);
  uint genExtRules(const char* rulesFileName, int pattern);
  int getExtCornerIndex(odb::dbBlock* block, const char* cornerName);

  void initExtractedCorners(odb::dbBlock* block);

  void addDummyCorners(uint cornerCnt);
  static void addDummyCorners(odb::dbBlock* block, uint cnt, Logger* logger);
  char* addRCCorner(const char* name, int model, int userDefined = 1);
  char* addRCCornerScaled(const char* name,
                          uint model,
                          float resFactor,
                          float ccFactor,
                          float gndFactor);
  void getCorners(std::list<std::string>& ecl);
  void deleteCorners();
  void cleanCornerTables();
  int getDbCornerIndex(const char* name);
  int getDbCornerModel(const char* name);
  bool setCorners(const char* rulesFileName);
  int getProcessCornerDbIndex(int pcidx);
  void getScaledCornerDbIndex(int pcidx, int& scidx, int& scdbIdx);
  void getScaledRC(int sidx, double& res, double& cap);
  void getScaledGndC(int sidx, double& cap);
  void getScaledCC(int sidx, double& cap);
  void genScaledExt();
  void makeCornerNameMap();
  void getExtractedCorners();
  void makeCornerMapFromExtControl();
  bool checkLayerResistance();

  uint getNetBbox(odb::dbNet* net, odb::Rect& maxRect);

  void resetNetSpefFlag(Ath__array1D<uint>* tmpNetIdTable);

  uint sBoxCounter(odb::dbNet* net, uint& maxWidth);
  uint powerWireCounter(uint& maxWidth);
  uint signalWireCounter(uint& maxWidth);
  bool getRotatedFlag();
  bool enableRotatedFlag();

  uint addMultipleRectsOnSearch(odb::Rect& r,
                                uint level,
                                uint dir,
                                uint id,
                                uint shapeId,
                                uint wtype);

  //--------------- Window
  uint addShapeOnGS(odb::dbNet* net,
                    uint sId,
                    odb::Rect& r,
                    bool plane,
                    odb::dbTechLayer* layer,
                    bool gsRotated,
                    bool swap_coords,
                    int dir);

  uint fill_gs4(int dir,
                int* ll,
                int* ur,
                int* lo_gs,
                int* hi_gs,
                uint layerCnt,
                uint* dirTable,
                uint* pitchTable,
                uint* widthTable);

  uint addInsts(uint dir,
                int* lo_gs,
                int* hi_gs,
                int* bb_ll,
                int* bb_ur,
                uint bucketSize,
                Ath__array1D<uint>*** wireBinTable,
                dbCreateNetUtil* createDbNet);

  uint getNetBbox(odb::dbNet* net, odb::Rect* maxRect[2]);

  static odb::dbRSeg* getRseg(odb::dbNet* net, uint shapeId, Logger* logger);

  void write_spef_nets(bool flatten, bool parallel);
  extSpef* getSpef();

  uint getLayerSearchBoundaries(odb::dbTechLayer* layer,
                                int* xyLo,
                                int* xyHi,
                                uint* pitch);
  void railConn(uint dir,
                odb::dbTechLayer* layer,
                odb::dbNet* net,
                int* xyLo,
                int* xyHi,
                uint* pitch);
  void railConn(odb::dbNet* net);
  void railConn2(odb::dbNet* net);
  bool isSignalNet(odb::dbNet* net);
  uint powerRCGen();
  uint mergeRails(uint dir,
                  std::vector<odb::dbBox*>& boxTable,
                  std::vector<odb::Rect*>& mergeTable);
  odb::dbITerm* findConnect(odb::dbInst* inst,
                            odb::dbNet* net,
                            odb::dbNet* targetNet);
  uint getITermConn2(uint dir,
                     odb::dbWireEncoder& encoder,
                     odb::dbWire* wire,
                     odb::dbNet* net,
                     int* xy,
                     int* xy2);
  uint getITermConn(uint dir,
                    odb::dbWireEncoder& encoder,
                    odb::dbWire* wire,
                    odb::dbNet* net,
                    int* xy,
                    int* xy2);
  uint viaAndInstConn(uint dir,
                      uint width,
                      odb::dbTechLayer* layer,
                      odb::dbWireEncoder& encoder,
                      odb::dbWire* wire,
                      odb::dbNet* net,
                      odb::Rect* w,
                      bool skipSideMetalFlag);
  odb::dbNet* createRailNet(odb::dbNet* pnet,
                            odb::dbTechLayer* layer,
                            odb::Rect* w);
  uint print_shapes(FILE* fp, odb::dbWire* wire);
  FILE* openNanoFile(const char* name,
                     const char* name2,
                     const char* suffix,
                     const char* perms);
  void openNanoFiles();
  void closeNanoFiles();
  void setupNanoFiles(odb::dbNet* net);
  void writeResNode(FILE* fp, odb::dbCapNode* capNode, uint level);
  double writeRes(FILE* fp,
                  odb::dbNet* net,
                  uint level,
                  uint width,
                  uint dir,
                  bool skipFirst);
  uint connectStackedVias(odb::dbNet* net,
                          odb::dbTechLayer* layer,
                          bool mergeViaRes);
  uint via2viaConn(odb::dbNet* net,
                   odb::dbBox* v,
                   odb::dbTechLayer* layer,
                   bool mergeviaRes);
  void writeSubckt(FILE* fp,
                   const char* keyword,
                   const char* vdd,
                   const char* std,
                   const char* cont);
  void writeCapNodes(FILE* fp,
                     odb::dbNet* net,
                     uint level,
                     bool onlyVias,
                     bool skipFirst);
  void writeCapNodes_0713(FILE* fp, odb::dbNet* net, uint level, bool onlyVias);
  bool specialMasterType(odb::dbInst* inst);
  uint iterm2Vias(odb::dbInst* inst, odb::dbNet* net);
  uint getPowerNets(std::vector<odb::dbNet*>& powerNetTable);
  float getPowerViaRes(odb::dbBox* v, float val);
  uint findHighLevelPinMacros(std::vector<odb::dbInst*>& instTable);
  uint writeViaInfo(FILE* fp,
                    std::vector<odb::dbBox*>& viaTable,
                    bool m1Vias,
                    bool power);

  uint writeViaInfo_old(FILE* fp,
                        std::vector<odb::dbBox*>& viaTable,
                        bool m1Vias);
  uint writeViaCoords(FILE* fp,
                      std::vector<odb::dbBox*>& viaTable,
                      bool m1Vias);
  void writeViaName(char* nodeName,
                    odb::dbBox* v,
                    uint level,
                    const char* post);
  void writeViaName(FILE* fp, odb::dbBox* v, uint level, const char* post);
  void writeViaNameCoords(FILE* fp, odb::dbBox* v);
  float computeViaResistance(odb::dbBox* viaBox, uint& cutCount);
  void printItermNodeSubCkt(FILE* fp, std::vector<uint>& iTable);
  void printViaNodeSubCkt(FILE* fp, std::vector<odb::dbBox*>& viaTable);

  uint mergeStackedVias(FILE* fp,
                        odb::dbNet* net,
                        std::vector<odb::dbBox*>& viaTable,
                        odb::dbBox* botVia,
                        FILE* fp1 = nullptr);
  uint stackedViaConn(FILE* fp, std::vector<odb::dbBox*>& allViaTable);
  bool skipSideMetal(std::vector<odb::dbBox*>& viaTable,
                     uint level,
                     odb::dbNet* net,
                     odb::Rect* w);
  bool overlapWithMacro(odb::Rect& w);

  void powerWireConn(odb::Rect* w,
                     uint dir,
                     odb::dbTechLayer* layer,
                     odb::dbNet* net);
  const char* getBlockType(odb::dbMaster* m);
  void sortViasXY(uint dir, std::vector<odb::dbBox*>& viaTable);
  void writeViaRes(FILE* fp, odb::dbNet* net, uint level);
  void addUpperVia(uint ii, odb::dbBox* v);
  void writeViaResistors(FILE* fp,
                         uint ii,
                         FILE* fp1,
                         bool skipWireConn = false);
  void writeGeomHeader(FILE* fp, const char* vdd);
  void writeResNode(char* nodeName, odb::dbCapNode* capNode, uint level);
  float micronCoords(int xy);
  void writeSubcktNode(char* capNodeName, bool highMetal, bool vdd);
  float distributeCap(FILE* fp, odb::dbNet* net);

  uint readPowerSupplyCoords(char* filename);
  uint addPowerSources(std::vector<odb::dbBox*>& viaTable,
                       bool power,
                       uint level,
                       odb::Rect* powerWire);
  char* getPowerSourceName(bool power, uint level, uint vid);
  char* getPowerSourceName(uint level, uint vid);
  void writeViaInfo(FILE* fp, bool power);
  void addPowerSourceName(uint ii, char* sname);
  void writeResCoords(FILE* fp,
                      odb::dbNet* net,
                      uint level,
                      uint width,
                      uint dir);
  void writeViaName_xy(char* nodeName,
                       odb::dbBox* v,
                       uint bot,
                       uint top,
                       uint level,
                       const char* post = "");
  void writeInternalNode_xy(odb::dbCapNode* capNode, FILE* fp);
  void writeInternalNode_xy(odb::dbCapNode* capNode, char* buff);
  void createNode_xy(odb::dbCapNode* capNode,
                     int x,
                     int y,
                     int level,
                     odb::dbITerm* t = nullptr);
  uint setNodeCoords_xy(odb::dbNet* net, int level);
  bool sameJunctionPoint(int xy[2], int BB[2], uint width, uint dir);

  bool fisrt_markInst_UserFlag(odb::dbInst* inst, odb::dbNet* net);

  bool matchLayerDir(odb::dbBox* rail,
                     odb::dbTechLayerDir layerDir,
                     int level,
                     bool debug);
  void addSubcktStatement(const char* cirFile1, const char* subcktFile1);
  void setPrefix(char* prefix);
  uint getITermPhysicalConn(uint dir,
                            uint level,
                            odb::dbWireEncoder& encoder,
                            odb::dbWire* wire,
                            odb::dbNet* net,
                            int* xy,
                            int* xy2);
  void getSpecialItermShapes(odb::dbInst* inst,
                             odb::dbNet* specialNet,
                             uint dir,
                             uint level,
                             int* xy,
                             int* xy2,
                             std::vector<odb::Rect*>& rectTable,
                             std::vector<odb::dbITerm*>& itermTable);
  bool topHierBlock();

  void writeNegativeCoords(char* buf,
                           int netId,
                           int x,
                           int y,
                           int level,
                           const char* post = "");

  void writeViasAndClose(odb::dbNet* net, bool m1Vias);
  void closeNanoFilesDomainVDD(char* netName);
  void closeNanoFilesDomainGND(char* netName);
  void netDirPrefix(char* prefix, char* netName);
  FILE* openNanoFileNet(char* netname,
                        const char* name,
                        const char* name2,
                        const char* suffix,
                        const char* perms);
  void openNanoFilesDomain(odb::dbNet* pNet);
  void addSubcktStatementDomain(const char* cirFile1,
                                const char* subcktFile1,
                                const char* netName);
  void initMappingTables();
  void allocMappingTables(int n1, int n2, int n3);
  uint addSboxesOnSearch(odb::dbNet* net);
  odb::Rect* getRect_SBox(Ath__array1D<uint>* table,
                          uint ii,
                          bool filter,
                          uint dir,
                          uint& maxWidth);
  uint mergePowerWires(uint dir,
                       uint level,
                       std::vector<odb::Rect*>& mergeTable);
  void railConnOpt(odb::dbNet* net);
  uint initPowerSearch();
  uint overlapPowerWires(std::vector<odb::Rect*>& mergeTableHi,
                         std::vector<odb::Rect*>& mergeTableLo,
                         std::vector<odb::Rect*>& resultTable);
  odb::dbBox* createMultiVia(uint top, uint bot, odb::Rect* r);
  void mergeViasOnMetal_1(odb::Rect* w,
                          odb::dbNet* pNet,
                          uint level,
                          std::vector<odb::dbBox*>& viaTable);
  uint addGroupVias(uint level,
                    odb::Rect* w,
                    std::vector<odb::dbBox*>& viaTable);
  uint mergeStackedViasOpt(FILE* fp,
                           odb::dbNet* net,
                           std::vector<odb::dbBox*>& viaSearchTable,
                           odb::dbBox* botVia,
                           FILE* fp1,
                           uint stackLevel = 1);
  odb::dbCapNode* getITermPhysicalConnRC(odb::dbCapNode* srcCapNode,
                                         uint level,
                                         uint dir,
                                         odb::dbNet* net,
                                         int* xy,
                                         int* xy2,
                                         bool macro);
  uint viaAndInstConnRC(uint dir,
                        uint width,
                        odb::dbTechLayer* layer,
                        odb::dbNet* net,
                        odb::dbNet* orig_power_net,
                        odb::Rect* w,
                        bool skipSideMetalFlag);
  void powerWireConnRC(odb::Rect* w,
                       uint dir,
                       odb::dbTechLayer* layer,
                       odb::dbNet* net);
  odb::dbCapNode* getITermConnRC(odb::dbCapNode* srcCapNode,
                                 uint level,
                                 uint dir,
                                 odb::dbNet* net,
                                 int* xy,
                                 int* xy2);
  odb::dbCapNode* getPowerCapNode(odb::dbNet* net, int xy, uint level);
  odb::dbCapNode* makePowerRes(odb::dbCapNode* srcCap,
                               uint dir,
                               int xy[2],
                               uint level,
                               uint width,
                               uint objId,
                               int type);
  void createNode_xy_RC(char* buf,
                        odb::dbCapNode* capNode,
                        int x,
                        int y,
                        int level);
  void writeResNodeRC(char* nodeName, odb::dbCapNode* capNode, uint level);
  void writeResNodeRC(FILE* fp, odb::dbCapNode* capNode, uint level);
  double writeResRC(FILE* fp,
                    odb::dbNet* net,
                    uint level,
                    uint width,
                    uint dir,
                    bool skipFirst,
                    bool reverse,
                    bool onlyVias,
                    bool caps,
                    int xy[2]);
  void writeCapNodesRC(FILE* fp,
                       odb::dbNet* net,
                       uint level,
                       bool onlyVias,
                       bool skipFirst);
  void writeViaResistorsRC(FILE* fp, uint ii, FILE* fp1);
  void viaTagByCapNode(odb::dbBox* v, odb::dbCapNode* cap);
  char* getViaResNode(odb::dbBox* v, const char* propName);
  void writeMacroItermConns(odb::dbNet* net);
  void setupDirNaming();
  bool filterPowerGeoms(odb::dbSBox* s, uint targetDir, uint& maxWidth);

  uint iterm2Vias_cells(odb::dbInst* inst, odb::dbITerm* connectedPowerIterm);
  void writeCapNodesRC(FILE* fp,
                       odb::dbNet* net,
                       uint level,
                       bool onlyVias,
                       std::vector<odb::dbCapNode*>& capNodeTable);
  void writeOneCapNode(FILE* fp,
                       odb::dbCapNode* capNode,
                       uint level,
                       bool onlyVias);

  void findViaMainCoord(odb::dbNet* net, char* buff);
  void replaceItermCoords(odb::dbNet* net, uint dir, int xy[2]);

  void formOverlapVias(std::vector<odb::Rect*> mergeTable[16],
                       odb::dbNet* pNet);

  uint benchVerilog(FILE* fp);
  uint benchVerilog_bterms(FILE* fp,
                           const odb::dbIoType& iotype,
                           const char* prefix,
                           const char* postfix,
                           bool skip_postfix_last = false);
  uint benchVerilog_assign(FILE* fp);

  void setMinRC(uint ii, uint jj, extDistRC* rc);
  void setMaxRC(uint ii, uint jj, extDistRC* rc);

 private:
  Logger* logger_;

  bool _batchScaleExt = true;
  Ath__array1D<extCorner*>* _processCornerTable = nullptr;
  Ath__array1D<extCorner*>* _scaledCornerTable = nullptr;

  Ath__array1D<extRCModel*>* _modelTable;
  Ath__array1D<uint> _modelMap;  // TO_TEST
  Ath__array1D<extMetRCTable*> _metRCTable;
  double _resistanceTable[20][20];
  double _capacitanceTable[20][20];  // 20 layers by 20 rc models
  double _minWidthTable[20];
  uint _minDistTable[20];
  double _tmpCapTable[20];
  double _tmpSumCapTable[20];
  double _tmpResTable[20];
  double _tmpSumResTable[20];
  int _sumUpdated;
  int _minModelIndex;  // TO_TEST
  int _typModelIndex;  //
  int _maxModelIndex;  //

  odb::dbDatabase* _db = nullptr;
  odb::dbTech* _tech = nullptr;
  odb::dbBlock* _block = nullptr;
  uint _blockId;
  extSpef* _spef = nullptr;
  bool _writeNameMap = true;
  bool _fullIncrSpef = false;
  bool _noFullIncrSpef = false;
  char* _origSpefFilePrefix = nullptr;
  char* _newSpefFilePrefix = nullptr;
  uint _bufSpefCnt;
  bool _incrNoBackSlash;
  uint _cornerCnt = 0;
  uint _extDbCnt;

  int _remote;
  bool _extracted;
  bool _allNet;

  bool _getBandWire = false;
  bool _printBandInfo = false;
  uint _ccUp = 0;
  uint _couplingFlag = 0;
  bool _rotatedGs = false;
  uint _ccContextDepth = 0;
  int _ccMinX;
  int _ccMinY;
  int _ccMaxX;
  int _ccMaxY;
  double _mergeResBound = 0.0;
  bool _mergeViaRes = false;
  bool _mergeParallelCC = false;
  bool _reportNetNoWire = false;
  int _netNoWireCnt = 0;

  double _resFactor = 1.0;
  bool _resModify = false;
  double _ccFactor = 1.0;
  bool _ccModify = false;
  double _gndcFactor = 1.0;
  bool _gndcModify = false;

  float _netGndcCalibFactor;
  bool _netGndcCalibration;

  bool _useDbSdb;

  Ath__array1D<int>* _nodeTable = nullptr;   // junction id -> cap node id
  Ath__array1D<int>* _btermTable = nullptr;  // bterm id -> cap node id
  Ath__array1D<int>* _itermTable = nullptr;  // iterm id -> cap node id

  uint _dbPowerId = 1;
  uint _dbSignalId = 2;
  uint _RsegId;
  uint _CCsegId = 3;

  uint _CCnoPowerSource = 0;
  uint _CCnoPowerTarget = 0;
  int _x1;
  int _y1;
  int _x2;
  int _y2;

  double _coupleThreshold = 0.1;  // fF

  uint _totCCcnt;
  uint _totSmallCCcnt;
  uint _totBigCCcnt;
  uint _totSignalSegCnt;
  uint _totSegCnt;

  bool _noModelRC = false;
  extRCModel* _currentModel = nullptr;

  uint* _singlePlaneLayerMap = nullptr;
  bool _usingMetalPlanes = false;

  gs* _geomSeq = nullptr;

  AthPool<SEQ>* _seqPool = nullptr;

  Ath__array1D<SEQ*>*** _dgContextArray = nullptr;
  uint _dgContextDepth;
  uint _dgContextPlanes;
  uint _dgContextTracks;
  uint _dgContextBaseLvl;
  int _dgContextLowLvl;
  int _dgContextHiLvl;
  uint* _dgContextBaseTrack = nullptr;
  int* _dgContextLowTrack = nullptr;
  int* _dgContextHiTrack = nullptr;
  int** _dgContextTrackBase = nullptr;

  Ath__array1D<int>** _ccContextArray = nullptr;
  Ath__array1D<int>** _ccMergedContextArray = nullptr;

  uint _extRun = 0;
  odb::dbExtControl* _prevControl = nullptr;

  bool _foreign = false;
  bool _rsegCoord;
  bool _diagFlow = false;

  std::vector<uint> _rsegJid;
  std::vector<uint> _shortSrcJid;
  std::vector<uint> _shortTgtJid;

  std::vector<odb::dbBTerm*> _connectedBTerm;
  std::vector<odb::dbITerm*> _connectedITerm;

  Ath__gridTable* _search = nullptr;

  int _noVariationIndex;

  bool _ignoreWarning_1st;
  bool _keepExtModel;

  friend class extMeasure;

  FILE* _blkInfoVDD = nullptr;
  FILE* _viaInfoVDD = nullptr;
  FILE* _blkInfoGND = nullptr;
  FILE* _viaInfoGND = nullptr;

  FILE* _stdCirVDD = nullptr;
  FILE* _globCirVDD = nullptr;
  FILE* _globGeomVDD = nullptr;
  FILE* _stdCirGND = nullptr;
  FILE* _globCirGND = nullptr;

  FILE* _stdCirHeadVDD = nullptr;
  FILE* _globCirHeadVDD = nullptr;
  FILE* _globGeomGND = nullptr;
  FILE* _stdCirHeadGND = nullptr;
  FILE* _globCirHeadGND = nullptr;
  FILE* _blkInfo = nullptr;
  FILE* _viaInfo = nullptr;
  FILE* _globCir = nullptr;
  FILE* _globGeom = nullptr;
  FILE* _stdCir = nullptr;
  FILE* _globCirHead = nullptr;
  FILE* _stdCirHead = nullptr;
  FILE* _viaStackGlobCir = nullptr;
  FILE* _viaStackGlobVDD = nullptr;
  FILE* _viaStackGlobGND = nullptr;

  Ath__array1D<int>* _junct2viaMap = nullptr;
  bool _dbgPowerFlow;
  dbCreateNetUtil* _netUtil = nullptr;

  std::vector<odb::dbBox*> _viaUp_VDDtable;
  std::vector<odb::dbBox*> _viaUp_GNDtable;
  std::vector<odb::dbBox*> _viaM1_GNDtable;
  std::vector<odb::dbBox*> _viaM1_VDDtable;
  std::vector<odb::dbBox*>* _viaM1Table = nullptr;
  std::vector<odb::dbBox*>* _viaUpTable = nullptr;

  uint _stackedViaResCnt;
  uint _totViaResCnt;
  Ath__array1D<int>* _via2JunctionMap = nullptr;
  std::map<odb::dbBox*, odb::dbNet*> _via_map;
  std::map<uint, odb::dbNet*> _via_id_map;
  std::map<uint, float> _capNode_map;
  std::vector<odb::dbInst*> _powerMacroTable;
  std::vector<odb::dbBox*> _viaUpperTable[2];
  Ath__array1D<char*>** _supplyViaMap[2]{nullptr, nullptr};
  Ath__array1D<odb::dbBox*>** _supplyViaTable[2]{nullptr, nullptr};
  char* _power_source_file = nullptr;
  std::vector<char*> _powerSourceTable[2];
  FILE* _coordsFP = nullptr;
  FILE* _coordsGND = nullptr;
  FILE* _coordsVDD = nullptr;
  std::vector<uint> _vddItermIdTable;
  std::vector<uint> _gndItermIdTable;
  FILE* _subCktNodeFP[2][2]{{nullptr, nullptr}, {nullptr, nullptr}};
  uint _subCktNodeCnt[2][2];
  bool _nodeCoords;
  int _prevX;
  int _prevY;
  char _node_blk_dir[1024];
  char _node_blk_prefix[1024];
  char _node_inst_prefix[1024];
  Ath__array1D<odb::dbITerm*>* _junct2iterm = nullptr;
  std::map<uint, odb::dbSBox*> _sbox_id_map;

  uint _powerWireCnt;
  uint _mergedPowerWireCnt;
  uint _overlapPowerWireCnt;
  uint _viaOverlapPowerCnt;
  uint _multiViaCnt;

  std::vector<odb::Rect*> _multiViaTable[20];
  std::vector<odb::dbBox*> _multiViaBoxTable[20];

  uint _debug_net_id = 0;
  float _previous_percent_extracted = 0;

  double _minCapTable[64][64];
  double _maxCapTable[64][64];
  double _minResTable[64][64];
  double _maxResTable[64][64];
  uint _rcLayerCnt;
  uint _rcCornerCnt;

 public:
  bool _lef_res;
  std::string _tmpLenStats;
  int _last_node_xy[2];
  bool _wireInfra;
  odb::Rect _extMaxRect;
};

}  // namespace rcx
