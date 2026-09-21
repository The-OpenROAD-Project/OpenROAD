// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dpl {

class Grid;
class Network;
class Node;
class PlacementDRC;

class NetBox
{
 public:
  NetBox() = default;
  NetBox(odb::dbNet* net, const odb::Rect& box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();
  bool isIgnore() const { return ignore_; }
  odb::dbNet* getNet() const { return net_; }
  const odb::Rect& getBox() const { return box_; }

  void setBox(const odb::Rect& box) { box_ = box; }

 private:
  odb::dbNet* net_ = nullptr;
  odb::Rect box_;
  odb::Rect box_saved_;
  bool ignore_ = false;
};

using NetBoxMap = std::unordered_map<odb::dbNet*, NetBox>;
using NetBoxes = std::vector<NetBox*>;

class OptimizeMirroring
{
 public:
  // network/grid/drc_engine must be initialized from the block: they are used
  // to check the cell edge spacing rules of the mirrored instances.
  OptimizeMirroring(utl::Logger* logger,
                    odb::dbDatabase* db,
                    Network* network,
                    Grid* grid,
                    PlacementDRC* drc_engine);

  void run();

 private:
  int mirrorCandidates(std::vector<odb::dbInst*>& mirror_candidates);
  void findNetBoxes();
  std::vector<odb::dbInst*> findMirrorCandidates(NetBoxes& net_boxes);

  void updateNetBoxes(odb::dbInst* inst);
  void saveNetBoxes(odb::dbInst* inst);
  void restoreNetBoxes(odb::dbInst* inst);

  int64_t hpwl(odb::dbInst* inst);  // Sum of ITerm hpwl's.

  // Mirroring swaps the left/right cell edges, so the LEF58 cell edge
  // spacing rules have to be rechecked for the mirrored orientation.
  bool isEdgeSpacingLegal(const Node* cell,
                          const odb::dbOrientType& orient) const;

  utl::Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;
  Network* network_ = nullptr;
  Grid* grid_ = nullptr;
  PlacementDRC* drc_engine_ = nullptr;

  NetBoxMap net_box_map_;

  // Candidates rejected because mirroring would violate the cell edge
  // spacing rules.
  int edge_spacing_reject_count_ = 0;

  // Net bounding box size on nets with more instance terminals
  // than this are ignored.
  static constexpr uint32_t mirror_max_iterm_count_ = 100;
};

}  // namespace dpl
