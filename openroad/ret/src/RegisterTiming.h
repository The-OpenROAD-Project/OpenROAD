// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace sta {
class dbSta;
class LibertyCell;
class LibertyPort;
class Pin;
}  // namespace sta

namespace ret {

// From the liberty tables of a flop: the slowest clock-to-output arc at a
// load (`to`: one output, or null for all) and the longest setup check.
// Negative: the cell has none.
float registerClockToQ(sta::dbSta* sta,
                       sta::LibertyCell* cell,
                       const sta::LibertyPort* to,
                       float clock_slew,
                       float load);
float registerSetupTime(sta::dbSta* sta,
                        sta::LibertyCell* cell,
                        float clock_slew,
                        float data_slew);
// The slower of the rise and fall slews STA has at a pin.
float pinSlew(sta::dbSta* sta, const sta::Pin* pin);
// The ideal clock's slew where the clock is ideal, else the pin's.
float clockPinSlew(sta::dbSta* sta, const sta::Pin* pin);

}  // namespace ret
