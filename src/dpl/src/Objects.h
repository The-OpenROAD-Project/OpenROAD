// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include "dpl/Grid.h"
#include "dpl/Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

struct Edge
{
  Edge(unsigned int type, const odb::Rect& box)
      : edge_type_idx_(type), bbox_(box)
  {
  }
  unsigned int getEdgeType() const { return edge_type_idx_; }
  const odb::Rect& getBBox() const { return bbox_; }

 private:
  unsigned int edge_type_idx_;
  odb::Rect bbox_;
};

struct EdgeSpacingEntry
{
  EdgeSpacingEntry(const int spc_in,
                   const bool is_exact_in,
                   const bool except_abutted_in)
      : spc(spc_in), is_exact(is_exact_in), except_abutted(except_abutted_in)
  {
  }
  bool operator<(const EdgeSpacingEntry& rhs) const { return spc < rhs.spc; }
  int spc;
  bool is_exact;
  bool except_abutted;
};

struct Master
{
  bool is_multi_row = false;
  std::vector<Edge> edges_;
};

struct Cell : public GridNode
{
 public:
  // setters
  void setDbInst(dbInst* inst) { db_inst_ = inst; }
  void setLeft(DbuX x) { x_ = x; }
  void setBottom(DbuY y) { y_ = y; }
  void setOrient(const dbOrientType& in) { orient_ = in; }
  void setWidth(DbuX width) { width_ = width; }
  void setHeight(DbuY height) { height_ = height; }
  void setPlaced(bool in) { is_placed_ = in; }
  void setHold(bool in) { hold_ = in; }
  void setGroup(Group* in) { group_ = in; }
  void setRegion(Rect* in) { region_ = in; }
  void setMaster(Master* in) { master_ = in; }

  // getters
  const char* name() const;
  dbInst* getDbInst() const override { return db_inst_; }
  DbuX xMin() const override { return x_; }
  DbuY yMin() const override { return y_; }
  DbuX dx() const override { return width_; }
  DbuY dy() const override { return height_; }
  bool isPlaced() const override { return is_placed_; }
  bool isFixed() const override;
  bool isHybrid() const override;
  DbuX siteWidth() const override;

  dbOrientType getOrient() const { return orient_; }
  bool isHold() const { return hold_; }
  Group* getGroup() const { return group_; }
  Rect* getRegion() const { return region_; }
  Master* getMaster() const { return master_; }
  DbuX xMax() const { return x_ + width_; }
  bool inGroup() const { return group_ != nullptr; }
  int64_t area() const;
  bool isStdCell() const;
  bool isHybridParent() const;
  dbSite* getSite() const;
  bool isBlock() const;

 private:
  dbInst* db_inst_ = nullptr;
  DbuX x_{0};  // lower left wrt core DBU
  DbuY y_{0};
  dbOrientType orient_;
  DbuX width_{0};
  DbuY height_{0};
  bool is_placed_ = false;
  bool hold_ = false;
  Group* group_ = nullptr;
  Rect* region_ = nullptr;  // group rect
  Master* master_ = nullptr;
};

struct Group
{
  string name;
  vector<Rect> region_boundaries;
  vector<Cell*> cells_;
  Rect boundary;
  double util = 0.0;
};

}  // namespace dpl
