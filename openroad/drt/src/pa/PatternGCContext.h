// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "gc/FlexGC.h"
#include "odb/geom.h"

namespace drt {

// One genPatterns operation owns the cache, including its reverse-order retry.
class PatternGCContext
{
 public:
  FlexGCWorker& check(
      std::unique_ptr<FlexGCWorker> seed,
      const std::vector<std::pair<frConnFig*, frBlockObject*>>& objects,
      const odb::Rect& extBox,
      const odb::Rect& drcBox);

 private:
  static constexpr std::size_t kMaxEntries = 8;
  // Workers own copied geometry in most-recently-used order.
  std::vector<std::unique_ptr<FlexGCWorker>> workers_;
};

}  // namespace drt
