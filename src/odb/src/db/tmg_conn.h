// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>
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
      : rect_(rect),
        layer_(layer),
        tech_via_(tech_via),
        block_via_(block_via),
        rule_(rule)
  {
  }

  const Rect& rect() const { return rect_; }
  int xMin() const { return rect_.xMin(); }
  int xMax() const { return rect_.xMax(); }
  int yMin() const { return rect_.yMin(); }
  int yMax() const { return rect_.yMax(); }
  uint32_t getDX() const { return (rect_.xMax() - rect_.xMin()); }
  uint32_t getDY() const { return (rect_.yMax() - rect_.yMin()); }

  bool isVia() const { return (tech_via_ || block_via_); }
  dbTechVia* getTechVia() const { return tech_via_; }
  dbVia* getVia() const { return block_via_; }
  dbTechLayer* getTechLayer() const { return layer_; }
  dbTechNonDefaultRule* getRule() const { return rule_; }

  void setXmin(int x) { rect_.set_xlo(x); }
  void setXmax(int x) { rect_.set_xhi(x); }
  void setYmin(int y) { rect_.set_ylo(y); }
  void setYmax(int y) { rect_.set_yhi(y); }

 private:
  Rect rect_;
  dbTechLayer* layer_{nullptr};
  dbTechVia* tech_via_{nullptr};
  dbVia* block_via_{nullptr};
  dbTechNonDefaultRule* rule_{nullptr};
};

struct tmg_rc
{
  tmg_rc(const int from_idx,
         const int to_idx,
         const tmg_rc_sh& shape,
         const bool is_vertical,
         const int width,
         const int default_ext)
      : from_idx(from_idx),
        to_idx(to_idx),
        shape(shape),
        is_vertical(is_vertical),
        width(width),
        default_ext(default_ext)
  {
  }
  const int from_idx;  // index to _ptV
  int to_idx;
  tmg_rc_sh shape;
  const bool is_vertical;
  const int width;
  const int default_ext;
};

struct tmg_rcpt
{
  tmg_rcpt(int x, int y, dbTechLayer* layer) : x(x), y(y), layer(layer) {}
  const int x;  // nominal point
  const int y;
  dbTechLayer* const layer;
  int tindex{-1};  // index to _termV
  tmg_rcpt* next_for_term{nullptr};
  tmg_rcpt* t_alt{nullptr};
  tmg_rcpt* next_for_clear{nullptr};
  tmg_rcpt* sring{nullptr};
  int dbwire_id{-1};
  bool fre{false};
  bool jct{false};
  bool pinpt{false};
  bool c2pinpt{false};
};

struct tmg_rcterm
{
  tmg_rcterm(dbITerm* iterm) : iterm(iterm), bterm(nullptr) {}
  tmg_rcterm(dbBTerm* bterm) : iterm(nullptr), bterm(bterm) {}
  dbITerm* const iterm;
  dbBTerm* const bterm;
  tmg_rcpt* pt;        // list of points
  tmg_rcpt* first_pt;  // first point in dfs
};

struct tmg_rcshort
{
  tmg_rcshort(int i0, int i1) : i0(i0), i1(i1) {}
  const int i0;
  const int i1;
  bool skip{false};
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
  bool isConnected() { return connected_; }
  int ptDist(int fr, int to) const;
  const tmg_rcpt& pt(const int index) const { return ptV_[index]; }
  void checkConnOrdered();

 private:
  tmg_rcpt& pt(const int index) { return ptV_[index]; }
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

  int slicedTilePinCnt_;
  int stbtx1_[200];
  int stbty1_[200];
  int stbtx2_[200];
  int stbty2_[200];
  dbBTerm* slicedTileBTerm_[200];
  std::unique_ptr<tmg_conn_search> search_;
  std::unique_ptr<tmg_conn_graph> graph_;
  std::vector<tmg_rc> rcV_;
  std::vector<tmg_rcpt> ptV_;
  std::vector<tmg_rcterm> termV_;
  std::vector<tmg_rcterm*> tstackV_;
  std::vector<tmg_rcshort> shortV_;
  dbNet* net_;
  bool hasSWire_;
  bool connected_;
  dbWireEncoder encoder_;
  dbWire* newWire_;
  dbTechNonDefaultRule* net_rule_;
  dbTechNonDefaultRule* path_rule_;
  bool need_short_wire_id_;
  std::vector<std::array<tmg_connect_shape, 32>> csVV_;
  std::array<tmg_connect_shape, 32>* csV_;
  std::vector<int> csNV_;
  int csN_;
  tmg_rcpt* first_for_clear_;

  int last_id_;
  int firstSegmentAfterVia_;
  utl::Logger* logger_;
};

}  // namespace odb
