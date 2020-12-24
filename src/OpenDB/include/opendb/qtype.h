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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "box.h"
#include "conn.h"
#include "tech.h"
#include "wire.h"

using namespace odb;

class Ath__grid;

class Ath__qCoord
{
 protected:
  int  _x[33];
  uint _mod;
  int  _min;
  int  _max;
  uint _num;

 public:
  Ath__qCoord(int n, int min, int max);
  int  get(uint index);
  int  getCenter(uint index, int offset);
  int  getNext(uint index);
  void set(uint index, int x0);
  int  getIndex(int x);
  int  getMinMaxIndex(int x);
  uint getIndex4(int x0);
  void snapOnPower(Ath__grid* g);
};

class Ath__qCounter
{
 private:
  uint  _row;
  uint  _col;
  uint  _localPin;
  uint  _topPin;
  int   _tracks[2];
  int   _next[2];
  int   _thru[2];
  int   _turn[4];
  int   _tjog[4];
  uint  _cross;
  uint  _hot;
  char* _name;

 public:
  char* HV[2];

  Ath__qCounter(uint ii, uint jj);
  ~Ath__qCounter();
  void  setName(uint hCnt, uint vCnt);
  char* getName();
  void  updateNext(uint n);
  void  updateTurn(uint n);
  void  updateThru(uint n);
  bool  checkTurn(uint n);
  uint  checkThru(uint n);
  void  resetTracks(uint dir, uint hCnt, uint turn);
  void  resetTracks(uint r, uint c, uint hCnt, uint vCnt);
  void  setMaxTracks(uint dir, uint hCnt);
  void  setTracks(uint r, uint c, uint hCnt, uint vCnt, int dir = -1);
  void  init(int* A, uint n, int v);
  int   sumTurns();
  bool  checkAllRes(uint dir, int threshold);
  uint  thruSummary(uint ii, uint jj, uint dir, uint threshold);
  uint  printSummary(uint ii, uint jj);
  uint  printHotSummary();

  friend class Ath__quadTable;
};

class Ath__gridTable;
class Ath__gridStack;
class Ath__track;
class Ath__wire;
class Ath__netTable;
class Ath__net;
class Ath__gridTile;

class Ath__quadTable
{
 private:
  uint _rowCnt;
  uint _colCnt;

  Ath__qCounter*** _quad;  // in old DB only
  Ath__tech*       _tech;

  AthArray<Ath__box*>*** _obsTable;

  Ath__array1D<uint>*** _tmpItermTable;
  Ath__array1D<uint>*** _localNetTable;
  Ath__array1D<uint>*** _ioNetTable;
  Ath__array1D<uint>*** _termTable;

  Ath__box***       _tmpBox;
  Ath__qPinTable*   _qPinTmpTable;
  Ath__qPinTable*** _qPinTable;
  Ath__connWord*    _tmpArray;
  uint              _tmpCnt;
  uint              _tmpNetArray[1000];
  Ath__connWord*    _targetArray;
  uint              _targetCnt;
  uint*             _fanoutArray;
  Ath__connWord     _tmpSrc;

  uint         _minTileSize;
  Ath__qCoord* _rowDivider;  // y coords
  Ath__qCoord* _colDivider;  // x coords

  Ath__connTable*        _connTable;
  Ath__connTable***      _thruPinTable;
  AthPool<Ath__p2pConn>* _thruPinPool;

  AthPool<Ath__qPin>* _qBoxPool;
  AthPool<Ath__box>*  _pinBoxPool;

  uint _hPitch;
  uint _vPitch;

  char* _tmpFullDir;
  char* _topDir;

  FILE* _gFP;
  FILE* _qFP;
  uint  _pinVisualGap;
  uint  _pinLength;
  uint  _pinWidth;
  uint  _pinStyle;
  uint  _pinSpace;
  uint  _topMetH;
  uint  _topMetV;

  Ath__gridStack*** _qShapeTable;  // NOT USED in new DB
  int*              _tmpIntArray;
  Ath__box**        _tmpBoxArray;
  Ath__box**        _tmpBoxArrayNext;

  AthPool<Ath__track>* _trackPool;
  AthPool<Ath__wire>*  _wirePool;

  uint _placePins_thru;
  uint _placePins_next;
  uint _placePins_corner;

  AthPool<Ath__qNet>* _qNetPool;

 public:
  Ath__gridTile*** _busTable;
  uint             _tileCnt;
  Ath__gridTile**  _tileTable;
  Ath__grid*       _tileSearchGrid;

 public:
  Ath__quadTable(uint       m,
                 int        minY,
                 int        maxY,
                 uint       n,
                 int        minX,
                 int        maxX,
                 Ath__grid* gH = NULL,
                 Ath__grid* gV = NULL);
  ~Ath__quadTable();
  void                  initPinTable(uint n = 0);
  void                  freePinTable(bool skipPool = false);
  Ath__array1D<uint>*** alloc2Darray1d(uint m, uint n, uint allocChunk);
  void de_alloc2Darray1d(Ath__array1D<uint>*** A, uint m, uint n);
  void qInit(AthPool<Ath__box>* _pinBoxPool, AthPool<Ath__qPin>* _qPinPool);
  void init_shapeTable(Ath__tech* tech,
                       uint       trackInit,
                       uint       wireInit,
                       uint       metH = 0,
                       uint       metV = 0);
  void free_shapeTable();
  void qFree();
  uint getRowCnt() { return _rowCnt; };
  uint getColCnt() { return _colCnt; };
  Ath__array1D<uint>* getLocalNets(uint ii, uint jj)
  {
    return _localNetTable[ii][jj];
  };
  Ath__array1D<uint>* getLocalIoNets(uint ii, uint jj)
  {
    return _ioNetTable[ii][jj];
  };
  void   printConnectivityStats(FILE* fp, uint netCnt, uint itermCnt);
  uint** alloc2Darray(uint m, uint n);
  void   de_alloc2Darray(uint** A, uint m);
  void   setQuadLineY(uint index, int xy);
  void   setQuadLineX(uint index, int xy);
  void   printEmptyQ(uint ii, uint jj, FILE* fp);
  void   printBbox(FILE* fp, uint ii, uint jj);
  void   printPattern(FILE* fp, char* word, uint n);
  void   printRow(uint row, uint** A, uint n, FILE* fp);
  void   printCounters(const char* msg, FILE* fp = stdout);
  void   print(FILE* fp = stdout);
  // Ath__connWord getRowCol(int x, int y);
  void          addPoint(int x, int y);
  Ath__connWord getRowCol(int x1, int y1, int x2, int y2, uint termtag);
  void          addPoint(int x1, int y1, int x2, int y2, uint termtag);
  bool          addTarget(Ath__connWord w, uint netId, uint termCnt);
  void          addPin(Ath__connWord w);
  void          printTargets(int type, int netIndex, FILE* fp = stdout);
  void          getHVsegs(uint r1, uint c1, uint r2, uint c2);
  void          printSquare(FILE*       fp,
                            uint        cnt,
                            const char* node,
                            char*       type,
                            const char* preName,
                            int         x1,
                            int         y1,
                            int         x2,
                            int         y2);
  void printHnext(int offsetX, int offsetY, uint width, FILE* fp = stdout);
  uint getCnt(uint** A);
  uint getNextCnt();
  void reset();
  void reset(Ath__connWord w);
  uint processConnections(uint netId, uint termCnt);
  void defineConn(uint netId, uint src, uint dst);
  void defineConn(uint netId, uint src, uint dst1, uint dst2);
  void defineConn3strV(uint          netId,
                       Ath__connWord w01,
                       Ath__connWord w02,
                       Ath__connWord w23,
                       uint          r0,
                       uint          r1,
                       uint          c1,
                       uint          r2,
                       uint          c2);
  void defineConn3strH(uint          netId,
                       Ath__connWord w01,
                       Ath__connWord w02,
                       Ath__connWord w23,
                       uint          c0,
                       uint          r1,
                       uint          c1,
                       uint          r2,
                       uint          c2);

  void defineConn3row(uint          netId,
                      Ath__connWord w01,
                      Ath__connWord w02,
                      Ath__connWord w23,
                      uint          r0,
                      uint          c0,
                      uint          r1,
                      uint          c1,
                      uint          r2,
                      uint          c2);
  void defineConn3col(uint          netId,
                      Ath__connWord w01,
                      Ath__connWord w02,
                      Ath__connWord w23,
                      uint          r0,
                      uint          c0,
                      uint          r1,
                      uint          c1,
                      uint          r2,
                      uint          c2);
  void defineConn3strH(uint netId, Ath__connWord w, uint r2, uint c2);
  void defineConn3strV(uint netId, Ath__connWord w, uint r2, uint c2);

  void  printFanoutTable(FILE* fp = stdout);
  void  printQuadRow(uint row, int offsetX, int offsetY, uint width, char* dir);
  void  printQuadCol(uint col, int offsetX, int offsetY, uint width, char* dir);
  void  printQuad(int offsetX, int offsetY, uint width, char* dir);
  void  printThruCnts(uint  ii,
                      uint  jj,
                      int   offsetX,
                      int   offsetY,
                      uint  width,
                      FILE* fp);
  FILE* openQuadFile(const char* dir, int row, int col);
  void  printRows(int offsetX, int offsetY, uint width, char* dir);
  void  printCols(int offsetX, int offsetY, uint width, char* dir);
  void  print1q(uint  row,
                uint  col,
                int   offsetX,
                int   offsetY,
                uint  width,
                char* dir);
  void  printAllQuads(int offsetX, int offsetY, uint width, char* dir);
  void  printNextCnts(uint  ii,
                      uint  jj,
                      int   offsetX,
                      int   offsetY,
                      uint  width,
                      FILE* fp);
  void  printHVCnts(uint  ii,
                    uint  jj,
                    int   offsetX,
                    int   offsetY,
                    uint  width,
                    FILE* fp);
  void  printVHCnts(uint  ii,
                    uint  jj,
                    int   offsetX,
                    int   offsetY,
                    uint  width,
                    FILE* fp);
  void  printSquareNode(FILE*       fp,
                        uint        cnt,
                        uint        ii,
                        uint        jj,
                        char*       type,
                        const char* prename,
                        int         x1,
                        int         y1,
                        int         xy2,
                        uint        width,
                        bool        horizFlag);

  void calcNextRes(Ath__connWord w);
  void calcThruResources(Ath__p2pConn* p2p);
  uint getBoundaries(uint ii, uint jj, Ath__zui* zui, uint w, uint layer);

  uint assignTracks(uint hPitch, uint vPitch, bool resetRes);
  void assignStraightTracks(uint dir, bool makePin = false);
  void checkResources(int threshold);
  bool checkMidRes(uint  r1,
                   uint  c1,
                   uint  r2,
                   uint  c2,
                   uint* midRow,
                   uint* midCol,
                   uint* turn);
  void setTracks(uint hPitch, uint vPitch);
  void createPlacedPins(uint       dir,
                        uint       cnt,
                        Ath__box** boxArray,
                        int*       trackHeight,
                        uint       width);
  uint assignTracksDist2();
  uint checkThruRes(Ath__connWord w);
  uint assignLongTurns(uint dir);
  uint assignThinTurns(uint dir);
  uint printHotSpots();
  void resetTracks(uint hPitch, uint vPitch);
  void makePinsH();
  void printRow(uint row, Ath__qPinTable* pinTable, Ath__zui* zui);
  void printQuads();
  void printQuadRow(uint row, Ath__zui* zui);
  uint stackLongConnsH(Ath__qPinTable* pinTable,
                       uint            dist1,
                       uint            dist2,
                       uint            row,
                       uint            cnt);
  void fillPinsH(Ath__qPinTable* pinTable, uint k, uint row, Ath__connWord v);
  void printPinTable(char*           qName,
                     uint            row,
                     Ath__qPinTable* pinTable,
                     Ath__zui*       zui);
  void printPinTable(char* qName, uint row, Ath__qPinTable* pinTable);
  void fillBeforePinsH(Ath__qPinTable* pintable,
                       uint            k,
                       uint            row,
                       uint            firstCol,
                       uint            c1);
  uint getQuadBoxes(Ath__zui* zui);
  uint getStraightThrus(uint      ii,
                        uint      jj,
                        uint      dir,
                        uint      pinWidth,
                        Ath__box* bb,
                        Ath__zui* zui);
  uint getThruCorner2(uint      ii,
                      uint      jj,
                      uint      dir,
                      uint      pinWidth,
                      Ath__box* bb,
                      Ath__zui* zui);
  uint getStraightNext(uint      ii,
                       uint      jj,
                       uint      dir,
                       uint      pinWidth,
                       Ath__box* bb,
                       Ath__zui* zui);
  uint assignNotThinTurns(uint dir);

  void getTileBounds(uint ii,
                     uint jj,
                     int* minX,
                     int* maxX,
                     int* minY,
                     int* maxY,
                     int* centerX,
                     int* centerY);

  uint       getTopPins(uint pinWidth, Ath__zui* zui);
  uint       getSpacing(uint cnt, int minX, int maxX, uint width, int* centerX);
  Ath__qPin* mkQpin(uint                r1,
                    uint                c1,
                    uint                netId,
                    uint                ioNetId,
                    Ath__connWord       w,
                    AthPool<Ath__qPin>* pool = NULL);
  void addConn2Pins(Ath__connWord w, Ath__qPin* srcPin, Ath__qPin* dstPin);
  void addConn3Pins(Ath__qPin*    srcPin,
                    Ath__connWord w,
                    Ath__qPin*    pin1,
                    Ath__connWord w23,
                    Ath__qPin*    pin2);
  void addConn4Pins(Ath__qPin*    srcPin,
                    Ath__connWord w1,
                    Ath__qPin*    pin1,
                    Ath__connWord w2,
                    Ath__qPin*    pin2,
                    Ath__connWord w3,
                    Ath__qPin*    pin3);

  void      freeBoxes(Ath__box* head);
  Ath__box* addPinBox(Ath__box* e, Ath__box* head);
  Ath__box* newPinBox(uint layer, int x1, int y1, int x2, int y2);
  Ath__box* newPinBox(Ath__searchBox* bb, uint ownId);

  uint makeTopPins(int connType,
                   int priority,
                   int style,
                   int space,
                   int width,
                   int length,
                   int top_h_layer,
                   int top_v_layer);

  uint getQuadBoundaries(Ath__zui* zui, uint busFlag, uint w, uint layer);

  uint makeNextPinShapes(uint ii, uint jj, int priority);

  uint makeCornerPinShapes(uint ii,
                           uint jj,
                           int  priority,
                           int  style,
                           int  space,
                           int  pinWidth,
                           int  length,
                           int  top_h_layer,
                           int  top_v_layer);

  uint makeCornerPinShapes(uint ii, uint jj);
  uint getTopPins(Ath__zui* zui);
  uint getBusPins(Ath__zui* zui);
  uint makeZuiObjects(Ath__box* head, Ath__zui* zui);

  void createEdgePins(Ath__qPin* src,
                      Ath__qPin* dst,
                      uint       row,
                      uint       col,
                      uint       dir,
                      int        layer,
                      Ath__box*  bb,
                      int        length);
  void createAreaPins(Ath__qPin* src,
                      Ath__qPin* dst,
                      uint       row,
                      uint       col,
                      uint       dir,
                      Ath__box*  bb,
                      int        layer);

  void cornerPinPlus(uint netId,
                     uint row,
                     uint col,
                     uint type,
                     uint dir,
                     int  x1,
                     int  y1,
                     int  x2,
                     int  y2);
  void cornerPinMinus(uint netId,
                      uint row,
                      uint col,
                      uint type,
                      uint dir,
                      int  x1,
                      int  y1,
                      int  x2,
                      int  y2);
  void jumperPins(uint netId,
                  uint row1,
                  uint col1,
                  uint row2,
                  uint col2,
                  uint dir,
                  int  x1,
                  int  y1,
                  int  x2,
                  int  y2);
  uint makeJumperPinShapes(uint ii, uint jj);

  uint getPinObsShapes(Ath__zui* zui);
  void sortX(Ath__box** A, uint cnt, uint dbg = 0);
  void sortY(Ath__box** A, uint cnt, uint dbg = 0);
  void sortByLength(Ath__box** A, Ath__box** B, uint cnt, uint dbg = 0);

  uint placeShapes(uint      row,
                   uint      col,
                   uint      dir,
                   Ath__box* head,
                   uint      width,
                   uint      space);

  uint makeTopBuses(uint ii, uint jj, uint width, uint space, uint opt);
  uint makeTopBuses(uint width, uint space, uint opt);

  uint makeTopBuses_1(uint ii, uint jj, uint opt, uint width, uint space);
  uint makeTopBuses_1(uint opt, uint width, uint space);

  uint placeCornerObsShapes(uint row,
                            uint col,
                            uint type,
                            uint width,
                            uint space);

  uint  getShapesSorted(uint dir, Ath__box* head);
  uint  sortObjectsY(Ath__box* head);
  uint  addObs(Ath__box* box);
  void  blockTracks();
  void  getDivideLines(int*  ll,
                       int*  ur,
                       uint* loDivide,
                       uint* hiDivide,
                       int*  xy1,
                       int*  xy2);
  FILE* defHeader(char*     design,
                  uint      ii,
                  uint      jj,
                  char*     defFile,
                  uint      units,
                  uint      defUnits,
                  Ath__box* bb);
  void  defEnd(FILE* fp);
  char* makeFullDir(uint ii, uint jj);
  void  setTileNetTable(Ath__netTable* tablePtr);
  uint  defIOtilePins(FILE* fp, uint ii, uint jj, uint defUnits, bool gravity);
  uint  qPinCnt(uint ii, uint jj);
  void  getBBox(Ath__box* bb, uint row, uint col);
  uint  defBlockages(FILE*     fp,
                     uint      defUnits,
                     Ath__box* qBB,
                     Ath__box* defBB,
                     Ath__zui* zui = NULL);
  void  defBlockage(FILE* fp,
                    uint  defUnits,
                    uint  levelCnt,
                    int   x1,
                    int   y1,
                    int   x2,
                    int   y2);
  uint  getBusShapes(Ath__zui* zui);
  uint  getBusObs(uint ii, uint jj, Ath__zui* zui);

  void placePin(Ath__qPin* pin, uint width);
  uint findPinTargets(uint ii, uint jj, Ath__qPin* srcPin, Ath__box* bb);

  Ath__wire* getWire(uint id);
  Ath__qPin* getPin(uint id);
  Ath__qPin* getPinFromBox(uint id);
  uint       getTilePins(Ath__zui* zui);
  uint       getTilePins_1(Ath__zui* zui);
  void       startQpinIterator(uint ii, uint jj);
  Ath__qPin* getNextSrcPin_next();
  Ath__qPin* getNextSrcPin_thru();
  Ath__qPin* getNextSrcPin_corner();
  Ath__qPin* getNextSrcPin_all();

  Ath__box* allocPinBox();

  uint          groupConnections(dbBlock* block);
  Ath__grid**** initGridTable(dbTechLayer** layerTable,
                              uint          dirCnt,
                              uint          schema = 1);
  void          destroyGridTable(Ath__grid**** gTable, uint dirCnt);
  void          destroyGridTables();
  void          initWireGridPools(uint trackInit, uint wireInit);
  void          destroyWireGridPools();
  uint          blockQuadTracks(dbBlock* block, Ath__gridTable* wireSearch);
  uint blockTracks(dbBlock* block, Ath__grid* g, Ath__array1D<uint>* idTable);

  void assignResourcesThru(Ath__p2pConn* p2p, uint level);
  void assignTracksForThru(uint dist2, uint dist1, uint dir, uint level);
  void assignTracksForThru(uint metH, uint metV);
  void setFreeTracks(uint dir);
  void initBusTable(dbBox* blkBB, dbTech* tech, uint trackInit, uint wireInit);
  uint getDir(dbTechLayer* layer);
  void addTileBlockages(dbBlock* block, dbTech* tech);
  uint makePinTargetsStraight(uint       row,
                              uint       col,
                              uint*      met,
                              Ath__qPin* srcPin,
                              int*       xlo = NULL,
                              int*       ylo = NULL,
                              int*       xhi = NULL,
                              int*       yhi = NULL);

  uint getTileBoxes(Ath__zui* zui, uint layer, uint w, bool flag = false);
  uint getAllTileBoxes(Ath__zui* zui, uint layer, uint w);
  uint getTileBoundLines(Ath__zui* zui, uint layer, uint w, bool ignore);
  uint getTileBuses_1(Ath__zui* zui);
  uint placePins_next(uint row, uint col);
  uint placePins_thru(uint row, uint col);
  int  preplacePin_thru(Ath__qPin* srcPin, uint row, uint col, uint level);
  uint makeTopBuses_thru(uint opt, uint space, uint width);
  uint makeTopBuses_next(uint opt, uint space, uint width);
  uint getTilePins_unplaced(Ath__zui* zui, Ath__array1D<uint>* idtable);
  uint getTilePins_unplaced(Ath__zui*           zui,
                            Ath__array1D<uint>* idtable,
                            Ath__qPin* (*func)());
  uint getTilePins_placed(Ath__searchBox*     bb,
                          Ath__zui*           zui,
                          Ath__array1D<uint>* tileIdTable);

  bool updateCornerResources(uint row, uint col);
  bool getCornerResources(uint  r1,
                          uint  c1,
                          uint  r2,
                          uint  c2,
                          uint* midRow,
                          uint* midCol,
                          uint* turn);

  uint assignTracksDist2(uint dist, uint minDist, uint metH, uint metV);
  int  preplacePin_corner(Ath__qPin* srcPin, uint row, uint col);
  uint placePins_corner(uint row, uint col, uint dist);

  int getTileXlo(uint row, uint col);
  int getTileXlo(uint col);
  int getTileXhi(uint row, uint col);
  int getTileXhi(uint col);
  int getTileYlo(uint row, uint col);
  int getTileYlo(uint row);
  int getTileYhi(uint row, uint col);
  int getTileYhi(uint row);

  void pinToTurnPathH(Ath__qPin*      srcPin,
                      Ath__qPinTable* pinTable,
                      uint            dist,
                      uint            type,
                      int             y1,
                      int             x1,
                      int             x2,
                      int             x3,
                      int             x4);
  void pinToTurnPathV(Ath__qPin*      pin,
                      Ath__qPinTable* pinTable,
                      uint            dist,
                      uint            type,
                      int             x1,
                      int             y1,
                      int             y2,
                      int             y3,
                      int             y4);
  void turnToPinPathV(Ath__qPin*      pin,
                      Ath__qPinTable* pinTable,
                      uint            dist,
                      uint            type,
                      int             x,
                      int             y1,
                      int             y2,
                      int             y3,
                      int             y4);
  void turnToPinPathH(Ath__qPin*      pin,
                      Ath__qPinTable* pinTable,
                      uint            dist,
                      uint            type,
                      int             y,
                      int             x1,
                      int             x2,
                      int             x3,
                      int             x4);

  uint makeCornerPinShapes(uint row, uint col, uint dist);
  void getConnPoints(Ath__box* srcBox,
                     Ath__box* dstBox,
                     uint      fromRow,
                     uint      fromCol,
                     uint      toRow,
                     uint      toCol,
                     int*      x1,
                     int*      y1,
                     int*      x2,
                     int*      y2);

  uint getTilePins_unplaced_corner(Ath__zui* zui, Ath__array1D<uint>* idtable);
  uint assignTracks(uint metH, uint metV);
  Ath__box* addBusBox(int       x1,
                      int       y1,
                      int       x2,
                      int       y2,
                      uint      layer,
                      uint      netId,
                      Ath__box* next);
  int       placePinOnGrid(Ath__qPin*      pin,
                           Ath__searchBox* pinBB,
                           uint            row,
                           uint            col,
                           int             initTrack);
  uint      makeTopBuses_corner(uint opt, uint space, uint width);
  int       placeObsOnGrid(Ath__qPin*      pin,
                           Ath__searchBox* obsBB,
                           uint            row,
                           uint            col,
                           int             initTrack);
  int  placeObsOnGrids(Ath__qPin* pin, uint row, uint col, int initTrackNum);
  uint addQnet(uint          netId,
               Ath__connWord w,
               uint          n1,
               uint          cnt,
               uint          row,
               uint          col);

  void setPinNetConn(Ath__qPin* pin);

  bool     createTblocks(dbBlock*        block,
                         dbTech*         tech,
                         Ath__gridTable* dbNetWireSearch);
  dbBlock* createTblock(dbBlock* block, dbTech* tech, uint row, uint col);
  void     getPowerWireIds(int                 x1,
                           int                 y1,
                           int                 x2,
                           int                 y2,
                           Ath__gridTable*     dbNetWireSearch,
                           Ath__array1D<uint>* wireDbIdTable);
  bool     addPowerWires(uint                row,
                         uint                col,
                         dbBlock*            topBlock,
                         dbBlock*            tblock,
                         dbTech*             tech,
                         Ath__gridTable*     dbNetWireSearch,
                         Ath__array1D<uint>* wireDbIdTable);
  bool     addTracks(Ath__box* bb,
                     dbBlock*  topBlock,
                     dbBlock*  tblock,
                     dbTech*   tech);
};


