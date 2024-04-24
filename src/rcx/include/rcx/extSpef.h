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

#include "extRCap.h"
#include "odb/array1.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/odb.h"
#include "odb/parse.h"

namespace utl {
class Logger;
}

namespace rcx {

using odb::Ath__array1D;
using odb::uint;
using utl::Logger;

class NameTable;

class extSpef
{
 public:
  extSpef(odb::dbTech* tech,
          odb::dbBlock* blk,
          Logger* logger,
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
  void writeNet(odb::dbNet* net, double resBound, uint debug);
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
  void setCornerCnt(uint n);
  uint readBlock(uint debug,
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
                 uint testParsing,
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
  uint getAppPrintLimit() { return _cc_app_print_limit; };
  int* getAppCnt() { return _appcnt; };
  uint readBlockIncr(uint debug);
  void setCalibLimit(float upperLimit, float lowerLimit);
  void printAppearance(const int* appcnt, int tapp);

 private:
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
  void writeNodeCoords(uint netId, odb::dbSet<odb::dbRSeg>& rSet);

  void setupMappingForWrite(uint btermCnt = 0, uint itermCnt = 0);
  void setupMapping(uint itermCnt = 0);

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

  void writeITerm(uint node);
  void writeBTerm(uint node);
  void writeNode(uint netId, uint node);
  void writePort(uint node);
  void writeDnet(uint netId, const double* totCap);
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
  const char* tinkerSpefName(const char* iname);
  const char* addEscChar(const char* iname, bool esc_bus_brkts);

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
  void writeCapPort(uint index);
  uint writeCapITerm(uint index);
  uint writeCapITerms();
  uint writeCapPorts();
  void writeNodeCaps(uint netId, uint minNode, uint maxNode);
  void writeBlockPorts();
  void writeNetMap(odb::dbSet<odb::dbNet>& nets);
  void writeInstMap();

  uint readDNet(uint debug);
  uint getSpefNode(char* nodeWord, uint* instNetId, int* nodeType);
  uint getITermId(uint instId, const char* name);
  uint getBTermId(const char* name);
  uint getBTermId(uint id);

  bool readHeaderInfo(uint debug, bool skip = false);
  void writeHeaderInfo();
  bool readPorts();
  bool readNameMap(uint debug, bool skip = false);

  uint getCapNode(char* nodeWord, char* capWord);
  uint getMappedBTermId(uint id);
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
  uint readMaxMapId(int* cornerCnt = nullptr);
  void addNameMapId(uint ii, uint id);
  uint getNameMapId(uint ii);
  void setCap(const double* cap, uint n, double* totCap, uint startIndex);
  void incrementCounter(double* cap, uint n);
  uint setRCCaps(odb::dbNet* net);

  uint getMinCapNode(odb::dbNet* net, uint* minNode);
  void computeCaps(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  uint getMappedCapNode(uint nodeId);
  void writePorts(odb::dbNet* net);
  void writeITerms(odb::dbNet* net);
  void writeCapPorts(odb::dbNet* net);
  void writeCapITerms(odb::dbNet* net);
  void writeNodeCaps(odb::dbNet* net, uint netId = 0);
  void writeCapPort(uint node, uint capIndex);
  void writeCapITerm(uint node, uint capIndex);
  void writeNodeCap(uint netId, uint capIndex, uint ii);
  void writeRes(uint netId, odb::dbSet<odb::dbRSeg>& rSet);
  void writeCapNode(uint capNodeId, uint netId);
  void writeCapNode(odb::dbCapNode* capNode, uint netId);
  uint getCapNodeId(const char* nodeWord, const char* capWord, uint* netId);
  uint getCapIdFromCapTable(const char* nodeWord);
  void addNewCapIdOnCapTable(const char* nodeWord, uint capId);
  uint getBtermCapNode(uint termId);
  uint writeSrcCouplingCapsNoSort(odb::dbNet* net);
  uint writeTgtCouplingCapsNoSort(odb::dbNet* net);
  void writeSrcCouplingCaps(odb::dbNet* net, uint netId = 0);
  void writeTgtCouplingCaps(odb::dbNet* net, uint netId = 0);
  void writeCouplingCaps(const std::vector<odb::dbCCSeg*>& vec_cc, uint netId);
  void writeCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, uint netId);
  void writeCouplingCapsNoSort(odb::dbSet<odb::dbCCSeg>& capSet, uint netId);
  void setSpefFlag(bool v);
  void setExtIds(odb::dbNet* net);
  void setExtIds();
  void resetNameTable(uint n);
  void createName(uint n, const char* name);
  const char* makeName(const char* name);
  odb::dbNet* getDbNet(uint* id, uint spefId = 0);
  odb::dbInst* getDbInst(uint id);
  odb::dbCapNode* createCapNode(uint nodeId, char* capWord = nullptr);
  void addCouplingCaps(odb::dbNet* net, double* totCap);
  void addCouplingCaps(odb::dbSet<odb::dbCCSeg>& capSet, double* totCap);
  void writeCapPortsAndIterms(odb::dbSet<odb::dbCapNode>& capSet, bool bterms);
  void writeSingleRC(double val, bool delimeter);
  void writeInternalCaps(odb::dbNet* net, odb::dbSet<odb::dbCapNode>& capSet);
  void printCapNode(uint capNodeId);
  void printAppearance(int app, int appc);

  void addNetNodeHash(odb::dbNet* net);
  void buildNodeHashTable();

  bool isNetExcluded();

  void computeCapsAdd2Target(odb::dbSet<odb::dbRSeg>& rcSet, double* totCap);
  void copyCap(double* totCap, const double* cap, uint n = 0);
  void adjustCap(double* totCap, const double* cap, uint n = 0);

  char* getDelimeter();
  void writeNameNode(odb::dbCapNode* node);
  void writeCapName(odb::dbCapNode* capNode, uint capIndex);

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

  Ath__parser* _parser = nullptr;

  Ath__parser* _nodeParser = nullptr;
  Ath__parser* _nodeCoordParser = nullptr;
  uint _tmpNetSpefId = 0;

  odb::dbTech* _tech;
  odb::dbBlock* _block;
  odb::dbBlock* _cornerBlock;
  const char* _version = nullptr;
  bool _useBaseCornerRc = false;
  uint _blockId = 0;

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

  uint _cornerCnt = 0;
  uint _cornersPerBlock;

  bool _extracted = false;

  Ath__array1D<uint>* _nodeTable = nullptr;
  Ath__array1D<uint>* _btermTable = nullptr;
  Ath__array1D<uint>* _itermTable = nullptr;
  Ath__array1D<double*>* _nodeCapTable = nullptr;
  Ath__array1D<double*>* _btermCapTable = nullptr;
  Ath__array1D<double*>* _itermCapTable = nullptr;

  const char* _spefName;
  uint _maxMapId;
  Ath__array1D<uint>* _idMapTable;
  Ath__array1D<const char*>* _nameMapTable = nullptr;
  uint _lastNameMapIndex = 0;

  uint _cCnt;
  uint _rCnt;

  bool _partial = false;
  bool _btermFound;

  bool _noBackSlash = false;

  uint _baseNameMap = 0;
  uint _firstCapNode;

  bool _preserveCapValues = false;
  bool _symmetricCCcaps = true;

  bool _testParsing = false;

  uint _tnetCnt;

  bool _wOnlyClock;
  bool _wConn = false;
  bool _wCap = false;
  bool _wOnlyCCcap = false;
  bool _wRes = false;
  bool _noCnum = false;
  bool _foreign = false;

  uint _rRun = 0;
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
  uint _tmpCapId = 1;
  char _tmpBuff1[1024];
  char _tmpBuff2[1024];

  uint _gndCapCnt = 0;
  uint _ccCapCnt = 0;
  uint _resCnt = 0;
  bool _statsOnly = false;

  uint _maxNetNode;
  uint _minNetNode;

  bool _gzipFlag = false;
  bool _stopAfterNameMap = false;
  float _upperCalibLimit;
  float _lowerCalibLimit;
  bool _calib = false;
  bool _diff = false;
  bool _match = false;
  odb::dbNet* _d_corner_net;
  odb::dbNet* _d_net;
  uint _unmatchedSpefNet;
  uint _unmatchedSpefInst;
  FILE* _diffLogFP = nullptr;
  FILE* _diffOutFP = nullptr;
  uint _diffLogCnt;
  double _upperThres;
  double _lowerThres;

  double _netCCapTable[10];
  double _netGndCapTable[10];
  double _netResTable[10];

  bool _stopBeforeDnets = false;
  double _cc_thres;
  bool _cc_thres_flag;
  uint _cc_break_cnt = 0;
  uint _cc_merge_cnt = 0;
  uint _cc_app_print_limit = 0;
  int _appcnt[16];
  Ath__array1D<int>* _ccidmap = nullptr;

  uint _dbCorner;
  const char* _tmpNetName = nullptr;
  char* _netSubWord = nullptr;
  char* _netExcludeSubWord = nullptr;
  FILE* _capStatsFP = nullptr;

  bool _singleP = false;

  std::vector<odb::dbNet*> _netV1;

  Ath__array1D<uint>* _capNodeTable = nullptr;
  Ath__array1D<double>* _xCoordTable = nullptr;
  Ath__array1D<double>* _yCoordTable = nullptr;
  Ath__array1D<int>* _x1CoordTable = nullptr;
  Ath__array1D<int>* _y1CoordTable = nullptr;
  Ath__array1D<int>* _x2CoordTable = nullptr;
  Ath__array1D<int>* _y2CoordTable = nullptr;
  Ath__array1D<uint>* _levelTable = nullptr;
  Ath__array1D<uint>* _idTable = nullptr;
  double _lengthUnit;
  double _nodeCoordFactor;
  bool _doSortRSeg = true;
  COORD_TYPE _readingNodeCoordsInput;
  COORD_TYPE _readingNodeCoords = C_NONE;
  COORD_TYPE _writingNodeCoords = C_NONE;

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

  bool _readAllCorners = false;
  int _in_spef_corner = -1;

  uint _childBlockInstBaseMap = 0;
  uint _childBlockNetBaseMap = 0;

  Logger* logger_;

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
