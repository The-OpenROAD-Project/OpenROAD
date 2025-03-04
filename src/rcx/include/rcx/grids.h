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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "box.h"
#include "odb/array1.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "rcx.h"
#include "rcx/extRCap.h"

namespace rcx {

using odb::Ath__array1D;
using odb::AthPool;
using odb::dbBlock;
using odb::dbBox;
using odb::dbNet;
using odb::Rect;

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
  SearchBox(int x1, int y1, int x2, int y2, uint l, int dir = -1);
  void set(int x1, int y1, int x2, int y2, uint l, int dir);
  int loXY(uint d) const;
  int loXY(uint d, int loBound) const;
  int hiXY(uint d) const;
  int hiXY(uint d, int hiBound) const;
  void setLo(uint d, int xy);
  void setHi(uint d, int xy);
  void setType(uint v);
  uint getType() const;

  uint getDir() const;
  uint getLevel() const;
  void setDir(int dir = -1);
  void setLevel(uint v);
  void setOwnerId(uint v, uint other = 0);
  uint getOwnerId() const;
  uint getOtherId() const;
  uint getLength() const;

 private:
  int _ll[2];
  int _ur[2];
  uint _level;
  uint _dir;
  uint _ownId;
  uint _otherId;
  uint _type;
};

class Wire
{
 public:
  int getLen() { return _len; }
  int getWidth() { return _width; }
  int getBase() { return _base; }

  uint getLevel();
  uint getPitch();

  int getShapeProperty(int id);
  int getRsegId();

  void reset();
  void set(uint dir, const int* ll, const int* ur);
  void search(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable);
  void search1(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable);

  void setNext(Wire* w) { _next = w; };
  Wire* getNext() const { return _next; };
  uint getFlags() const { return _flags; };
  uint getBoxId();
  void setExt(uint ext) { _ext = ext; };
  uint getExt() { return _ext; };
  void setOtherId(uint id);
  uint getOtherId();
  bool isPower();
  bool isVia();
  bool isTileBus();
  uint getOwnerId();
  uint getSrcId();
  void getCoords(SearchBox* box);
  int getXY() { return _xy; }
  void getCoords(int* x1, int* y1, int* x2, int* y2, uint* dir);

  // Extraction
  int wireOverlap(Wire* w, int* len1, int* len2, int* len3);
  Wire* getPoolWire(AthPool<Wire>* wirePool);
  Wire* makeWire(AthPool<Wire>* wirePool, int xy1, uint len);
  Wire* makeCoupleWire(AthPool<Wire>* wirePool,
                       int targetHighTracks,
                       Wire* w2,
                       int xy1,
                       uint len,
                       uint wtype);
  void setXY(int xy1, uint len);
  dbNet* getNet();
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

  uint _id;
  uint _srcId;  // TODO-OPTIMIZE
  uint _boxId;
  uint _otherId;
  Wire* _srcWire;

  Track* _track;
  Wire* _next;

  int _xy;  // TODO offset from track start in large dimension
  int _len;
  int _ouLen;

  int _base;
  int _width : 24;

  uint _flags : 6;
  // 0=wire, 2=obs, 1=pin, 3=power or SET BY USER

  uint _dir : 1;
  uint _ext : 1;
  uint _visited : 1;

  friend class Track;
  friend class Grid;
  friend class GridTable;
};

class Track
{
 public:
  uint getTrackNum() { return _num; };
  void set(Grid* g,
           int x,
           int y,
           uint n,
           uint width,
           uint markerLen,
           uint markerCnt,
           int base);
  void freeWires(AthPool<Wire>* pool);
  bool place(Wire* w, int markIndex1, int markIndex2);
  bool place(Wire* w, int markIndex1);
  uint setExtrusionMarker(int markerCnt, int start, uint markerLen);
  bool placeTrail(Wire* w, uint m1, uint m2);

  bool overlapCheck(Wire* w, int markIndex1, int markIndex2);
  bool isAscendingOrdered(uint markerCnt, uint* wCnt);
  Grid* getGrid();
  Wire* getWire_Linear(uint markerCnt, uint id);
  Wire* getNextWire(Wire* wire);
  uint search(int xy1,
              int xy2,
              uint markIndex1,
              uint markIndex2,
              Ath__array1D<uint>* idtable);
  uint search1(int xy1,
               int xy2,
               uint markIndex1,
               uint markIndex2,
               Ath__array1D<uint>* idTable);

  bool checkAndplace(Wire* w, int markIndex1);
  bool checkMarker(int markIndex);
  bool checkAndplacerOnMarker(Wire* w, int markIndex);
  uint getAllWires(Ath__array1D<Wire*>* boxTable, uint markerCnt);
  void resetExtFlag(uint markerCnt);
  void linkWire(Wire*& w1, Wire*& w2);

  Track* getNextSubTrack(Track* subt, bool tohi);
  int getBase() { return _base; };
  void setHiTrack(Track* hitrack);
  void setLowTrack(Track* lowtrack);
  Track* getHiTrack();
  Track* getLowTrack();
  Track* nextTrackInRange(uint& delt, uint trackDist, uint srcTrack, bool tohi);
  int nextSubTrackInRange(Track*& tstrack,
                          uint& delt,
                          uint trackDist,
                          uint srcTrack,
                          bool tohi);
  void setLowest(uint lst) { _lowest = lst; };

  uint removeMarkedNetWires();

  // EXTRACTION

  bool place2(Wire* w, int mark1, int mark2);
  void insertWire(Wire* w, int mark1, int mark2);
  uint initTargetTracks(uint srcTrack, uint trackDist, bool tohi);
  void findNeighborWire(Wire*, Ath__array1D<Wire*>*, bool);
  void getTrackWires(std::vector<Wire*>& ctxwire);
  void buildDgContext(Ath__array1D<SEQ*>* dgContext,
                      std::vector<Wire*>& allWire);
  int getBandWires(Ath__array1D<Wire*>* bandWire);
  uint couplingCaps(Grid* ccGrid,
                    uint srcTrack,
                    uint trackDist,
                    uint ccThreshold,
                    Ath__array1D<uint>* ccIdTable,
                    uint met,
                    CoupleAndCompute coupleAndCompute,
                    void* compPtr,
                    bool ttttGetDgOverlap);

  uint findOverlap(Wire* origWire,
                   uint ccThreshold,
                   Ath__array1D<Wire*>* wTable,
                   Ath__array1D<Wire*>* nwTable,
                   Grid* ccGrid,
                   Ath__array1D<Wire*>* ccTable,
                   uint met,
                   CoupleAndCompute coupleAndCompute,
                   void* compPtr);

  void initTargetWire(int noPowerWire);
  Wire* nextTargetWire(int noPowerWire);
  Wire* getTargetWire();
  void adjustOverlapMakerEnd(uint markerCnt);
  void adjustOverlapMakerEnd(uint markerCnt, int start, uint markerLen);
  uint trackContextOn(int orig,
                      int end,
                      int base,
                      int width,
                      uint firstContextTrack,
                      Ath__array1D<int>* context);

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
  uint _markerCnt;
  uint _searchMarkerIndex;

  uint _targetMarker;
  Wire* _targetWire;

  Grid* _grid;

  uint _num : 20;

  int _width : 19;
  uint _lowest : 1;
  uint _shift : 4;
  uint _centered : 1;
  uint _blocked : 1;
  uint _full : 1;
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
  int initCouplingCapLoops_v2(uint couplingDist,
                              bool startSearchTrack = true,
                              int startXY = 0);
  uint placeWire_v2(SearchBox* bb);

  Grid(GridTable* gt,
       AthPool<Track>* trackPool,
       AthPool<Wire>* wirePool,
       uint level,
       uint markerCnt);
  ~Grid();

  GridTable* getGridTable() { return _gridtable; };
  void setBoundaries(uint dir, const odb::Rect& rect);
  void setTracks(uint dir,
                 uint width,
                 uint pitch,
                 int xlo,
                 int ylo,
                 int xhi,
                 int yhi,
                 uint markerLen = 0);
  void setPlaced();
  bool isPlaced();

  bool anyTrackAvailable();

  uint getTrackCnt() { return _trackCnt; };
  Track* getTrackPtr(uint n) { return _trackTable[n]; };
  uint getTrackNum1(int xy);
  uint getWidth();
  int getXYbyWidth(int xy, uint* mark);
  Track* addTrack(uint ii, uint markerCnt, int base);
  Track* addTrack(uint ii, uint markerCnt);
  void makeTracks(uint space, uint width);
  void getBbox(Box* bb);
  void getBbox(SearchBox* bb);
  uint setExtrusionMarker();
  uint addWire(Box* box, int check);
  uint addWire(Box* box);

  uint placeWire(SearchBox* bb);
  uint placeBox(uint id, int x1, int y1, int x2, int y2);
  uint placeBox(dbBox* box, uint wtype, uint id);
  uint placeBox(Box* box);
  uint placeBox(SearchBox* bb);
  uint getBucketNum(int xy);
  uint getTrackNum(int* ll, uint d, uint* marker);
  Wire* getWirePtr(uint wireId);
  void getBoxIds(Ath__array1D<uint>* wireIdTable, Ath__array1D<uint>* idtable);
  void getWireIds(Ath__array1D<uint>* wireIdTable, Ath__array1D<uint>* idtable);

  int findEmptyTrack(int ll[2], int ur[2]);
  uint getFirstTrack(uint divider);
  int getClosestTrackCoord(int xy);
  uint addWire(uint initTrack, Box* box, int sortedOrder, int* height);
  Wire* getPoolWire();
  Wire* makeWire(Box* box, uint* id, uint* m1, uint* m2, uint fullTrack);
  Wire* makeWire(Box* box, uint id, uint* m1);
  Wire* makeWire(int* ll, int* ur, uint id, uint* m1);
  Wire* makeWire(uint dir, int* ll, int* ur, uint id1, uint id2, uint type = 0);

  Wire* makeWire(Wire* w, uint type = 0);

  void makeTrackTable(uint width, uint pitch, uint space = 0);
  float updateFreeTracks(float v);

  void freeTracksAndTables();
  int getAbsTrackNum(int xy);
  int getMinMaxTrackNum(int xy);
  bool addOnTrack(uint track, Wire* w, uint mark1, uint mark2);
  int getTrackHeight(uint track);
  uint getTrackNum(Box* box);
  Track* getTrackPtr(int* ll);
  Track* getTrackPtr(int xy);
  Track* getTrackPtr(uint ii, uint markerCnt, int base);
  Track* getTrackPtr(uint ii, uint markerCnt);
  bool isOrdered(bool ascending, uint* cnt);
  uint search(SearchBox* bb,
              Ath__array1D<uint>* idtable,
              bool wireIdFlag = false);

  uint placeWire(uint initTrack,
                 Wire* w,
                 uint mark1,
                 uint mark2,
                 int sortedOrder,
                 int* height);

  void getBoxes(Ath__array1D<uint>* table);
  uint getBoxes(uint ii, Ath__array1D<uint>* table);

  uint getDir();
  uint getLevel();
  Wire* getWire_Linear(uint id);

  void getBuses(Ath__array1D<Box*>* boxtable, uint width);

  uint removeMarkedNetWires();
  void setSearchDomain(int domainAdjust);
  uint searchLowMarker() { return _searchLowMarker; };
  uint searchHiMarker() { return _searchHiMarker; };

  // EXTRACTION
  void buildDgContext(int gridn, int base);
  int getBandWires(int hiXY,
                   uint couplingDist,
                   uint& wireCnt,
                   Ath__array1D<Wire*>* bandWire,
                   int* limitArray);
  AthPool<Wire>* getWirePoolPtr();
  uint placeWire(Wire* w);
  uint defaultWireType();
  void setDefaultWireType(uint v);
  uint search(SearchBox* bb,
              const uint* gxy,
              Ath__array1D<uint>* idtable,
              Grid* g);
  void adjustOverlapMakerEnd();
  void initContextGrids();
  void initContextTracks();
  void contextsOn(int orig, int len, int base, int width);
  void gridContextOn(int orig, int len, int base, int width);

  int initCouplingCapLoops(uint couplingDist,
                           CoupleAndCompute coupleAndCompute,
                           void* compPtr,
                           bool startSearchTrack = true,
                           int startXY = 0);
  int couplingCaps(int hiXY,
                   uint couplingDist,
                   uint& wireCnt,
                   CoupleAndCompute coupleAndCompute,
                   void* compPtr,
                   int* limitArray,
                   bool ttttGetDgOverlap);
  int dealloc(int hiXY);
  void dealloc();
  int getPitch() const { return _pitch; }

 private:
  GridTable* _gridtable;
  Track** _trackTable;
  uint* _blockedTrackTable;
  int _trackCnt;
  uint* _subTrackCnt;
  int _base;
  int _max;

  int _start;  // laterally
  int _end;

  int _lo[2];
  int _hi[2];

  int _width;
  int _pitch;
  uint _level;
  uint _dir;
  int _markerLen;
  uint _markerCnt;
  uint _searchLowTrack;
  uint _searchHiTrack;
  uint _searchLowMarker;
  uint _searchHiMarker;

  AthPool<Track>* _trackPoolPtr;
  AthPool<Wire>* _wirePoolPtr;

  uint _wireType;

  uint _currentTrack;
  uint _lastFreeTrack;

  friend class GridTable;
};

class GridTable
{
  // -------------------------------------------------------------- v2
 public:
  void initCouplingCapLoops_v2(uint dir,
                               uint couplingDist,
                               int* startXY = nullptr);
  int initCouplingCapLoops_v2(uint couplingDist,
                              bool startSearchTrack,
                              int startXY);

  void setExtControl_v2(dbBlock* block,
                        bool useDbSdb,
                        uint adj,
                        uint npsrc,
                        uint nptgt,
                        uint ccUp,
                        bool allNet,
                        uint contextDepth,
                        Ath__array1D<int>** contextArray,
                        uint* contextLength,
                        Ath__array1D<SEQ*>*** dgContextArray,
                        uint* dgContextDepth,
                        uint* dgContextPlanes,
                        uint* dgContextTracks,
                        uint* dgContextBaseLvl,
                        int* dgContextLowLvl,
                        int* dgContextHiLvl,
                        uint* dgContextBaseTrack,
                        int* dgContextLowTrack,
                        int* dgContextHiTrack,
                        int** dgContextTrackBase,
                        AthPool<SEQ>* seqPool);

  // -------------------------------------------------------------
  GridTable(Rect* bb,
            uint rowCnt,
            uint colCnt,
            uint* pitch,
            const int* X1 = nullptr,
            const int* Y1 = nullptr);
  ~GridTable();
  Grid* getGrid(uint row, uint col);
  uint getColCnt();
  uint getRowCnt();
  Wire* getWirePtr(uint id);
  void releaseWire(uint wireId);
  Box* maxSearchBox() { return &_maxSearchBox; };
  int xMin();
  int xMax();
  int yMin();
  int yMax();
  uint getRowNum(int x);
  uint getColNum(int y);
  bool getRowCol(int x1, int y1, uint* row, uint* col);
  Wire* addBox(Box* bb);
  Wire* addBox(dbBox* bb, uint wtype, uint id);
  bool addBox(uint row, uint col, dbBox* bb);

  uint getBoxes(Box* bb, Ath__array1D<Box*>* table);
  uint search(SearchBox* bb,
              uint row,
              uint col,
              Ath__array1D<uint>* idTable,
              bool wireIdFlag);
  uint search(SearchBox* bb, Ath__array1D<uint>* idTable);
  uint search(Box* bb);

  uint addBox(int x1,
              int y1,
              int x2,
              int y2,
              uint level,
              uint id1,
              uint id2,
              uint wireType);
  uint search(int x1,
              int y1,
              int x2,
              int y2,
              uint row,
              uint col,
              Ath__array1D<uint>* idTable,
              bool wireIdFlag);
  void getCoords(SearchBox* bb, uint wireId);
  void setMaxArea(int x1, int y1, int x2, int y2);
  void resetMaxArea();

  // EXTRACTION

  void setDefaultWireType(uint v);
  void buildDgContext(int base, uint level, uint dir);
  Ath__array1D<SEQ*>* renewDgContext(uint gridn, uint trackn);
  void getBox(uint wid,
              int* x1,
              int* y1,
              int* x2,
              int* y2,
              uint* level,
              uint* id1,
              uint* id2,
              uint* wireType);
  uint search(SearchBox* bb,
              uint* gxy,
              uint row,
              uint col,
              Ath__array1D<uint>* idtable,
              Grid* g);
  uint getOverlapAdjust() { return _overlapAdjust; };
  uint getOverlapTouchCheck() { return _overlapTouchCheck; };
  uint targetHighTracks() { return _CCtargetHighTracks; };
  uint targetHighMarkedNet() { return _CCtargetHighMarkedNet; };
  void setCCFlag(uint ccflag) { _ccFlag = ccflag; };
  uint getCcFlag() { return _ccFlag; };
  uint contextDepth() { return _ccContextDepth; };
  Ath__array1D<int>** contextArray() { return _ccContextArray; };
  AthPool<SEQ>* seqPool() { return _seqPool; };
  Ath__array1D<SEQ*>*** dgContextArray() { return _dgContextArray; };
  int** dgContextTrackBase() { return _dgContextTrackBase; };
  uint* dgContextBaseTrack() { return _dgContextBaseTrack; };
  int* dgContextLowTrack() { return _dgContextLowTrack; };
  int* dgContextHiTrack() { return _dgContextHiTrack; };
  bool allNet() { return _allNet; };
  void setAllNet(bool allnet) { _allNet = allnet; };
  bool handleEmptyOnly() { return _handleEmptyOnly; };
  void setHandleEmptyOnly(bool handleEmptyOnly)
  {
    _handleEmptyOnly = handleEmptyOnly;
  };
  uint noPowerSource() { return _noPowerSource; };
  void setNoPowerSource(uint nps) { _noPowerSource = nps; };
  uint noPowerTarget() { return _noPowerTarget; };
  void setNoPowerTarget(uint npt) { _noPowerTarget = npt; };
  void incrCCshorts() { _CCshorts++; };

  void setExtControl(dbBlock* block,
                     bool useDbSdb,
                     uint adj,
                     uint npsrc,
                     uint nptgt,
                     uint ccUp,
                     bool allNet,
                     uint contextDepth,
                     Ath__array1D<int>** contextArray,
                     Ath__array1D<SEQ*>*** dgContextArray,
                     uint* dgContextDepth,
                     uint* dgContextPlanes,
                     uint* dgContextTracks,
                     uint* dgContextBaseLvl,
                     int* dgContextLowLvl,
                     int* dgContextHiLvl,
                     uint* dgContextBaseTrack,
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
  dbBlock* getBlock() { return _block; };
  void setBlock(dbBlock* block) { _block = block; };

  int couplingCaps(int hiXY,
                   uint couplingDist,
                   uint dir,
                   uint& wireCnt,
                   CoupleAndCompute coupleAndCompute,
                   void* compPtr,
                   bool getBandWire,
                   int** limitArray);
  void initCouplingCapLoops(uint dir,
                            uint couplingDist,
                            CoupleAndCompute coupleAndCompute,
                            void* compPtr,
                            int* startXY = nullptr);
  int dealloc(uint dir, int hiXY);
  void dealloc();

  uint getWireCnt();
  void setV2(bool v2)
  {
    _no_sub_tracks = v2;
    _v2 = v2;
  }

 private:
  uint setExtrusionMarker(uint startRow, uint startCol);
  Wire* getWire_Linear(uint instId);
  bool isOrdered(bool ascending);
  void removeMarkedNetWires();

  bool _no_sub_tracks = false;
  bool _v2 = false;
  // indexed [row][col].  Rows = dirs (2); cols = layers+1 (1-indexing)
  Grid*** _gridTable;
  Box _bbox;
  Box _maxSearchBox;
  bool _setMaxArea;
  Rect _rectBB;
  uint _rowCnt;
  uint _colCnt;
  uint _rowSize;
  uint _colSize;
  AthPool<Track>* _trackPool;
  AthPool<Wire>* _wirePool;
  uint _overlapAdjust{Z_noAdjust};
  uint _powerMultiTrackWire{0};
  uint _signalMultiTrackWire{0};
  uint _overlapTouchCheck{1};
  uint _noPowerSource{0};
  uint _noPowerTarget{0};
  uint _CCshorts{0};
  uint _CCtargetHighTracks{1};
  uint _CCtargetHighMarkedNet;
  bool _targetTrackReversed{false};
  bool _allNet{true};
  bool _handleEmptyOnly;
  bool _useDbSdb{true};
  uint _ccFlag;

  uint _ccContextDepth{0};

  // _v2
  uint* _ccContextLength;

  Ath__array1D<int>** _ccContextArray{nullptr};

  AthPool<SEQ>* _seqPool;
  Ath__array1D<SEQ*>*** _dgContextArray;  // array

  uint* _dgContextDepth;      // not array
  uint* _dgContextPlanes;     // not array
  uint* _dgContextTracks;     // not array
  uint* _dgContextBaseLvl;    // not array
  int* _dgContextLowLvl;      // not array
  int* _dgContextHiLvl;       // not array
  uint* _dgContextBaseTrack;  // array
  int* _dgContextLowTrack;    // array
  int* _dgContextHiTrack;     // array
  int** _dgContextTrackBase;  // array

  dbBlock* _block;

  uint _wireCnt;

  Ath__array1D<Wire*>* _bandWire{nullptr};

  bool _ttttGetDgOverlap{false};
};

}  // namespace rcx
