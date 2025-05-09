// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

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
  tmg_rc_sh(Rect rect,
            dbTechLayer* layer,
            dbTechVia* tech_via,
            dbVia* block_via,
            dbTechNonDefaultRule* rule = nullptr)
      : _rect(rect),
        _layer(layer),
        _tech_via(tech_via),
        _block_via(block_via),
        _rule(rule)
  {
  }

  const Rect& rect() const { return _rect; }
  int xMin() const { return _rect.xMin(); }
  int xMax() const { return _rect.xMax(); }
  int yMin() const { return _rect.yMin(); }
  int yMax() const { return _rect.yMax(); }
  uint getDX() const { return (_rect.xMax() - _rect.xMin()); }
  uint getDY() const { return (_rect.yMax() - _rect.yMin()); }

  bool isVia() const { return (_tech_via || _block_via); }
  dbTechVia* getTechVia() const { return _tech_via; }
  dbVia* getVia() const { return _block_via; }
  dbTechLayer* getTechLayer() const { return _layer; }
  dbTechNonDefaultRule* getRule() const { return _rule; }

  void setXmin(int x) { _rect.set_xlo(x); }
  void setXmax(int x) { _rect.set_xhi(x); }
  void setYmin(int y) { _rect.set_ylo(y); }
  void setYmax(int y) { _rect.set_yhi(y); }

 private:
  Rect _rect;
  dbTechLayer* _layer{nullptr};
  dbTechVia* _tech_via{nullptr};
  dbVia* _block_via{nullptr};
  dbTechNonDefaultRule* _rule{nullptr};
};

struct tmg_rc
{
  tmg_rc(const int from_idx,
         const int to_idx,
         const tmg_rc_sh& shape,
         const bool is_vertical,
         const int width,
         const int default_ext)
      : _from_idx(from_idx),
        _to_idx(to_idx),
        _shape(shape),
        _is_vertical(is_vertical),
        _width(width),
        _default_ext(default_ext)
  {
  }
  const int _from_idx;  // index to _ptV
  int _to_idx;
  tmg_rc_sh _shape;
  const bool _is_vertical;
  const int _width;
  const int _default_ext;
};

struct tmg_rcpt
{
  tmg_rcpt(int x, int y, dbTechLayer* layer) : _x(x), _y(y), _layer(layer) {}
  const int _x;  // nominal point
  const int _y;
  dbTechLayer* const _layer;
  int _tindex{-1};  // index to _termV
  tmg_rcpt* _next_for_term{nullptr};
  tmg_rcpt* _t_alt{nullptr};
  tmg_rcpt* _next_for_clear{nullptr};
  tmg_rcpt* _sring{nullptr};
  int _dbwire_id{-1};
  bool _fre{false};
  bool _jct{false};
  bool _pinpt{false};
  bool _c2pinpt{false};
};

struct tmg_rcterm
{
  tmg_rcterm(dbITerm* iterm) : _iterm(iterm), _bterm(nullptr) {}
  tmg_rcterm(dbBTerm* bterm) : _iterm(nullptr), _bterm(bterm) {}
  dbITerm* const _iterm;
  dbBTerm* const _bterm;
  tmg_rcpt* _pt;        // list of points
  tmg_rcpt* _first_pt;  // first point in dfs
};

struct tmg_rcshort
{
  tmg_rcshort(int i0, int i1) : _i0(i0), _i1(i1) {}
  const int _i0;
  const int _i1;
  bool _skip{false};
};

// This stores shapes by level through addShape.  Once all the shapes
// have been added then searchStart/Next can be used for querying.
// Internally a simple tree of space bisections is generated for
// efficiency.
//
// The code uses an odd convention:
// is_via = 0 ==> wire
//        = 1 ==> via
//        = 2 ==> pin
class tmg_conn_search
{
 public:
  tmg_conn_search();
  ~tmg_conn_search();
  void clear();
  void addShape(int level, const Rect& bounds, int is_via, int id);
  void searchStart(int level, const Rect& bounds, int is_via);
  bool searchNext(int* id);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

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
  const tmg_rcpt& pt(const int index) const { return _ptV[index]; }
  void checkConnOrdered();

 private:
  tmg_rcpt& pt(const int index) { return _ptV[index]; }
  void splitTtop();
  void splitBySj(int j, int rt, int sjxMin, int sjyMin, int sjxMax, int sjyMax);
  void findConnections();
  void removeShortLoops();
  void removeWireLoops();
  void treeReorder(bool no_convert);
  bool checkConnected();
  void checkVisited();
  tmg_rcpt* allocPt(int x, int y, dbTechLayer* layer);
  void addRc(const dbShape& s,
             int from_idx,
             int to_idx,
             dbTechNonDefaultRule* rule = nullptr);
  void addRc(int k,
             const tmg_rc_sh& s,
             int from_idx,
             int to_idx,
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
  int isVisited(int j) const;
  void addToWire(int fr, int to, int k, bool is_short, bool is_loop);
  int getExtension(int ipt, const tmg_rc* rc);
  int addPoint(int ipt, const tmg_rc* rc);
  int addPoint(int from_idx, int ipt, const tmg_rc* rc);
  int addPointIfExt(int ipt, const tmg_rc* rc);
  tmg_rc* addRcPatch(int from_idx, int to_idx);
  int getDisconnectedStart();
  void copyWireIdToVisitedShorts(int j);

  int _slicedTilePinCnt;
  int _stbtx1[200];
  int _stbty1[200];
  int _stbtx2[200];
  int _stbty2[200];
  dbBTerm* _slicedTileBTerm[200];
  std::unique_ptr<tmg_conn_search> _search;
  std::unique_ptr<tmg_conn_graph> _graph;
  std::vector<tmg_rc> _rcV;
  std::vector<tmg_rcpt> _ptV;
  std::vector<tmg_rcterm> _termV;
  std::vector<tmg_rcterm*> _tstackV;
  std::vector<tmg_rcshort> _shortV;
  dbNet* _net;
  bool _hasSWire;
  bool _connected;
  dbWireEncoder _encoder;
  dbWire* _newWire;
  dbTechNonDefaultRule* _net_rule;
  dbTechNonDefaultRule* _path_rule;
  bool _need_short_wire_id;
  std::vector<std::array<tmg_connect_shape, 32>> _csVV;
  std::array<tmg_connect_shape, 32>* _csV;
  std::vector<int> _csNV;
  int _csN;
  tmg_rcpt* _first_for_clear;

  int _last_id;
  int _firstSegmentAfterVia;
  utl::Logger* logger_;
};

}  // namespace odb
