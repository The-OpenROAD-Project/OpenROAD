// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace utl {
class Logger;
}

#include "odb/db.h"
#include "odb/geom.h"

namespace dpl {

using odb::dbNet;

using utl::Logger;

class NetBox
{
 public:
  NetBox() = default;
  NetBox(dbNet* net, const odb::Rect& box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();
  bool isIgnore() const { return ignore_; }
  dbNet* getNet() const { return net_; }
  const odb::Rect& getBox() const { return box_; }

  void setBox(const odb::Rect& box) { box_ = box; }

 private:
  dbNet* net_ = nullptr;
  odb::Rect box_;
  odb::Rect box_saved_;
  bool ignore_ = false;
};

using NetBoxMap = std::unordered_map<dbNet*, NetBox>;
using NetBoxes = std::vector<NetBox*>;

class OptimizeMirroring
{
 public:
  OptimizeMirroring(Logger* logger, odb::dbDatabase* db);

  void run();

 private:
  int mirrorCandidates(std::vector<odb::dbInst*>& mirror_candidates);
  void findNetBoxes();
  std::vector<odb::dbInst*> findMirrorCandidates(NetBoxes& net_boxes);

  void updateNetBoxes(odb::dbInst* inst);
  void saveNetBoxes(odb::dbInst* inst);
  void restoreNetBoxes(odb::dbInst* inst);

  int64_t hpwl(odb::dbInst* inst);  // Sum of ITerm hpwl's.

  Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  NetBoxMap net_box_map_;

  // Net bounding box size on nets with more instance terminals
  // than this are ignored.
  static constexpr int mirror_max_iterm_count_ = 100;
};

}  // namespace dpl
