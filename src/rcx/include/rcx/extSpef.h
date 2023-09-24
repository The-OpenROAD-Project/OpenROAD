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

#include "array1.h"
#include "db.h"
#include "dbShape.h"
#include "extRCap.h"
#include "odb.h"
#include "parse.h"

namespace utl {
class Logger;
}

namespace rcx {

using odb::Ath__array1D;
using odb::Ath__gridTable;
using odb::uint;
using utl::Logger;

class NameTable;

class extRcTripplet
{
 private:
  uint _srcId;
  uint _dstId;
  uint _dstNetId;
  char _dstWord[32];
  char _valueWord[64];
  bool _coupleFlag;

  friend class extMain;
  friend class extSpef;
};

class extSpef
{
 public:
  extSpef(odb::dbTech* tech,
          odb::dbBlock* blk,
          Logger* logger,
          extMain* extmain,
          uint btermCnt = 0,
          uint itermCnt = 0);
  ~extSpef();

  void setLogger(Logger* logger);
  bool matchNetGndCap(odb::dbNet* net,
                      uint dbCorner,
                      double dbCap,
                      double refCap);
  bool calibrateNetGndCap(odb::dbNet* net,
                          uint dbCorner,
                          double dbCap,
                          double refCap);
  bool computeFactor(double db, double ref, float& factor);

  void initNodeCoordTables(uint memChunk);
  void resetNodeCoordTables();
  void deleteNodeCoordTables();
  bool readNodeCoords(uint cpos);
  void checkCCterm();
  int findNodeIndexFromNodeCoords(uint targetCapNodeId);
  uint writeNodeCoords(uint netId, odb::dbSet<odb::dbRSeg>& rSet);

  void setupMappingForWrite(uint btermCnt = 0, uint itermCnt = 0);
  void setupMapping(uint itermCnt = 0);
  void preserveFlag(bool v);
  void setCornerCnt(uint n);

  void incr_rRun() { _rRun++; };
  void writeCNodeNumber();
  odb::dbBlock* getBlock() { return _block; }
  uint writeNet(odb::dbNet* net, double resBound, uint debug);
  uint stopWrite();
  uint readBlock(uint debug,
                 std::vector<odb::dbNet*> tnets,
                 bool force,
                 bool rConn,
                 char* nodeCoord,
                 bool rCap,
                 bool rOnlyCCcap,
                 bool rRes,
                 float cc_thres,
                 float length_unit,
                 bool extracted,
                 bool keepLoadedCorner,
                 bool stampWire,
                 uint testParsing,
                 int app_print_limit,
                 bool m_map,
                 int corner,
                 double low,
                 double up,
                 char* excludeNetSubWord,
                 char* netSubWord,
                 char* capStatsFile,
                 const char* dbCornerName,
                 const char* calibrateBaseCorner,
                 int spefCorner,
                 int fixLoop,
                 bool& rsegCoord);
  uint readBlockIncr(uint debug);

  bool setOutSpef(char* filename);
  bool closeOutFile();
  void setGzipFlag(bool gzFlag);
  bool setInSpef(char* filename, bool onlyOpen = false);
  bool isCapNodeExcluded(odb::dbCapNode* node);
  uint writeBlock(char* nodeCoord,
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
  uint writeBlock(const char* nodeCoord,
                  const char* capUnit,
                  const char* resUnit,
                  bool stopAfterNameMap,
                  std::vector<odb::dbNet*> tnets,
                  bool wClock,
                  bool wConn,
                  bool wCap,
                  bool wOnlyCCcap,
                  bool wRes,
                  bool noCnum,
                  bool stopBeforeDnets,
                  bool noBackSlash,
                  bool parallel);

  int getWriteCorner(int corner, const char* name);
  bool writeITerm(uint node);
  bool writeBTerm(uint node);
  bool writeNode(uint netId, uint node);
  uint writePort(uint node);
  void writeDnet(uint netId, double* totCap);
  void writeKeyword(const char* keyword);
  uint writePorts();
  uint writeITerms();
  uint getInstMapId(uint id);
  void writeITermNode(uint node);

  bool isBTermMarked(uint node);
  void markBTerm(uint node);
  void unMarkBTerm(uint node);
  uint getBTermMapping(uint b);

  uint getNetMapId(uint netId);
  char* tinkerSpefName(char* iname);
  char* addEscChar(const char* iname, bool esc_bus_brkts);

  bool isITermMarked(uint node);
  void markITerm(uint node);
  void unMarkITerm(uint node);
  uint getITermMapping(uint b);
  void resetTermTables();
  void resetCapTables(uint maxNode);
  void initCapTable(Ath__array1D<double*>* table);
  void deleteTableCap(Ath__array1D<double*>* table);
  void reinitCapTable(Ath__array1D<double*>* table, uint n);
  void addCap(const double* cap, double* totCap, uint n);
  void addHalfCap(double* totCap, const double* cap, uint n = 0);
  void getCaps(odb::dbNet* net, double* totCap);
  void resetCap(double* cap);
  void resetCap(double* cap, uint cnt);
  void writeRCvalue(const double* totCap, double units);
  uint writeCapPort(uint index);
  uint writeCapITerm(uint index);
  uint writeCapITerms();
  uint writeCapPorts();
  void writeNodeCaps(uint netId, uint minNode, uint maxNode);
  uint writeBlockPorts();
  uint writeNetMap(odb::dbSet<odb::dbNet>& nets);
  uint writeInstMap();

  uint readDNet(uint debug);
  uint getSpefNode(char* nodeWord, uint* instNetId, int* nodeType);
  uint getITermId(uint instId, char* name);
  uint getBTermId(char* name);
  uint getBTermId(uint id);

  bool readHeaderInfo(uint debug, bool skip = false);
  bool writeHeaderInfo(uint debug);
  bool readPorts(uint debug);
  bool readNameMap(uint debug, bool skip = false);

  void setDesign(char* name);
  uint getCapNode(char* nodeWord, char* capWord);
  uint getMappedBTermId(uint id);
  void setUseIdsFlag(bool diff = false, bool calib = false);
  void setCalibLimit(float upperLimit, float lowerLimit);
  uint diffGndCap(odb::dbNet* net, uint capCnt, uint capId);
  uint diffNetCcap(odb::dbNet* net);
  double printDiff(odb::dbNet* net,
                   double dbCap,
                   double refCap,
                   const char* ctype,
                   int ii,
                   int id = -1);
  uint diffCCap(odb::dbNet* srcNet,
                uint srcId,
                odb::dbNet* tgtNet,
                uint dstId,
                uint capCnt);
  double printDiffCC(odb::dbNet* net1,
                     odb::dbNet* net2,
                     uint node1,
                     uint node2,
                     double dbCap,
                     double refCap,
                     const char* ctype,
                     int ii);
  uint diffNetCap(odb::dbNet* net);
  uint diffNetGndCap(odb::dbNet* net);
  uint diffNetRes(odb::dbNet* net);
  uint matchNetRes(odb::dbNet* net);
  void resetExtIds(uint rit);
  uint endNet(odb::dbNet* net, uint resCnt);
  uint sortRSegs();
  bool getFirstShape(odb::dbNet* net, odb::dbShape& s);
  uint getNetLW(odb::dbNet* net, uint& w);
  bool mkCapStats(odb::dbNet* net);
  void collectDbCCap(odb::dbNet* net);
  void matchCcValue(odb::dbNet* net);
  uint matchNetCcap(odb::dbNet* net);
  uint collectRefCCap(odb::dbNet* srcNet, odb::dbNet* tgtNet, uint capCnt);

  uint getNodeCap(odb::dbSet<odb::dbRSeg>& rcSet,
                  uint capNodeId,
                  double* totCap);
  uint getMultiples(uint cnt, uint base);
  uint readMaxMapId(int* cornerCnt = NULL);
  void addNameMapId(uint ii, uint id);
  uint getNameMapId(uint ii);
  void setCap(const double* cap, uint n, double* totCap, uint startIndex);
  void incrementCounter(double* cap, uint n);
  uint setRCCaps(odb::dbNet* net);

  uint getMinCapNode(odb::dbNet* net, uint* minNode);
  uint computeCaps(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  uint getMappedCapNode(uint nodeId);
  uint writePorts(odb::dbNet* net);
  uint writeITerms(odb::dbNet* net);
  uint writeCapPorts(odb::dbNet* net);
  uint writeCapITerms(odb::dbNet* net);
  uint writeNodeCaps(odb::dbNet* net, uint netId = 0);
  uint writeCapPort(uint node, uint capIndex);
  uint writeCapITerm(uint node, uint capIndex);
  void writeNodeCap(uint netId, uint capIndex, uint ii);
  uint writeRes(uint netId, odb::dbSet<odb::dbRSeg>& rSet);
  bool writeCapNode(uint capNodeId, uint netId);
  bool writeCapNode(odb::dbCapNode* capNode, uint netId);
  uint getCapNodeId(char* nodeWord, char* capWord, uint* netId);
  uint getCapNodeId(char* nodeWord);
  uint getCapIdFromCapTable(char* nodeWord);
  void addNewCapIdOnCapTable(char* nodeWord, uint capId);
  uint getItermCapNode(uint termId);
  uint getBtermCapNode(uint termId);
  uint writeSrcCouplingCapsNoSort(odb::dbNet* net);
  uint writeTgtCouplingCapsNoSort(odb::dbNet* net);
  uint writeSrcCouplingCaps(odb::dbNet* net, uint netId = 0);
  uint writeTgtCouplingCaps(odb::dbNet* net, uint netId = 0);
  uint writeCouplingCaps(std::vector<odb::dbCCSeg*>& vec_cc, uint netId);
  uint writeCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, uint netId);
  uint writeCouplingCapsNoSort(odb::dbSet<odb::dbCCSeg>& capSet, uint netId);
  void setSpefFlag(bool v);
  void setExtIds(odb::dbNet* net);
  void setExtIds();
  void resetNameTable(uint n);
  void createName(uint n, char* name);
  char* makeName(char* name);
  odb::dbNet* getDbNet(uint* id, uint spefId = 0);
  odb::dbInst* getDbInst(uint id);
  odb::dbCapNode* createCapNode(uint nodeId, char* capWord = NULL);
  void addCouplingCaps(odb::dbNet* net, double* totCap);
  void addCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, double* totCap);
  uint writeCapPortsAndIterms(odb::dbSet<odb::dbCapNode>& capSet, bool bterms);
  void writeSingleRC(double val, bool delimeter);
  uint writeInternalCaps(odb::dbNet* net, odb::dbSet<odb::dbCapNode>& capSet);
  void printCapNode(uint capNodeId);
  void printAppearance(int app, int appc);
  void printAppearance(int* appcnt, int tapp);

  void reinit();
  void addNetNodeHash(odb::dbNet* net);
  void buildNodeHashTable();

  bool isNetExcluded();

  uint computeCapsAdd2Target(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  void copyCap(double* totCap, const double* cap, uint n = 0);
  void adjustCap(double* totCap, const double* cap, uint n = 0);
  void set_single_pi(bool v);

  uint getAppPrintLimit() { return _cc_app_print_limit; };
  int* getAppCnt() { return _appcnt; };
  uint write_spef_nets(bool flatten, bool parallel);
  char* getDelimeter();
  void writeNameNode(odb::dbCapNode* node);
  uint writeCapName(odb::dbCapNode* capNode, uint capIndex);

  void setBlock(odb::dbBlock* blk);

  const char* comp_bounds(double val, double min, double max, double& percent);
  double percentDiff(double dbCap, double refCap);

 private:
  enum COORD_TYPE
  {
    C_NONE,
    C_ON
  };

  char _inFile[1024];
  FILE* _inFP;

  char _outFile[1024];
  FILE* _outFP;

  Ath__parser* _parser;

  Ath__parser* _nodeParser;
  Ath__parser* _nodeCoordParser;
  uint _tmpNetSpefId;

  odb::dbTech* _tech;
  odb::dbBlock* _block;
  odb::dbBlock* _cornerBlock;
  bool _useBaseCornerRc;
  uint _blockId;

  char _design[1024];
  char _bus_delimiter[5];
  char _delimiter[5];
  char _divider[5];

  char _res_unit_word[5];
  char _cap_unit_word[5];
  char _time_unit_word[5];
  char _ind_unit_word[7];

  double _res_unit;
  double _cap_unit;
  int _time_unit;
  int _ind_unit;

  uint _cornerCnt;
  uint _cornersPerBlock;

  bool _extracted;

  Ath__array1D<uint>* _nodeTable;
  Ath__array1D<uint>* _btermTable;
  Ath__array1D<uint>* _itermTable;
  Ath__array1D<double*>* _nodeCapTable;
  Ath__array1D<double*>* _btermCapTable;
  Ath__array1D<double*>* _itermCapTable;

  char* _spefName;
  uint _maxMapId;
  Ath__array1D<uint>* _idMapTable;
  Ath__array1D<char*>* _nameMapTable;
  uint _lastNameMapIndex;

  uint _cCnt;
  uint _rCnt;

  bool _partial;
  bool _btermFound;

  bool _noBackSlash;

  uint _baseNameMap;
  uint _firstCapNode;

  bool _preserveCapValues;
  bool _symmetricCCcaps;

  bool _testParsing;

  uint _tnetCnt;

  bool _wOnlyClock;
  bool _wConn;
  bool _wCap;
  bool _wOnlyCCcap;
  bool _wRes;
  bool _noCnum;
  bool _foreign;

  uint _rRun;
  bool _stampWire;
  bool _rConn;
  bool _rCap;
  bool _rOnlyCCcap;
  bool _rRes;
  bool _mMap;
  bool _noNameMap;
  bool _noPorts;
  bool _keep_loaded_corner;
  char _mMapName[2000];
  char _nDvdName[2000];
  bool _inputNet;

  NameTable* _notFoundInst;
  NameTable* _nodeHashTable;
  uint _tmpCapId;
  char _tmpBuff1[1024];
  char _tmpBuff2[1024];

  uint _gndCapCnt;
  uint _ccCapCnt;
  uint _resCnt;
  bool _statsOnly;

  AthPool<extRcTripplet>* _rcPool;
  Ath__array1D<extRcTripplet*>* _rcTrippletTable;

  uint _maxNetNode;
  uint _minNetNode;

  bool _gzipFlag;
  bool _stopAfterNameMap;
  float _upperCalibLimit;
  float _lowerCalibLimit;
  bool _calib;
  bool _diff;
  bool _match;
  odb::dbNet* _d_corner_net;
  odb::dbNet* _d_net;
  uint _unmatchedSpefNet;
  uint _unmatchedSpefInst;
  FILE* _diffLogFP;
  FILE* _diffOutFP;
  uint _diffLogCnt;
  double _upperThres;
  double _lowerThres;

  double _netCCapTable[10];
  double _netGndCapTable[10];
  double _netResTable[10];

  bool _stopBeforeDnets;
  double _cc_thres;
  bool _cc_thres_flag;
  uint _cc_break_cnt;
  uint _cc_merge_cnt;
  uint _cc_app_print_limit;
  int _appcnt[16];
  Ath__array1D<int>* _ccidmap;

  uint _dbCorner;
  char* _tmpNetName;
  char* _netSubWord;
  char* _netExcludeSubWord;
  FILE* _capStatsFP;

  bool _singleP;

  std::vector<odb::dbNet*> _netV1;

  Ath__array1D<uint>* _capNodeTable;
  Ath__array1D<double>* _xCoordTable;
  Ath__array1D<double>* _yCoordTable;
  Ath__array1D<int>* _x1CoordTable;
  Ath__array1D<int>* _y1CoordTable;
  Ath__array1D<int>* _x2CoordTable;
  Ath__array1D<int>* _y2CoordTable;
  Ath__array1D<uint>* _levelTable;
  Ath__array1D<uint>* _idTable;
  double _lengthUnit;
  double _nodeCoordFactor;
  bool _doSortRSeg;
  COORD_TYPE _readingNodeCoordsInput;
  COORD_TYPE _readingNodeCoords;
  COORD_TYPE _writingNodeCoords;

  int _fixloop;
  uint _breakLoopNet;
  uint _loopNet;
  uint _bigLoop;
  uint _multipleLoop;
  uint _srii;
  Ath__array1D<uint>* _srsegi;
  Ath__array1D<odb::dbRSeg*>* _nrseg;
  Ath__array1D<Ath__array1D<int>*>* _hcnrc;
  uint _rsegCnt;

  bool _readAllCorners;
  int _in_spef_corner;

  uint _childBlockInstBaseMap;
  uint _childBlockNetBaseMap;

  Logger* logger_;

 public:
  bool _addRepeatedCapValue;
  bool _noCapNumCollapse;
  FILE* _capNodeFile;
  int _db_calibbase_corner;
  int _db_ext_corner;
  int _active_corner_cnt;
  int _active_corner_number[32];
  bool _writeNameMap;
  bool _moreToRead;
  bool _termJxy;
  bool _incrPlusCcNets;
  odb::dbBTerm* _ccbterm1;
  odb::dbBTerm* _ccbterm2;
  odb::dbITerm* _cciterm1;
  odb::dbITerm* _cciterm2;
  char* _bufString;
  char* _msgBuf1;
  char* _msgBuf2;

  extMain* _ext;
};

}  // namespace rcx
