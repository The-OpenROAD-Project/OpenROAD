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

#include <array>
#include <memory>
#include <vector>

#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace odb {

class tmg_rc_sh
{
 public:
  Rect _rect;
  dbTechLayer* _layer;
  dbTechVia* _tech_via;
  dbVia* _block_via;
  dbTechNonDefaultRule* _rule;
  Rect _via_lower_rect;
  Rect _via_upper_rect;

 public:
  const Rect& rect() const { return _rect; }
  int xMin() const { return _rect.xMin(); }
  int xMax() const { return _rect.xMax(); }
  int yMin() const { return _rect.yMin(); }
  int yMax() const { return _rect.yMax(); }
  bool isVia() const { return (_tech_via || _block_via); }
  dbTechVia* getTechVia() const { return _tech_via; }
  dbVia* getVia() const { return _block_via; }
  dbTechLayer* getTechLayer() const { return _layer; }
  uint getDX() const { return (_rect.xMax() - _rect.xMin()); }
  uint getDY() const { return (_rect.yMax() - _rect.yMin()); }
  void setXmin(int x) { _rect.set_xlo(x); }
  void setXmax(int x) { _rect.set_xhi(x); }
  void setYmin(int y) { _rect.set_ylo(y); }
  void setYmax(int y) { _rect.set_yhi(y); }
};

struct tmg_rc
{
  int _ifr;  // index to _ptV
  int _ito;
  tmg_rc_sh _shape;
  int _vert;
  int _width;
  int _default_ext;
};

struct tmg_rcpt
{
  tmg_rcpt();
  int _x;  // nominal point
  int _y;
  dbTechLayer* _layer;
  int _tindex;  // index to _termV
  tmg_rcpt* _next_for_term;
  tmg_rcpt* _t_alt;
  tmg_rcpt* _next_for_clear;
  tmg_rcpt* _sring;
  int _dbwire_id;
  uint _fre : 1;
  uint _jct : 1;
  uint _pinpt : 1;
  uint _c2pinpt : 1;
};

struct tmg_rcterm
{
  dbITerm* _iterm;
  dbBTerm* _bterm;
  tmg_rcpt* _pt;        // list of points
  tmg_rcpt* _first_pt;  // first point in dfs
};

struct tmg_rcshort
{
  int _i0;
  int _i1;
  bool _skip;
};

class tmg_conn_search;
class tmg_conn_graph;
struct tmg_connect_shape
{
  int k;
  Rect rect;
  int rtlev;
};

class tmg_conn
{
 public:
  tmg_conn(utl::Logger* logger);
  ~tmg_conn();
  void analyzeNet(dbNet* net);
  void loadNet(dbNet* net);
  void loadWire(dbWire* wire);
  void loadSWire(dbNet* net);
  bool isConnected() { return _connected; }
  int ptDist(int fr, int to) const;

  void checkConnOrdered();

 private:
  void splitTtop();
  void splitBySj(int j,
                 const tmg_rc_sh* sj,
                 int rt,
                 int sjxMin,
                 int sjyMin,
                 int sjxMax,
                 int sjyMax);
  void findConnections();
  void removeShortLoops();
  void removeWireLoops();
  void treeReorder(bool no_convert);
  bool checkConnected();
  void checkVisited();
  tmg_rcpt* allocPt();
  void addRc(const dbShape& s, int ifr, int ito);
  void addRc(int k,
             const tmg_rc_sh& s,
             int ifr,
             int ito,
             int xmin,
             int ymin,
             int xmax,
             int ymax);
  void addITerm(dbITerm* iterm);
  void addBTerm(dbBTerm* bterm);
  void connectShapes(int j, int k);
  void connectTerm(int j, bool soft);
  void connectTermSoft(int j, int rt, Rect& rect, int k);
  void addShort(int i0, int i1);
  void relocateShorts();
  void setSring();
  void detachTilePins();
  void getBTermSearchBox(dbBTerm* bterm, dbShape& pin, Rect& rect);

  int getStartNode();
  void dfsClear();
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);
  int isVisited(int j);
  void addToWire(int fr, int to, int k, bool is_short, bool is_loop);
  int getExtension(int ipt, const tmg_rc* rc);
  int addPoint(int ipt, const tmg_rc* rc);
  int addPoint(int ifr, int ipt, const tmg_rc* rc);
  int addPointIfExt(int ipt, const tmg_rc* rc);
  tmg_rc* addRcPatch(int ifr, int ito);
  int getDisconnectedStart();
  void copyWireIdToVisitedShorts(int j);

  int _slicedTilePinCnt;
  int _stbtx1[200];
  int _stbty1[200];
  int _stbtx2[200];
  int _stbty2[200];
  dbBTerm* _slicedTileBTerm[200];
  tmg_conn_search* _search;
  tmg_conn_graph* _graph;
  std::vector<tmg_rc> _rcV;
  std::vector<tmg_rcpt> _ptV;
  tmg_rcterm* _termV;
  tmg_rcterm** _tstackV;
  int _termN;
  tmg_rcshort* _shortV;
  int _shortN;
  dbNet* _net;
  bool _hasSWire;
  bool _preserveSWire;
  int _swireNetCnt;
  bool _connected;
  dbWireEncoder _encoder;
  dbWire* _newWire;
  dbTechNonDefaultRule* _net_rule;
  dbTechNonDefaultRule* _path_rule;
  int _misc_cnt;
  int _cut_end_extMin;
  int _need_short_wire_id;
  std::vector<std::array<tmg_connect_shape, 32>> _csVV;
  std::array<tmg_connect_shape, 32>* _csV;
  int* _csNV;
  int _csN;
  tmg_rcpt* _first_for_clear;

  int _termNmax;
  int _shortNmax;
  int _last_id;
  int _firstSegmentAfterVia;
  utl::Logger* logger_;
  friend class tmg_conn_graph;
};

class tmg_conn_search
{
 public:
  tmg_conn_search();
  ~tmg_conn_search();
  void clear();
  void addShape(int lev, const Rect& bounds, int isVia, int id);
  void searchStart(int lev, const Rect& bounds, int isVia);
  bool searchNext(int* id);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace odb
