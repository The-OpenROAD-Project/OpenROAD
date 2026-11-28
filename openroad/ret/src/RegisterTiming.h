// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>

namespace sta {
class dbSta;
class Instance;
class LibertyCell;
class LibertyPort;
class Pin;
class Pvt;
class TimingArcSet;
}  // namespace sta

namespace ret {

// The instance's PVT, then the SDC operating conditions, then the library
// default; nullptr is nominal.
const sta::Pvt* tablePvt(sta::dbSta* sta,
                         const sta::LibertyCell* cell,
                         const sta::Instance* inst = nullptr);

struct TableArc
{
  float delay;
  float slew;  // at the arc's output
};
// slowest, replaced by each arc of the set with a delay table that is slower,
// or that comes first when slowest is nullopt.
std::optional<TableArc> slowestTableArc(sta::dbSta* sta,
                                        const sta::TimingArcSet* set,
                                        const sta::Pvt* pvt,
                                        float in_slew,
                                        float load,
                                        std::optional<TableArc> slowest);

// to: one output, or nullptr for the slowest.  Negative: the cell has none.
float registerClockToQ(sta::dbSta* sta,
                       const sta::LibertyCell* cell,
                       const sta::LibertyPort* to,
                       float clock_slew,
                       float load);
float registerSetupTime(sta::dbSta* sta,
                        const sta::LibertyCell* cell,
                        float clock_slew,
                        float data_slew);
// The slower of the rise and fall slews.
float pinSlew(sta::dbSta* sta, const sta::Pin* pin);
// The ideal clock's slew where the clock is ideal.
float clockPinSlew(sta::dbSta* sta, const sta::Pin* pin);

}  // namespace ret
