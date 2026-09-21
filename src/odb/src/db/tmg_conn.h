// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "odb/db.h"
#include "odb/dbWireCodec.h"
#include "odb/geom.h"

namespace odb {

struct CandidateSection;

inline constexpr int kMaxCandidateSections = 32;

using CandidateSections = std::array<CandidateSection, kMaxCandidateSections>;

struct WireSection
{
  class Shape
  {
   public:
    Shape(Rect rect,
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

  WireSection(const int from_idx,
              const int to_idx,
              const Shape& shape,
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

  const int from_idx;  // index to wire_points_
  int to_idx;
  Shape shape;
  const bool is_vertical;
  const int width;
  const int default_ext;
};

struct WirePoint
{
  WirePoint(int x, int y, dbTechLayer* layer) : x(x), y(y), layer(layer) {}

  const int x;  // nominal point
  const int y;
  dbTechLayer* const layer;
  int tindex{-1};  // index to terminals_
  WirePoint* next_for_term{nullptr};
  WirePoint* t_alt{nullptr};
  WirePoint* next_for_clear{nullptr};
  WirePoint* sring{nullptr};
  int dbwire_id{-1};
  bool fre{false};
  bool jct{false};
  bool pinpt{false};
  bool c2pinpt{false};
};

struct Terminal
{
  Terminal(dbITerm* iterm) : iterm(iterm) {}
  Terminal(dbBTerm* bterm) : bterm(bterm) {}

  dbITerm* const iterm{nullptr};
  dbBTerm* const bterm{nullptr};

  WirePoint* pt{nullptr};        // list of points
  WirePoint* first_pt{nullptr};  // first point in dfs

  // Only for bterms: the part of the bpin that sticks out past an overlapping
  // iterm. This is what will actually be used when we search for candidate
  // sections instead of the whole bpin geometry.
  std::optional<Rect> sliced_bpin_box;
};

// This is how we keep the information that two sections from different paths
// are touching each other: two points, one from each section. Each point is
// the end of the section that is closest to where the two touch.
// Usually, the two points are the same spot, but not necessarily.
struct Short
{
  Short(int i0, int i1) : i0(i0), i1(i1) {}

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
class ShapeSearch
{
 public:
  ShapeSearch();
  ~ShapeSearch();

  void clear();
  void addShape(int level, const Rect& bounds, int is_via, int id);
  void searchStart(int level, const Rect& bounds, int is_via);
  bool searchNext(int* id);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class ConnectionGraph;

// A wire section whose geometry is touching a terminal's shape, so it
// is considered a candidate to possess the wire point that represents
// the connection of the wire with that terminal.
struct CandidateSection
{
  int index;
  int routing_level;  // From the database.
  Rect terminal_box;
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
  int distance(int fr, int to) const;
  const WirePoint& wirePoint(const int point_index) const
  {
    return wire_points_[point_index];
  }
  void checkConnOrdered();

 private:
  WirePoint& wirePoint(const int point_index)
  {
    return wire_points_[point_index];
  }
  void splitTtop();
  void splitBySj(int j, int rt, int sjxMin, int sjyMin, int sjxMax, int sjyMax);
  void identifyShorts();
  void removeShortLoops();
  void removeWireLoops();
  void identifyTerminalWirePoints();
  void treeReorder(bool no_convert);
  bool checkConnected();
  void checkVisited();
  WirePoint* addWirePoint(int x, int y, dbTechLayer* layer);
  void addWireSection(const dbShape& s,
                      int from_idx,
                      int to_idx,
                      dbTechNonDefaultRule* rule = nullptr);
  void addWireSection(int k,
                      const WireSection::Shape& s,
                      int from_idx,
                      int to_idx,
                      int xmin,
                      int ymin,
                      int xmax,
                      int ymax);
  void addITerm(dbITerm* iterm);
  void addBTerm(dbBTerm* bterm);
  void connectShapes(int j, int k);
  void connectTerm(int terminal_index, bool soft);
  void connectTermSoft(int terminal_index, int rt, const Rect& rect, int k);
  void addShort(int i0, int i1);
  void relocateShorts();
  void setSring();
  void sliceBPinsOverlappingITerms();

  int getStartNode();
  void dfsClear();
  bool dfsStart(int& j);
  bool dfsNext(int* from, int* to, int* k, bool* is_short, bool* is_loop);
  int isVisited(int j) const;
  void addToWire(int fr, int to, int k, bool is_short, bool is_loop);
  int getExtension(int ipt, const WireSection* wire_section);
  int addPoint(int ipt, const WireSection* wire_section);
  int addPoint(int from_idx, int ipt, const WireSection* wire_section);
  int addPointIfExt(int ipt, const WireSection* wire_section);
  int getDisconnectedStart();
  void copyWireIdToVisitedShorts(int j);

  utl::Logger* logger_{nullptr};

  std::unique_ptr<ShapeSearch> shape_search_;
  std::unique_ptr<ConnectionGraph> connection_graph_;

  dbNet* net_{nullptr};
  bool has_special_wires_{false};

  // The description of the wire.
  std::vector<WireSection> wire_sections_;
  std::vector<WirePoint> wire_points_;
  std::vector<Terminal> terminals_;
  std::vector<Short> shorts_;

  // Used for determining the wire points that represent the connection
  // with terminals.
  std::vector<CandidateSections> candidate_sections_;
  std::vector<int> candidate_section_count_;
  WirePoint* first_for_clear_{nullptr};

  // Graph walk and writing of the new wire encoding.
  // Note that the restart terminals are also used in the section above,
  // during the connectivity check between the hard and soft passes.
  std::vector<Terminal*> restart_terminals_;
  int last_id_{-1};
  dbTechNonDefaultRule* net_rule_{nullptr};
  dbTechNonDefaultRule* path_rule_{nullptr};
  bool need_short_wire_id_{false};
  bool first_segment_after_via_{false};
  dbWireEncoder encoder_;
  dbWire* new_wire_{nullptr};

  // Post-process connectivity check.
  bool connected_{false};
};

}  // namespace odb
