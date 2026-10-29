// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <vector>

#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class Graph;
class Pin;
}  // namespace sta
namespace utl {
class Logger;
}

namespace ret {

// Forward retiming search for the best worst-case arrival.
//
// Delays are read once from STA: per-edge wire delay (arrival difference
// between driver and sink pin) and per-gate arcs.  Each gate arc is
// characterized twice from the liberty table model, under its combinational
// output load and under the register D-pin load, and the arrival sweep picks
// the variant matching the current register configuration (a gate whose
// output is registered after a slide sees the register load).
//
// The loop repeatedly slides one register forward across a gate (r[v] -= 1),
// keeps the slide only if the exact recomputed worst arrival improves, and
// restarts; a cheap estimate over the near-critical cone filters candidates
// before the exact recompute.
class Retime
{
 public:
  Retime(RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMaster* flop_master);

  // max_moves <= 0 means unlimited.  With stop_at_period the loop stops as
  // soon as the model's worst arrival meets the clock period.
  void run(int max_moves, bool stop_at_period);
  const std::vector<int>& getRetiming() const { return r_; }

 private:
  // Fanin edge carrying both arc variants of the target gate for this pin.
  struct FaninEdge
  {
    int u;
    int weight;
    float wire;
    float arc_comb;      // target's arc when its output is combinational
    float arc_reg;       // target's arc when its output drives a register
    odb::dbITerm* sink;  // the target's input pin this edge drives
  };

  enum class ArcCharStage
  {
    ok,
    no_cell,
    no_from_port,
    no_pvt,
    no_table_model
  };

  float pinArrival(odb::dbITerm* pin);
  float getGateArc(odb::dbITerm* input_pin);
  float getPinSlew(sta::Pin* pin);

  void computeArcDelays();
  void resolveFlopDCap();
  void computeWireArcs();
  void computeCombArcs();
  void computeSeqArcs();
  float characterizeArc(int v,
                        odb::dbITerm* sink,
                        float load,
                        float fallback,
                        ArcCharStage* stage);

  bool outputRegistered(int v) const;
  void computeArrivals();
  double markCriticalCone();
  double estimateSlideDelta(int v);
  void calibratePeriod();
  bool forwardMovable(int v) const;
  void reportWorstPathStart();

  RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<int> r_;
  std::vector<double> gate_floor_;
  std::vector<double> arrivals_;
  std::vector<std::vector<FaninEdge>> fanin_;
  std::vector<char> in_critical_cone_;

  // Scratch for the arrival sweeps and the cone recompute, sized once in run().
  struct Scratch
  {
    std::vector<int> indeg;
    std::vector<int> queue;
    std::vector<int> local_indeg;
    std::vector<double> local_arr;
    std::vector<char> in_cone;
    std::vector<int> cone;
    void init(int n)
    {
      indeg.assign(n, 0);
      queue.assign(n, 0);
      local_indeg.assign(n, 0);
      local_arr.assign(n, 0.0);
      in_cone.assign(n, 0);
      cone.clear();
      cone.reserve(n);
    }
  };
  Scratch scratch_;

  odb::dbMaster* flop_master_ = nullptr;
  float flop_d_cap_ = 0.0f;
  double eff_period_ = 0.0;
  double critical_threshold_ = 0.0;
  int dcalc_ap_ = 0;
  bool stop_at_period_ = false;
};

}  // namespace ret
