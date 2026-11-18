// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <vector>

#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class Pin;
}  // namespace sta
namespace utl {
class Logger;
}

namespace ret {

// Greedy forward retiming on a delay model read from STA: wire delays per
// edge, and two NLDM arcs per gate (combinational load, register D-pin
// load) chosen by the current register configuration.  Register-to-register
// paths are the objective; a path rooted at a pinned gate cannot be
// shortened by a forward move and is a constraint instead: it may not grow
// past the clock period, or past its starting value when it already
// violates.  Each slide r[v] -= 1 is kept only if the exact recomputed worst
// arrival improves; a cone estimate filters the candidates.
class Retime
{
 public:
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMaster* flop_master);

  void run(int max_moves);  // <= 0: unlimited
  const std::vector<int>& getRetiming() const { return r_; }

 private:
  struct FaninEdge
  {
    int u;
    int weight;
    float wire;
    float arc_comb;  // target's arc when its output is combinational
    float arc_reg;   // target's arc when its output drives a register
    odb::dbITerm* sink;
  };

  enum class ArcCharStage
  {
    kOk,
    kNoCell,
    kNoFromPort,
    kNoPvt,
    kNoTableModel
  };
  static constexpr size_t kArcCharStages = 5;

  float pinArrival(odb::dbITerm* db_pin);
  float getGateArc(odb::dbITerm* input_pin);
  float getPinSlew(sta::Pin* pin);
  // The wire delay of an edge once no register is left on it: the wire into
  // each of its registers, then the wire from the last one to the sink.
  float wireDelay(const EdgePhys& phys);

  void computeArcDelays();
  void resolveFlopDCap();
  void computeWireArcs();
  void computeCombArcs();
  void computeSeqArcs();
  float characterizeArc(int v,
                        odb::dbITerm* sink,
                        float load,
                        float fallback,
                        ArcCharStage* stage_out);

  bool outputRegistered(int v) const;
  bool isCombinational(int u, const Edge& e) const;
  bool isCombinational(int t, const FaninEdge& fe) const;
  // Arrival of t from the fanins it reads without a register in between; a
  // gate without any is a path root.
  void setArrival(int t);
  void computeArrivals();
  // A slide across v changes the arrivals of v, of its fanins (their output
  // may stop being registered), of its fanouts (they may become roots), and
  // of everything downstream of those without a register in between.  Only
  // that set is recomputed, in topological order, and saved for undoSlide.
  // Returns the worst arrival after the slide.
  double slide(int v);
  void undoSlide(int v);
  double worstArrival();
  void markCriticalCone(double worst);
  double excess(double arrival) const;
  double arrivalAfterSlide(int v) const;
  void collectSlideCone(int v);
  void sweepSlideCone(int v);
  double estimateSlideDelta(int v);
  bool forwardMovable(int v) const;
  void reportSlide(int v, double worst, int moves);
  void reportResult(double worst, int moves, int pi_rejects);

  const RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<int> r_;
  std::vector<double> gate_floor_;
  std::vector<double> arrivals_;     // register-rooted, the objective
  std::vector<double> arrivals_pi_;  // pinned-rooted, the constraint
  std::vector<float> pi_source_;     // STA arrival at a pinned source gate
  std::vector<std::vector<FaninEdge>> fanin_;
  std::vector<char> in_critical_cone_;

  struct Scratch
  {
    std::vector<int> indeg;
    std::vector<int> queue;
    std::vector<int> local_indeg;
    std::vector<double> local_arr;
    std::vector<char> in_cone;
    std::vector<int> cone;
    std::vector<char> in_dirty;
    std::vector<int> dirty;
    std::vector<double> saved_arrivals;
    std::vector<double> saved_arrivals_pi;
    void init(int n)
    {
      indeg.assign(n, 0);
      queue.assign(n, 0);
      local_indeg.assign(n, 0);
      local_arr.assign(n, 0.0);
      in_cone.assign(n, 0);
      cone.clear();
      cone.reserve(n);
      in_dirty.assign(n, 0);
      dirty.clear();
      dirty.reserve(n);
      saved_arrivals.reserve(n);
      saved_arrivals_pi.reserve(n);
    }
  };
  Scratch scratch_;

  odb::dbMaster* flop_master_ = nullptr;
  float flop_d_cap_ = 0.0f;
  double critical_threshold_ = 0.0;
  double worst_pi_ = 0.0;  // worst pinned-rooted arrival of the last sweep
  double pi_limit_ = 0.0;  // pinned-rooted arrivals may not grow past this
  int dcalc_ap_ = 0;
};

}  // namespace ret
