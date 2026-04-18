// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#include "clockBase.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Sta.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace gpl {

using utl::GPL;

ClockBase::ClockBase() = default;

ClockBase::ClockBase(sta::dbSta* sta, odb::dbDatabase* db, utl::Logger* log)
    : sta_(sta), db_(db), log_(log)
{
}

ClockBase::~ClockBase() = default;

void ClockBase::initOverflowChk()
{
  overflow_done_.assign(overflows_.size(), false);
}

void ClockBase::setVirtualCtsOverflows(const std::vector<int>& overflows)
{
  overflows_ = overflows;
  std::sort(overflows_.begin(), overflows_.end(), std::greater<int>());
  initOverflowChk();
}

size_t ClockBase::getVirtualCtsOverflowSize() const
{
  return overflows_.size();
}

bool ClockBase::isVirtualCtsOverflow(float overflow)
{
  if (overflows_.empty()) {
    return false;
  }

  const int int_overflow = static_cast<int>(std::round(overflow * 100));

  // Trigger once for each threshold when overflow drops below it,
  // matching the same pattern used in TimingBase.
  if (int_overflow > overflows_[0]) {
    return false;
  }

  bool needs_run = false;
  for (size_t i = 0; i < overflows_.size(); ++i) {
    if (overflows_[i] > int_overflow) {
      if (!overflow_done_[i]) {
        overflow_done_[i] = true;
        needs_run = true;
      }
      continue;
    }
    return needs_run;
  }
  return needs_run;
}

bool ClockBase::executeVirtualCts()
{
  if (!sta_ || !db_) {
    return false;
  }

  // Remove any previous virtual insertions before building a fresh model.
  removeVirtualCts();

  const sta::ClockSeq& clocks = sta_->cmdSdc()->clocks();
  if (clocks.empty()) {
    log_->warn(GPL, 160, "Virtual CTS: no clocks defined in design. Skipping.");
    return false;
  }

  int total_insertions = 0;
  for (const sta::Clock* clk : clocks) {
    const size_t before = virtual_inserts_.size();
    buildVirtualTreeForClock(clk);
    total_insertions += static_cast<int>(virtual_inserts_.size() - before);
  }

  if (total_insertions == 0) {
    log_->warn(
        GPL, 161, "Virtual CTS: no register clock pins found. Skipping.");
    return false;
  }

  log_->info(GPL,
             162,
             "Virtual CTS: set {} virtual clock insertion delays.",
             total_insertions);
  return true;
}

void ClockBase::buildVirtualTreeForClock(const sta::Clock* clk)
{
  if (!clk) {
    return;
  }

  // Build a one-element ClockSet for the query.
  sta::ClockSet clk_set;
  clk_set.insert(const_cast<sta::Clock*>(clk));

  // Find all register clock sink pins for this clock.
  sta::PinSet sink_pins
      = sta_->findRegisterClkPins(&clk_set,
                                  sta::RiseFallBoth::riseFall(),
                                  /*registers=*/true,
                                  /*latches=*/true,
                                  sta_->cmdMode());

  if (sink_pins.empty()) {
    return;
  }

  // Collect sink positions.
  struct SinkInfo
  {
    const sta::Pin* pin;
    int x;
    int y;
  };
  std::vector<SinkInfo> sinks;
  sinks.reserve(sink_pins.size());

  for (const sta::Pin* pin : sink_pins) {
    int x = 0;
    int y = 0;
    if (getPinLocation(pin, x, y)) {
      sinks.push_back({pin, x, y});
    }
  }

  if (sinks.empty()) {
    return;
  }

  // Compute centroid of all sink positions.  In a balanced clock tree the
  // driver aims for the geometric center, so we use the centroid as a proxy
  // for the virtual clock tree root.
  double sum_x = 0.0;
  double sum_y = 0.0;
  for (const auto& s : sinks) {
    sum_x += s.x;
    sum_y += s.y;
  }
  const double centroid_x = sum_x / sinks.size();
  const double centroid_y = sum_y / sinks.size();

  // Estimate insertion delay for each sink as the Manhattan distance from the
  // centroid scaled by the wire RC coefficient.  This models a balanced
  // H-tree where sinks close to the center get lower insertion delay and
  // sinks far away get higher delay – capturing relative clock skew without
  // introducing a large uniform offset.
  sta::Sdc* sdc = sta_->cmdSdc();

  for (const auto& s : sinks) {
    const double dist = std::abs(s.x - centroid_x) + std::abs(s.y - centroid_y);
    const float delay = static_cast<float>(dist * wire_rc_per_unit_);

    sta_->setClockInsertion(clk,
                            s.pin,
                            sta::RiseFallBoth::riseFall(),
                            sta::MinMaxAll::all(),
                            sta::EarlyLateAll::all(),
                            delay,
                            sdc);
    virtual_inserts_.push_back({clk, s.pin});
  }
}

void ClockBase::removeVirtualCts()
{
  if (virtual_inserts_.empty()) {
    return;
  }

  sta::Sdc* sdc = sta_->cmdSdc();
  for (const auto& vi : virtual_inserts_) {
    sta_->removeClockInsertion(vi.clk, vi.pin, sdc);
  }
  virtual_inserts_.clear();

  log_->info(GPL, 163, "Virtual CTS: removed virtual clock insertion delays.");
}

bool ClockBase::getPinLocation(const sta::Pin* pin, int& x, int& y) const
{
  if (!pin) {
    return false;
  }

  sta::dbNetwork* network = sta_->getDbNetwork();

  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
  odb::dbModITerm* moditerm = nullptr;
  network->staToDb(pin, iterm, bterm, moditerm);

  if (iterm) {
    odb::dbInst* inst = iterm->getInst();
    if (!inst || !inst->isPlaced()) {
      return false;
    }
    inst->getLocation(x, y);
    return true;
  }

  if (bterm) {
    // Clock port on the block boundary – use its placement location.
    int px = 0;
    int py = 0;
    if (bterm->getFirstPinLocation(px, py)) {
      x = px;
      y = py;
      return true;
    }
  }

  return false;
}

}  // namespace gpl
