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

#ifndef ADS_EXTSPEF_H
#define ADS_EXTSPEF_H

#include "ISdb.h"
#include "array1.h"
#include "db.h"
#include "dbShape.h"
#include "extRCap.h"
#include "name.h"
#include "odb.h"
#include "parse.h"

//#define AFILE FILE

#include <map>

namespace utl {
class Logger;
}

namespace rcx {

using utl::Logger;

class extRcTripplet
{
  friend class extMain;

 private:
  uint _srcId;
  uint _dstId;
  uint _dstNetId;
  char _dstWord[32];
  char _valueWord[64];
  bool _coupleFlag;

  friend class extSpef;
};

class extSpef
{
 private:
  enum COORD_TYPE
  {
    C_NONE,
    C_STARRC,
    C_MAGMA
  };

  char _inFile[1024];
  FILE* _inFP;

  char _outFile[1024];
  //	AFILE *_outFP;
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
  bool _useIds;  // Net/inst/bterm names expected to be look like : N1, I1, B1
                 // name is same as in save_def option

  bool _preserveCapValues;
  bool _symmetricCCcaps;

  bool _testParsing;

  uint _tnetCnt;

  uint _wRun;
  bool _wOnlyClock;
  bool _wConn;
  bool _wCap;
  bool _wOnlyCCcap;
  bool _wRes;
  bool _noCnum;
  bool _foreign;

  uint _rRun;
  odb::ZPtr<odb::ISdb> _netSdb;
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

  Ath__nameTable* _notFoundInst;
  Ath__nameTable* _nodeHashTable;
  // AthHash<uint> *_nodeHashTable;
  // HashN<uint> _nodeHashTable;
  uint _tmpCapId;
  Ath__nameTable* _node2nodeHashTable;
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
  bool _NsLayer;

  std::vector<odb::dbNet*> _netV1;

  Ath__array1D<uint>* _capNodeTable;
  Ath__array1D<double>* _xCoordTable;
  Ath__array1D<double>* _yCoordTable;
  Ath__array1D<int>* _x1CoordTable;
  Ath__array1D<int>* _y1CoordTable;
  Ath__array1D<int>* _x2CoordTable;
  Ath__array1D<int>* _y2CoordTable;
  Ath__array1D<uint>* _levelTable;
  Ath__gridTable* _search;
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

  // 021810D BEGIN
  uint _childBlockInstBaseMap;
  uint _childBlockNetBaseMap;
  // 021810D END

 protected:
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
  bool _independentExtCorners;
  bool _incrPlusCcNets;
  odb::dbBTerm* _ccbterm1;
  odb::dbBTerm* _ccbterm2;
  odb::dbITerm* _cciterm1;
  odb::dbITerm* _cciterm2;
  char* _bufString;
  char* _msgBuf1;
  char* _msgBuf2;

  extMain* _ext;

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
  void adjustNodeCoords();
  void checkCCterm();
  int findNodeIndexFromNodeCoords(uint targetCapNodeId);
  uint getShapeIdFromNodeCoords(uint targetCapNodeId);
  uint getITermShapeId(odb::dbITerm* iterm);
  uint getBTermShapeId(odb::dbBTerm* bterm);
  void initSearchForNets();
  uint addNetShapesOnSearch(uint netId);
  uint findShapeId(uint netId, int x, int y);
  uint parseAndFindShapeId();
  void readNmCoords();
  uint findShapeId(uint netId,
                   int x1,
                   int y1,
                   int x2,
                   int y2,
                   char* layer,
                   bool matchLayer = false);
  uint findShapeId(uint netId, int x1, int y1, int x2, int y2, uint level);
  void searchDealloc();
  void getAnchorCoords(odb::dbNet* net,
                       uint shapeId,
                       int* x1,
                       int* y1,
                       int* x2,
                       int* y2,
                       odb::dbTechLayer** layer);
  uint writeNodeCoords(uint netId, odb::dbSet<odb::dbRSeg>& rSet);

  void setupMappingForWrite(uint btermCnt = 0, uint itermCnt = 0);
  void setupMapping(uint itermCnt = 0);
  void preserveFlag(bool v);
  void setCornerCnt(uint n);

  void incr_wRun() { _wRun++; };
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
                 odb::ZPtr<odb::ISdb> netSdb,
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
                 bool& resegCoord);
  uint readBlockIncr(uint debug);

  bool setOutSpef(char* filename);
  bool closeOutFile();
  void setGzipFlag(bool gzFlag);
  bool setInSpef(char* filename, bool onlyOpen = false);
  bool isCapNodeExcluded(odb::dbCapNode* node);
  uint writeBlock(char* nodeCoord,
                  const char* excludeCell,
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
                  bool flatten,
                  bool parallel);
  uint writeBlock(char* nodeCoord,
                  const char* excludeCell,
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
                  bool flatten,
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

  bool isITermMarked(uint node);
  void markITerm(uint node);
  void unMarkITerm(uint node);
  uint getITermMapping(uint b);
  void resetTermTables();
  void resetCapTables(uint maxNode);
  void initCapTable(Ath__array1D<double*>* table);
  void deleteTableCap(Ath__array1D<double*>* table);
  void reinitCapTable(Ath__array1D<double*>* table, uint n);
  void addCap(double* cap, double* totCap, uint n);
  void addHalfCap(double* totCap, double* cap, uint n = 0);
  void getCaps(odb::dbNet* net, double* totCap);
  void resetCap(double* cap);
  void resetCap(double* cap, uint cnt);
  void writeRCvalue(double* totCap, double units);
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
  void setUseIdsFlag(bool useIds, bool diff = false, bool calib = false);
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
  void setJunctionId(odb::dbCapNode* capnode, odb::dbRSeg* rseg);
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
  void setCap(double* cap, uint n, double* totCap, uint startIndex);
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
  uint writeRes(uint netId, odb::dbSet<odb::dbRSeg>& rcSet);
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
  bool newCouplingCap(char* nodeWord1, char* nodeword2, char* capWord);
  uint getCouplingCapId(uint ccNode1, uint ccNode2);
  void addCouplingCapId(uint ccId);
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
  void copyCap(double* totCap, double* cap, uint n = 0);
  void adjustCap(double* totCap, double* cap, uint n = 0);
  void set_single_pi(bool v);
  // uint getCapNodeId(std::map<uint,uint> &node_map, char *nodeWord, char
  // *capWord);

  uint getAppPrintLimit() { return _cc_app_print_limit; };
  int* getAppCnt() { return _appcnt; };
#if 0
	bool writeITerm(uint node);
	bool writeBTerm(uint node);
	void writeITermNode(uint node);
	uint getMappedBTermId(uint id);
	uint getITermId(uint id, char *name);
	uint getITermId(char *name);
	uint getBTermId(uint id);
	uint getBTermId(char *name);
#endif
  // 021610D BEGIN
  uint writeHierInstNameMap();
  uint writeHierNetNameMap();
  static int getIntProperty(odb::dbBlock* block, const char* name);
  uint write_spef_nets(bool flatten, bool parallel);
  char* getDelimeter();
  void writeNameNode(odb::dbCapNode* node);
  uint writeCapName(odb::dbCapNode* capNode, uint capIndex);
  // 021610D END

  // 021810D BEGIN
  void writeDnetHier(uint mapId, double* totCap);
  bool writeHierNet(odb::dbNet* net, double resBound, uint debug);
  void setHierBaseNameMap(uint instBase, uint netBase);
  // 021810D END

  // 022310D BEGIN
  void setBlock(odb::dbBlock* blk);
  // 022310D END

  // 620 DF DIFF SPEF
  const char* comp_bounds(double val, double min, double max, double& percent);
  double percentDiff(double dbCap, double refCap);
};

}  // namespace rcx

#endif
