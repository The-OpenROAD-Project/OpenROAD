// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "rcx/array1.h"
#include "rcx/box.h"
#include "rcx/rcx.h"
#include "rcx/util.h"

namespace rcx {

enum OverlapAdjust
{
  Z_noAdjust,
  Z_merge,
  Z_endAdjust
};

class Track;
class Grid;
class GridTable;
struct SEQ;

class SearchBox
{
 public:
  SearchBox(int x1, int y1, int x2, int y2, uint32_t l, int dir = -1);
  void set(int x1, int y1, int x2, int y2, uint32_t l, int dir);
  int loXY(uint32_t d) const;
  int loXY(uint32_t d, int loBound) const;
  int hiXY(uint32_t d) const;
  int hiXY(uint32_t d, int hiBound) const;
  void setLo(uint32_t d, int xy);
  void setHi(uint32_t d, int xy);
  void setType(uint32_t v);
  uint32_t getType() const;

  uint32_t getDir() const;
  uint32_t getLevel() const;
  void setDir(int dir = -1);
  void setLevel(uint32_t v);
  void setOwnerId(uint32_t v, uint32_t other = 0);
  uint32_t getOwnerId() const;
  uint32_t getOtherId() const;
  uint32_t getLength() const;

 private:
  int _ll[2];
  int _ur[2];
  uint32_t _level;
  uint32_t _dir;
  uint32_t _ownId;
  uint32_t _otherId;
  uint32_t _type;
};

class Wire
{
 public:
  int getLen() { return _len; }
  int getWidth() { return _width; }
  int getBase() { return _base; }

  uint32_t getLevel();
  uint32_t getPitch();

  int getShapeProperty(int id);
  int getRsegId();

  void reset();
  void set(uint32_t dir, const int* ll, const int* ur);
  void search(int xy1, int xy2, uint32_t& cnt, Array1D<uint32_t>* idTable);
  void search1(int xy1, int xy2, uint32_t& cnt, Array1D<uint32_t>* idTable);

  void setNext(Wire* w) { _next = w; };
  Wire* getNext() const { return _next; };
  uint32_t getFlags() const { return _flags; };
  uint32_t getBoxId();
  void setExt(uint32_t ext) { _ext = ext; };
  uint32_t getExt() { return _ext; };
  void setOtherId(uint32_t id);
  uint32_t getOtherId();
  bool isPower();
  bool isVia();
  bool isTileBus();
  uint32_t getOwnerId();
  uint32_t getSrcId();
  void getCoords(SearchBox* box);
  int getXY() { return _xy; }
  void getCoords(int* x1, int* y1, int* x2, int* y2, uint32_t* dir);

  // Extraction
  int wireOverlap(Wire* w, int* len1, int* len2, int* len3);
  Wire* getPoolWire(AthPool<Wire>* wirePool);
  Wire* makeWire(AthPool<Wire>* wirePool, int xy1, uint32_t len);
  Wire* makeCoupleWire(AthPool<Wire>* wirePool,
                       int targetHighTracks,
                       Wire* w2,
                       int xy1,
                       uint32_t len,
                       uint32_t wtype);
  void setXY(int xy1, uint32_t len);
  odb::dbNet* getNet();
  Wire* getUpNext() const { return _upNext; }
  Wire* getDownNext() const { return _downNext; }
  Wire* getAboveNext() const { return _aboveNext; }
  Wire* getBelowNext() const { return _belowNext; }
  void setUpNext(Wire* wire) { _upNext = wire; }
  void setDownNext(Wire* wire) { _downNext = wire; }
  void setAboveNext(Wire* wire) { _aboveNext = wire; }
  void setBelowNext(Wire* wire) { _belowNext = wire; }

 private:
  Wire* _upNext = nullptr;
  Wire* _downNext = nullptr;
  Wire* _aboveNext = nullptr;  // vertical
  Wire* _belowNext = nullptr;  // vertical

  uint32_t _id;
  uint32_t _srcId;  // TODO-OPTIMIZE
  uint32_t _boxId;
  uint32_t _otherId;
  Wire* _srcWire;

  Track* _track;
  Wire* _next;

  int _xy;  // TODO offset from track start in large dimension
  int _len;
  int _ouLen;

  int _base;
  int _width : 24;

  uint32_t _flags : 6;
  // 0=wire, 2=obs, 1=pin, 3=power or SET BY USER

  uint32_t _dir : 1;
  uint32_t _ext : 1;
  uint32_t _visited : 1;

  friend class Track;
  friend class Grid;
  friend class GridTable;
};

class Track
{
 public:
  uint32_t getTrackNum() { return _num; };
  void set(Grid* g,
           int x,
           int y,
           uint32_t n,
           uint32_t width,
           uint32_t markerLen,
           uint32_t markerCnt,
           int base);
  void freeWires(AthPool<Wire>* pool);
  bool place(Wire* w, int markIndex1, int markIndex2);
  bool place(Wire* w, int markIndex1);
  uint32_t setExtrusionMarker(int markerCnt, int start, uint32_t markerLen);
  bool placeTrail(Wire* w, uint32_t m1, uint32_t m2);

  bool overlapCheck(Wire* w, int markIndex1, int markIndex2);
  bool isAscendingOrdered(uint32_t markerCnt, uint32_t* wCnt);
  Grid* getGrid();
  Wire* getWire_Linear(uint32_t markerCnt, uint32_t id);
  Wire* getNextWire(Wire* wire);
  uint32_t search(int xy1,
                  int xy2,
                  uint32_t markIndex1,
                  uint32_t markIndex2,
                  Array1D<uint32_t>* idtable);
  uint32_t search1(int xy1,
                   int xy2,
                   uint32_t markIndex1,
                   uint32_t markIndex2,
                   Array1D<uint32_t>* idTable);

  bool checkAndplace(Wire* w, int markIndex1);
  bool checkMarker(int markIndex);
  bool checkAndplacerOnMarker(Wire* w, int markIndex);
  uint32_t getAllWires(Array1D<Wire*>* boxTable, uint32_t markerCnt);
  void resetExtFlag(uint32_t markerCnt);
  void linkWire(Wire*& w1, Wire*& w2);

  Track* getNextSubTrack(Track* subt, bool tohi);
  int getBase() { return _base; };
  void setHiTrack(Track* hitrack);
  void setLowTrack(Track* lowtrack);
  Track* getHiTrack();
  Track* getLowTrack();
  Track* nextTrackInRange(uint32_t& delt,
                          uint32_t trackDist,
                          uint32_t srcTrack,
                          bool tohi);
  int nextSubTrackInRange(Track*& tstrack,
                          uint32_t& delt,
                          uint32_t trackDist,
                          uint32_t srcTrack,
                          bool tohi);
  void setLowest(uint32_t lst) { _lowest = lst; };

  uint32_t removeMarkedNetWires();

  // EXTRACTION

  bool place2(Wire* w, int mark1, int mark2);
  void insertWire(Wire* w, int mark1, int mark2);
  uint32_t initTargetTracks(uint32_t srcTrack, uint32_t trackDist, bool tohi);
  void findNeighborWire(Wire*, Array1D<Wire*>*, bool);
  void getTrackWires(std::vector<Wire*>& ctxwire);
  void buildDgContext(Array1D<SEQ*>* dgContext, std::vector<Wire*>& allWire);
  int getBandWires(Array1D<Wire*>* bandWire);
  uint32_t couplingCaps(Grid* ccGrid,
                        uint32_t srcTrack,
                        uint32_t trackDist,
                        uint32_t ccThreshold,
                        Array1D<uint32_t>* ccIdTable,
                        uint32_t met,
                        CoupleAndCompute coupleAndCompute,
                        void* compPtr,
                        bool ttttGetDgOverlap);

  uint32_t findOverlap(Wire* origWire,
                       uint32_t ccThreshold,
                       Array1D<Wire*>* wTable,
                       Array1D<Wire*>* nwTable,
                       Grid* ccGrid,
                       Array1D<Wire*>* ccTable,
                       uint32_t met,
                       CoupleAndCompute coupleAndCompute,
                       void* compPtr);

  void initTargetWire(int noPowerWire);
  Wire* nextTargetWire(int noPowerWire);
  Wire* getTargetWire();
  void adjustOverlapMakerEnd(uint32_t markerCnt);
  void adjustOverlapMakerEnd(uint32_t markerCnt, int start, uint32_t markerLen);
  uint32_t trackContextOn(int orig,
                          int end,
                          int base,
                          int width,
                          uint32_t firstContextTrack,
                          Array1D<int>* context);

  void dealloc(AthPool<Wire>* pool);
  Wire* getMarker(int index) const { return _marker[index]; }
  void setMarker(int index, Wire* wire) { _marker[index] = wire; }

 private:
  int _x;  // you need only one
  int _y;

  int _base;
  Track* _hiTrack;
  Track* _lowTrack;

  Wire** _eMarker;
  uint32_t _markerCnt;
  uint32_t _searchMarkerIndex;

  uint32_t _targetMarker;
  Wire* _targetWire;

  Grid* _grid;

  uint32_t _num : 20;

  int _width : 19;
  uint32_t _lowest : 1;
  uint32_t _shift : 4;
  uint32_t _centered : 1;
  uint32_t _blocked : 1;
  uint32_t _full : 1;
  bool _ordered;

  // -------------------------------------------------------- v2
  Wire** _marker;
  // --------------------------------------------------------

  friend class GridTable;
  friend class Grid;
  friend class Wire;
};

class Grid
{
 public:
  int initCouplingCapLoops_v2(uint32_t couplingDist,
                              bool startSearchTrack = true,
                              int startXY = 0);
  uint32_t placeWire_v2(SearchBox* bb);

  Grid(GridTable* gt,
       AthPool<Track>* trackPool,
       AthPool<Wire>* wirePool,
       uint32_t level,
       uint32_t markerCnt);
  ~Grid();

  GridTable* getGridTable() { return _gridtable; };
  void setBoundaries(uint32_t dir, const odb::Rect& rect);
  void setTracks(uint32_t dir,
                 uint32_t width,
                 uint32_t pitch,
                 int xlo,
                 int ylo,
                 int xhi,
                 int yhi,
                 uint32_t markerLen = 0);
  void setPlaced();
  bool isPlaced();

  bool anyTrackAvailable();

  uint32_t getTrackCnt() { return _trackCnt; };
  Track* getTrackPtr(uint32_t n) { return _trackTable[n]; };
  uint32_t getTrackNum1(int xy);
  uint32_t getWidth();
  int getXYbyWidth(int xy, uint32_t* mark);
  Track* addTrack(uint32_t ii, uint32_t markerCnt, int base);
  Track* addTrack(uint32_t ii, uint32_t markerCnt);
  void makeTracks(uint32_t space, uint32_t width);
  void getBbox(Box* bb);
  void getBbox(SearchBox* bb);
  uint32_t setExtrusionMarker();
  uint32_t addWire(Box* box, int check);
  uint32_t addWire(Box* box);

  uint32_t placeWire(SearchBox* bb);
  uint32_t placeBox(uint32_t id, int x1, int y1, int x2, int y2);
  uint32_t placeBox(odb::dbBox* box, uint32_t wtype, uint32_t id);
  uint32_t placeBox(Box* box);
  uint32_t placeBox(SearchBox* bb);
  uint32_t getBucketNum(int xy);
  uint32_t getTrackNum(int* ll, uint32_t d, uint32_t* marker);
  Wire* getWirePtr(uint32_t wireId);
  void getBoxIds(Array1D<uint32_t>* wireIdTable, Array1D<uint32_t>* idtable);
  void getWireIds(Array1D<uint32_t>* wireIdTable, Array1D<uint32_t>* idtable);

  int findEmptyTrack(int ll[2], int ur[2]);
  uint32_t getFirstTrack(uint32_t divider);
  int getClosestTrackCoord(int xy);
  uint32_t addWire(uint32_t initTrack, Box* box, int sortedOrder, int* height);
  Wire* getPoolWire();
  Wire* makeWire(Box* box,
                 uint32_t* id,
                 uint32_t* m1,
                 uint32_t* m2,
                 uint32_t fullTrack);
  Wire* makeWire(Box* box, uint32_t id, uint32_t* m1);
  Wire* makeWire(int* ll, int* ur, uint32_t id, uint32_t* m1);
  Wire* makeWire(uint32_t dir,
                 int* ll,
                 int* ur,
                 uint32_t id1,
                 uint32_t id2,
                 uint32_t type = 0);

  Wire* makeWire(Wire* w, uint32_t type = 0);

  void makeTrackTable(uint32_t width, uint32_t pitch, uint32_t space = 0);
  float updateFreeTracks(float v);

  void freeTracksAndTables();
  int getAbsTrackNum(int xy);
  int getMinMaxTrackNum(int xy);
  bool addOnTrack(uint32_t track, Wire* w, uint32_t mark1, uint32_t mark2);
  int getTrackHeight(uint32_t track);
  uint32_t getTrackNum(Box* box);
  Track* getTrackPtr(int* ll);
  Track* getTrackPtr(int xy);
  Track* getTrackPtr(uint32_t ii, uint32_t markerCnt, int base);
  Track* getTrackPtr(uint32_t ii, uint32_t markerCnt);
  bool isOrdered(bool ascending, uint32_t* cnt);
  uint32_t search(SearchBox* bb,
                  Array1D<uint32_t>* idtable,
                  bool wireIdFlag = false);

  uint32_t placeWire(uint32_t initTrack,
                     Wire* w,
                     uint32_t mark1,
                     uint32_t mark2,
                     int sortedOrder,
                     int* height);

  void getBoxes(Array1D<uint32_t>* table);
  uint32_t getBoxes(uint32_t ii, Array1D<uint32_t>* table);

  uint32_t getDir();
  uint32_t getLevel();
  Wire* getWire_Linear(uint32_t id);

  void getBuses(Array1D<Box*>* boxtable, uint32_t width);

  uint32_t removeMarkedNetWires();
  void setSearchDomain(int domainAdjust);
  uint32_t searchLowMarker() { return _searchLowMarker; };
  uint32_t searchHiMarker() { return _searchHiMarker; };

  // EXTRACTION
  void buildDgContext(int gridn, int base);
  int getBandWires(int hiXY,
                   uint32_t couplingDist,
                   uint32_t& wireCnt,
                   Array1D<Wire*>* bandWire,
                   int* limitArray);
  AthPool<Wire>* getWirePoolPtr();
  uint32_t placeWire(Wire* w);
  uint32_t defaultWireType();
  void setDefaultWireType(uint32_t v);
  uint32_t search(SearchBox* bb,
                  const uint32_t* gxy,
                  Array1D<uint32_t>* idtable,
                  Grid* g);
  void adjustOverlapMakerEnd();
  void initContextGrids();
  void initContextTracks();
  void contextsOn(int orig, int len, int base, int width);
  void gridContextOn(int orig, int len, int base, int width);

  int initCouplingCapLoops(uint32_t couplingDist,
                           CoupleAndCompute coupleAndCompute,
                           void* compPtr,
                           bool startSearchTrack = true,
                           int startXY = 0);
  int couplingCaps(int hiXY,
                   uint32_t couplingDist,
                   uint32_t& wireCnt,
                   CoupleAndCompute coupleAndCompute,
                   void* compPtr,
                   int* limitArray,
                   bool ttttGetDgOverlap);
  int dealloc(int hiXY);
  void dealloc();
  int getPitch() const { return _pitch; }

 private:
  GridTable* _gridtable = nullptr;
  Track** _trackTable = nullptr;
  uint32_t* _blockedTrackTable = nullptr;
  int _trackCnt;
  uint32_t* _subTrackCnt = nullptr;
  int _base;
  int _max;

  int _start;  // laterally
  int _end;

  int _lo[2];
  int _hi[2];

  int _width;
  int _pitch;
  uint32_t _level;
  uint32_t _dir;
  int _markerLen;
  uint32_t _markerCnt;
  uint32_t _searchLowTrack;
  uint32_t _searchHiTrack;
  uint32_t _searchLowMarker;
  uint32_t _searchHiMarker;

  AthPool<Track>* _trackPoolPtr;
  AthPool<Wire>* _wirePoolPtr;

  uint32_t _wireType;

  uint32_t _currentTrack;
  uint32_t _lastFreeTrack;

  friend class GridTable;
};

class GridTable
{
  // -------------------------------------------------------------- v2
 public:
  void initCouplingCapLoops_v2(uint32_t dir,
                               uint32_t couplingDist,
                               int* startXY = nullptr);
  int initCouplingCapLoops_v2(uint32_t couplingDist,
                              bool startSearchTrack,
                              int startXY);

  void setExtControl_v2(odb::dbBlock* block,
                        bool useDbSdb,
                        uint32_t adj,
                        uint32_t npsrc,
                        uint32_t nptgt,
                        uint32_t ccUp,
                        bool allNet,
                        uint32_t contextDepth,
                        Array1D<int>** contextArray,
                        uint32_t* contextLength,
                        Array1D<SEQ*>*** dgContextArray,
                        uint32_t* dgContextDepth,
                        uint32_t* dgContextPlanes,
                        uint32_t* dgContextTracks,
                        uint32_t* dgContextBaseLvl,
                        int* dgContextLowLvl,
                        int* dgContextHiLvl,
                        uint32_t* dgContextBaseTrack,
                        int* dgContextLowTrack,
                        int* dgContextHiTrack,
                        int** dgContextTrackBase,
                        AthPool<SEQ>* seqPool);

  // -------------------------------------------------------------
  GridTable(odb::Rect* bb,
            uint32_t rowCnt,
            uint32_t colCnt,
            uint32_t* pitch,
            const int* X1 = nullptr,
            const int* Y1 = nullptr);
  ~GridTable();
  Grid* getGrid(uint32_t row, uint32_t col);
  uint32_t getColCnt();
  uint32_t getRowCnt();
  Wire* getWirePtr(uint32_t id);
  void releaseWire(uint32_t wireId);
  Box* maxSearchBox() { return &_maxSearchBox; };
  int xMin();
  int xMax();
  int yMin();
  int yMax();
  uint32_t getRowNum(int x);
  uint32_t getColNum(int y);
  bool getRowCol(int x1, int y1, uint32_t* row, uint32_t* col);
  Wire* addBox(Box* bb);
  Wire* addBox(odb::dbBox* bb, uint32_t wtype, uint32_t id);
  bool addBox(uint32_t row, uint32_t col, odb::dbBox* bb);

  uint32_t getBoxes(Box* bb, Array1D<Box*>* table);
  uint32_t search(SearchBox* bb,
                  uint32_t row,
                  uint32_t col,
                  Array1D<uint32_t>* idTable,
                  bool wireIdFlag);
  uint32_t search(SearchBox* bb, Array1D<uint32_t>* idTable);
  uint32_t search(Box* bb);

  uint32_t addBox(int x1,
                  int y1,
                  int x2,
                  int y2,
                  uint32_t level,
                  uint32_t id1,
                  uint32_t id2,
                  uint32_t wireType);
  uint32_t search(int x1,
                  int y1,
                  int x2,
                  int y2,
                  uint32_t row,
                  uint32_t col,
                  Array1D<uint32_t>* idTable,
                  bool wireIdFlag);
  void getCoords(SearchBox* bb, uint32_t wireId);
  void setMaxArea(int x1, int y1, int x2, int y2);
  void resetMaxArea();

  // EXTRACTION

  void setDefaultWireType(uint32_t v);
  void buildDgContext(int base, uint32_t level, uint32_t dir);
  Array1D<SEQ*>* renewDgContext(uint32_t gridn, uint32_t trackn);
  void getBox(uint32_t wid,
              int* x1,
              int* y1,
              int* x2,
              int* y2,
              uint32_t* level,
              uint32_t* id1,
              uint32_t* id2,
              uint32_t* wireType);
  uint32_t search(SearchBox* bb,
                  uint32_t* gxy,
                  uint32_t row,
                  uint32_t col,
                  Array1D<uint32_t>* idtable,
                  Grid* g);
  uint32_t getOverlapAdjust() { return _overlapAdjust; };
  uint32_t getOverlapTouchCheck() { return _overlapTouchCheck; };
  uint32_t targetHighTracks() { return _ccTargetHighTracks; };
  uint32_t targetHighMarkedNet() { return _ccTargetHighMarkedNet; };
  void setCCFlag(uint32_t ccflag) { _ccFlag = ccflag; };
  uint32_t getCcFlag() { return _ccFlag; };
  uint32_t contextDepth() { return _ccContextDepth; };
  Array1D<int>** contextArray() { return _ccContextArray; };
  AthPool<SEQ>* seqPool() { return _seqPool; };
  Array1D<SEQ*>*** dgContextArray() { return _dgContextArray; };
  int** dgContextTrackBase() { return _dgContextTrackBase; };
  uint32_t* dgContextBaseTrack() { return _dgContextBaseTrack; };
  int* dgContextLowTrack() { return _dgContextLowTrack; };
  int* dgContextHiTrack() { return _dgContextHiTrack; };
  bool allNet() { return _allNet; };
  void setAllNet(bool allnet) { _allNet = allnet; };
  bool handleEmptyOnly() { return _handleEmptyOnly; };
  void setHandleEmptyOnly(bool handleEmptyOnly)
  {
    _handleEmptyOnly = handleEmptyOnly;
  };
  uint32_t noPowerSource() { return _noPowerSource; };
  void setNoPowerSource(uint32_t nps) { _noPowerSource = nps; };
  uint32_t noPowerTarget() { return _noPowerTarget; };
  void setNoPowerTarget(uint32_t npt) { _noPowerTarget = npt; };
  void incrCCshorts() { _ccShorts++; };

  void setExtControl(odb::dbBlock* block,
                     bool useDbSdb,
                     uint32_t adj,
                     uint32_t npsrc,
                     uint32_t nptgt,
                     uint32_t ccUp,
                     bool allNet,
                     uint32_t contextDepth,
                     Array1D<int>** contextArray,
                     Array1D<SEQ*>*** dgContextArray,
                     uint32_t* dgContextDepth,
                     uint32_t* dgContextPlanes,
                     uint32_t* dgContextTracks,
                     uint32_t* dgContextBaseLvl,
                     int* dgContextLowLvl,
                     int* dgContextHiLvl,
                     uint32_t* dgContextBaseTrack,
                     int* dgContextLowTrack,
                     int* dgContextHiTrack,
                     int** dgContextTrackBase,
                     AthPool<SEQ>* seqPool);

  bool usingDbSdb() { return _useDbSdb; }
  void reverseTargetTrack();
  bool targetTrackReversed() { return _targetTrackReversed; };
  void incrMultiTrackWireCnt(bool isPower);
  void adjustOverlapMakerEnd();
  void dumpTrackCounts(FILE* fp);
  odb::dbBlock* getBlock() { return _block; };
  void setBlock(odb::dbBlock* block) { _block = block; };

  int couplingCaps(int hiXY,
                   uint32_t couplingDist,
                   uint32_t dir,
                   uint32_t& wireCnt,
                   CoupleAndCompute coupleAndCompute,
                   void* compPtr,
                   bool getBandWire,
                   int** limitArray);
  void initCouplingCapLoops(uint32_t dir,
                            uint32_t couplingDist,
                            CoupleAndCompute coupleAndCompute,
                            void* compPtr,
                            int* startXY = nullptr);
  int dealloc(uint32_t dir, int hiXY);
  void dealloc();

  uint32_t getWireCnt();
  void setV2(bool v2)
  {
    _no_sub_tracks = v2;
    _v2 = v2;
  }

 private:
  uint32_t setExtrusionMarker(uint32_t startRow, uint32_t startCol);
  Wire* getWire_Linear(uint32_t instId);
  bool isOrdered(bool ascending);
  void removeMarkedNetWires();

  bool _no_sub_tracks = false;
  bool _v2 = false;
  // indexed [row][col].  Rows = dirs (2); cols = layers+1 (1-indexing)
  Grid*** _gridTable;
  Box _bbox;
  Box _maxSearchBox;
  bool _setMaxArea;
  odb::Rect _rectBB;
  uint32_t _rowCnt;
  uint32_t _colCnt;
  uint32_t _rowSize;
  uint32_t _colSize;
  AthPool<Track>* _trackPool;
  AthPool<Wire>* _wirePool;
  uint32_t _overlapAdjust{Z_noAdjust};
  uint32_t _powerMultiTrackWire{0};
  uint32_t _signalMultiTrackWire{0};
  uint32_t _overlapTouchCheck{1};
  uint32_t _noPowerSource{0};
  uint32_t _noPowerTarget{0};
  uint32_t _ccShorts{0};
  uint32_t _ccTargetHighTracks{1};
  uint32_t _ccTargetHighMarkedNet;
  bool _targetTrackReversed{false};
  bool _allNet{true};
  bool _handleEmptyOnly;
  bool _useDbSdb{true};
  uint32_t _ccFlag;

  uint32_t _ccContextDepth{0};

  // _v2
  uint32_t* _ccContextLength;

  Array1D<int>** _ccContextArray{nullptr};

  AthPool<SEQ>* _seqPool;
  Array1D<SEQ*>*** _dgContextArray;  // array

  uint32_t* _dgContextDepth;      // not array
  uint32_t* _dgContextPlanes;     // not array
  uint32_t* _dgContextTracks;     // not array
  uint32_t* _dgContextBaseLvl;    // not array
  int* _dgContextLowLvl;          // not array
  int* _dgContextHiLvl;           // not array
  uint32_t* _dgContextBaseTrack;  // array
  int* _dgContextLowTrack;        // array
  int* _dgContextHiTrack;         // array
  int** _dgContextTrackBase;      // array

  odb::dbBlock* _block;

  uint32_t _wireCnt;

  Array1D<Wire*>* _bandWire{nullptr};

  bool _ttttGetDgOverlap{false};
};

}  // namespace rcx
