// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbExtControl.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"
#include "odb/util.h"
#include "rcx/array1.h"
#include "rcx/dbUtil.h"
#include "rcx/ext2dBox.h"
#include "rcx/extPattern.h"
#include "rcx/extSegment.h"
#include "rcx/extSolverGen.h"
#include "rcx/extViaModel.h"
#include "rcx/ext_options.h"
#include "rcx/extprocess.h"
#include "rcx/util.h"
#include "utl/Logger.h"

namespace rcx {

class extMeasure;
class extMeasureRC;
struct SEQ;

class extSpef;
class GridTable;

// CoupleOptions seriously needs to be rewriten to use a class with named
// members. -cherry 05/09/2021
using CoupleOptions = std::array<int, 21>;
using CoupleAndCompute = void (*)(CoupleOptions&, void*);

class extDistRC
{
 public:
  // ----------------------------------------------- v2
  void Reset();
  void printBound(FILE* fp,
                  const char* loHi,
                  const char* layer_name,
                  uint32_t met,
                  uint32_t corner,
                  double res);

  // -----------------------------------------------------------

  void setLogger(utl::Logger* logger) { logger_ = logger; }
  void printDebug(const char*,
                  const char*,
                  uint32_t,
                  uint32_t,
                  extDistRC* rcUnit = nullptr);
  void printDebugRC_values(const char* msg);
  void printDebugRC(const char*, utl::Logger* logger);
  void printDebugRC(int met,
                    int overMet,
                    int underMet,
                    int width,
                    int dist,
                    int dbUnit,
                    utl::Logger* logger);
  void printDebugRC_sum(int len, int dbUnit, utl::Logger* logger);
  void printDebugRC_diag(int met,
                         int overMet,
                         int underMet,
                         int width,
                         int dist,
                         int dbUnit,
                         utl::Logger* logger);
  double GetDBcoords(int x, int db_factor);
  void set(uint32_t d, double cc, double fr, double a, double r);
  void readRC(Parser* parser, double dbFactor = 1.0);
  void readRC_res2(Parser* parser, double dbFactor = 1.0);
  double getFringe();
  double getFringeW();
  double getCoupling();
  double getDiag();
  double getRes();
  int getSep();
  double getTotalCap() { return coupling_ + fringe_ + diag_; };
  void setCoupling(double coupling);
  void setFringe(double fringe);
  void setFringeW(double fringew);
  void setRes(double res);
  void addRC(extDistRC* rcUnit, uint32_t len, bool addCC);
  void writeRC(FILE* fp, bool bin);
  void writeRC();
  void interpolate(uint32_t d, extDistRC* rc1, extDistRC* rc2);
  double interpolate_res(uint32_t d, extDistRC* rc2);

 private:
  int sep_;
  double coupling_;
  double fringe_;
  double fringeW_;
  double diag_;
  double res_;
  utl::Logger* logger_;

  friend class extDistRCTable;
  friend class extDistWidthRCTable;
  friend class extMeasure;
  friend class extMain;
};

class extDistRCTable
{
 public:
  // -------------------------------------- v2
  void getFringeTable(Array1D<int>* sTable,
                      Array1D<double>* rcTable,
                      bool compute);
  extDistRC* findRes(int dist1, int dist2, bool compute);

  // -------------------------------------------------------------

 public:
  extDistRCTable(uint32_t distCnt);
  ~extDistRCTable();

  extDistRC* getRC_99();
  void ScaleRes(double SUB_MULT_RES, Array1D<extDistRC*>* table);

  uint32_t addMeasureRC(extDistRC* rc);
  void makeComputeTable(uint32_t maxDist, uint32_t distUnit);
  extDistRC* getLastRC();
  extDistRC* getRC_index(int n);
  extDistRC* getComputeRC(double dist);
  extDistRC* getComputeRC(uint32_t dist);
  extDistRC* getRC(uint32_t s, bool compute);
  extDistRC* getComputeRC_res(uint32_t dist1, uint32_t dist2);
  extDistRC* findIndexed_res(uint32_t dist1, uint32_t dist2);
  int getComputeRC_maxDist();
  uint32_t writeRules(FILE* fp, Array1D<extDistRC*>* table, double w, bool bin);
  uint32_t writeRules(FILE* fp, double w, bool compute, bool bin);
  uint32_t writeDiagRules(FILE* fp,
                          Array1D<extDistRC*>* table,
                          double w1,
                          double w2,
                          double s,
                          bool bin);
  uint32_t writeDiagRules(FILE* fp,
                          double w1,
                          double w2,
                          double s,
                          bool compute,
                          bool bin);
  uint32_t readRules(Parser* parser,
                     AthPool<extDistRC>* rcPool,
                     bool compute,
                     bool bin,
                     bool ignore,
                     double dbFactor = 1.0);
  uint32_t readRules_res2(Parser* parser,
                          AthPool<extDistRC>* rcPool,
                          bool compute,
                          bool bin,
                          bool ignore,
                          double dbFactor = 1.0);
  uint32_t interpolate(uint32_t distUnit,
                       int maxDist,
                       AthPool<extDistRC>* rcPool);
  uint32_t mapInterpolate(extDistRC* rc1,
                          extDistRC* rc2,
                          uint32_t distUnit,
                          int maxDist,
                          AthPool<extDistRC>* rcPool);
  uint32_t mapExtrapolate(uint32_t loDist,
                          extDistRC* rc2,
                          uint32_t distUnit,
                          AthPool<extDistRC>* rcPool);

 private:
  void makeCapTableOver();
  void makeCapTableUnder();

  Array1D<extDistRC*>* measureTable_;
  Array1D<extDistRC*>* computeTable_;
  Array1D<extDistRC*>* measureTableR_[16];
  Array1D<extDistRC*>* computeTableR_[16];  // OPTIMIZE
  bool measureInR_;
  int maxDist_;
  uint32_t distCnt_;
  uint32_t unit_;
  utl::Logger* logger_;
};

class extDistWidthRCTable
{
 public:
  // ---------------------------------------------------------------------- v2
  // ---------
  uint32_t readRulesUnder(Parser* parser,
                          uint32_t widthCnt,
                          bool bin,
                          bool ignore,
                          const char* keyword,
                          double dbFactor = 1.0);
  void getFringeTable(uint32_t mou,
                      uint32_t w,
                      Array1D<int>* sTable,
                      Array1D<double>* rcTable,
                      bool map);

  // ---------------------------------------------------------------------------------------

  extDistWidthRCTable(bool dummy,
                      uint32_t met,
                      uint32_t layerCnt,
                      uint32_t width,
                      bool OUREVERSEORDER);
  extDistWidthRCTable(bool over,
                      uint32_t met,
                      uint32_t layerCnt,
                      uint32_t metCnt,
                      Array1D<double>* widthTable,
                      AthPool<extDistRC>* rcPool,
                      bool OUREVERSEORDER,
                      double dbFactor = 1.0);
  extDistWidthRCTable(bool over,
                      uint32_t met,
                      uint32_t layerCnt,
                      uint32_t metCnt,
                      Array1D<double>* widthTable,
                      int diagWidthCnt,
                      int diagDistCnt,
                      AthPool<extDistRC>* rcPool,
                      bool OUREVERSEORDER,
                      double dbFactor = 1.0);
  extDistWidthRCTable(bool over,
                      uint32_t met,
                      uint32_t layerCnt,
                      uint32_t metCnt,
                      uint32_t maxWidthCnt,
                      AthPool<extDistRC>* rcPool,
                      bool OUREVERSEORDER);
  void addRCw(uint32_t n, uint32_t w, extDistRC* rc);
  void createWidthMap();
  void makeWSmapping();

  // ------------------------------------ DKF 09212024
  // -------------------------------------
  uint32_t writeRulesOver(FILE* fp, const char* keyword, bool bin);
  uint32_t writeRulesUnder(FILE* fp, const char* keyword, bool bin);
  uint32_t writeRulesOverUnder(FILE* fp, const char* keyword, bool bin);

  ~extDistWidthRCTable();
  void setDiagUnderTables(uint32_t met,
                          Array1D<double>* diagWidthTable,
                          Array1D<double>* diagDistTable,
                          double dbFactor = 1.0);
  uint32_t getWidthIndex(uint32_t w);
  uint32_t getDiagWidthIndex(uint32_t m, uint32_t w);
  uint32_t getDiagDistIndex(uint32_t m, uint32_t s);
  uint32_t addCapOver(uint32_t met, uint32_t metUnder, extDistRC* rc);
  extDistRC* getCapOver(uint32_t met, uint32_t metUnder);
  uint32_t writeWidthTable(FILE* fp, bool bin);
  uint32_t writeDiagWidthTable(FILE* fp, uint32_t met, bool bin);
  uint32_t writeDiagDistTable(FILE* fp, uint32_t met, bool bin);
  void writeDiagTablesCnt(FILE* fp, uint32_t met, bool bin);
  uint32_t writeRulesOver(FILE* fp, bool bin);
  uint32_t writeRulesOver_res(FILE* fp, bool bin);
  uint32_t writeRulesUnder(FILE* fp, bool bin);
  uint32_t writeRulesDiagUnder(FILE* fp, bool bin);
  uint32_t writeRulesDiagUnder2(FILE* fp, bool bin);
  uint32_t writeRulesOverUnder(FILE* fp, bool bin);
  uint32_t getMetIndexUnder(uint32_t mOver);
  uint32_t readRulesOver(Parser* parser,
                         uint32_t widthCnt,
                         bool bin,
                         bool ignore,
                         const char* OVER,
                         double dbFactor = 1.0);
  uint32_t readRulesUnder(Parser* parser,
                          uint32_t widthCnt,
                          bool bin,
                          bool ignore,
                          double dbFactor = 1.0);
  uint32_t readRulesDiagUnder(Parser* parser,
                              uint32_t widthCnt,
                              uint32_t diagWidthCnt,
                              uint32_t diagDistCnt,
                              bool bin,
                              bool ignore,
                              double dbFactor = 1.0);
  uint32_t readRulesDiagUnder(Parser* parser,
                              uint32_t widthCnt,
                              bool bin,
                              bool ignore,
                              double dbFactor = 1.0);
  uint32_t readRulesOverUnder(Parser* parser,
                              uint32_t widthCnt,
                              bool bin,
                              bool ignore,
                              double dbFactor = 1.0);
  uint32_t readMetalHeader(Parser* parser,
                           uint32_t& met,
                           const char* keyword,
                           bool bin,
                           bool ignore);

  extDistRC* getRes(uint32_t mou, uint32_t w, int dist1, int dist2);
  extDistRC* getRC(uint32_t mou, uint32_t w, uint32_t s);
  extDistRC* getRC(uint32_t mou,
                   uint32_t w,
                   uint32_t dw,
                   uint32_t ds,
                   uint32_t s);
  extDistRC* getFringeRC(uint32_t mou, uint32_t w, int index_dist = -1);

  extDistRC* getLastWidthFringeRC(uint32_t mou);
  extDistRC* getRC_99(uint32_t mou, uint32_t w, uint32_t dw, uint32_t ds);
  extDistRCTable* getRuleTable(uint32_t mou, uint32_t w);

 public:
  bool _ouReadReverse;
  bool _over;
  uint32_t _layerCnt;
  uint32_t _met;

  static constexpr int diagDepth = 32;

  Array1D<int>* _widthTable = nullptr;
  Array1D<uint32_t>* _widthMapTable = nullptr;
  Array1D<int>* _diagWidthTable[diagDepth];
  Array1D<int>* _diagDistTable[diagDepth];
  Array1D<uint32_t>* _diagWidthMapTable[diagDepth];
  Array1D<uint32_t>* _diagDistMapTable[diagDepth];

  uint32_t _modulo;
  int _firstWidth = 0;
  int _lastWidth = -1;
  Array1D<int>* _firstDiagWidth = nullptr;
  Array1D<int>* _lastDiagWidth = nullptr;
  Array1D<int>* _firstDiagDist = nullptr;
  Array1D<int>* _lastDiagDist = nullptr;
  bool _widthTableAllocFlag;  // if false widthtable is pointer

  extDistRCTable*** _rcDistTable = nullptr;  // per over/under metal, per width
  extDistRCTable***** _rcDiagDistTable = nullptr;
  uint32_t _metCnt;  // if _over==false _metCnt???
  uint32_t _widthCnt;
  uint32_t _diagWidthCnt;
  uint32_t _diagDistCnt;

  AthPool<extDistRC>* _rcPoolPtr = nullptr;
  extDistRC* _rc31 = nullptr;
};

class extMetRCTable
{
 public:
  // -------------------------------------------- v2
  // ------------------------------
  void mkWidthAndSpaceMappings(uint32_t ii,
                               extDistWidthRCTable* table,
                               const char* keyword);
  // dkf 12272023
  bool GetViaRes(Parser* p, Parser* w, odb::dbNet* net, FILE* logFP);
  extViaModel* addViaModel(char* name,
                           double R,
                           uint32_t cCnt,
                           uint32_t dx,
                           uint32_t dy,
                           uint32_t top,
                           uint32_t bot);
  extViaModel* getViaModel(char* name);
  void printViaModels();
  // dkf 12282023
  void writeViaRes(FILE* fp);
  bool ReadRules(Parser* p);
  // dkf 12302023
  bool SkipPattern(Parser* p, odb::dbNet* net, FILE* logFP);
  // dkf 01022024
  uint32_t SetDefaultTechViaRes(odb::dbTech* tech, bool dbg);
  // ----------------------------------------------------------------------------------------
  extMetRCTable(uint32_t layerCnt,
                AthPool<extDistRC>* rcPool,
                utl::Logger* logger_,
                bool OUREVERSEORDER);
  ~extMetRCTable();

  // ---------------------- DKF 092024 -----------------------------------
  void allocOverUnderTable(uint32_t met,
                           bool open,
                           Array1D<double>* wTable,
                           double dbFactor);
  void allocUnderTable(uint32_t met,
                       bool open,
                       Array1D<double>* wTable,
                       double dbFactor);
  extDistWidthRCTable*** allocTable();
  void deleteTable(extDistWidthRCTable*** table);

  void allocateInitialTables(uint32_t widthCnt,
                             bool over,
                             bool under,
                             bool diag = false);
  void allocOverTable(uint32_t met,
                      Array1D<double>* wTable,
                      double dbFactor = 1.0);
  void allocDiagUnderTable(uint32_t met,
                           Array1D<double>* wTable,
                           int diagWidthCnt,
                           int diagDistCnt,
                           double dbFactor = 1.0);
  void allocDiagUnderTable(uint32_t met,
                           Array1D<double>* wTable,
                           double dbFactor = 1.0);
  void allocUnderTable(uint32_t met,
                       Array1D<double>* wTable,
                       double dbFactor = 1.0);
  void allocOverUnderTable(uint32_t met,
                           Array1D<double>* wTable,
                           double dbFactor = 1.0);
  void setDiagUnderTables(uint32_t met,
                          uint32_t overMet,
                          Array1D<double>* diagWTable,
                          Array1D<double>* diagSTable,
                          double dbFactor = 1.0);
  void addRCw(extMeasure* m);
  uint32_t readRCstats(Parser* parser);
  void mkWidthAndSpaceMappings();

  uint32_t addCapOver(uint32_t met, uint32_t metUnder, extDistRC* rc);
  uint32_t addCapUnder(uint32_t met, uint32_t metOver, extDistRC* rc);
  extDistRC* getCapOver(uint32_t met, uint32_t metUnder);
  extDistRC* getCapUnder(uint32_t met, uint32_t metOver);
  extDistRC* getOverFringeRC(extMeasure* m, int index_dist = -1);
  extDistRC* getOverFringeRC_last(int met, int width);
  AthPool<extDistRC>* getRCPool();

 public:
  uint32_t _layerCnt;
  uint32_t _wireCnt;
  char _name[128];
  extDistWidthRCTable*** _capOver_open;
  extDistWidthRCTable** _resOver;
  extDistWidthRCTable** _capOver;
  extDistWidthRCTable** _capUnder;
  extDistWidthRCTable*** _capUnder_open;
  extDistWidthRCTable** _capOverUnder;
  extDistWidthRCTable*** _capOverUnder_open;
  extDistWidthRCTable** _capDiagUnder;

  AthPool<extDistRC>* _rcPoolPtr;
  double _rate;
  utl::Logger* logger_;

  // dkf 092024
  Array1D<extViaModel*> _viaModel;
  AthHash<int> _viaModelHash;

 private:
  bool OUReverseOrder_;
};

class extRCTable
{
 public:
  extRCTable(bool over, uint32_t layerCnt);
  ~extRCTable();
  uint32_t addCapOver(uint32_t met, uint32_t metUnder, extDistRC* rc);
  extDistRC* getCapOver(uint32_t met, uint32_t metUnder);

 private:
  void makeCapTableOver();
  void makeCapTableUnder();

  bool _over;
  uint32_t _maxCnt1;
  Array1D<extDistRC*>*** _inTable;  // per metal per width
};

class extMain;
class extMeasure;
class extMainOptions;
class gs;

class extRCModel
{
 public:
  //------------------------------------------------------------------------ v2
  //----------------
  bool _v2_flow = false;

  extDistRC* getUnderRC(int met, int overMet, int width, int dist);
  extDistRC* getOverUnderRC(uint32_t met,
                            uint32_t underMet,
                            int overMet,
                            int width,
                            int dist);
  bool isRulesFile_v2(char* name, bool bin);

  // --------------------------------------------------------------- v2 CLEANUP
  // ----------
  bool readRules_v2(char* name,
                    bool bin,
                    bool over,
                    bool under,
                    bool overUnder,
                    bool diag,
                    double dbFactor);
  bool createModelProcessTable(uint32_t rulesFileModelCnt, uint32_t cornerCnt);
  // --------------------------------------------------------------- v2 CLEANUP
  // ----------

  // dkf 09222023
  uint32_t readRules_v2(Parser* parser,
                        uint32_t m,
                        uint32_t ii,
                        const char* ouKey,
                        const char* wKey,
                        bool over,
                        bool under,
                        bool bin,
                        bool diag,
                        bool ignore,
                        double dbFactor = 1.0);

  bool spotModelsInRules(char* name,
                         bool bin,
                         bool& res_over,
                         bool& over,
                         bool& under,
                         bool& overUnder,
                         bool& diag_under,
                         bool& over0,
                         bool& over1,
                         bool& under0,
                         bool& under1,
                         bool& overunder0,
                         bool& overunder1,
                         bool& via_res);
  uint32_t DefWires(extMainOptions* opt);
  uint32_t OverRulePat(extMainOptions* opt,
                       int len,
                       int LL[2],
                       int UR[2],
                       bool res,
                       bool diag,
                       uint32_t overDist);
  uint32_t UnderRulePat(extMainOptions* opt,
                        int len,
                        int LL[2],
                        int UR[2],
                        bool diag,
                        uint32_t overDist);
  uint32_t DiagUnderRulePat(extMainOptions* opt, int len, int LL[2], int UR[2]);
  uint32_t OverUnderRulePat(extMainOptions* opt, int len, int LL[2], int UR[2]);

  // dkf 12182023
  uint32_t ViaRulePat(extMainOptions* opt,
                      int len,
                      int origin[2],
                      int UR[2],
                      bool res,
                      bool diag,
                      uint32_t overDist);

  //------------------------------------------------------------------------ v2
  //----------------

  // --------------------------- DKF 092024 -------------------------
  // dkf 0323204

  // dkf 09172024
  uint32_t calcMinMaxRC(odb::dbTech* tech, const char* out_file);
  uint32_t getViaTechRes(odb::dbTech* tech, const char* out_file);

  extMain* get_extMain() { return _extMain; };
  bool getDiagFlag() { return _diag; };

  static int getMetIndexOverUnder(int met,
                                  int mUnder,
                                  int mOver,
                                  int layerCnt,
                                  int maxCnt = 1000);

  void clear_corners();
  bool addCorner(const std::string& w, int ii);
  uint32_t initModel(std::list<std::string>& corners, int met_cnt);
  uint32_t readRCvalues(const char* corner,
                        const char* filename,
                        int wire,
                        bool over,
                        bool under,
                        bool over_under,
                        bool diag);
  bool getAllowedPatternWireNums(Parser& p,
                                 extMeasure& m,
                                 const char* fullPatternName,
                                 int input_target_wire,
                                 int& pattern_num);
  uint32_t defineCorners(std::list<std::string>& corners);
  uint32_t allocateTables(uint32_t m, uint32_t ii, uint32_t diagModel);
  static int getMaxMetIndexOverUnder(int met, int layerCnt);

  bool parseMets(Parser& parser, extMeasure& m);
  double parseWidthDistLen(Parser& parser, extMeasure& m);

  uint32_t writeRulesPattern(uint32_t ou,
                             uint32_t layer,
                             int modelIndex,
                             extDistWidthRCTable* table_m,
                             extDistWidthRCTable* table_0,
                             const char* patternKeyword,
                             FILE* fp,
                             bool binary);
  uint32_t getCorners(std::list<std::string>& corners);
  uint32_t GenExtModel(std::list<std::string>& corner_list,
                       const char* out_file,
                       const char* comment,
                       const char* version,
                       int pattern);

  // --------------------------- DKF 092024 -------------------------

  // DKF 7/25/2024 -- 3d pattern generation
  bool _winDirFlat;
  int _len;
  int _simVersion;
  int _maxLevelDist;
  FILE* _filesFP;

  bool measurePatternVar_3D(extMeasure* m,
                            double top_width,
                            double bot_width,
                            double thickness,
                            uint32_t wireCnt,
                            char* wiresNameSuffix,
                            double res);
  uint32_t measureWithVar(extMeasure* measure);
  uint32_t measureDiagWithVar(extMeasure* measure);
  uint32_t linesOver(uint32_t wireCnt,
                     uint32_t widthCnt,
                     uint32_t spaceCnt,
                     uint32_t dCnt,
                     uint32_t metLevel = 0);
  uint32_t linesOverUnder(uint32_t wireCnt,
                          uint32_t widthCnt,
                          uint32_t spaceCnt,
                          uint32_t dCnt,
                          uint32_t metLevel = 0);
  uint32_t linesDiagUnder(uint32_t wireCnt,
                          uint32_t widthCnt,
                          uint32_t spaceCnt,
                          uint32_t dCnt,
                          uint32_t metLevel = 0);
  uint32_t linesUnder(uint32_t wireCnt,
                      uint32_t widthCnt,
                      uint32_t spaceCnt,
                      uint32_t dCnt,
                      uint32_t metLevel = 0);
  void setOptions(const char* topDir,
                  const char* pattern,
                  bool keepFile,
                  uint32_t metLevel);
  void writeRuleWires(FILE* fp, extMeasure* measure, uint32_t wireCnt);
  void writeWires2_3D(FILE* fp, extMeasure* measure, uint32_t wireCnt);
  void writeWires(FILE* fp, extMeasure* measure, uint32_t wireCnt);

  double writeWirePatterns(FILE* fp,
                           extMeasure* measure,
                           uint32_t wireCnt,
                           double height_offset,
                           double& len,
                           double& max_x);
  double writeWirePatterns_w3(FILE* fp,
                              extMeasure* measure,
                              uint32_t wireCnt,
                              double height_offset,
                              double& len,
                              double& max_x);
  // ------------------------------------------------------------------

  extMetRCTable* getMetRCTable(uint32_t ii) { return _modelTable[ii]; };

  int getModelCnt() { return _modelCnt; };
  int getLayerCnt() { return _layerCnt; };
  void setLayerCnt(uint32_t n) { _layerCnt = n; };
  int getDiagModel() { return _diagModel; };
  bool getVerticalDiagFlag() { return _verticalDiag; };
  void setDiagModel(uint32_t i) { _diagModel = i; }
  extRCModel(uint32_t layerCnt, const char* name, utl::Logger* logger);
  extRCModel(const char* name, utl::Logger* logger);
  extProcess* getProcess();
  uint32_t findBiggestDatarateIndex(double d);
  ~extRCModel();
  void setExtMain(extMain* x);
  void createModelTable(uint32_t n, uint32_t layerCnt);

  uint32_t addLefTotRC(uint32_t met, uint32_t underMet, double fr, double r);
  uint32_t addCapOver(uint32_t met,
                      uint32_t underMet,
                      uint32_t d,
                      double cc,
                      double fr,
                      double a,
                      double r);
  double getTotCapOverSub(uint32_t met);
  double getRes(uint32_t met);

  uint32_t benchWithVar_density(extMainOptions* opt, extMeasure* measure);
  uint32_t benchWithVar_lists(extMainOptions* opt, extMeasure* measure);

  uint32_t runWiresSolver(uint32_t netId, int shapeId);

  void setProcess(extProcess* p);
  void setDataRateTable(uint32_t met);
  uint32_t singleLineOver(uint32_t widthCnt);
  uint32_t twoLineOver(uint32_t widthCnt, uint32_t spaceCnt);
  uint32_t threeLineOver(uint32_t widthCnt, uint32_t spaceCnt);
  uint32_t getCapValues(uint32_t lastNode,
                        double& cc1,
                        double& cc2,
                        double& fr,
                        double& tot,
                        extMeasure* m);
  uint32_t getCapMatrixValues(uint32_t lastNode, extMeasure* m);
  uint32_t readCapacitance(uint32_t wireNum,
                           double& cc1,
                           double& cc2,
                           double& fr,
                           double& tot,
                           bool readCapLog,
                           extMeasure* m = nullptr);
  uint32_t readCapacitanceBench(bool readCapLog, extMeasure* m);
  uint32_t readCapacitanceBenchDiag(bool readCapLog, extMeasure* m);
  extDistRC* readCap(uint32_t wireCnt, double w, double s, double r);
  uint32_t readCap(uint32_t wireCnt,
                   double cc1,
                   double cc2,
                   double fr,
                   double tot);
  FILE* openFile(const char* topDir,
                 const char* name,
                 const char* suffix,
                 const char* permissions);
  void mkNet_prefix(extMeasure* m, const char* wiresNameSuffix);
  void mkFileNames(extMeasure* m, char* wiresNameSuffix);
  void writeWires2(FILE* fp, extMeasure* measure, uint32_t wireCnt);
  int writeBenchWires(FILE* fp, extMeasure* measure);
  void setOptions(const char* topDir, const char* pattern);
  void setOptions(const char* topDir, const char* pattern, bool keepFile);
  void cleanFiles();

  extDistRC* measurePattern(uint32_t met,
                            int underMet,
                            int overMet,
                            double width,
                            double space,
                            uint32_t wireCnt,
                            char* ou,
                            bool readCapLog);
  bool measurePatternVar(extMeasure* m,
                         double top_width,
                         double bot_width,
                         double thickness,
                         uint32_t wireCnt,
                         char* wiresNameSuffix,
                         double res = 0.0);
  double measureResistance(extMeasure* m,
                           double ro,
                           double top_widthR,
                           double bot_widthR,
                           double thicknessR);

  uint32_t linesOverBench(extMainOptions* opt);
  uint32_t linesUnderBench(extMainOptions* opt);
  uint32_t linesDiagUnderBench(extMainOptions* opt);
  uint32_t linesOverUnderBench(extMainOptions* opt);

  uint32_t benchWithVar(extMeasure* measure);
  void addRC(extMeasure* m);
  int getOverUnderIndex(extMeasure* m, uint32_t maxCnt);
  uint32_t getUnderIndex(extMeasure* m);
  extDistWidthRCTable* getWidthDistRCtable(uint32_t met,
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
                     uint32_t wireCnt,
                     uint32_t widthCnt,
                     uint32_t spaceCnt,
                     uint32_t dCnt);

  void getDiagTables(extMeasure* m, uint32_t widthCnt, uint32_t spaceCnt);
  void setDiagUnderTables(extMeasure* m);
  void getRCtable(uint32_t met,
                  int mUnder,
                  int OverMet,
                  double w,
                  double dRate);
  void getRCtable(Array1D<int>* sTable,
                  Array1D<double>* rcTable,
                  uint32_t valType,
                  uint32_t met,
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
                 uint32_t cornerCnt = 0,
                 const uint32_t* cornerTable = nullptr,
                 double dbFactor = 1.0);
  bool readRules_v1(char* name,
                    bool binary,
                    bool over,
                    bool under,
                    bool overUnder,
                    bool diag,
                    uint32_t cornerCnt = 0,
                    const uint32_t* cornerTable = nullptr,
                    double dbFactor = 1.0);
  uint32_t readMetalHeader(Parser* parser,
                           uint32_t& met,
                           const char* keyword,
                           bool bin,
                           bool ignore);
  Array1D<double>* readHeaderAndWidth(Parser* parser,
                                      uint32_t& met,
                                      const char* ouKey,
                                      const char* wKey,
                                      bool bin,
                                      bool ignore);
  uint32_t readRules(Parser* parser,
                     uint32_t m,
                     uint32_t ii,
                     const char* ouKey,
                     const char* wKey,
                     bool over,
                     bool under,
                     bool bin,
                     bool diag,
                     bool ignore,
                     double dbFactor = 1.0);

  extDistRC* getOverFringeRC(uint32_t met, uint32_t underMet, uint32_t width);
  double getFringeOver(uint32_t met, uint32_t mUnder, uint32_t w, uint32_t s);
  double getCouplingOver(uint32_t met, uint32_t mUnder, uint32_t w, uint32_t s);
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
  uint32_t getMaxCnt(int met)
  {
    return _modelTable[_tmpDataRate]->_capOverUnder[met]->_metCnt;
  }
  uint32_t benchDB_WS(extMainOptions* opt, extMeasure* measure);
  int writeBenchWires_DB(extMeasure* measure);
  int writeBenchWires_DB_res(extMeasure* measure);

  int writeBenchWires_DB_diag(extMeasure* measure);
  extMetRCTable* initCapTables(uint32_t layerCnt, uint32_t widthCnt);

  extDistRC* getMinRC(int met, int width);
  extDistRC* getMaxRC(int met, int width, int dist);

 private:
  // -------------------------------------- DKF 092024
  // -------------------------------------
  std::map<std::string, int> _cornerMap;
  std::vector<std::string> _cornerTable;
  // -------------------------------------- DKF 092024
  // -------------------------------------

  bool _ouReadReverse;
  uint32_t _layerCnt;
  char _name[128];

  int _noVariationIndex;
  uint32_t _modelCnt;
  Array1D<double>* _dataRateTable;
  Array1D<int>* _dataRateTableMap;
  extMetRCTable** _modelTable;
  uint32_t _tmpDataRate;
  bool _diag;
  bool _verticalDiag;
  bool _maxMinFlag;
  uint32_t _diagModel;
  uint32_t _metLevel;

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
  Parser* _parser;
  char* _solverFileName;

  FILE* _capLogFP;

  bool _writeFiles;
  bool _readSolver;
  bool _runSolver;

  bool _readCapLog;
  char _commentLine[10000];
  bool _commentFlag;

  uint32_t* _singlePlaneLayerMap;

  extMain* _extMain;

  bool OUReverseOrder_{false};

 protected:
  utl::Logger* logger_;
};

class extLenOU  // assume cross-section on the z-direction
{
 public:
  void reset();
  void addOverOrUnderLen(int met, bool over, uint32_t len);
  void addOULen(int underMet, int overMet, uint32_t len);

 public:
  int _met;
  int _underMet;
  int _overMet;
  uint32_t _len;
  bool _over;
  bool _overUnder;
  bool _under;
};

class extMeasure
{
 public:
  // ------------------------------------------------- v2
  double _topWidthR;
  double _botWidthR;
  double _teffR;
  double _peffR;
  bool _skipResCalc = false;

  GridTable* _search = nullptr;
  bool IsDebugNet1();
  static int getMetIndexOverUnder(int met,
                                  int mUnder,
                                  int mOver,
                                  int layerCnt,
                                  int maxCnt = 10000);

  // dkf 09122023
  int SingleDiagTrackDist_opt(SEQ* s,
                              Array1D<SEQ*>* dgContext,
                              bool skipZeroDist,
                              bool skipNegativeDist,
                              Array1D<int>* sortedDistTable,
                              Array1D<SEQ*>* segFilteredTable);
  // dkf 08022023
  int SingleDiagTrackDist(SEQ* s,
                          Array1D<SEQ*>* dgContext,
                          bool skipZeroDist,
                          bool skipNegativeDist,
                          std::vector<int>& distTable,
                          Array1D<SEQ*>* segFilteredTable);
  // dkf 08022023
  int DebugPrint(SEQ* s, Array1D<SEQ*>* dgContext, int trackNum, int plane);
  // dkf 08022023
  uint32_t computeResLoop(Array1D<SEQ*>* tmpTable,
                          Array1D<SEQ*>* dgTable,
                          uint32_t targetMet,
                          uint32_t dir,
                          uint32_t planeIndex,
                          uint32_t trackn,
                          Array1D<SEQ*>* residueSeq);
  uint32_t computeRes(SEQ* s,
                      Array1D<SEQ*>*,
                      uint32_t targetMet,
                      uint32_t dir,
                      uint32_t planeIndex,
                      uint32_t trackn,
                      Array1D<SEQ*>* residueSeq);
  void DebugRes_calc(FILE* fp,
                     const char* msg,
                     int rsegId1,
                     const char* msg_len,
                     uint32_t len,
                     int dist1,
                     int dist2,
                     int tgtMet,
                     double tot,
                     double R,
                     double unit,
                     double prev);

  extDistRC* findRes(int dist1, int dist2, bool compute);
  bool DebugDiagCoords(int met,
                       int targetMet,
                       int len1,
                       int diagDist,
                       int ll[2],
                       int ur[2],
                       const char* = "");

  // ------------------------------------------------------------
  // DKF 7/25/2024 -- 3d pattern generation
  bool _3dFlag;
  bool _benchFlag;
  bool _rcValid;
  int _simVersion;
  double getCCfringe3D(uint32_t lastNode,
                       uint32_t n,
                       uint32_t start,
                       uint32_t end);

  extMeasure(utl::Logger* logger);
  ~extMeasure();

  void rcNetInfo();
  bool rcSegInfo();
  bool ouCovered_debug(int covered);
  void segInfo(const char* msg, uint32_t netId, int rsegId);
  bool isVia(uint32_t rsegId);
  bool ouRCvalues(const char* msg, uint32_t jj);
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
  double GetDBcoords(uint32_t coord);
  double GetDBcoords(int coord);
  void printNetCaps();

  void printTraceNetInfo(const char* msg, uint32_t netId, int rsegId);
  bool printTraceNet(const char* msg,
                     bool init,
                     odb::dbCCSeg* cc = nullptr,
                     uint32_t overSub = 0,
                     uint32_t covered = 0);

  extDistRC* areaCapOverSub(uint32_t modelNum, extMetRCTable* rcModel);

  extDistRC* getUnderLastWidthDistRC(extMetRCTable* rcModel, uint32_t overMet);
  void createCap(int rsegId1, uint32_t rsegId2, double* capTable);
  void areaCap(int rsegId1, uint32_t rsegId2, uint32_t len, uint32_t tgtMet);
  bool verticalCap(int rsegId1,
                   uint32_t rsegId2,
                   uint32_t len,
                   uint32_t tgtWidth,
                   uint32_t diagDist,
                   uint32_t tgtMet);
  extDistRC* getVerticalUnderRC(extMetRCTable* rcModel,
                                uint32_t diagDist,
                                uint32_t tgtWidth,
                                uint32_t overMet);

  odb::dbRSeg* getRseg(const char* netname,
                       const char* capMsg,
                       const char* tableEntryName);
  bool getFirstShape(odb::dbNet* net, odb::dbShape& s);

  void swap_coords(SEQ* s);
  uint32_t swap_coords(uint32_t initCnt,
                       uint32_t endCnt,
                       Array1D<SEQ*>* resTable);
  uint32_t getOverlapSeq(uint32_t met, SEQ* s, Array1D<SEQ*>* resTable);
  uint32_t getOverlapSeq(uint32_t met,
                         int* ll,
                         int* ur,
                         Array1D<SEQ*>* resTable);

  bool isConnectedToBterm(odb::dbRSeg* rseg1);
  uint32_t defineBox(CoupleOptions& options);
  void printCoords(FILE* fp);
  void printNet(odb::dbRSeg* rseg, uint32_t netId);
  void updateBox(uint32_t w_layout, uint32_t s_layout, int dir = -1);
  void printBox(FILE* fp);
  uint32_t initWS_box(extMainOptions* opt, uint32_t gridCnt);
  odb::dbRSeg* getFirstDbRseg(uint32_t netId);
  uint32_t createNetSingleWire(char* dirName,
                               uint32_t idCnt,
                               uint32_t w_layout,
                               uint32_t s_layout,
                               int dir = -1);
  uint32_t createDiagNetSingleWire(char* dirName,
                                   uint32_t idCnt,
                                   int begin,
                                   int w_layout,
                                   int s_layout,
                                   int dir = -1);
  uint32_t createNetSingleWire_cntx(int met,
                                    char* dirName,
                                    uint32_t idCnt,
                                    int d,
                                    int ll[2],
                                    int ur[2],
                                    int s_layout = -1);
  uint32_t createContextNets(char* dirName,
                             const int bboxLL[2],
                             const int bboxUR[2],
                             int met,
                             double pitchMult);
  uint32_t getPatternExtend();

  uint32_t createContextObstruction(const char* dirName,
                                    int x1,
                                    int y1,
                                    int bboxUR[2],
                                    int met,
                                    double pitchMult);
  uint32_t createContextGrid(char* dirName,
                             const int bboxLL[2],
                             const int bboxUR[2],
                             int met,
                             int s_layout = -1);
  uint32_t createContextGrid_dir(char* dirName,
                                 const int bboxLL[2],
                                 const int bboxUR[2],
                                 int met);

  double getCCfringe(uint32_t lastNode,
                     uint32_t n,
                     uint32_t start,
                     uint32_t end);

  void updateForBench(extMainOptions* opt, extMain* extMain);
  uint32_t measureOverUnderCap();
  uint32_t measureOverUnderCap_orig(gs* pixelTable,
                                    uint32_t** ouPixelTableIndexMap);
  uint32_t getSeqOverOrUnder(Array1D<SEQ*>* seqTable,
                             gs* pixelTable,
                             uint32_t met,
                             Array1D<SEQ*>* resTable);
  uint32_t computeOverOrUnderSeq(Array1D<SEQ*>* seqTable,
                                 uint32_t met,
                                 Array1D<SEQ*>* resTable,
                                 bool over);
  uint32_t computeOverUnder(int* ll, int* ur, Array1D<SEQ*>* resTable);

  void release(Array1D<SEQ*>* seqTable, gs* pixelTable = nullptr);
  void addSeq(Array1D<SEQ*>* seqTable, gs* pixelTable);
  void addSeq(const int* ll,
              const int* ur,
              Array1D<SEQ*>* seqTable,
              gs* pixelTable = nullptr);
  SEQ* addSeq(const int* ll, const int* ur);
  void copySeq(SEQ* t, Array1D<SEQ*>* seqTable, gs* pixelTable);
  void tableCopyP(Array1D<SEQ*>* src, Array1D<SEQ*>* dst);
  void tableCopy(Array1D<SEQ*>* src, Array1D<SEQ*>* dst, gs* pixelTable);

  uint32_t measureDiagFullOU();
  uint32_t ouFlowStep(Array1D<SEQ*>* overTable);
  int underFlowStep(Array1D<SEQ*>* srcTable, Array1D<SEQ*>* overTable);

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

  void copySeqUsingPool(SEQ* t, Array1D<SEQ*>* seqTable);
  void seq_release(Array1D<SEQ*>* table);
  void calcOU(uint32_t len);
  void calcRC(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, uint32_t totLenCovered);
  int getMaxDist(int tgtMet, uint32_t modelIndex);
  void calcRes(int rsegId1, uint32_t len, int dist1, int dist2, int tgtMet);
  void calcRes0(double* deltaRes,
                uint32_t tgtMet,
                uint32_t len,
                int dist1 = 0,
                int dist2 = 0);
  uint32_t computeRes(SEQ* s,
                      uint32_t targetMet,
                      uint32_t dir,
                      uint32_t planeIndex,
                      uint32_t trackn,
                      Array1D<SEQ*>* residueSeq);
  int computeResDist(SEQ* s,
                     uint32_t trackMin,
                     uint32_t trackMax,
                     uint32_t targetMet,
                     Array1D<SEQ*>* diagTable);
  uint32_t computeDiag(SEQ* s,
                       uint32_t targetMet,
                       uint32_t dir,
                       uint32_t planeIndex,
                       uint32_t trackn,
                       Array1D<SEQ*>* residueSeq);

  odb::dbCCSeg* makeCcap(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, double ccCap);
  void addCCcap(odb::dbCCSeg* ccap, double v, uint32_t model);
  void addFringe(odb::dbRSeg* rseg1,
                 odb::dbRSeg* rseg2,
                 double frCap,
                 uint32_t model);
  void calcDiagRC(int rsegid1,
                  uint32_t rsegid2,
                  uint32_t len,
                  uint32_t diagWidth,
                  uint32_t diagDist,
                  uint32_t tgtMet);
  void calcDiagRC(int rsegid1,
                  uint32_t rsegid2,
                  uint32_t len,
                  uint32_t dist,
                  uint32_t tgtMet);
  int calcDist(const int* ll, const int* ur);

  void ccReportProgress();
  uint32_t getOverUnderIndex();

  uint32_t getLength(SEQ* s, int dir);
  uint32_t blackCount(uint32_t start, Array1D<SEQ*>* resTable);
  extDistRC* computeR(uint32_t len, double* valTable);
  extDistRC* computeOverFringe(uint32_t overMet,
                               uint32_t overWidth,
                               uint32_t len,
                               uint32_t dist);
  extDistRC* computeUnderFringe(uint32_t underMet,
                                uint32_t underWidth,
                                uint32_t len,
                                uint32_t dist);

  double getDiagUnderCC(extMetRCTable* rcModel,
                        uint32_t dist,
                        uint32_t overMet);
  double getDiagUnderCC(extMetRCTable* rcModel,
                        uint32_t diagWidth,
                        uint32_t diagDist,
                        uint32_t overMet);
  extDistRC* getDiagUnderCC2(extMetRCTable* rcModel,
                             uint32_t diagWidth,
                             uint32_t diagDist,
                             uint32_t overMet);
  extDistRC* computeOverUnderRC(uint32_t len);
  extDistRC* computeOverRC(uint32_t len);
  extDistRC* computeUnderRC(uint32_t len);
  extDistRC* getOverUnderFringeRC(extMetRCTable* rcModel);
  extDistRC* getOverUnderRC(extMetRCTable* rcModel);
  extDistRC* getOverRC(extMetRCTable* rcModel);
  uint32_t getUnderIndex();
  uint32_t getUnderIndex(uint32_t overMet);
  extDistRC* getUnderRC(extMetRCTable* rcModel);

  extDistRC* getFringe(uint32_t len, double* valTable);

  void tableCopyP(Array1D<int>* src, Array1D<int>* dst);
  void getMinWidth(odb::dbTech* tech);
  uint32_t measureOverUnderCapCJ();
  uint32_t computeOverUnder(int xy1, int xy2, Array1D<int>* resTable);
  uint32_t computeOUwith2planes(int* ll, int* ur, Array1D<SEQ*>* resTable);
  uint32_t intersectContextArray(int pmin,
                                 int pmax,
                                 uint32_t met1,
                                 uint32_t met2,
                                 Array1D<int>* tgtContext);
  uint32_t computeOverOrUnderSeq(Array1D<int>* seqTable,
                                 uint32_t met,
                                 Array1D<int>* resTable,
                                 bool over);
  bool updateLengthAndExit(int& remainder, int& totCovered, int len);
  int compute_Diag_Over_Under(Array1D<SEQ*>* seqTable, Array1D<SEQ*>* resTable);
  int compute_Diag_OverOrUnder(Array1D<SEQ*>* seqTable,
                               bool over,
                               uint32_t met,
                               Array1D<SEQ*>* resTable);
  uint32_t measureUnderOnly(bool diagFlag);
  uint32_t measureOverOnly(bool diagFlag);
  uint32_t measureDiagOU(uint32_t ouLevelLimit, uint32_t diagLevelLimit);

  uint32_t mergeContextArray(Array1D<int>* srcContext,
                             int minS,
                             Array1D<int>* tgtContext);
  uint32_t mergeContextArray(Array1D<int>* srcContext,
                             int minS,
                             int pmin,
                             int pmax,
                             Array1D<int>* tgtContext);
  uint32_t intersectContextArray(Array1D<int>* s1Context,
                                 Array1D<int>* s2Context,
                                 int minS1,
                                 int minS2,
                                 Array1D<int>* tgtContext);

  extDistRC* addRC(extDistRC* rcUnit, uint32_t len, uint32_t jj);

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
  void printMets(FILE* fp);

  ext2dBox* addNew2dBox(odb::dbNet* net,
                        int* ll,
                        int* ur,
                        uint32_t m,
                        bool cntx);
  void clean2dBoxTable(int met, bool cntx);
  void writeRaphaelPointXY(FILE* fp, double X, double Y);
  void getBox(int met, bool cntx, int& xlo, int& ylo, int& xhi, int& yhi);

  int getDgPlaneAndTrackIndex(uint32_t tgt_met,
                              int trackDist,
                              int& loTrack,
                              int& hiTrack);
  int computeDiagOU(SEQ* s, uint32_t targetMet, Array1D<SEQ*>* residueSeq);
  int computeDiagOU(SEQ* s,
                    uint32_t trackMin,
                    uint32_t trackMax,
                    uint32_t targetMet,
                    Array1D<SEQ*>* diagTable);
  void printDgContext();
  void initTargetSeq();
  void getDgOverlap(CoupleOptions& options);
  void getDgOverlap(SEQ* sseq,
                    uint32_t dir,
                    Array1D<SEQ*>* dgContext,
                    Array1D<SEQ*>* overlapSeq,
                    Array1D<SEQ*>* residueSeq);
  void getDgOverlap_res(SEQ* sseq,
                        uint32_t dir,
                        Array1D<SEQ*>* dgContext,
                        Array1D<SEQ*>* overlapSeq,
                        Array1D<SEQ*>* residueSeq);

  uint32_t getRSeg(odb::dbNet* net, uint32_t shapeId);

  void allocOUpool();
  int get_nm(double n) { return 1000 * (n / _dbunit); };

  bool _skip_delims;
  bool _no_debug;

  int _met;
  int _underMet;
  int _overMet;
  uint32_t _wireCnt;

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

  uint32_t _wIndex;
  uint32_t _sIndex;
  uint32_t _dwIndex;
  uint32_t _dsIndex;
  uint32_t _rIndex;
  uint32_t _pIndex;

  double _topWidth;
  double _botWidth;
  double _teff;
  double _heff;
  double _seff;

  bool _varFlag;
  bool _open;
  bool _over1;
  bool _over;
  bool _res;
  bool _overUnder;
  bool _diag;
  bool _verticalDiag;
  bool _plate;
  bool _thickVarFlag;
  bool _metExtFlag;
  uint32_t _diagModel;

  extDistRC* _rc[20];
  extDistRC* _tmpRC;
  extRCTable* _capTable;
  Array1D<double> _widthTable;
  Array1D<double> _spaceTable;
  Array1D<double> _dataTable;
  Array1D<double> _pTable;
  Array1D<double> _widthTable0;
  Array1D<double> _spaceTable0;
  Array1D<double> _diagSpaceTable0;
  Array1D<double> _diagWidthTable0;

  Array1D<SEQ*>*** _dgContextArray;  // array
  uint32_t* _dgContextDepth;         // not array
  uint32_t* _dgContextPlanes;        // not array
  uint32_t* _dgContextTracks;        // not array
  uint32_t* _dgContextBaseLvl;       // not array
  int* _dgContextLowLvl;             // not array
  int* _dgContextHiLvl;              // not array
  uint32_t* _dgContextBaseTrack;     // array
  int* _dgContextLowTrack;           // array
  int* _dgContextHiTrack;            // array
  int** _dgContextTrackBase;         // array
  FILE* _dgContextFile;
  uint32_t _dgContextCnt;

  Array1D<int>** _ccContextArray;

  Array1D<ext2dBox*>
      _2dBoxTable[2][20];  // assume 20 layers; 0=main net; 1=context
  AthPool<ext2dBox>* _2dBoxPool;
  Array1D<int>** _ccMergedContextArray;

  int _ll[2];
  int _ur[2];

  uint32_t _len;
  int _dist;
  uint32_t _width;
  uint32_t _dir;
  uint32_t _layerCnt;
  odb::dbBlock* _block;
  odb::dbTech* _tech;
  uint32_t _idTable[10000];
  uint32_t _mapTable[10000];
  uint32_t _maxCapNodeCnt;

  extMain* _extMain;
  extRCModel* _currentModel;
  Array1D<extMetRCTable*> _metRCTable;
  uint32_t _minModelIndex;
  uint32_t _maxModelIndex;

  uint32_t _totCCcnt;
  uint32_t _totSmallCCcnt;
  uint32_t _totBigCCcnt;
  uint32_t _totSignalSegCnt;
  uint32_t _totSegCnt;

  double _resFactor;
  bool _resModify;
  double _ccFactor;
  bool _ccModify;
  double _gndcFactor;
  bool _gndcModify;

  gs* _pixelTable;

  Array1D<SEQ*>* _diagTable;
  Array1D<SEQ*>* _tmpSrcTable;
  Array1D<SEQ*>* _tmpDstTable;
  Array1D<SEQ*>* _tmpTable;
  Array1D<SEQ*>* _underTable;
  Array1D<SEQ*>* _ouTable;
  Array1D<SEQ*>* _overTable;

  int _diagLen;
  uint32_t _netId;
  int _rsegSrcId;
  int _rsegTgtId;
  int _netSrcId;
  int _netTgtId;
  FILE* _debugFP;

  AthPool<SEQ>* _seqPool;

  AthPool<extLenOU>* _lenOUPool;
  Array1D<extLenOU*>* _lenOUtable;

  bool _diagFlow;
  bool _toHi;
  bool _sameNetFlag;

  bool _rotatedGs;

  dbCreateNetUtil _create_net_util;
  int _dbunit;

 private:
  utl::Logger* logger_;
};

class extMainOptions
{
 public:
  extMainOptions();

  uint32_t _overDist;
  uint32_t _underDist;
  int _met_cnt;
  int _met;
  int _underMet;
  int _overMet;
  uint32_t _wireCnt;
  const char* _topDir;
  const char* _name;
  const char* _wTable;
  const char* _sTable;
  const char* _thTable;
  const char* _dTable;

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

  Array1D<double> _widthTable;
  Array1D<double> _spaceTable;
  Array1D<double> _densityTable;
  Array1D<double> _thicknessTable;
  Array1D<double> _gridTable;

  int _ll[2];
  int _ur[2];
  uint32_t _len;
  int _dist;
  uint32_t _width;
  uint32_t _dir;
  extRCModel* _rcModel;
  uint32_t _layerCnt;
  bool _v1;
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
// CLEANUP dkf 10302024
struct LayerDimensionData
{
  uint32_t pitchTable[32];
  uint32_t widthTable[32];
  uint32_t dirTable[32];
  int baseX[32];
  int baseY[32];
  uint32_t minRes[2];

  uint32_t maxWidth;
  uint32_t minPitch;
};

struct BoundaryData
{
  int ll[2];  // lower left
  int ur[2];  // upper right
  int lo_gs[2];
  int hi_gs[2];
  int lo_search[2];
  int hi_search[2];
  uint32_t maxCouplingTracks;
  uint32_t diag_met_limit;
  uint32_t minPitch;
  uint32_t maxPitch;
  uint32_t iterationIncrement;
  uint32_t maxExtractBuffer;
  int extractLimitXY;
  int releaseMemoryLimitXY;

  int lo_search_limit;
  int lo_gs_limit;

  void setBBox(const odb::Rect& extRect)
  {
    ll[0] = extRect.xMin();
    ll[1] = extRect.yMin();
    ur[0] = extRect.xMax();
    ur[1] = extRect.yMax();
  }

  bool update(uint32_t dir)
  {
    // get wires for coupling
    lo_search_limit = hi_search[dir];
    lo_search[dir] = lo_search_limit;
    hi_search[dir] = lo_search_limit + iterationIncrement;

    bool lastIteration = lo_search_limit + iterationIncrement - ur[dir] > 0;
    if (lastIteration) {
      hi_search[dir] = ur[dir] + 5 * maxExtractBuffer;
    }

    // Wires should be present for up coupling
    extractLimitXY = hi_search[dir] - maxExtractBuffer;

    // Wires should be present for down coupling
    releaseMemoryLimitXY = extractLimitXY - maxExtractBuffer;

    // get wires for context
    lo_gs_limit = lo_search[dir] - maxExtractBuffer;
    lo_gs[dir] = lo_gs_limit;
    hi_gs[dir] = hi_search[dir] + maxExtractBuffer;

    return lastIteration;
  }
  uint32_t init(uint32_t ccTrackDist,
                uint32_t diag_limit,
                uint32_t pitch1,
                uint32_t pitch2,
                uint32_t maxWidth,
                uint32_t iterationTrackCount)
  {
    maxCouplingTracks = ccTrackDist;
    diag_met_limit = diag_limit;

    minPitch = pitch1;
    maxPitch = pitch2;
    maxExtractBuffer = ccTrackDist * pitch2;

    iterationIncrement = iterationTrackCount * minPitch;
    if (maxWidth > maxCouplingTracks * maxPitch) {
      iterationIncrement = std::max(ur[1] - ll[1], ur[0] - ll[0]);
    }

    for (uint32_t dir = 0; dir < 2; dir++) {
      lo_gs[dir] = ll[dir];
      hi_gs[dir] = ll[dir];
      lo_search[dir] = ll[dir];
      hi_search[dir] = ll[dir];
    }
    return iterationIncrement;
  }
};

class extMain
{
  // --------------------- dkf 092024 ------------------------
 public:
  extSolverGen* _currentSolverGen;

  // v2 -----------------------------------------------------

  uint32_t getPeakMemory(const char* msg, int n = -1);

  void setupBoundaries(BoundaryData& bounds, const odb::Rect& extRect);
  void updateBoundaries(BoundaryData& bounds,
                        uint32_t dir,
                        uint32_t ccDist,
                        uint32_t maxPitch);
  void initializeLayerTables(LayerDimensionData& tables);
  int initSearch(LayerDimensionData& tables,
                 odb::Rect& extRect,
                 uint32_t& totWireCnt);

  // CLEANUP dkf 10242024 ----------------------------------
  void makeBlockRCsegs_v2(const char* netNames, const char* extRules);
  bool markNetsToExtract_v2(const char* netNames,
                            std::vector<odb::dbNet*>& inets);

  bool makeRCNetwork_v2();
  bool couplingExtEnd_v2();
  void update_wireAltered_v2(std::vector<odb::dbNet*>& inets);
  void initSomeValues_v2();
  bool SetCornersAndReadModels_v2(const char* extRules);
  double getDbFactor_v2();
  bool ReadModels_v2(const char* rulesFileName,
                     extRCModel* m,
                     uint32_t extDbCnt,
                     uint32_t* cornerTable);

  void setExtractionOptions_v2(ExtractOptions options);
  uint32_t makeNetRCsegs_v2(odb::dbNet* net, bool skipStartWarning = false);
  uint32_t resetMapNodes_v2(odb::dbWire* wire);

  uint32_t getCapNodeId_v2(odb::dbITerm* iterm, uint32_t junction);
  uint32_t getCapNodeId_v2(odb::dbBTerm* bterm, uint32_t junction);
  uint32_t getCapNodeId_v2(odb::dbNet* net, int junction, bool branch);
  uint32_t getCapNodeId_v2(odb::dbNet* net,
                           odb::dbWirePath& path,
                           uint32_t junction,
                           bool branch);
  uint32_t getCapNodeId_v2(odb::dbNet* net,
                           const odb::dbWirePathShape& pshape,
                           int junct_id,
                           bool branch);
  void initJunctionIdMaps(odb::dbNet* net);

  void print_debug(bool branch,
                   uint32_t junction,
                   uint32_t capId,
                   const char* old_new);
  odb::dbRSeg* addRSeg_v2(odb::dbNet* net,
                          uint32_t& srcId,
                          odb::Point& prevPoint,
                          const odb::dbWirePath& path,
                          const odb::dbWirePathShape& pshape,
                          bool isBranch,
                          const double* restbl = nullptr,
                          const double* captbl = nullptr);

  void loopWarning(odb::dbNet* net, const odb::dbWirePathShape& pshape);
  void getShapeRC_v2(odb::dbNet* net,
                     const odb::dbShape& s,
                     odb::Point& prevPoint,
                     const odb::dbWirePathShape& pshape);
  void getShapeRC_v3(odb::dbNet* net,
                     const odb::dbShape& s,
                     odb::Point& prevPoint,
                     const odb::dbWirePathShape& pshape);
  double getViaRes_v2(odb::dbNet* net, odb::dbTechVia* tvia);
  double getDbViaRes_v2(odb::dbNet* net, const odb::dbShape& s);
  double getMetalRes_v2(odb::dbNet* net,
                        const odb::dbShape& s,
                        const odb::dbWirePathShape& pshape);
  void setResAndCap_v2(odb::dbRSeg* rc,
                       const double* restbl,
                       const double* captbl);
  extRCModel* createCornerMap(const char* rulesFileName);

  uint32_t getResCapTable_lefRC_v2();
  void infoBeforeCouplingExt();
  void setExtControl_v2(AthPool<SEQ>* seqPool);

  // --------------------- dkf 092024 ------------------------
  extRCModel* getCurrentModel() { return _currentModel; }
  void setCurrentModel(extRCModel* m) { _currentModel = m; }
  uint32_t GenExtModel(std::list<std::string> spef_file_list,
                       std::list<std::string> corner_list,
                       const char* out_file,
                       const char* comment,
                       const char* version,
                       int pattern);
  // CLEANUP dkf 10242024 ----------------------------------

  uint32_t benchVerilog_bterms(FILE* fp,
                               odb::dbIoType iotype,
                               const char* prefix,
                               const char* postfix,
                               bool v = false);
  bool modelExists(const char* extRules);

  void addInstsGeometries(const Array1D<uint32_t>* instTable,
                          Array1D<uint32_t>* tmpInstIdTable,
                          uint32_t dir);
  void addObsShapesOnPlanes(odb::dbInst* inst,
                            bool rotatedFlag,
                            bool swap_coords);
  void addItermShapesOnPlanes(odb::dbInst* inst,
                              bool rotatedFlag,
                              bool swap_coords);
  void addShapeOnGs(odb::dbShape* s, bool swap_coords);

  void initRunEnv(extMeasureRC& m);
  uint32_t _ccContextDepth = 0;

  bool _lefRC = false;
  uint32_t _dbgOption = 0;

  bool _overCell = true;

  uint32_t* _ccContextLength = nullptr;
  //  uint32_t* _ccContextLength= nullptr;

  bool _skip_via_wires;
  float _version;                           // dkf: 06242024
  int _metal_flag_22;                       // dkf: 06242024
  uint32_t _wire_extracted_progress_count;  // dkf: 06242024

  bool _v2;  // new flow dkf: 10302023

  void skip_via_wires(bool v) { _skip_via_wires = v; };
  void printUpdateCoup(uint32_t netId1,
                       uint32_t netId2,
                       double v,
                       double org,
                       double totCC);
  uint32_t DefWires(extMainOptions* opt);

 public:
  // v2 ------------------------------------------------------------
  uint32_t _debug_net_id = 0;

  uint32_t couplingFlow_v2_opt(odb::Rect& extRect,
                               uint32_t ccFlag,
                               extMeasure* m);
  uint32_t couplingFlow_v2(odb::Rect& extRect, uint32_t ccFlag, extMeasure* m);
  void setBranchCapNodeId(odb::dbNet* net, uint32_t junction);
  void markPathHeadTerm(odb::dbWirePath& path);

  extSolverGen* getCurrentSolverGen() { return _currentSolverGen; }
  void setCurrentSolverGen(extSolverGen* p) { _currentSolverGen = p; }

  // --------------------- dkf 092024 ------------------------
  // DKF 07/25/24 -- 3d pattern generation
  uint32_t rulesGen(const char* name,
                    const char* topDir,
                    const char* rulesFile,
                    int pattern,
                    bool keepFile,
                    int wLen,
                    int version,
                    bool win);
  uint32_t readProcess(const char* name, const char* filename);

  void init(odb::dbDatabase* db, utl::Logger* logger);
  double getTotalCouplingCap(odb::dbNet* net,
                             const char* filterNet,
                             uint32_t corner);

  uint32_t calcMinMaxRC();
  void resetMinMaxRC(uint32_t ii, uint32_t jj);
  uint32_t getExtStats(odb::dbNet* net,
                       uint32_t corner,
                       int& wlen,
                       double& min_cap,
                       double& max_cap,
                       double& min_res,
                       double& max_res,
                       double& via_res,
                       uint32_t& via_cnt);

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

  uint32_t getDir(int x1, int y1, int x2, int y2);
  uint32_t getDir(odb::Rect& r);

  uint32_t initSearchForNets(int* X1,
                             int* Y1,
                             uint32_t* pitchTable,
                             uint32_t* widthTable,
                             uint32_t* dirTable,
                             odb::Rect& extRect,
                             bool skipBaseCalc);
  uint32_t addNetSBoxes(odb::dbNet* net,
                        uint32_t dir,
                        int* bb_ll,
                        int* bb_ur,
                        uint32_t wtype,
                        dbCreateNetUtil* netUtil = nullptr);
  uint32_t addNetSBoxes2(odb::dbNet* net,
                         uint32_t dir,
                         int* bb_ll,
                         int* bb_ur,
                         uint32_t wtype,
                         uint32_t step = 0);
  uint32_t addPowerNets(uint32_t dir,
                        int* bb_ll,
                        int* bb_ur,
                        uint32_t wtype,
                        dbCreateNetUtil* netUtil = nullptr);
  uint32_t addNetShapesOnSearch(odb::dbNet* net,
                                uint32_t dir,
                                int* bb_ll,
                                int* bb_ur,
                                uint32_t wtype,
                                FILE* fp,
                                dbCreateNetUtil* netUtil = nullptr);
  int GetDBcoords2(int coord);
  void GetDBcoords2(odb::Rect& r);
  double GetDBcoords1(int coord);
  uint32_t addViaBoxes(odb::dbShape& sVia,
                       odb::dbNet* net,
                       uint32_t shapeId,
                       uint32_t wtype);
  void getViaCapacitance(odb::dbShape svia, odb::dbNet* net);

  uint32_t addSignalNets(uint32_t dir,
                         int* bb_ll,
                         int* bb_ur,
                         uint32_t wtype,
                         dbCreateNetUtil* createDbNet = nullptr);
  uint32_t addNets(uint32_t dir,
                   int* bb_ll,
                   int* bb_ur,
                   uint32_t wtype,
                   uint32_t ptype,
                   Array1D<uint32_t>* sdbSignalTable);
  uint32_t addNetOnTable(uint32_t netId,
                         uint32_t dir,
                         odb::Rect* maxRect,
                         uint32_t* nm_step,
                         int* bb_ll,
                         int* bb_ur,
                         Array1D<uint32_t>*** wireTable);
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
  void addNetShapesGs(odb::dbNet* net,
                      bool gsRotated,
                      bool swap_coords,
                      int dir);
  void addNetSboxesGs(odb::dbNet* net,
                      bool gsRotated,
                      bool swap_coords,
                      int dir);

  uint32_t getBucketNum(int base, int max, uint32_t step, int xy);
  int getXY_gs(int base, int XY, uint32_t minRes);
  uint32_t couplingFlow(odb::Rect& extRect,
                        uint32_t ccFlag,
                        extMeasure* m,
                        CoupleAndCompute coupleAndCompute);
  void initPlanes(uint32_t dir,
                  const int* wLL,
                  const int* wUR,
                  uint32_t layerCnt,
                  const uint32_t* pitchTable,
                  const uint32_t* widthTable,
                  const uint32_t* dirTable,
                  const int* bb_ll);

  bool isIncluded(odb::Rect& r, uint32_t dir, const int* ll, const int* ur);
  bool matchDir(uint32_t dir, const odb::Rect& r);
  bool isIncludedInsearch(odb::Rect& r,
                          uint32_t dir,
                          const int* bb_ll,
                          const int* bb_ur);

  uint32_t makeTree(uint32_t netId);

  void resetSumRCtable();
  void addToSumRCtable();
  void copyToSumRCtable();
  uint32_t getResCapTable();
  double getLoCoupling();
  void ccReportProgress();
  void measureRC(CoupleOptions& options);
  void updateTotalRes(odb::dbRSeg* rseg1,
                      odb::dbRSeg* rseg2,
                      extMeasure* m,
                      const double* delta,
                      uint32_t modelCnt);
  void updateTotalCap(odb::dbRSeg* rseg,
                      double frCap,
                      double ccCap,
                      double deltaFr,
                      uint32_t modelIndex);
  void updateTotalCap(odb::dbRSeg* rseg,
                      extMeasure* m,
                      const double* deltaFr,
                      uint32_t modelCnt,
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
                   uint32_t extDbCnt);

  extRCModel* getRCmodel(uint32_t n);

  void calcRes0(double* deltaRes,
                uint32_t tgtMet,
                uint32_t width,
                uint32_t len,
                int dist1 = 0,
                int dist2 = 0);
  double getLefResistance(uint32_t level,
                          uint32_t width,
                          uint32_t length,
                          uint32_t model);
  double getResistance(uint32_t level,
                       uint32_t width,
                       uint32_t len,
                       uint32_t model);
  double getFringe(uint32_t met,
                   uint32_t width,
                   uint32_t modelIndex,
                   double& areaCap);
  void printNet(odb::dbNet* net, uint32_t netId);
  double calcFringe(extDistRC* rc, double deltaFr, bool includeCoupling);
  double updateTotalCap(odb::dbRSeg* rseg, double cap, uint32_t modelIndex);
  bool updateCoupCap(odb::dbRSeg* rseg1, odb::dbRSeg* rseg2, int jj, double v);
  double updateRes(odb::dbRSeg* rseg, double res, uint32_t model);

  uint32_t getExtBbox(int* x1, int* y1, int* x2, int* y2);

  void setupMapping(uint32_t itermCnt = 0);
  uint32_t getMultiples(uint32_t cnt, uint32_t base);
  uint32_t getExtLayerCnt(odb::dbTech* tech);

  void setBlockFromChip();
  void setBlock(odb::dbBlock* block);
  odb::dbBlock* getBlock() { return _block; }
  odb::dbTech* getTech() { return _tech; }
  extRCModel* getRCModel() { return _modelTable->get(0); }

  void print_RC(odb::dbRSeg* rc);
  void resetMapping(odb::dbBTerm* term, odb::dbITerm* iterm, uint32_t junction);
  uint32_t resetMapNodes(odb::dbNet* net);
  void setResCapFromLef(odb::dbRSeg* rc,
                        uint32_t targetCapId,
                        odb::dbShape& s,
                        uint32_t len);
  bool isTermPathEnded(odb::dbBTerm* bterm, odb::dbITerm* iterm);
  uint32_t getCapNodeId(odb::dbWirePath& path, odb::dbNet* net, bool branch);
  uint32_t getCapNodeId(odb::dbNet* net,
                        odb::dbBTerm* bterm,
                        odb::dbITerm* iterm,
                        uint32_t junction,
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
                       uint32_t cc_up,
                       uint32_t ccFlag,
                       double resBound,
                       bool mergeViaRes,
                       double ccThres,
                       int contextDepth,
                       const char* extRules);

  uint32_t getShortSrcJid(uint32_t jid);
  void make1stRSeg(odb::dbNet* net,
                   odb::dbWirePath& path,
                   uint32_t cnid,
                   bool skipStartWarning);
  uint32_t makeNetRCsegs_old(odb::dbNet* net,
                             double resBound,
                             uint32_t debug = 0);
  uint32_t makeNetRCsegs(odb::dbNet* net, bool skipStartWarning = false);
  double getViaResistance(odb::dbTechVia* tvia);
  double getViaResistance_b(odb::dbVia* via, odb::dbNet* net = nullptr);

  void getShapeRC(odb::dbNet* net,
                  const odb::dbShape& s,
                  odb::Point& prevPoint,
                  const odb::dbWirePathShape& pshape);
  void setResAndCap(odb::dbRSeg* rc,
                    const double* restbl,
                    const double* captbl);
  odb::dbRSeg* addRSeg(odb::dbNet* net,
                       std::vector<uint32_t>& rsegJid,
                       uint32_t& srcId,
                       odb::Point& prevPoint,
                       const odb::dbWirePath& path,
                       const odb::dbWirePathShape& pshape,
                       bool isBranch,
                       const double* restbl,
                       const double* captbl);
  uint32_t print_shape(const odb::dbShape& shape, uint32_t j1, uint32_t j2);
  uint32_t getNodeId(odb::dbWirePath& path, bool branch, uint32_t* nodeType);
  uint32_t getNodeId(odb::dbWirePathShape& pshape, uint32_t* nodeType);
  uint32_t computePathDir(const odb::Point& p1,
                          const odb::Point& p2,
                          uint32_t* length);
  uint32_t openSpefFile(char* filename, uint32_t mode);

  //-------------------------------------------------------------- SPEF
  uint32_t calibrate(char* filename,
                     bool m_map,
                     float upperLimit,
                     float lowerLimit,
                     const char* dbCornerName,
                     int corner,
                     int spefCorner);
  uint32_t readSPEF(char* filename,
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
                    uint32_t testParsing = 0,
                    bool moreToRead = false,
                    bool diff = false,
                    bool calib = false,
                    int app_print_limit = 0);
  uint32_t readSPEFincr(char* filename);
  void writeSPEF(bool stop);
  uint32_t writeSPEF(uint32_t netId,
                     bool single_pi,
                     uint32_t debug,
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
  uint32_t writeNetSPEF(odb::dbNet* net, double resBound, uint32_t debug);
  uint32_t makeITermCapNode(uint32_t id, odb::dbNet* net);
  uint32_t makeBTermCapNode(uint32_t id, odb::dbNet* net);

  double getTotalNetCap(uint32_t netId, uint32_t cornerNum);
  void initContextArray();
  void removeContextArray();
  void initDgContextArray();
  void removeDgContextArray();

  // ruLESgeNf
  bool getFirstShape(odb::dbNet* net, odb::dbShape& shape);
  uint32_t writeRules(const char* name, const char* rulesFile);
  uint32_t benchWires(extMainOptions* options);
  uint32_t GenExtRules(const char* rulesFileName);
  int getExtCornerIndex(odb::dbBlock* block, const char* cornerName);

  void initExtractedCorners(odb::dbBlock* block);

  void addDummyCorners(uint32_t cornerCnt);
  static void addDummyCorners(odb::dbBlock* block,
                              uint32_t cnt,
                              utl::Logger* logger);
  char* addRCCorner(const char* name, int model, int userDefined = 1);
  char* addRCCornerScaled(const char* name,
                          uint32_t model,
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

  uint32_t getNetBbox(odb::dbNet* net, odb::Rect& maxRect);

  void resetNetSpefFlag(Array1D<uint32_t>* tmpNetIdTable);

  uint32_t sBoxCounter(odb::dbNet* net, uint32_t& maxWidth);
  uint32_t powerWireCounter(uint32_t& maxWidth);
  uint32_t signalWireCounter(uint32_t& maxWidth);
  bool getRotatedFlag();
  bool enableRotatedFlag();

  uint32_t addMultipleRectsOnSearch(odb::Rect& r,
                                    uint32_t level,
                                    uint32_t dir,
                                    uint32_t id,
                                    uint32_t shapeId,
                                    uint32_t wtype);

  //--------------- Window
  void addShapeOnGS(const odb::Rect& r,
                    bool plane,
                    odb::dbTechLayer* layer,
                    bool gsRotated,
                    bool swap_coords,
                    int dir);

  void fill_gs4(int dir,
                const int* ll,
                const int* ur,
                const int* lo_gs,
                const int* hi_gs,
                uint32_t layerCnt,
                const uint32_t* dirTable,
                const uint32_t* pitchTable,
                const uint32_t* widthTable);

  uint32_t addInsts(uint32_t dir,
                    int* lo_gs,
                    int* hi_gs,
                    int* bb_ll,
                    int* bb_ur,
                    uint32_t bucketSize,
                    Array1D<uint32_t>*** wireBinTable,
                    dbCreateNetUtil* createDbNet);

  uint32_t getNetBbox(odb::dbNet* net, odb::Rect* maxRect[2]);

  static odb::dbRSeg* getRseg(odb::dbNet* net,
                              uint32_t shapeId,
                              utl::Logger* logger);

  void write_spef_nets(bool flatten, bool parallel);
  extSpef* getSpef();

  uint32_t getLayerSearchBoundaries(odb::dbTechLayer* layer,
                                    int* xyLo,
                                    int* xyHi,
                                    uint32_t* pitch);
  void railConn(uint32_t dir,
                odb::dbTechLayer* layer,
                odb::dbNet* net,
                int* xyLo,
                int* xyHi,
                uint32_t* pitch);
  void railConn(odb::dbNet* net);
  void railConn2(odb::dbNet* net);
  bool isSignalNet(odb::dbNet* net);
  uint32_t powerRCGen();
  uint32_t mergeRails(uint32_t dir,
                      std::vector<odb::dbBox*>& boxTable,
                      std::vector<odb::Rect*>& mergeTable);
  odb::dbITerm* findConnect(odb::dbInst* inst,
                            odb::dbNet* net,
                            odb::dbNet* targetNet);
  uint32_t getITermConn2(uint32_t dir,
                         odb::dbWireEncoder& encoder,
                         odb::dbWire* wire,
                         odb::dbNet* net,
                         int* xy,
                         int* xy2);
  uint32_t getITermConn(uint32_t dir,
                        odb::dbWireEncoder& encoder,
                        odb::dbWire* wire,
                        odb::dbNet* net,
                        int* xy,
                        int* xy2);
  uint32_t viaAndInstConn(uint32_t dir,
                          uint32_t width,
                          odb::dbTechLayer* layer,
                          odb::dbWireEncoder& encoder,
                          odb::dbWire* wire,
                          odb::dbNet* net,
                          odb::Rect* w,
                          bool skipSideMetalFlag);
  odb::dbNet* createRailNet(odb::dbNet* pnet,
                            odb::dbTechLayer* layer,
                            odb::Rect* w);
  uint32_t print_shapes(FILE* fp, odb::dbWire* wire);
  FILE* openNanoFile(const char* name,
                     const char* name2,
                     const char* suffix,
                     const char* perms);
  void openNanoFiles();
  void closeNanoFiles();
  void setupNanoFiles(odb::dbNet* net);
  void writeResNode(FILE* fp, odb::dbCapNode* capNode, uint32_t level);
  double writeRes(FILE* fp,
                  odb::dbNet* net,
                  uint32_t level,
                  uint32_t width,
                  uint32_t dir,
                  bool skipFirst);
  uint32_t connectStackedVias(odb::dbNet* net,
                              odb::dbTechLayer* layer,
                              bool mergeViaRes);
  uint32_t via2viaConn(odb::dbNet* net,
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
                     uint32_t level,
                     bool onlyVias,
                     bool skipFirst);
  void writeCapNodes_0713(FILE* fp,
                          odb::dbNet* net,
                          uint32_t level,
                          bool onlyVias);
  bool specialMasterType(odb::dbInst* inst);
  uint32_t iterm2Vias(odb::dbInst* inst, odb::dbNet* net);
  uint32_t getPowerNets(std::vector<odb::dbNet*>& powerNetTable);
  float getPowerViaRes(odb::dbBox* v, float val);
  uint32_t findHighLevelPinMacros(std::vector<odb::dbInst*>& instTable);
  uint32_t writeViaInfo(FILE* fp,
                        std::vector<odb::dbBox*>& viaTable,
                        bool m1Vias,
                        bool power);

  uint32_t writeViaInfo_old(FILE* fp,
                            std::vector<odb::dbBox*>& viaTable,
                            bool m1Vias);
  uint32_t writeViaCoords(FILE* fp,
                          std::vector<odb::dbBox*>& viaTable,
                          bool m1Vias);
  void writeViaName(char* nodeName,
                    odb::dbBox* v,
                    uint32_t level,
                    const char* post);
  void writeViaName(FILE* fp, odb::dbBox* v, uint32_t level, const char* post);
  void writeViaNameCoords(FILE* fp, odb::dbBox* v);
  float computeViaResistance(odb::dbBox* viaBox, uint32_t& cutCount);
  void printItermNodeSubCkt(FILE* fp, std::vector<uint32_t>& iTable);
  void printViaNodeSubCkt(FILE* fp, std::vector<odb::dbBox*>& viaTable);

  uint32_t mergeStackedVias(FILE* fp,
                            odb::dbNet* net,
                            std::vector<odb::dbBox*>& viaTable,
                            odb::dbBox* botVia,
                            FILE* fp1 = nullptr);
  uint32_t stackedViaConn(FILE* fp, std::vector<odb::dbBox*>& allViaTable);
  bool skipSideMetal(std::vector<odb::dbBox*>& viaTable,
                     uint32_t level,
                     odb::dbNet* net,
                     odb::Rect* w);
  bool overlapWithMacro(odb::Rect& w);

  void powerWireConn(odb::Rect* w,
                     uint32_t dir,
                     odb::dbTechLayer* layer,
                     odb::dbNet* net);
  const char* getBlockType(odb::dbMaster* m);
  void sortViasXY(uint32_t dir, std::vector<odb::dbBox*>& viaTable);
  void writeViaRes(FILE* fp, odb::dbNet* net, uint32_t level);
  void addUpperVia(uint32_t ii, odb::dbBox* v);
  void writeViaResistors(FILE* fp,
                         uint32_t ii,
                         FILE* fp1,
                         bool skipWireConn = false);
  void writeGeomHeader(FILE* fp, const char* vdd);
  void writeResNode(char* nodeName, odb::dbCapNode* capNode, uint32_t level);
  float micronCoords(int xy);
  void writeSubcktNode(char* capNodeName, bool highMetal, bool vdd);
  float distributeCap(FILE* fp, odb::dbNet* net);

  uint32_t readPowerSupplyCoords(char* filename);
  uint32_t addPowerSources(std::vector<odb::dbBox*>& viaTable,
                           bool power,
                           uint32_t level,
                           odb::Rect* powerWire);
  char* getPowerSourceName(bool power, uint32_t level, uint32_t vid);
  char* getPowerSourceName(uint32_t level, uint32_t vid);
  void writeViaInfo(FILE* fp, bool power);
  void addPowerSourceName(uint32_t ii, char* sname);
  void writeResCoords(FILE* fp,
                      odb::dbNet* net,
                      uint32_t level,
                      uint32_t width,
                      uint32_t dir);
  void writeViaName_xy(char* nodeName,
                       odb::dbBox* v,
                       uint32_t bot,
                       uint32_t top,
                       uint32_t level,
                       const char* post = "");
  void writeInternalNode_xy(odb::dbCapNode* capNode, FILE* fp);
  void writeInternalNode_xy(odb::dbCapNode* capNode, char* buff);
  void createNode_xy(odb::dbCapNode* capNode,
                     int x,
                     int y,
                     int level,
                     odb::dbITerm* t = nullptr);
  uint32_t setNodeCoords_xy(odb::dbNet* net, int level);
  bool sameJunctionPoint(int xy[2], int BB[2], uint32_t width, uint32_t dir);

  bool fisrt_markInst_UserFlag(odb::dbInst* inst, odb::dbNet* net);

  bool matchLayerDir(odb::dbBox* rail,
                     odb::dbTechLayerDir layerDir,
                     int level,
                     bool debug);
  void addSubcktStatement(const char* cirFile1, const char* subcktFile1);
  void setPrefix(char* prefix);
  uint32_t getITermPhysicalConn(uint32_t dir,
                                uint32_t level,
                                odb::dbWireEncoder& encoder,
                                odb::dbWire* wire,
                                odb::dbNet* net,
                                int* xy,
                                int* xy2);
  void getSpecialItermShapes(odb::dbInst* inst,
                             odb::dbNet* specialNet,
                             uint32_t dir,
                             uint32_t level,
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
  uint32_t addSboxesOnSearch(odb::dbNet* net);
  odb::Rect* getRect_SBox(Array1D<uint32_t>* table,
                          uint32_t ii,
                          bool filter,
                          uint32_t dir,
                          uint32_t& maxWidth);
  uint32_t mergePowerWires(uint32_t dir,
                           uint32_t level,
                           std::vector<odb::Rect*>& mergeTable);
  void railConnOpt(odb::dbNet* net);
  uint32_t initPowerSearch();
  uint32_t overlapPowerWires(std::vector<odb::Rect*>& mergeTableHi,
                             std::vector<odb::Rect*>& mergeTableLo,
                             std::vector<odb::Rect*>& resultTable);
  odb::dbBox* createMultiVia(uint32_t top, uint32_t bot, odb::Rect* r);
  void mergeViasOnMetal_1(odb::Rect* w,
                          odb::dbNet* pNet,
                          uint32_t level,
                          std::vector<odb::dbBox*>& viaTable);
  uint32_t addGroupVias(uint32_t level,
                        odb::Rect* w,
                        std::vector<odb::dbBox*>& viaTable);
  uint32_t mergeStackedViasOpt(FILE* fp,
                               odb::dbNet* net,
                               std::vector<odb::dbBox*>& viaSearchTable,
                               odb::dbBox* botVia,
                               FILE* fp1,
                               uint32_t stackLevel = 1);
  odb::dbCapNode* getITermPhysicalConnRC(odb::dbCapNode* srcCapNode,
                                         uint32_t level,
                                         uint32_t dir,
                                         odb::dbNet* net,
                                         int* xy,
                                         int* xy2,
                                         bool macro);
  uint32_t viaAndInstConnRC(uint32_t dir,
                            uint32_t width,
                            odb::dbTechLayer* layer,
                            odb::dbNet* net,
                            odb::dbNet* orig_power_net,
                            odb::Rect* w,
                            bool skipSideMetalFlag);
  void powerWireConnRC(odb::Rect* w,
                       uint32_t dir,
                       odb::dbTechLayer* layer,
                       odb::dbNet* net);
  odb::dbCapNode* getITermConnRC(odb::dbCapNode* srcCapNode,
                                 uint32_t level,
                                 uint32_t dir,
                                 odb::dbNet* net,
                                 int* xy,
                                 int* xy2);
  odb::dbCapNode* getPowerCapNode(odb::dbNet* net, int xy, uint32_t level);
  odb::dbCapNode* makePowerRes(odb::dbCapNode* srcCap,
                               uint32_t dir,
                               int xy[2],
                               uint32_t level,
                               uint32_t width,
                               uint32_t objId,
                               int type);
  void createNode_xy_RC(char* buf,
                        odb::dbCapNode* capNode,
                        int x,
                        int y,
                        int level);
  void writeResNodeRC(char* nodeName, odb::dbCapNode* capNode, uint32_t level);
  void writeResNodeRC(FILE* fp, odb::dbCapNode* capNode, uint32_t level);
  double writeResRC(FILE* fp,
                    odb::dbNet* net,
                    uint32_t level,
                    uint32_t width,
                    uint32_t dir,
                    bool skipFirst,
                    bool reverse,
                    bool onlyVias,
                    bool caps,
                    int xy[2]);
  void writeCapNodesRC(FILE* fp,
                       odb::dbNet* net,
                       uint32_t level,
                       bool onlyVias,
                       bool skipFirst);
  void writeViaResistorsRC(FILE* fp, uint32_t ii, FILE* fp1);
  void viaTagByCapNode(odb::dbBox* v, odb::dbCapNode* cap);
  char* getViaResNode(odb::dbBox* v, const char* propName);
  void writeMacroItermConns(odb::dbNet* net);
  void setupDirNaming();
  bool filterPowerGeoms(odb::dbSBox* s, uint32_t targetDir, uint32_t& maxWidth);

  uint32_t iterm2Vias_cells(odb::dbInst* inst,
                            odb::dbITerm* connectedPowerIterm);
  void writeCapNodesRC(FILE* fp,
                       odb::dbNet* net,
                       uint32_t level,
                       bool onlyVias,
                       std::vector<odb::dbCapNode*>& capNodeTable);
  void writeOneCapNode(FILE* fp,
                       odb::dbCapNode* capNode,
                       uint32_t level,
                       bool onlyVias);

  void findViaMainCoord(odb::dbNet* net, char* buff);
  void replaceItermCoords(odb::dbNet* net, uint32_t dir, int xy[2]);

  void formOverlapVias(std::vector<odb::Rect*> mergeTable[16],
                       odb::dbNet* pNet);

  uint32_t benchVerilog(FILE* fp);
  /* v2 up uint32_t benchVerilog_bterms(FILE* fp,
                           const odb::dbIoType& iotype,
                           const char* prefix,
                           const char* postfix,
                           bool skip_postfix_last = false); */
  uint32_t benchVerilog_assign(FILE* fp);

  void setMinRC(uint32_t ii, uint32_t jj, extDistRC* rc);
  void setMaxRC(uint32_t ii, uint32_t jj, extDistRC* rc);

  utl::Logger* getLogger() { return logger_; }

 private:
  utl::Logger* logger_;

  bool _batchScaleExt = true;
  Array1D<extCorner*>* _processCornerTable = nullptr;
  Array1D<extCorner*>* _scaledCornerTable = nullptr;

  Array1D<extRCModel*>* _modelTable;
  Array1D<uint32_t> _modelMap;  // TO_TEST
  Array1D<extMetRCTable*> _metRCTable;
  double _resistanceTable[20][20];
  double _capacitanceTable[20][20];  // 20 layers by 20 rc models
  double _minWidthTable[20];
  uint32_t _minDistTable[20];
  double _tmpCapTable[20];
  double _tmpSumCapTable[20];
  double* _tmpResTable = new double[10];
  double* _tmpSumResTable = new double[10];
  int _sumUpdated;
  int _minModelIndex;  // TO_TEST
  int _typModelIndex;  //
  int _maxModelIndex;  //

  odb::dbDatabase* _db = nullptr;
  odb::dbTech* _tech = nullptr;
  odb::dbBlock* _block = nullptr;
  uint32_t _blockId;
  extSpef* _spef = nullptr;
  bool _writeNameMap = true;
  bool _fullIncrSpef = false;
  bool _noFullIncrSpef = false;
  char* _origSpefFilePrefix = nullptr;
  char* _newSpefFilePrefix = nullptr;
  uint32_t _bufSpefCnt;
  bool _incrNoBackSlash;
  uint32_t _cornerCnt = 0;
  uint32_t _extDbCnt;

  int _remote;
  bool _extracted;
  bool _allNet;

  bool _getBandWire = false;
  bool _printBandInfo = false;
  uint32_t _ccUp = 0;
  uint32_t _couplingFlag = 0;
  bool _rotatedGs = false;
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

  Array1D<int>* _nodeTable = nullptr;   // junction id -> cap node id
  Array1D<int>* _btermTable = nullptr;  // bterm id -> cap node id
  Array1D<int>* _itermTable = nullptr;  // iterm id -> cap node id

  uint32_t _dbPowerId = 1;
  uint32_t _dbSignalId = 2;
  uint32_t _rSegId;
  uint32_t _ccSegId = 3;

  uint32_t _ccNoPowerSource = 0;
  uint32_t _ccNoPowerTarget = 0;
  int _x1;
  int _y1;
  int _x2;
  int _y2;

  double _coupleThreshold = 0.1;  // fF

  uint32_t _totCCcnt;
  uint32_t _totSmallCCcnt;
  uint32_t _totBigCCcnt;
  uint32_t _totSignalSegCnt;
  uint32_t _totSegCnt;

  bool _noModelRC = false;
  extRCModel* _currentModel = nullptr;

  uint32_t* _singlePlaneLayerMap = nullptr;
  bool _usingMetalPlanes = false;

  gs* _geomSeq = nullptr;

  AthPool<SEQ>* _seqPool = nullptr;

  Array1D<SEQ*>*** _dgContextArray = nullptr;
  uint32_t _dgContextDepth;
  uint32_t _dgContextPlanes;
  uint32_t _dgContextTracks;
  uint32_t _dgContextBaseLvl;
  int _dgContextLowLvl;
  int _dgContextHiLvl;
  uint32_t* _dgContextBaseTrack = nullptr;
  int* _dgContextLowTrack = nullptr;
  int* _dgContextHiTrack = nullptr;
  int** _dgContextTrackBase = nullptr;

  Array1D<int>** _ccContextArray = nullptr;
  Array1D<int>** _ccMergedContextArray = nullptr;
  uint32_t _ccContextPlanes;

  uint32_t _extRun = 0;
  odb::dbExtControl* _prevControl = nullptr;

  bool _foreign = false;
  bool _rsegCoord;
  bool _diagFlow = false;

  std::vector<uint32_t> _rsegJid;
  std::vector<uint32_t> _shortSrcJid;
  std::vector<uint32_t> _shortTgtJid;

  std::vector<odb::dbBTerm*> _connectedBTerm;
  std::vector<odb::dbITerm*> _connectedITerm;

  std::unique_ptr<GridTable> _search;

  int _noVariationIndex;

  friend class extMeasure;

  float _previous_percent_extracted = 0;

  double _minCapTable[64][64];
  double _maxCapTable[64][64];
  double _minResTable[64][64];
  double _maxResTable[64][64];

 public:
  bool _lef_res;
  std::string _tmpLenStats;
  int _last_node_xy[2];
  bool _wireInfra;
  odb::Rect _extMaxRect;

  // ----------------------------------------- 060623
  uint32_t benchPatternsGen(const PatternOptions& opt);
  uint32_t overPatterns(const PatternOptions& opt,
                        int origin[2],
                        dbCreateNetUtil* db_net_util);  // 060823
  uint32_t UnderPatterns(const PatternOptions& opt,
                         int origin[2],
                         dbCreateNetUtil* db_net_util);  // 061123
  uint32_t OverUnderPatterns(const PatternOptions& opt,
                             int origin[2],
                             dbCreateNetUtil* db_net_util);  // 061123
  // ---------------------------------------------------------
};

}  // namespace rcx
