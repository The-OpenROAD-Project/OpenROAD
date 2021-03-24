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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ZInterface.h"
#include "array1.h"
#include "atypes.h"
#include "box.h"
#include "db.h"
#include "geom.h"
#include "gseq.h"
#include "qtype.h"
#include "tech.h"

using namespace odb;

enum Ath__overlapAdjust
{
  Z_noAdjust,
  Z_merge,
  Z_endAdjust
};

class Ath__net;

class Ath__path
{
 private:
  Ath__net* _net;

  union
  {
    struct
    {
      uint _level : 5;
      uint _trackNum : 12;
      uint _qRow : 5;
      uint _qCol : 5;
    } _from;

    uint _fromIterm;
  };
  union
  {
    struct
    {
      uint _level : 5;
      uint _trackNum : 12;
      uint _qRow : 5;
      uint _qCol : 5;
    } _to;

    uint _toIterm;
  };
  uint _num;

 public:
};

class Ath__via
{
 private:
  uint _trackNum : 12;
  uint _xy : 10;
  uint _masterNum : 10;

  uint _wire1 : 10;
  uint _wire2 : 10;
  uint _node : 12;  // TODO

 public:
};

class Ath__track;

class Ath__searchBox
{
 private:
  int  _ll[2];
  int  _ur[2];
  uint _level;
  uint _dir;
  uint _ownId;
  uint _otherId;
  uint _type;

 public:
  Ath__searchBox(int x1, int y1, int x2, int y2, uint l, int dir = -1);
  Ath__searchBox(Ath__box* bb, uint l, int dir = -1);
  Ath__searchBox(Ath__searchBox* bb, uint l, int dir = -1);
  Ath__searchBox();
  void set(int x1, int y1, int x2, int y2, uint l, int dir);
  void setMidPointSearch();
  int  loXY(uint d);
  int  loXY(uint d, int loBound);
  int  hiXY(uint d);
  int  hiXY(uint d, int hiBound);
  void setLo(uint d, int xy);
  void setHi(uint d, int xy);
  void setType(uint v);
  uint getType();

  uint getDir();
  uint getLevel();
  void setDir(int v = -1);
  void setLevel(uint v);
  void setOwnerId(uint v, uint other = 0);
  uint getOwnerId();
  uint getOtherId();
  uint getLength();
  void printCoords(FILE* fp, char* msg);
  void setMaxBox(Ath__searchBox* bb);
  bool outside(int x1, int y1, int x2, int y2);
};

class Ath__wire
{
 private:
  uint _id;
  uint _srcId;  // TODO-OPTIMIZE
  uint _boxId;
  uint _otherId;
	Ath__wire *_srcWire; // OpenRCX

  Ath__track* _track;
  Ath__wire*  _next;

  int _xy;  // TODO offset from track start in large dimension
  int _len;
	int _ouLen; // OpenRCX

  int _base;
  int _width : 24;

  uint _flags : 6;
  // 0=wire, 2=obs, 1=pin, 3=power or SET BY USER

  uint _dir : 1;
  uint _ext : 1;
	uint _visited:1; // OpenRCX

 public:
  int getShapeProperty(int id); // OpenRCX
	int getRsegId(); // OpenRCX
  
  void reset();
  // void set(int xy1, int xy2);
  void set(uint dir, int* ll, int* ur);
  void search(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable);
  void search1(int xy1, int xy2, uint& cnt, Ath__array1D<uint>* idTable);

  void       setNext(Ath__wire* w) { _next = w; };
  Ath__wire* getNext() { return _next; };
  uint       getFlags() { return _flags; };
  uint       getBoxId();
  void       setExt(uint ext) { _ext = ext; };
  uint       getExt() { return _ext; };
  uint       getId() { return _id; };
  void       setOtherId(uint id);
  uint       getOtherId();
  bool       isPower();
  bool       isVia(); // OpenRCX
  bool       isTilePin();
  bool       isTileBus();
  uint       getOwnerId();
  uint       getSrcId();
  void       getCoords(Ath__searchBox* box);
  int        getXY() { return _xy; }
  void       getCoords(int* x1, int* y1, int* x2, int* y2, uint* dir);

  // void makeZuiObject(Ath__zui *zui, Ath__boxType type, int layer);

  friend class Ath__track;
  friend class Ath__grid;
  friend class Ath__gridTable;
  friend class dbBlockHandle;

  // Extraction
  void       printOneWire(FILE* ptfile);
  void       printWireNeighbor(uint                      met,
                               Ath__array1D<Ath__wire*>* topNeighbor,
                               Ath__array1D<Ath__wire*>* botNeighbor);
  int        wireOverlap(Ath__wire* w, int* len1, int* len2, int* len3);
  Ath__wire* getPoolWire(AthPool<Ath__wire>* wirePool);
  Ath__wire* makeWire(AthPool<Ath__wire>* wirePool, int xy1, uint len);
  Ath__wire* makeCoupleWire(AthPool<Ath__wire>* wirePool,
                            int                 targetHighTracks,
                            Ath__wire*          w2,
                            int                 xy1,
                            uint                len,
                            uint                wtype);
  void       setXY(int xy1, uint len);
  dbNet*     getNet();
};

class Ath__grid;

class Ath__track
{
 private:
  int _x;  // you need only one
  int _y;

  int         _base;
  Ath__track* _hiTrack;
  Ath__track* _lowTrack;

  Ath__wire** _marker;
  Ath__wire** _eMarker;
  uint        _markerCnt;
  uint        _searchMarkerIndex;

  uint       _targetMarker;
  Ath__wire* _targetWire;

  Ath__grid* _grid;

  uint _num : 20;
  uint _markerModulo : 12;

  // int _width:20; // to be removed
  int        _width : 19;
  uint       _lowest : 1;
  uint       _widthIndex : 4;  // 0 is std width
  uint       _shift : 4;
  uint       _centered : 1;
  uint       _blocked : 1;
  uint       _full : 1;
  Ath__wire* _tmpHead;
  bool       _ordered;

 public:
  uint getTrackNum() { 
    return _num;
  };
  void set(Ath__grid* g,
           int        x,
           int        y,
           uint       n,
           uint       width,
           uint       markerLen,
           uint       markerCnt,
           int        base);
  void freeWires(AthPool<Ath__wire>* pool);
  bool place(Ath__wire* w, int markIndex1, int markIndex2);
  bool place(Ath__wire* w, int markIndex1);
  uint setExtrusionMarker(int markerCnt, int start, uint markerLen);
  bool placeTrail(Ath__wire* w, uint m1, uint m2);

  bool       overlapCheck(Ath__wire* w, int markIndex1, int markIndex2);
  bool       isAscendingOrdered(uint markerCnt, uint* wCnt);
  Ath__grid* getGrid();
  Ath__wire* getWire_Linear(uint markerCnt, uint id);
  Ath__wire* getNextWire(Ath__wire* wire);
  uint       search(int                 xy1,
                    int                 xy2,
                    uint                markIndex1,
                    uint                markIndex2,
                    Ath__array1D<uint>* idtable);
  uint       search1(int                 xy1,
                     int                 xy2,
                     uint                markIndex1,
                     uint                markIndex2,
                     Ath__array1D<uint>* idTable);

  bool checkAndplace(Ath__wire* w, int markIndex1);
  bool checkMarker(int markIndex);
  bool checkAndplacerOnMarker(Ath__wire* w, int markIndex);
  uint getAllWires(Ath__array1D<Ath__wire*>* boxTable, uint markerCnt);
  void resetExtFlag(uint markerCnt);
  void linkWire(Ath__wire*& w1, Ath__wire*& w2);

  Ath__track* getNextSubTrack(Ath__track* subt, bool tohi);
  int         getBase() { return _base; };
  void        setHiTrack(Ath__track* hitrack);
  void        setLowTrack(Ath__track* lowtrack);
  Ath__track* getHiTrack();
  Ath__track* getLowTrack();
  Ath__track* nextTrackInRange(uint& delt,
                               uint  trackDist,
                               uint  srcTrack,
                               bool  tohi);
  int         nextSubTrackInRange(Ath__track*& tstrack,
                                  uint&        delt,
                                  uint         trackDist,
                                  uint         srcTrack,
                                  bool         tohi);
  void        setLowest(uint lst) { _lowest = lst; };

  friend class Ath__gridTable;
  friend class Ath__grid;
  friend class Ath__wire;

  uint removeMarkedNetWires();

  // EXTRACTION

  bool place2(Ath__wire* w, int mark1, int mark2);
  void insertWire(Ath__wire* w, int mark1, int mark2);
  uint initTargetTracks(uint sourceTrack, uint trackDist, bool tohi);
  void findNeighborWire(Ath__wire*, Ath__array1D<Ath__wire*>*, bool);
  uint searchSpread(uint  srcTrack,
                    void* ip,
                    uint  trackDist,
                    uint  met,
                    void (*doSpread)(void*, uint, void*, void*, void*));
  void getTrackWires(std::vector<Ath__wire*>& ctxwire);
  void buildDgContext(Ath__array1D<odb::SEQ*>* dgContext,
                      Ath__wire**&             allWire,
                      int&                     awcnt,
                      int&                     a1wcnt);
  int  getBandWires(Ath__array1D<Ath__wire*>* bandWire);
  uint couplingCaps(Ath__grid*          resGrid,
                    uint                currentTrack,
                    uint                ccTrackDist,
                    uint                ccDomain,
                    ZInterface*         context,
                    Ath__array1D<uint>* ccTable,
                    uint                met,
                    void (*coupleAndCompute)(int*, void*),
                    void* compPtr);

  uint findOverlap(Ath__wire*                origWire,
                   uint                      ccDomain,
                   Ath__array1D<Ath__wire*>* wTable,
                   Ath__array1D<Ath__wire*>* nwTable,
                   Ath__grid*                ccGrid,
                   Ath__array1D<Ath__wire*>* ccTable,
                   ZInterface*               context,
                   uint                      met,
                   void (*coupleAndCompute)(int*, void*),
                   void* compPtr);

  void       initTargetWire(int noPowerWire);
  Ath__wire* nextTargetWire(int noPowerWire);
  Ath__wire* getTargetWire();
  void       adjustMetalFill();
  void       adjustOverlapMakerEnd(uint markerCnt);
  void       adjustOverlapMakerEnd(uint markerCnt, int start, uint markerLen);
  uint       trackContextOn(int                orig,
                            int                end,
                            int                base,
                            int                width,
                            uint               firstContextTrack,
                            Ath__array1D<int>* context);

  void dealloc(AthPool<Ath__wire>* pool);
};
class Ath__quadTable;
class Ath__gridTable;

class Ath__grid
{
 private:
  Ath__gridTable* _gridtable;
  Ath__track**    _trackTable;
  uint*           _blockedTrackTable;
  uint            _trackFilledCnt;
  float           _emptyTrackCnt;
  uint            _freeTrackCnt;
  uint            _trackCnt;
  uint*           _subTrackCnt;
  int             _base;
  int             _max;

  int _start;  // laterraly
  int _end;

  int _lo[2];
  int _hi[2];

  int  _width;
  int  _pitch;
  uint _level;
  uint _layer;
  uint _dir;
  int  _markerLen;
  uint _markerCnt;
  uint _searchLowTrack;
  uint _searchHiTrack;
  uint _searchLowMarker;
  uint _searchHiMarker;
  uint _placed : 1;

  // void (*coupleAndCompute)(int *, void *), void *compPtr;
  void* _coupleAndCompute;
  void* _compPtr;

  // AthArray<Ath__wire*> *_wireTable;
  // AthArray<Ath__via*> *_viaTable;

  uint                 _widthTable[8];
  uint                 _shiftTable[8];
  AthPool<Ath__track>* _trackPoolPtr;
  AthPool<Ath__wire>*  _wirePoolPtr;

  uint _ignoreFlag;
  uint _schema;
  uint _wireType;

  uint _currentTrack;
  uint _lastFreeTrack;

 public:
  Ath__grid(AthPool<Ath__track>* trackPool,
            AthPool<Ath__wire>*  _wirePool,
            Ath__box*            bb,
            Ath__layer*          layer,
            uint                 markerCnt = 4);
  Ath__grid(Ath__gridTable*      gt,
            AthPool<Ath__track>* trackPool,
            AthPool<Ath__wire>*  wirePool,
            Ath__box*            bb,
            uint                 level,
            uint                 dir,
            uint                 num,
            uint                 width,
            uint                 pitch,
            uint                 markerCnt = 4);
  Ath__grid(Ath__gridTable*      gt,
            AthPool<Ath__track>* trackPool,
            AthPool<Ath__wire>*  wirePool,
            uint                 level,
            uint                 num,
            uint                 markerCnt);
  ~Ath__grid();

  Ath__gridTable* getGridTable() { return _gridtable; };
  void            setBoundaries(uint dir, int xlo, int ylo, int xhi, int yhi);
  void            setTracks(uint dir,
                            uint width,
                            uint pitch,
                            int  xlo,
                            int  ylo,
                            int  xhi,
                            int  yhi,
                            uint markerLen = 0);
  void            setPlaced();
  void            setSchema(uint v);
  bool            isPlaced();

  uint setFreeTracks();
  bool anyTrackAvailable();

  uint        addWireList(Ath__box* box);
  uint        getTrackCnt() { return _trackCnt; };
  Ath__track* getTrackPtr(uint n) { return _trackTable[n]; };
  uint        getTrackNum1(int xy);
  uint        getWidth();
  int         getXYbyWidth(int xy, uint* mark);
  Ath__track* addTrack(uint ii, uint markerCnt, int base);
  Ath__track* addTrack(uint ii, uint markerCnt);
  void        makeTracks(uint space, uint width);
  void        getBbox(Ath__box* bb);
  void        getBbox(Ath__searchBox* bb);
  uint        setExtrusionMarker();
  uint        getBoundaries(Ath__zui* zui, uint dd, uint layer);
  uint        addWire(Ath__box* box, int check);
  uint        addWire(Ath__box* box);

  uint       placeWire(Ath__searchBox* bb);
  uint       placeBox(uint id, int x1, int y1, int x2, int y2);
  uint       placeBox(dbBox* box, uint wtype, uint id);
  uint       placeBox(Ath__box* box);
  uint       placeBox(Ath__searchBox* bb);
  uint       getBucketNum(int xy);
  uint       getTrackNum(int* ll, uint d, uint* marker);
  Ath__wire* getWirePtr(uint wireId);
  void getBoxIds(Ath__array1D<uint>* wireIdTable, Ath__array1D<uint>* idtable);
  void getWireIds(Ath__array1D<uint>* wireIdTable, Ath__array1D<uint>* idtable);

  int  findEmptyTrack(int ll[2], int ur[2]);
  uint getFirstTrack(uint divider);
  int  getClosestTrackCoord(int xy);
  void makeZuiObjects(Ath__zui* zui);
  uint addWire(uint initTrack, Ath__box* box, int sortedOrder, int* height);
  Ath__wire* getPoolWire();
  Ath__wire* makeWire(Ath__box* box,
                      uint*     id,
                      uint*     m1,
                      uint*     m2,
                      uint      fullTrack);
  Ath__wire* makeWire(Ath__box* box, uint id, uint* m1);
  Ath__wire* makeWire(int* ll, int* ur, uint id, uint* m1);
  // Ath__wire *makeWire(int xy1, int xy2, uint id1, uint id2, uint type=0);
  Ath__wire* makeWire(uint dir,
                      int* ll,
                      int* ur,
                      uint id1,
                      uint id2,
                      uint type = 0);

  Ath__wire* makeWire(Ath__wire* w, uint type = 0);

  void  makeTrackTable(uint width, uint pitch, uint space = 0);
  uint  blockTracks(Ath__searchBox* box, bool ignoreLevel = true);
  uint  blockTracks(dbBlock* block, Ath__array1D<uint>* idTable);
  uint  blockTracks(Ath__box* box, uint ignoreLevel);
  uint  blockTracks(dbBlock* block, Ath__gridTable* wireSearch);
  float updateFreeTracks(float v);
  void  resetFreeTracks();

  void        freeTracksAndTables();
  uint        getAbsTrackNum(int xy);
  uint        getMinMaxTrackNum(int xy);
  bool        addOnTrack(uint track, Ath__wire* w, uint mark1, uint mark2);
  int         getTrackHeight(uint track);
  uint        getTrackNum(Ath__box* box);
  Ath__track* getTrackPtr(int* ll);
  Ath__track* getTrackPtr(int xy);
  Ath__track* getTrackPtr(uint ii, uint markerCnt, int base);
  Ath__track* getTrackPtr(uint ii, uint markerCnt);
  bool        isOrdered(bool ascending, uint* cnt);
  uint        search(Ath__searchBox*     bb,
                     Ath__array1D<uint>* idtable,
                     bool                wireIdFlag = false);
  uint        searchAllMarkers(Ath__searchBox*     bb,
                               Ath__array1D<uint>* idtable,
                               bool                wireIdFlag = false);

  uint addWireNext(uint initTrack, Ath__box* box, int sortedOrder, int* height);
  // Ath__wire *getNewWire(Ath__box *box, int xy1, int xy2, uint *id);
  Ath__wire* getNewWire(Ath__box* box, int* ll, int* ur, uint* id);
  Ath__wire* makeWireCut(Ath__box* box,
                         uint*     id,
                         uint*     m1,
                         uint*     m2,
                         uint      cutFlag);
  Ath__wire* makeWireExt(Ath__box* box,
                         uint*     id,
                         uint*     m1,
                         uint*     m2,
                         uint      extFlag,
                         int       height);
  uint       placeWire(uint       initTrack,
                       Ath__wire* w,
                       uint       mark1,
                       uint       mark2,
                       int        sortedOrder,
                       int*       height);
  uint       addWireCut(uint      cutFlag,
                        uint      initTrack,
                        Ath__box* box,
                        int       sortedOrder,
                        int*      height);
  uint       addWireExt(uint      cutFlag,
                        uint      initTrack,
                        Ath__box* box,
                        int       sortedOrder,
                        int*      height);

  void getBusObs(Ath__zui* zui);
  void getBoxes(Ath__array1D<uint>* table);
  uint getBoxes(uint ii, Ath__array1D<uint>* table);

  void       placeAgainSortedThru(uint                 space,
                                  uint                 width,
                                  AthArray<Ath__box*>* obsTable);
  uint       getDir();
  uint       getLevel();
  Ath__wire* getWire_Linear(uint id);
  void       setIgnoreFlag(uint v);

  int  placeTileWire(uint            wireType,
                     uint            id,
                     Ath__searchBox* bb,
                     int             loMarker,
                     int             initTrack,
                     bool            skipResetSize);
  void getBuses(Ath__array1D<Ath__box*>* boxtable, uint width);
  uint white(Ath__searchBox* bb, Ath__array1D<uint>* idtable, Ath__grid* g);
  Ath__wire* makeWhite(uint                dir,
                       int*                lo,
                       int*                hi,
                       Ath__array1D<uint>* idtable,
                       Ath__grid*          g);

  friend class Ath__quadTable;
  friend class Ath__gridTable;

  uint removeMarkedNetWires();
  void setSearchDomain(uint domainAdjust);
  uint searchLowMarker() { return _searchLowMarker; };
  uint searchHiMarker() { return _searchHiMarker; };

  // EXTRACTION
  uint                searchSpread(void* ip,
                                   uint  spreadTrackDist,
                                   void (*doSpread)(void*, uint, void*, void*, void*));
  void                buildDgContext(int gridn, int base);
  int                 getBandWires(int                       hiXY,
                                   uint                      couplingDist,
                                   uint&                     wireCnt,
                                   Ath__array1D<Ath__wire*>* bandWire,
                                   int*                      limitArray);
  uint                couplingCaps(Ath__grid*          resGrid,
                                   uint                couplingDist,
                                   ZInterface*         context,
                                   Ath__array1D<uint>* ccTable,
                                   void (*coupleAndCompute)(int*, void*),
                                   void* compPtr);
  AthPool<Ath__wire>* getWirePoolPtr();
  uint                placeWire(Ath__wire* w);
  uint                defaultWireType();
  void                setDefaultWireType(uint v);
  uint                search(Ath__searchBox*     bb,
                             uint*               gxy,
                             Ath__array1D<uint>* idtable,
                             Ath__grid*          g);
  void                adjustMetalFill();
  void                adjustOverlapMakerEnd();
  void                initContextGrids();
  void                initContextTracks();
  void                contextsOn(int orig, int len, int base, int width);
  void                gridContextOn(int orig, int len, int base, int width);

  int  initCouplingCapLoops(uint couplingDist,
                            void (*coupleAndCompute)(int*, void*),
                            void* compPtr,
                            bool  startSearchTrack = true,
                            int   startXY          = 0);
  int  couplingCaps(int   hiXY,
                    uint  couplingDist,
                    uint& wireCnt,
                    void (*coupleAndCompute)(int*, void*),
                    void* compPtr,
                    int*  limitArray);
  int  dealloc(int hiXY);
  void dealloc();
};
class Ath__gridStack  // DELETE after porting on new DB
{
 private:
  Ath__grid* _nextGridTable[2];
  Ath__grid* _thruGridTable[2];
  Ath__grid* _cornerGridTable[4][2];
  int        _loDivide[2];
  int        _hiDivide[2];
  uint       _level[2];
  uint       _nextLevel[2];
  Rect       _bb;

 public:
  Ath__gridStack(Ath__layer**         met,
                 Ath__layer**         nextMet,
                 uint*                loFlag,
                 uint*                hiFlag,
                 int*                 loLine,
                 int*                 hiLine,
                 int*                 ll,
                 int*                 ur,
                 AthPool<Ath__track>* trackPool,
                 AthPool<Ath__wire>*  wirePool);
  ~Ath__gridStack();
  bool isThruPlaced(uint dir);
  uint getGridOutlines(Ath__zui* zui);
  uint blockTracks(AthArray<Ath__box*>* obsTable);
  uint addWire(Ath__box* box, uint trackRange);

  uint        placeSortedThru(uint       dir,
                              Ath__box** boxArray,
                              uint       cnt,
                              uint       width,
                              uint       space);
  uint        blockTracks(Ath__box* boxList, uint dir);
  uint        blockTracks(Ath__box* box);
  Ath__grid** getCornerGrid(uint type,
                            uint space,
                            uint width,
                            uint layer0,
                            uint layer2);
  uint        placeCornerSorted(uint       type,
                                Ath__box** boxArray,
                                Ath__box** boxArrayNext,
                                uint       cnt,
                                uint       space,
                                uint       width);
  Ath__grid*  getThruGrid(uint dir, uint layer, uint space, uint width);
  Ath__grid*  getThruGrid(uint dir);
  Ath__grid*  getNextGrid(uint dir);
  Ath__grid*  getNextGrid(uint dir, uint layer, uint space, uint width);
  uint        placeSortedNext(uint       dir,
                              Ath__box** boxArray,
                              int*       heightArray,
                              uint       cnt,
                              uint       space,
                              uint       width);
  void        getBusObs(Ath__zui* zui);
  bool        isPlacedCorner(uint type);
};
class Ath__gridTile
{
 private:
  Ath__grid** _gTable;
  uint        _levelCnt;
  Rect        _bb;

  bool                 _poolFlag;
  AthPool<Ath__track>* _trackPool;
  AthPool<Ath__wire>*  _wirePool;

 public:
  Ath__gridTile(uint                 levelCnt,
                int                  x1,
                int                  y1,
                int                  x2,
                int                  y2,
                AthPool<Ath__track>* trackPoolPtr = NULL,
                AthPool<Ath__wire>*  wirePoolPtr  = NULL);
  ~Ath__gridTile();
  Ath__grid* addGrid(Ath__box* bb,
                     uint      level,
                     uint      dir,
                     uint      layerNum,
                     uint      width,
                     uint      pitch);
  Ath__grid* addGrid(int  x1,
                     int  y1,
                     int  x2,
                     int  y2,
                     uint level,
                     uint dir,
                     uint layerNum,
                     uint width,
                     uint pitch);
  void       searchWires(Ath__searchBox* bb, Ath__array1D<uint>* idtable);

  void       getBounds(int* x1, int* y1, int* x2, int* y2);
  Ath__grid* getGrid(uint level);
  void       addGrid(Ath__grid* g);

  void getBuses(Ath__array1D<Ath__box*>* boxTable, dbTech* tech);
};

class Ath__gridTable
{
 private:
  Ath__grid***         _gridTable;
  Ath__box             _bbox;
  Ath__box             _maxSearchBox;
  bool                 _setMaxArea;
  Rect                 _rectBB;
  uint                 _rowCnt;
  uint                 _colCnt;
  uint                 _rowSize;
  uint                 _colSize;
  AthPool<Ath__track>* _trackPool;
  AthPool<Ath__wire>*  _wirePool;
  uint                 _schema;
  uint                 _overlapAdjust;
  uint                 _powerMultiTrackWire;
  uint                 _signalMultiTrackWire;
  uint                 _offlineOverlapTouch;
  uint                 _offlineOverlapCnt;
  uint                 _overlapTouchCheck;
  uint                 _noPowerSource;
  uint                 _noPowerTarget;
  uint                 _CCshorts;
  uint                 _CCtargetHighTracks;
  uint                 _CCtargetHighMarkedNet;
  bool                 _targetTrackReversed;
  bool                 _allNet;
  bool                 _handleEmptyOnly;
  bool                 _useDbSdb;
  uint                 _ccFlag;

  uint  _ccContextDepth;
  uint* _ccContextLength;

  Ath__array1D<int>** _ccContextArray;

  AthPool<odb::SEQ>*         _seqPool;
  Ath__array1D<odb::SEQ*>*** _dgContextArray;  // array

  uint* _dgContextDepth;      // not array
  uint* _dgContextPlanes;     // not array
  uint* _dgContextTracks;     // not array
  uint* _dgContextBaseLvl;    // not array
  int*  _dgContextLowLvl;     // not array
  int*  _dgContextHiLvl;      // not array
  uint* _dgContextBaseTrack;  // array
  int*  _dgContextLowTrack;   // array
  int*  _dgContextHiTrack;    // array
  int** _dgContextTrackBase;  // array

  int _signalPowerNotAlignedOverlap;
  int _powerNotAlignedOverlap;
  int _signalNotAlignedOverlap;
  int _signalOverlap;
  int _powerOverlap;
  int _signalPowerOverlap;
  int _powerSignalOverlap;

  dbBlock* _block;

  uint _wireCnt;

  // void (*coupleAndCompute)(int *, void *), void *compPtr;
  void* _coupleAndCompute;
  void* _compPtr;

  Ath__array1D<Ath__wire*>* _bandWire;

 public:
  Ath__gridTable(Ath__box* bb,
                 uint      rowSize,
                 uint      colSize,
                 uint      layer,
                 uint      dir,
                 uint      width,
                 uint      pitch);
  Ath__gridTable(dbBox* bb,
                 uint   rowSize,
                 uint   colSize,
                 uint   layer,
                 uint   dir,
                 uint   width,
                 uint   pitch,
                 uint   minWidth);
  Ath__gridTable(Rect* bb,
                 uint  layer,
                 uint  dir,
                 uint  width,
                 uint  pitch,
                 uint  minWidth);
  Ath__gridTable(Rect* bb,
                 uint  rowCnt,
                 uint  colCnt,
                 uint* width,
                 uint* pitch,
                 uint* spacing,
                 int*  X1 = NULL,
                 int*  Y1 = NULL);
  ~Ath__gridTable();
  Ath__grid* getGrid(uint row, uint col);
  void       init1(uint memChunk, uint rowSize, uint colSize, uint dx, uint dy);
  uint       getColCnt();
  uint       getRowCnt();
  Ath__wire* getWirePtr(uint id);
  void       releaseWire(uint wireId);
  Ath__box*  maxSearchBox() { return &_maxSearchBox; };
  int        xMin();
  int        xMax();
  int        yMin();
  int        yMax();
  uint       getRowNum(int x);
  uint       getColNum(int y);
  bool       getRowCol(int x1, int y1, uint* row, uint* col);
  Ath__wire* addBox(Ath__box* bb);
  Ath__wire* addBox(dbBox* bb, uint wtype, uint id);
  bool       addBox(uint row, uint col, dbBox* bb);
  uint       setExtrusionMarker(uint startRow, uint startCol);

  uint       getBoxes(Ath__box* bb, Ath__array1D<Ath__box*>* table);
  bool       isOrdered(bool ascending);
  uint       search(Ath__zui* zui, uint row, uint col);
  uint       search(Ath__searchBox*     bb,
                    uint                row,
                    uint                col,
                    Ath__array1D<uint>* idTable,
                    bool                wireIdFlag);
  uint       search(Ath__searchBox* bb, Ath__array1D<uint>* idTable);
  uint       search(Ath__box* bb);
  Ath__wire* getWire_Linear(uint instId);
  uint       white(Ath__searchBox*     bb,
                   uint                row,
                   uint                col,
                   Ath__array1D<uint>* idTable,
                   Ath__grid*          g);

  uint addBox(int  x1,
              int  y1,
              int  x2,
              int  y2,
              uint level,
              uint id1,
              uint id2,
              uint wireType);
  uint search(int                 x1,
              int                 y1,
              int                 x2,
              int                 y2,
              uint                row,
              uint                col,
              Ath__array1D<uint>* idTable,
              bool                wireIdFlag);
  void getCoords(Ath__searchBox* bb, uint wireId);
  void setMaxArea(int x1, int y1, int x2, int y2);
  void resetMaxArea();

  void removeMarkedNetWires();

  // EXTRACTION

  void                     setDefaultWireType(uint v);
  uint                     searchSpread(void*                ip,
                                        uint                 spreadTrackDist,
                                        std::vector<dbNet*>& inets,
                                        char*                bbox,
                                        void (*doSpread)(void*, uint, void*, void*, void*));
  void                     buildDgContext(int base, uint level, uint dir);
  Ath__array1D<odb::SEQ*>* renewDgContext(uint gridn, uint trackn);
  uint                     couplingCaps(Ath__gridTable*     resGridTable,
                                        uint                couplingDist,
                                        ZInterface*         context,
                                        Ath__array1D<uint>* ccTable,
                                        void (*coupleAndCompute)(int*, void*),
                                        void* compPtr);
  uint                     couplingCaps(uint        row,
                                        uint        col,
                                        Ath__grid*  resGrid,
                                        uint        couplingDist,
                                        ZInterface* context);
  void                     getBox(uint  wid,
                                  int*  x1,
                                  int*  y1,
                                  int*  x2,
                                  int*  y2,
                                  uint* level,
                                  uint* id1,
                                  uint* id2,
                                  uint* wireType);
  void getCCdist(uint wid, uint* width, uint* level, uint* id1, uint* id2);
  void getIds(uint wid, uint* id1, uint* id2, uint* wtype);
  uint search(Ath__searchBox*     bb,
              uint*               gxy,
              uint                row,
              uint                col,
              Ath__array1D<uint>* idtable,
              Ath__grid*          g);
  uint getOverlapAdjust() { return _overlapAdjust; };
  uint getOverlapTouchCheck() { return _overlapTouchCheck; };
  uint targetHighTracks() { return _CCtargetHighTracks; };
  uint targetHighMarkedNet() { return _CCtargetHighMarkedNet; };
  void setCCFlag(uint ccflag) { _ccFlag = ccflag; };
  uint getCcFlag() { return _ccFlag; };
  uint contextDepth() { return _ccContextDepth; };
  void setContextLength(uint level, uint contextLength)
  {
    _ccContextLength[level] = contextLength;
  };
  Ath__array1D<int>**        contextArray() { return _ccContextArray; };
  AthPool<odb::SEQ>*         seqPool() { return _seqPool; };
  Ath__array1D<odb::SEQ*>*** dgContextArray() { return _dgContextArray; };
  int** dgContextTrackBase() { return _dgContextTrackBase; };
  uint* dgContextBaseTrack() { return _dgContextBaseTrack; };
  int*  dgContextLowTrack() { return _dgContextLowTrack; };
  int*  dgContextHiTrack() { return _dgContextHiTrack; };
  bool  allNet() { return _allNet; };
  void  setAllNet(bool allnet) { _allNet = allnet; };
  bool  handleEmptyOnly() { return _handleEmptyOnly; };
  void  setHandleEmptyOnly(bool handleEmptyOnly)
  {
    _handleEmptyOnly = handleEmptyOnly;
  };
  uint     noPowerSource() { return _noPowerSource; };
  void     setNoPowerSource(uint nps) { _noPowerSource = nps; };
  uint     noPowerTarget() { return _noPowerTarget; };
  void     setNoPowerTarget(uint npt) { _noPowerTarget = npt; };
  void     incrOfflineOverlapCnt() { _offlineOverlapCnt++; };
  void     incrOfflineOverlapTouch() { _offlineOverlapTouch++; };
  void     incrCCshorts() { _CCshorts++; };
  void     setExtControl(dbBlock*                   block,
                         bool                       useDbSdb,
                         uint                       adj,
                         uint                       npsrc,
                         uint                       nptgt,
                         uint                       ccUp,
                         bool                       allNet,
                         uint                       contextDepth,
                         Ath__array1D<int>**        contextArray,
                         uint*                      contextLength,
                         Ath__array1D<odb::SEQ*>*** dgContextArray,
                         uint*                      dgContextDepth,
                         uint*                      dgContextPlanes,
                         uint*                      dgContextTracks,
                         uint*                      dgContextBaseLvl,
                         int*                       dgContextLowLvl,
                         int*                       dgContextHiLvl,
                         uint*                      dgContextBaseTrack,
                         int*                       dgContextLowTrack,
                         int*                       dgContextHiTrack,
                         int**                      dgContextTrackBase,
                         AthPool<odb::SEQ>*         seqPool);
  bool     usingDbSdb() { return _useDbSdb; }
  void     reverseTargetTrack();
  bool     targetTrackReversed() { return _targetTrackReversed; };
  void     adjustMetalFill();
  void     incrNotAlignedOverlap(Ath__wire* w1, Ath__wire* w2);
  void     incrSignalOverlap();
  void     incrPowerOverlap();
  void     incrSignalToPowerOverlap();
  void     incrPowerToSignallOverlap();
  void     incrMultiTrackWireCnt(bool isPower);
  void     adjustOverlapMakerEnd();
  void     dumpTrackCounts(FILE* fp);
  dbBlock* getBlock() { return _block; };
  void     setBlock(dbBlock* block) { _block = block; };

  int  couplingCaps(int   hiXY,
                    uint  couplingDist,
                    uint  dir,
                    uint& wireCnt,
                    void (*coupleAndCompute)(int*, void*),
                    void* compPtr,
                    bool  getBandWire,
                    int** limitArray);
  void initCouplingCapLoops(uint dir,
                            uint couplingDist,
                            void (*coupleAndCompute)(int*, void*),
                            void* compPtr,
                            int*  startXY = NULL);
  int  dealloc(uint dir, int hiXY);
  void dealloc();

  uint getWireCnt();
};
