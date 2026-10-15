// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <vector>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/geom.h"

namespace dpl {

// A uniform grid of buckets over the die holding the block's instances, so a
// query over a small region scans the instances near it instead of all of
// them.  odb callbacks keep it current, so it never hands back a stale
// location and never holds a pointer to a destroyed instance.
//
// Each instance sits in the single bucket holding its lower left corner.
// That only works if no instance is wider or taller than a bucket, so the
// ones that are -- macros, mostly -- go in a list scanned by every query
// instead.  Every other instance reaches at most one bucket past its own,
// which is what lets a query widen its bucket range by exactly one.
class InstanceIndex : public odb::dbBlockCallBackObj
{
 public:
  explicit InstanceIndex(odb::dbBlock* block);

  // Visits every instance that can reach region, each at most once.  Some
  // that turn out not to overlap it are visited too.
  void visit(const odb::Rect& region,
             const std::function<void(odb::dbInst* inst)>& visitor) const;

  // The instance is still intact at destroy time, so erase() can find the
  // bucket it went into.  Moves and master swaps bracket the change, so the
  // erase sees the old geometry and the insert the new.
  void inDbInstCreate(odb::dbInst* inst) override { insert(inst); }
  void inDbInstDestroy(odb::dbInst* inst) override { erase(inst); }
  void inDbPreMoveInst(odb::dbInst* inst) override { erase(inst); }
  void inDbPostMoveInst(odb::dbInst* inst) override { insert(inst); }
  void inDbInstSwapMasterBefore(odb::dbInst* inst, odb::dbMaster*) override
  {
    erase(inst);
  }
  void inDbInstSwapMasterAfter(odb::dbInst* inst) override { insert(inst); }

 private:
  // Where an instance went, remembered by instance id rather than recomputed
  // from its geometry, so an erase lands in the right place no matter what
  // moved in between.
  static constexpr int kAbsent = -1;
  static constexpr int kOversized = -2;

  int bucketIndex(const odb::Rect& bbox) const;
  void insert(odb::dbInst* inst);
  void erase(odb::dbInst* inst);
  int& slotOf(odb::dbInst* inst);

  odb::Rect extent_;
  int bucket_size_ = 1;
  int count_x_ = 1;
  int count_y_ = 1;
  std::vector<std::vector<odb::dbInst*>> buckets_;
  std::vector<odb::dbInst*> oversized_;
  std::vector<int> bucket_of_id_;
};

}  // namespace dpl
