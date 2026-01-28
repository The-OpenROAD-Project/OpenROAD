// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>

#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbShape.h"
#include "rcx/array1.h"
#include "rcx/extRCap.h"
#include "utl/Logger.h"

namespace rcx {

class NameTable;
class Parser;

class extSpef
{
 public:
  extSpef(odb::dbTech* tech,
          odb::dbBlock* blk,
          utl::Logger* logger,
          const char* version,
          extMain* extmain);
  ~extSpef();

  void reinit();
  bool setOutSpef(const char* filename);
  bool setInSpef(const char* filename, bool onlyOpen = false);
  void stopWrite();
  void write_spef_nets(bool flatten, bool parallel);
  odb::dbBlock* getBlock() { return _block; }
  void set_single_pi(bool v);
  void writeNet(odb::dbNet* net, double resBound, uint32_t debug);
  void preserveFlag(bool v);
  int getWriteCorner(int corner, const char* name);
  void setUseIdsFlag(bool diff = false, bool calib = false);
  void setGzipFlag(bool gzFlag);
  void setDesign(const char* name);
  void writeBlock(const char* nodeCoord,
                  const char* capUnit,
                  const char* resUnit,
                  bool stopAfterNameMap,
                  const std::vector<odb::dbNet*>& tnets,
                  bool wClock,
                  bool wConn,
                  bool wCap,
                  bool wOnlyCCcap,
                  bool wRes,
                  bool noCnum,
                  bool stopBeforeDnets,
                  bool noBackSlash,
                  bool parallel);
  void incr_rRun() { _rRun++; };
  void setCornerCnt(uint32_t n);
  uint32_t readBlock(uint32_t debug,
                     const std::vector<odb::dbNet*>& tnets,
                     bool force,
                     bool rConn,
                     const char* nodeCoord,
                     bool rCap,
                     bool rOnlyCCcap,
                     bool rRes,
                     float cc_thres,
                     float length_unit,
                     bool extracted,
                     bool keepLoadedCorner,
                     bool stampWire,
                     uint32_t testParsing,
                     int app_print_limit,
                     bool m_map,
                     int corner,
                     double low,
                     double up,
                     const char* excludeNetSubWord,
                     const char* netSubWord,
                     const char* capStatsFile,
                     const char* dbCornerName,
                     const char* calibrateBaseCorner,
                     int spefCorner,
                     int fixLoop,
                     bool& rsegCoord);
  uint32_t getAppPrintLimit() { return _cc_app_print_limit; };
  int* getAppCnt() { return _appcnt; };
  uint32_t readBlockIncr(uint32_t debug);
  void setCalibLimit(float upperLimit, float lowerLimit);
  void printAppearance(const int* appcnt, int tapp);

 private:
  void setLogger(utl::Logger* logger);
  bool matchNetGndCap(odb::dbNet* net,
                      uint32_t dbCorner,
                      double dbCap,
                      double refCap);
  bool calibrateNetGndCap(odb::dbNet* net,
                          uint32_t dbCorner,
                          double dbCap,
                          double refCap);
  bool computeFactor(double db, double ref, float& factor);

  void initNodeCoordTables(uint32_t memChunk);
  void resetNodeCoordTables();
  void deleteNodeCoordTables();
  bool readNodeCoords(uint32_t cpos);
  void checkCCterm();
  int findNodeIndexFromNodeCoords(uint32_t targetCapNodeId);
  void writeNodeCoords(uint32_t netId, odb::dbSet<odb::dbRSeg>& rSet);

  void setupMappingForWrite(uint32_t btermCnt = 0, uint32_t itermCnt = 0);
  void setupMapping(uint32_t itermCnt = 0);

  void writeCNodeNumber();

  bool closeOutFile();
  bool isCapNodeExcluded(odb::dbCapNode* node);
  void writeBlock(char* nodeCoord,
                  const char* capUnit,
                  const char* resUnit,
                  bool stopAfterNameMap,
                  std::vector<odb::dbNet*>* tnets,
                  bool wClock,
                  bool wConn,
                  bool wCap,
                  bool wOnlyCCcap,
                  bool wRes,
                  bool noCnum,
                  bool stopBeforeDnets,
                  bool noBackSlash,
                  bool parallel);

  void writeITerm(uint32_t node);
  void writeBTerm(uint32_t node);
  void writeNode(uint32_t netId, uint32_t node);
  void writePort(uint32_t node);
  void writeDnet(uint32_t netId, const double* totCap);
  void writeKeyword(const char* keyword);
  uint32_t writePorts();
  uint32_t writeITerms();
  uint32_t getInstMapId(uint32_t id);
  void writeITermNode(uint32_t node);

  bool isBTermMarked(uint32_t node);
  void markBTerm(uint32_t node);
  void unMarkBTerm(uint32_t node);
  uint32_t getBTermMapping(uint32_t b);

  uint32_t getNetMapId(uint32_t netId);
  const char* tinkerSpefName(const char* iname);
  const char* addEscChar(const char* iname, bool esc_bus_brkts);

  bool isITermMarked(uint32_t node);
  void markITerm(uint32_t node);
  void unMarkITerm(uint32_t node);
  uint32_t getITermMapping(uint32_t b);
  void resetTermTables();
  void resetCapTables(uint32_t maxNode);
  void initCapTable(Array1D<double*>* table);
  void deleteTableCap(Array1D<double*>* table);
  void reinitCapTable(Array1D<double*>* table, uint32_t n);
  void addCap(const double* cap, double* totCap, uint32_t n);
  void addHalfCap(double* totCap, const double* cap, uint32_t n = 0);
  void getCaps(odb::dbNet* net, double* totCap);
  void resetCap(double* cap);
  void resetCap(double* cap, uint32_t cnt);
  void writeRCvalue(const double* totCap, double units);
  void writeCapPort(uint32_t index);
  uint32_t writeCapITerm(uint32_t index);
  uint32_t writeCapITerms();
  uint32_t writeCapPorts();
  void writeNodeCaps(uint32_t netId, uint32_t minNode, uint32_t maxNode);
  void writeBlockPorts();
  void writeNetMap(odb::dbSet<odb::dbNet>& nets);
  void writeInstMap();

  uint32_t readDNet(uint32_t debug);
  uint32_t getSpefNode(char* nodeWord, uint32_t* instNetId, int* nodeType);
  uint32_t getITermId(uint32_t instId, const char* name);
  uint32_t getBTermId(const char* name);
  uint32_t getBTermId(uint32_t id);

  bool readHeaderInfo(uint32_t debug, bool skip = false);
  void writeHeaderInfo();
  bool readPorts();
  bool readNameMap(uint32_t debug, bool skip = false);

  uint32_t getCapNode(char* nodeWord, char* capWord);
  uint32_t getMappedBTermId(uint32_t id);
  uint32_t diffGndCap(odb::dbNet* net, uint32_t capCnt, uint32_t capId);
  uint32_t diffNetCcap(odb::dbNet* net);
  double printDiff(odb::dbNet* net,
                   double dbCap,
                   double refCap,
                   const char* ctype,
                   int ii,
                   int id = -1);
  uint32_t diffCCap(odb::dbNet* srcNet,
                    uint32_t srcId,
                    odb::dbNet* tgtNet,
                    uint32_t dstId,
                    uint32_t capCnt);
  double printDiffCC(odb::dbNet* net1,
                     odb::dbNet* net2,
                     uint32_t node1,
                     uint32_t node2,
                     double dbCap,
                     double refCap,
                     const char* ctype,
                     int ii);
  uint32_t diffNetCap(odb::dbNet* net);
  uint32_t diffNetGndCap(odb::dbNet* net);
  uint32_t diffNetRes(odb::dbNet* net);
  uint32_t matchNetRes(odb::dbNet* net);
  void resetExtIds(uint32_t rit);
  uint32_t endNet(odb::dbNet* net, uint32_t resCnt);
  uint32_t sortRSegs();
  bool getFirstShape(odb::dbNet* net, odb::dbShape& s);
  uint32_t getNetLW(odb::dbNet* net, uint32_t& w);
  bool mkCapStats(odb::dbNet* net);
  void collectDbCCap(odb::dbNet* net);
  void matchCcValue(odb::dbNet* net);
  uint32_t matchNetCcap(odb::dbNet* net);
  uint32_t collectRefCCap(odb::dbNet* srcNet,
                          odb::dbNet* tgtNet,
                          uint32_t capCnt);

  uint32_t getNodeCap(odb::dbSet<odb::dbRSeg>& rcSet,
                      uint32_t capNodeId,
                      double* totCap);
  uint32_t getMultiples(uint32_t cnt, uint32_t base);
  uint32_t readMaxMapId(int* cornerCnt = nullptr);
  void addNameMapId(uint32_t ii, uint32_t id);
  uint32_t getNameMapId(uint32_t ii);
  void setCap(const double* cap,
              uint32_t n,
              double* totCap,
              uint32_t startIndex);
  void incrementCounter(double* cap, uint32_t n);
  uint32_t setRCCaps(odb::dbNet* net);

  uint32_t getMinCapNode(odb::dbNet* net, uint32_t* minNode);
  void computeCaps(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  uint32_t getMappedCapNode(uint32_t nodeId);
  void writePorts(odb::dbNet* net);
  void writeITerms(odb::dbNet* net);
  void writeCapPorts(odb::dbNet* net);
  void writeCapITerms(odb::dbNet* net);
  void writeNodeCaps(odb::dbNet* net, uint32_t netId = 0);
  void writeCapPort(uint32_t node, uint32_t capIndex);
  void writeCapITerm(uint32_t node, uint32_t capIndex);
  void writeNodeCap(uint32_t netId, uint32_t capIndex, uint32_t ii);
  void writeRes(uint32_t netId, odb::dbSet<odb::dbRSeg>& rSet);
  void writeCapNode(uint32_t capNodeId, uint32_t netId);
  void writeCapNode(odb::dbCapNode* capNode, uint32_t netId);
  uint32_t getCapNodeId(const char* nodeWord,
                        const char* capWord,
                        uint32_t* netId);
  uint32_t getCapIdFromCapTable(const char* nodeWord);
  void addNewCapIdOnCapTable(const char* nodeWord, uint32_t capId);
  uint32_t getBtermCapNode(uint32_t termId);
  uint32_t writeSrcCouplingCapsNoSort(odb::dbNet* net);
  uint32_t writeTgtCouplingCapsNoSort(odb::dbNet* net);
  void writeSrcCouplingCaps(odb::dbNet* net, uint32_t netId = 0);
  void writeTgtCouplingCaps(odb::dbNet* net, uint32_t netId = 0);
  void writeCouplingCaps(const std::vector<odb::dbCCSeg*>& vec_cc,
                         uint32_t netId);
  void writeCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, uint32_t netId);
  void writeCouplingCapsNoSort(odb::dbSet<odb::dbCCSeg>& capSet,
                               uint32_t netId);
  void setSpefFlag(bool v);
  void setExtIds(odb::dbNet* net);
  void setExtIds();
  void resetNameTable(uint32_t n);
  void createName(uint32_t n, const char* name);
  const char* makeName(const char* name);
  odb::dbNet* getDbNet(uint32_t* id, uint32_t spefId = 0);
  odb::dbInst* getDbInst(uint32_t id);
  odb::dbCapNode* createCapNode(uint32_t nodeId, char* capWord = nullptr);
  void addCouplingCaps(odb::dbNet* net, double* totCap);
  void addCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, double* totCap);
  void writeCapPortsAndIterms(odb::dbSet<odb::dbCapNode>& capSet, bool bterms);
  void writeSingleRC(double val, bool delimiter);
  void writeInternalCaps(odb::dbNet* net, odb::dbSet<odb::dbCapNode>& capSet);
  void printCapNode(uint32_t capNodeId);
  void printAppearance(int app, int appc);

  void addNetNodeHash(odb::dbNet* net);
  void buildNodeHashTable();

  bool isNetExcluded();

  void computeCapsAdd2Target(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  void copyCap(double* totCap, const double* cap, uint32_t n = 0);
  void adjustCap(double* totCap, const double* cap, uint32_t n = 0);

  char* getDelimiter();
  void writeNameNode(odb::dbCapNode* node);
  void writeCapName(odb::dbCapNode* capNode, uint32_t capIndex);

  void setBlock(odb::dbBlock* blk);

  const char* comp_bounds(double val, double min, double max, double& percent);
  double percentDiff(double dbCap, double refCap);

  enum COORD_TYPE
  {
    C_NONE,
    C_ON
  };

  char _inFile[1024];
  FILE* _inFP;

  char _outFile[1024];
  FILE* _outFP = nullptr;

  Parser* _parser = nullptr;

  Parser* _nodeParser = nullptr;
  Parser* _nodeCoordParser = nullptr;
  uint32_t _tmpNetSpefId = 0;

  odb::dbTech* _tech;
  odb::dbBlock* _block;
  odb::dbBlock* _cornerBlock;
  const char* _version = nullptr;
  bool _useBaseCornerRc = false;
  uint32_t _blockId = 0;

  char _design[1024];
  char _bus_delimiter[5];
  char _delimiter[5];
  char _divider[5];

  char _res_unit_word[5];
  char _cap_unit_word[5];
  char _time_unit_word[5];
  char _ind_unit_word[7];

  double _res_unit = 1.0;
  double _cap_unit = 1.0;
  int _time_unit = 1;
  int _ind_unit = 1;

  uint32_t _cornerCnt = 0;
  uint32_t _cornersPerBlock;

  bool _extracted = false;

  Array1D<uint32_t>* _nodeTable = nullptr;
  Array1D<uint32_t>* _btermTable = nullptr;
  Array1D<uint32_t>* _itermTable = nullptr;
  Array1D<double*>* _nodeCapTable = nullptr;
  Array1D<double*>* _btermCapTable = nullptr;
  Array1D<double*>* _itermCapTable = nullptr;

  const char* _spefName;
  uint32_t _maxMapId;
  Array1D<uint32_t>* _idMapTable;
  Array1D<const char*>* _nameMapTable = nullptr;
  uint32_t _lastNameMapIndex = 0;

  uint32_t _cCnt;
  uint32_t _rCnt;

  bool _partial = false;
  bool _btermFound;

  bool _noBackSlash = false;

  uint32_t _baseNameMap = 0;
  uint32_t _firstCapNode;

  bool _preserveCapValues = false;
  bool _symmetricCCcaps = true;

  bool _testParsing = false;

  uint32_t _tnetCnt;

  bool _wOnlyClock;
  bool _wConn = false;
  bool _wCap = false;
  bool _wOnlyCCcap = false;
  bool _wRes = false;
  bool _noCnum = false;
  bool _foreign = false;

  uint32_t _rRun = 0;
  bool _stampWire = false;
  bool _rConn = false;
  bool _rCap = false;
  bool _rOnlyCCcap = false;
  bool _rRes = false;
  bool _mMap;
  bool _noNameMap;
  bool _noPorts;
  bool _keep_loaded_corner;
  char _mMapName[2000];
  char _nDvdName[2000];
  bool _inputNet = false;

  NameTable* _notFoundInst;
  NameTable* _nodeHashTable = nullptr;
  uint32_t _tmpCapId = 1;
  char _tmpBuff1[1024];
  char _tmpBuff2[1024];

  uint32_t _gndCapCnt = 0;
  uint32_t _ccCapCnt = 0;
  uint32_t _resCnt = 0;
  bool _statsOnly = false;

  uint32_t _maxNetNode;
  uint32_t _minNetNode;

  bool _gzipFlag = false;
  bool _stopAfterNameMap = false;
  float _upperCalibLimit;
  float _lowerCalibLimit;
  bool _calib = false;
  bool _diff = false;
  bool _match = false;
  odb::dbNet* _d_corner_net;
  odb::dbNet* _d_net;
  uint32_t _unmatchedSpefNet;
  uint32_t _unmatchedSpefInst;
  FILE* _diffLogFP = nullptr;
  FILE* _diffOutFP = nullptr;
  uint32_t _diffLogCnt;
  double _upperThres;
  double _lowerThres;

  double _netCCapTable[10];
  double _netGndCapTable[10];
  double _netResTable[10];

  bool _stopBeforeDnets = false;
  double _cc_thres;
  bool _cc_thres_flag;
  uint32_t _cc_break_cnt = 0;
  uint32_t _cc_merge_cnt = 0;
  uint32_t _cc_app_print_limit = 0;
  int _appcnt[16];
  Array1D<int>* _ccidmap = nullptr;

  uint32_t _dbCorner;
  const char* _tmpNetName = nullptr;
  char* _netSubWord = nullptr;
  char* _netExcludeSubWord = nullptr;
  FILE* _capStatsFP = nullptr;

  bool _singleP = false;

  std::vector<odb::dbNet*> _netV1;

  Array1D<uint32_t>* _capNodeTable = nullptr;
  Array1D<double>* _xCoordTable = nullptr;
  Array1D<double>* _yCoordTable = nullptr;
  Array1D<int>* _x1CoordTable = nullptr;
  Array1D<int>* _y1CoordTable = nullptr;
  Array1D<int>* _x2CoordTable = nullptr;
  Array1D<int>* _y2CoordTable = nullptr;
  Array1D<uint32_t>* _levelTable = nullptr;
  Array1D<uint32_t>* _idTable = nullptr;
  double _lengthUnit;
  double _nodeCoordFactor;
  bool _doSortRSeg = true;
  COORD_TYPE _readingNodeCoordsInput;
  COORD_TYPE _readingNodeCoords = C_NONE;
  COORD_TYPE _writingNodeCoords = C_NONE;

  int _fixloop;
  uint32_t _breakLoopNet;
  uint32_t _loopNet;
  uint32_t _bigLoop;
  uint32_t _multipleLoop;
  uint32_t _srii;
  Array1D<uint32_t>* _srsegi;
  Array1D<odb::dbRSeg*>* _nrseg;
  Array1D<Array1D<int>*>* _hcnrc;
  uint32_t _rsegCnt;

  bool _readAllCorners = false;
  int _in_spef_corner = -1;

  uint32_t _childBlockInstBaseMap = 0;
  uint32_t _childBlockNetBaseMap = 0;

  utl::Logger* logger_;

 public:
  bool _addRepeatedCapValue = true;
  bool _noCapNumCollapse = false;
  FILE* _capNodeFile = nullptr;
  int _db_calibbase_corner;
  int _db_ext_corner = -1;
  int _active_corner_cnt = 0;
  int _active_corner_number[32];
  bool _writeNameMap;
  bool _moreToRead;
  bool _termJxy = false;
  bool _incrPlusCcNets = false;
  odb::dbBTerm* _ccbterm1;
  odb::dbBTerm* _ccbterm2;
  odb::dbITerm* _cciterm1;
  odb::dbITerm* _cciterm2;
  char* _bufString = nullptr;
  char* _msgBuf1;
  char* _msgBuf2;

  extMain* _ext;
};

}  // namespace rcx
