// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pa/PatternGCContext.h"

#include <utility>
#include <vector>

namespace drt {
namespace {

void checkFresh(
    FlexGCWorker& worker,
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objects,
    const odb::Rect& extBox,
    const odb::Rect& drcBox)
{
  worker.setExtBox(extBox);
  worker.setDrcBox(drcBox);
  for (const auto& [object, owner] : objects) {
    worker.addPAObj(object, owner);
  }
  worker.initPA1();
  worker.main();
}

}  // namespace

FlexGCWorker& PatternGCContext::check(
    std::unique_ptr<FlexGCWorker> seed,
    const std::vector<std::pair<frConnFig*, frBlockObject*>>& objects,
    const odb::Rect& extBox,
    const odb::Rect& drcBox)
{
  for (std::size_t i = 0; i < workers_.size(); ++i) {
    auto& worker = *workers_[i];
    if (!worker.hasSamePAFixedGeometry(*seed)
        || !worker.replacePAObjects(objects)) {
      continue;
    }
    worker.setExtBox(extBox);
    worker.setDrcBox(drcBox);
    worker.main();
    auto used = std::move(workers_[i]);
    workers_.erase(workers_.begin() + i);
    workers_.insert(workers_.begin(), std::move(used));
    return *workers_.front();
  }
  checkFresh(*seed, objects, extBox, drcBox);
  if (workers_.size() == kMaxEntries) {
    workers_.pop_back();
  }
  workers_.insert(workers_.begin(), std::move(seed));
  return *workers_.front();
}

}  // namespace drt
