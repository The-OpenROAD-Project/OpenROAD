// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"

// Small synthetic designs whose good placement is known in advance, for
// global placement tests. The netlists are built directly in odb, so they
// need no LEF, DEF or liberty.
//
// Every design has the same core: kRows rows of kSites sites, with one row
// of margin on each side for the die. Cells are one row tall and cell_sites
// wide, with a pin at the middle of each side (W, E, S, N). IO pins sit on
// the die edge and are fixed.

namespace gpl {

class DesignBuilder
{
 public:
  static constexpr int kDbuPerMicron = 1000;
  static constexpr int kSiteWidth = 200;
  static constexpr int kRowHeight = 2000;
  static constexpr int kSites = 100;
  static constexpr int kRows = 10;
  // The core is 20000 x 20000.
  static constexpr int kCoreSize = kSites * kSiteWidth;

  explicit DesignBuilder(odb::dbDatabase* db, int cell_sites = 4);

  odb::dbBlock* block() const { return block_; }
  odb::Rect core() const { return block_->getCoreArea(); }
  odb::Rect die() const { return block_->getDieArea(); }

  // An unplaced movable cell.
  odb::dbInst* addCell(const std::string& name);

  // A fixed IO pin on the die edge, centered at fraction t (0 to 1) of the
  // core's extent along the edge, so it lines up with the rows or columns
  // of sites: left to right for the bottom and top edges, bottom to top for
  // the left and right ones.
  enum class Edge
  {
    kLeft,
    kRight,
    kBottom,
    kTop
  };
  odb::dbBTerm* addIo(const std::string& name, Edge edge, double t);

  // Connects the cell's pin ("W", "E", "S" or "N") to the named net,
  // creating the net if needed. Throws std::invalid_argument for an
  // unknown pin.
  void connect(odb::dbInst* inst, const char* pin, const std::string& net);
  // Connects an IO pin's net to the cell's pin.
  void connect(odb::dbInst* inst, const char* pin, odb::dbBTerm* io);

 private:
  odb::dbNet* net(const std::string& name);
  static odb::dbITerm* iterm(odb::dbInst* inst, const char* pin);

  odb::dbBlock* block_ = nullptr;
  odb::dbMaster* master_ = nullptr;
  odb::dbTechLayer* layer_ = nullptr;
};

// A chain of cells between an IO on the left die edge and one on the right,
// both at mid height: in -> c0 -> c1 -> ... -> out. Returns the cells in
// chain order. Throws std::invalid_argument if length < 1.
std::vector<odb::dbInst*> makeChain(DesignBuilder& builder, int length);

// A cols x rows grid of cells, each joined to its four neighbors (E to W,
// N to S). Each boundary cell's outward pin goes to an IO on that die edge,
// at the cell's position along it. Returns cells[i][j] for column i, row j.
std::vector<std::vector<odb::dbInst*>> makeLattice(DesignBuilder& builder,
                                                   int cols,
                                                   int rows);

// Cells on no nets.
std::vector<odb::dbInst*> makeUnconnected(DesignBuilder& builder, int count);

// Two side x side lattices with no nets between them, without boundary IOs.
// One IO on the left die edge anchors the first and one on the right edge
// the second, each at mid height.
struct TwoClusters
{
  std::vector<odb::dbInst*> left;
  std::vector<odb::dbInst*> right;
};
TwoClusters makeTwoClusters(DesignBuilder& builder, int side);

}  // namespace gpl
