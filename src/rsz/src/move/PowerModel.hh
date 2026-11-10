// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "sta/NetworkClass.hh"

namespace sta {
class LibertyCell;
class Pvt;
class Scene;
}  // namespace sta

namespace rsz {

class Resizer;

// The power a cell swap saves, at the STA activities (propagated, or from
// SAIF).  The leakage and internal power of each instance come from STA once,
// before any swap, with the activities: swaps keep the logic, so the
// activities do not change, but each commit would make STA propagate them
// again.  A swap scales that power by the leakage and table-energy ratio of
// the new cell; the switching power of the fanin nets changes with the input
// pin capacitances.
class PowerModel
{
 public:
  explicit PowerModel(Resizer& resizer);

  // Read the STA power and pin activities of every instance.
  void init();
  // Forget the slews and loads read for the last batch.
  void clear() { insts_.clear(); }
  // Main thread: read what gain() needs for inst.
  void prepare(const sta::Instance* inst);
  // Thread safe once inst is prepared: the watts saved by swapping inst to
  // cell, which may be negative; zero for an instance without STA power.
  float gain(const sta::Instance* inst, sta::LibertyCell* cell) const;

 private:
  struct PinInput
  {
    std::string port;
    bool output;
    // Transitions per second.
    float density;
    // Slew at an input pin, by rise/fall index.
    float slews[2];
    // Capacitance an output pin drives.
    float load_cap;
  };
  // The STA power at init() and the cell it was read with.
  struct Anchor
  {
    sta::LibertyCell* cell;
    float leakage;
    float internal;
    std::vector<std::pair<std::string, float>> densities;
  };
  struct InstInput
  {
    const Anchor* anchor;
    sta::LibertyCell* cell;
    float voltage;
    std::vector<PinInput> pins;
  };

  // The instance's table energy per second with cell, from its pin
  // activities, slews and loads.
  float energy(const InstInput& inst, sta::LibertyCell* cell) const;
  const PinInput* findPin(const InstInput& inst, const std::string& port) const;

  Resizer& resizer_;
  const sta::Scene* scene_{nullptr};
  const sta::Pvt* pvt_{nullptr};
  std::unordered_map<const sta::Instance*, Anchor> anchors_;
  std::unordered_map<const sta::Instance*, InstInput> insts_;
};

}  // namespace rsz
