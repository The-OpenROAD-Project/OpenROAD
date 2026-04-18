// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2018-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <vector>

namespace odb {
class dbDatabase;
}

namespace sta {
class dbSta;
class Clock;
class Pin;
}  // namespace sta

namespace utl {
class Logger;
}

namespace gpl {

class NesterovBaseCommon;

// ClockBase builds a virtual clock tree model during global placement and
// sets per-sink clock insertion delays in STA to improve timing analysis.
// This enables timing-driven placement to account for expected clock skew.
class ClockBase
{
 public:
  ClockBase();
  ClockBase(sta::dbSta* sta, odb::dbDatabase* db, utl::Logger* log);
  ~ClockBase();

  // Execute virtual CTS: build a virtual clock tree model using current
  // placement locations and set clock insertion delays in STA.
  // Returns true if any insertions were applied.
  bool executeVirtualCts();

  // Remove all virtual clock insertion delays from STA.
  void removeVirtualCts();

  // Check whether virtual CTS should run at the current overflow level.
  bool isVirtualCtsOverflow(float overflow);

  void setVirtualCtsOverflows(const std::vector<int>& overflows);

  // Wire RC per DBU unit (seconds per DBU).
  // Default: 1.5e-16 s/dbu (~0.15 ps/um for typical clock routing).
  void setWireRcPerUnit(float rc) { wire_rc_per_unit_ = rc; }
  float getWireRcPerUnit() const { return wire_rc_per_unit_; }

  size_t getVirtualCtsOverflowSize() const;

 private:
  sta::dbSta* sta_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  utl::Logger* log_ = nullptr;

  // Wire RC coefficient: seconds per DBU of wire length.
  float wire_rc_per_unit_ = 1.5e-16f;

  // Overflow thresholds at which to trigger virtual CTS.
  std::vector<int> overflows_;
  std::vector<bool> overflow_done_;

  // Track virtual insertions set in STA so we can remove them later.
  struct VirtualInsert
  {
    const sta::Clock* clk;
    const sta::Pin* pin;  // nullptr = whole-clock insertion
  };
  std::vector<VirtualInsert> virtual_inserts_;

  // Build virtual clock tree for a single clock and set per-sink delays.
  void buildVirtualTreeForClock(const sta::Clock* clk);

  // Get the instance center (in DBU) for a given STA pin.
  // Returns false if the pin has no placement location.
  bool getPinLocation(const sta::Pin* pin, int& x, int& y) const;

  void initOverflowChk();
};

}  // namespace gpl
