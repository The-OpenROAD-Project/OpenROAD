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

#include "array1.h"
#include "array2d.h"
#include "atypes.h"
#include "box.h"

class Ath__connWord
{
 protected:
  union
  {
    struct
    {
      uint _fromRow : 5;
      uint _fromCol : 5;
      uint _toRow : 5;
      uint _toCol : 5;
      uint _colCnt : 5;
      uint _straight : 1;
      uint _ordered : 1;
      uint _placed : 1;
      uint _dir : 1;
      uint _posFlow : 1;
      uint _corner : 2;
    } _conn;

    uint _all;
  };

 public:
  Ath__connWord(uint r1, uint c1);
  Ath__connWord(uint r1, uint c1, uint type);
  bool ordered();
  void orderByRow(uint r1, uint c1, uint r2, uint c2, uint colCnt);
  Ath__connWord(uint r1, uint c1, uint r2, uint c2, uint colCnt = 0);
  int  getStraight();
  int  getRowDist();
  int  getColDist();
  uint getDist();
  uint getMinDist();
  uint getMaxDist();
  uint getRowDistAbs();
  uint getSeg(uint* c1, uint* c2);
  uint getColDistAbs();
  Ath__connWord(uint v = 0);
  uint getAll();
  void setAll(uint v);
  uint getFromRowCol(uint* col);
  uint getToRowCol(uint* col);
  uint getFrom();
  uint getTo();
  uint set(uint x1, uint y1, uint x2, uint y2, uint colCnt = 0);
};

class Ath__qPin;

class Ath__p2pConn
{
 private:
  uint          _netId;  // TODO
  Ath__connWord _conn;   // TODO
  Ath__qPin*    _srcPin;
  Ath__p2pConn* _next;

  // Ath__box *_busList;

 public:
  // Ath__box *_targetObs;

  void       setNext(Ath__p2pConn* v);
  void       setPin(Ath__qPin* q);
  Ath__qPin* getSrcPin();
  // void addBusBox(Ath__box *bb);
  // Ath__box *getBusList();

  friend class Ath__connTable;
  friend class Ath__quadTable;
  friend class Ath__qPin;
};

class Ath__connTable
{
 private:
  uint                         _maxBankCnt;
  Ath__array2d<Ath__p2pConn*>* _straightTable[2];
  Ath__array2d<Ath__p2pConn*>* _nextTable[2];
  Ath__array2d<Ath__p2pConn*>* _cornerTable;
  Ath__array2d<Ath__p2pConn*>* _tmpTablePtr;
  Ath__array2d<Ath__p2pConn*>* _tmpArrayPtr;
  AthPool<Ath__p2pConn>*       _poolPtr;
  uint                         _tmpCurrentIndex[32];
  uint                         _tmpCnt[32];

  uint _nextSegCnt;
  uint _tileSize;
  uint _pinLength;

 public:
  Ath__connTable(AthPool<Ath__p2pConn>* pool,
                 uint                   n,
                 uint                   pinLength,
                 uint                   tileSize);
  Ath__p2pConn* addConn(uint netId, Ath__connWord w, uint dx = 0, uint dy = 0);
  uint          addStraightConn(uint dir, uint dist, Ath__p2pConn* p2p);
  uint          addCornerConn2(uint type, Ath__p2pConn* p2p, uint ii, uint jj);
  void          printConnStats(FILE* fp);
  uint          startNextIterator(uint dir, uint seg);
  uint          startCornerIterator(uint dist);
  uint          startStraightIterator(uint dir, uint dist);
  int           getNextConn(Ath__connWord* conn, uint* netId);
  Ath__p2pConn* getNextConn();
  Ath__array2d<Ath__p2pConn*>* startStraightArrayIterator(uint dir);
  bool getNextArrayConn(uint ii, Ath__connWord* conn, uint* netId);
  uint getSegmentIndex(uint dir, uint dx, uint dy);
};
class Ath__zui;
class Ath__searchBox;

class Ath__qPin
{
 public:
  uint          _netId;
  uint          _ioNetId;
  uint          _nameId;
  Ath__connWord _conn;
  Ath__qPin*    _next;
  Ath__p2pConn* _head;
  Ath__box*     _instBB;
  Ath__box*     _targetBB;
  Ath__box*     _portBB;

  Ath__box*
      _obsList;  // All target wire bboxes, NULL if straight wire and _src=0
                 // For corner nets boxes include everything to the turn
  Ath__box*
       _busList;  // All actual wire bboxes, NULL if straight wire and _src=0
  uint _portWireId;
  uint _src : 1;
  uint _tjunction : 2;
  uint _placed : 1;
  uint _fixed : 1;
  uint _targeted : 1;
  uint _assigned : 1;
  uint _layer : 5;

  friend class Ath__qPinTable;

  int        defX(uint defUnits);
  int        defY(uint defUnits);
  Ath__box*  getInstBox();
  Ath__box*  getBusList();
  void       setInstBox(Ath__box* bb);
  bool       isSrc();
  bool       isAssigned();
  bool       isTargeted();
  bool       isPlaced();
  void       setPlaced();
  void       setTargeted();
  Ath__qPin* getSrcPin();
  Ath__qPin* getDstPin();

  uint getToRowCol(uint* col);
  uint getTurnRowCol(uint* row, uint* col);
  int  getTurnDist(uint* row, uint* col, int* dir);

  void      reset();
  void      pinBoxDef(FILE* fp, char* layerName, char* orient, int defUnits);
  uint      makeZuiObject(Ath__zui* zui,
                          uint      width,
                          bool      actual   = false,
                          bool      instFlag = false);
  uint      makeBusZuiObject(Ath__zui* zui, uint width);
  void      getTargetBox(Ath__searchBox* bb);
  void      getObsBox(Ath__searchBox* bb);
  void      getNextObsBox(Ath__searchBox* sbb);
  void      addBusBox(Ath__box* bb);
  void      setPortWireId(uint id);
  uint      getLayer();
  Ath__box* getPortCoords(int* x1, int* y1, int* x2, int* y2, uint* layer);
  uint      makePinObsZui(Ath__zui* zui,
                          int       width,
                          int       x1,
                          int       y1,
                          int       x2,
                          int       y2,
                          int       layer);
};
class Ath__qNet
{
 private:
  uint          _id;
  uint          _origNetId;
  Ath__connWord _conn;
  Ath__qPin*    _qPin;

  uint _startTermIndex;
  uint _itermCnt;

  uint _startBTermIndex;
  uint _btermCnt;
  uint _row;
  uint _col;

 public:
  void updateBterms(uint n1, uint cnt);
  void setSrcPin(Ath__qPin* pin);

  friend class Ath__quadTable;
};

class Ath__qPinTable
{
 private:
  Ath__array1D<Ath__qPin*>* _table;
  AthPool<Ath__qPin>*       _pool;

  Ath__box*          _nextPinShape[2];
  Ath__box*          _straightPinShape[2];
  Ath__box*          _cornerPinShape[4];
  Ath__box*          _cornerObsShape[4];
  Ath__box*          _thruObsShape[2];
  AthPool<Ath__box>* _pinBoxPool;

  friend class Ath__quadTable;

 public:
  Ath__qPinTable(AthPool<Ath__qPin>* qPinPool,
                 AthPool<Ath__box>*  boxPool,
                 uint                n = 0);
  Ath__box* getHeadStraightPinShape(uint dir);
  Ath__box* getHeadNextPinShape(uint dir);
  Ath__box* getHeadCornerPinShape(uint type);
  Ath__box* getHeadCornerObsShape(uint type);
  Ath__box* getHeadObsShape(uint dir);
  void      freeBoxes(Ath__box* head);
  void      freeNextPinShapes();
  Ath__box* newPinBox(uint layer, int x1, int y1, int x2, int y2);
  Ath__box* addPinBox(Ath__box* e, Ath__box* head);
  Ath__box* addNextPinBox(uint netId,
                          uint dir,
                          uint layer,
                          int  x1,
                          int  y1,
                          int  x2,
                          int  y2);
  Ath__box* addThruObsBox(uint netId,
                          uint dir,
                          uint layer,
                          int  x1,
                          int  y1,
                          int  x2,
                          int  y2);
  Ath__box* addCornerPinBox(uint netId,
                            uint type,
                            uint layer,
                            int  x1,
                            int  y1,
                            int  x2,
                            int  y2);
  Ath__box* addCornerObsBox(uint netId,
                            uint type,
                            uint layer,
                            int  x1,
                            int  y1,
                            int  x2,
                            int  y2);
  Ath__box* addStraightPinBox(uint netId,
                              uint dir,
                              uint layer,
                              int  x1,
                              int  y1,
                              int  x2,
                              int  y2);
  ~Ath__qPinTable();
  Ath__qPin* addPin(Ath__qPin*          next,
                    uint                netId,
                    uint                ioNetNameId,
                    Ath__connWord       w,
                    AthPool<Ath__qPin>* pool);
  uint       getCnt();
  Ath__qPin* get(uint ii);
  bool       startIterator();
  Ath__qPin* getNextSrcPin_next();
  Ath__qPin* getNextSrcPin_thru();
  Ath__qPin* getNextSrcPin_corner();
  Ath__qPin* getNextSrcPin_all();
};
class Ath__qBus
{
 private:
  Ath__array1D<Ath__p2pConn*>* _table;
  // AthPool<Ath__p2pConn> *_pool;

 public:
  Ath__qBus(uint n);
  void          addConn(Ath__p2pConn* conn);
  uint          getCnt();
  Ath__p2pConn* get(uint ii);
};


