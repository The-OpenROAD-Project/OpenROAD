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

using odb::dbInst;
using odb::dbNet;
using odb::Rect;

using utl::Logger;

class NetBox
{
 public:
  NetBox() = default;
  NetBox(dbNet* net, const Rect& box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();
  bool isIgnore() const { return ignore_; }
  dbNet* getNet() const { return net_; }
  const Rect& getBox() const { return box_; }

  void setBox(const Rect& box) { box_ = box; }

 private:
  dbNet* net_ = nullptr;
  Rect box_;
  Rect box_saved_;
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
  int mirrorCandidates(std::vector<dbInst*>& mirror_candidates);
  void findNetBoxes();
  std::vector<dbInst*> findMirrorCandidates(NetBoxes& net_boxes);

  void updateNetBoxes(dbInst* inst);
  void saveNetBoxes(dbInst* inst);
  void restoreNetBoxes(dbInst* inst);

  int64_t hpwl(dbInst* inst);  // Sum of ITerm hpwl's.

  Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  NetBoxMap net_box_map_;

  // Net bounding box size on nets with more instance terminals
  // than this are ignored.
  static constexpr int mirror_max_iterm_count_ = 100;
};

}  // namespace dpl
