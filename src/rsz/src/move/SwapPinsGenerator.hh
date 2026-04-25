// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"

namespace sta {
class FuncExpr;
class Instance;
class LibertyCell;
class LibertyPort;
class MinMax;
class Pin;
class Scene;
}  // namespace sta

namespace rsz {

// Finds functionally equivalent input pins and produces SwapPinsCandidates
// that move the critical signal onto a faster Liberty arc.
//
// Equivalence is determined by Boolean simulation of the cell's function
// expression (simulateExpr / equivCellPins).  For each equivalent pair
// the generator evaluates the arc delay under the current load cap and
// input slew; if swapping improves delay, a candidate is produced.
// Requests kArcDelayState from the prepare stage for prepared-target mode.
class SwapPinsGenerator : public MoveGenerator
{
 public:
  // === Construction =========================================================
  explicit SwapPinsGenerator(const GeneratorContext& context);

  // === MoveGenerator API ====================================================
  MoveType type() const override { return MoveType::kSwapPins; }
  PrepareCacheMask prepareRequirements() const override
  {
    return kArcDelayStateCache;
  }
  bool isApplicable(const Target& target) const override;
  std::vector<std::unique_ptr<MoveCandidate>> generate(
      const Target& target) override;

 private:
  // === Helper types =========================================================
  using LibertyPortVec = std::vector<sta::LibertyPort*>;
  using EquivPinMap = std::unordered_map<sta::LibertyPort*, LibertyPortVec>;

  // === Driver and candidate selection helpers ==============================
  bool resolveDriverContext(const Target& target,
                            sta::Pin*& drvr_pin,
                            sta::Instance*& drvr,
                            sta::LibertyPort*& drvr_port,
                            const sta::Scene*& scene,
                            const sta::MinMax*& min_max) const;
  std::vector<std::unique_ptr<MoveCandidate>> buildCandidates(
      const Target& target,
      std::optional<float> load_cap_override,
      float current_delay_override) const;
  bool loadInputPort(const Target& target, sta::LibertyPort*& input_port) const;
  bool selectSwapPort(sta::Instance* drvr,
                      sta::LibertyPort* drvr_port,
                      sta::LibertyPort* input_port,
                      const sta::Scene* scene,
                      const sta::MinMax* min_max,
                      float load_cap,
                      sta::LibertyPort*& swap_port,
                      float& current_delay,
                      float& swap_delay) const;

  // === Functional-equivalence helpers ======================================
  void equivCellPins(const sta::LibertyCell* cell,
                     sta::LibertyPort* input_port,
                     LibertyPortVec& ports) const;
  bool isPortEqiv(const sta::FuncExpr* expr,
                  const sta::LibertyCell* cell,
                  const sta::LibertyPort* port_a,
                  const sta::LibertyPort* port_b) const;
  bool simulateExpr(sta::FuncExpr* expr,
                    std::unordered_map<const sta::LibertyPort*,
                                       std::vector<bool>>& port_stimulus,
                    size_t table_index) const;
  std::vector<bool> simulateExpr(
      sta::FuncExpr* expr,
      std::unordered_map<const sta::LibertyPort*, std::vector<bool>>&
          port_stimulus) const;

  // === Functional-equivalence cache ========================================
  mutable EquivPinMap equiv_pin_map_;
};

}  // namespace rsz
