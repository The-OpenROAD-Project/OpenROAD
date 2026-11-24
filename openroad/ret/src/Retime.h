// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyCell;
class Pin;
}  // namespace sta
namespace utl {
class Logger;
}

namespace ret {

// Forward retiming on a delay model read from STA.  A path starts at what
// its register launches, gate arcs are calibrated to STA's and an endpoint
// is charged its setup.  A slide r[v] -= 1 moves the registers on the inputs
// of gate v to its output.  The search takes the worst failing gate whose
// slide raises the slack through it without lowering the total negative
// slack; when none is left, slides that raise the total without lowering the
// worst slack.  A path rooted at a pinned gate cannot be shortened by a
// forward move, but its slack counts like any other.
class Retime
{
 public:
  using CellsOf = std::function<std::vector<odb::dbMaster*>(odb::dbMaster*)>;
  // flop_d_mterm: the data pin of the base cell.  base_cells: what a new
  // register can be built from.  cells_of: what a register of a given cell
  // can be rebuilt from.
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMTerm* flop_d_mterm,
         std::vector<odb::dbMaster*> base_cells,
         CellsOf cells_of);

  void run(int max_moves);  // <= 0: unlimited
  const std::vector<int>& getRetiming() const { return r_; }
  int getMoves() const { return moves_; }

 private:
  struct FaninEdge
  {
    int u;
    int weight;
    float wire;
    float arc_comb;  // target's arc when its output is combinational
    float arc_reg;   // target's arc when its output drives a register
    odb::dbITerm* sink;
    float launch;  // STA arrival at the sink
    const EdgePhys* phys;
    float unmap;  // added once a slide unmaps the edge's registers
    // STA arrival at the edge's driver pin minus that at the source gate's
    // latest output.
    float driver_shift;
    // Added once a slide takes the edge's registers away: the source gate
    // gets a new load and the gate a new input slew.
    float cross_extra;
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
  // forced_slew < 0: the input slew STA has.  out_slew: the output slew of
  // the slowest arc.
  float characterizeArc(int v,
                        odb::dbITerm* sink,
                        float load,
                        float fallback,
                        ArcCharStage* stage_out,
                        float forced_slew = -1.0f,
                        float* out_slew = nullptr);

  bool outputRegistered(int v) const;
  bool isCombinational(int u, const Edge& e) const;
  bool isCombinational(int t, const FaninEdge& fe) const;
  void setArrival(int t);
  void computeArrivals();
  bool hasRegisterPath() const;
  // A slide across v changes the arrivals of v, of its fanins (their output
  // may stop being registered), of its fanouts (they may become roots), and
  // of everything downstream of those without a register in between.  Only
  // that set is recomputed, in topological order, and saved for undoSlide.
  void applySlide(int v);
  void undoSlide(int v);
  bool forwardMovable(int v) const;

  // The slack through gate v before a slide across it, into and out of
  // the register the slide leaves behind v, and what the slide adds to the
  // total negative slack.
  struct SlideEffect
  {
    double before = 0.0;
    double into = 0.0;
    double out = 0.0;
    double total_change = 0.0;
  };
  struct Pick
  {
    int gate = -1;         // -1: no slide left
    bool skipped = false;  // a refused spot was passed over
  };
  void search(int max_moves);
  // A spot neither rule of the search takes is marked in `refused`.
  Pick pickSlide(double margin, std::vector<char>& refused);
  // The gates a slide can cross whose worst path fails, worst first.
  std::vector<std::pair<double, int>> failingSpots(
      const std::vector<char>& refused,
      bool& skipped) const;
  SlideEffect slideEffect(int v);  // the slide is undone
  void updateTails(std::vector<char>& refused);
  void reopenAround(const std::vector<int>& gates,
                    std::vector<char>& refused) const;
  double worstSlack() const;
  // The arrival that leaves no slack before an endpoint's charge.
  double requiredTime() const;
  // Infinite when no constrained path runs there.
  double slackOf(double arrival, double tail) const;
  double slackThrough(int v) const;
  // Zero when t's output is no endpoint or meets timing.
  double negativeSlackAt(int t) const;
  // What an endpoint at v's output is charged on top of v's arrival.
  double endShift(int v) const;
  // The arrival a registered edge into t starts from.
  double launchArrival(int t, const FaninEdge& fe) const;
  // What a slide that took the registers off the edge left on it.
  double crossingDelay(int t, const FaninEdge& fe) const;
  void computeTails();

  void computeRegisterTiming();
  void computeCrossExtra();
  void computeLaunch();
  void calibrateArcs();
  void computeUnmapCost();
  void computeEndpoints();
  void computeDriverShifts();
  // The output pin of the source gate that the edge leaves from; null when
  // it is not on that gate.
  odb::dbITerm* edgeSource(const FaninEdge& fe) const;
  odb::dbITerm* latestOutput(int v);
  odb::dbITerm* firstOutput(int v) const;
  float pinCap(odb::dbITerm* pin) const;
  float loadCap(odb::dbITerm* driver) const;
  // The source gate's load once the gate behind the edge's registers reads
  // its net directly.
  float sourceLoadAfterSlide(const FaninEdge& fe, odb::dbITerm* source);
  // How much later gate u's output gets at `new_load`; `slew`: its output
  // slew then, negative without an arc.
  float slowdownAtLoad(int u, float load, float new_load, float& slew);
  float slowdownAtSlew(int t, odb::dbITerm* sink, float slew);
  // The gate's delay from an input to an output as STA timed it.
  std::optional<float> staArc(odb::dbITerm* input, odb::dbITerm* output);
  // The arrival at t's output that the model's arcs give from STA's input
  // arrivals.
  double modeledOutputArrival(int t, bool registered);
  // launch_offset_: the middle value, over a sample of registers, of what
  // STA has a register launch at beyond the table's clock-to-Q (the clock
  // arrival).  clock_slew_: the clock slew of the last register sampled.
  void sampleLaunchOffset();
  void sampleLaunch(odb::dbInst* flop, std::vector<float>& offsets);
  // The cells the register a slide across u builds can be made from.
  std::vector<sta::LibertyCell*> newRegisterCells(int u) const;
  // Sets launch_cell_[u] to the fastest one; returns its clock-to-Q at the
  // load.
  float pickLaunchCell(int u, float load);
  // The registers a first slide across t moves, and whether they keep
  // their class across it.
  bool slideKeepsClass(int t, std::vector<odb::dbInst*>& regs) const;
  float typicalGateDelay() const;
  // The gates unmapping the edge's registers leaves on the path: one for an
  // inverting register, two for another class that is not plain.
  int unmappedGates(const FaninEdge& fe) const;
  double registerEndShift(int v, double arrival);
  double portEndShift(int v, double arrival);

  const RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<int> r_;
  std::vector<double> gate_floor_;
  std::vector<double> arrivals_;     // register-rooted
  std::vector<double> arrivals_pi_;  // pinned-rooted
  std::vector<float> pi_source_;     // STA arrival at a pinned source gate
  std::vector<std::vector<FaninEdge>> fanin_;

  struct Scratch
  {
    std::vector<int> indeg;
    std::vector<int> queue;
    std::vector<int> local_indeg;
    std::vector<char> in_dirty;
    std::vector<int> dirty;
    std::vector<double> saved_arrivals;
    std::vector<double> saved_arrivals_pi;
    void init(int n)
    {
      indeg.assign(n, 0);
      queue.assign(n, 0);
      local_indeg.assign(n, 0);
      in_dirty.assign(n, 0);
      dirty.clear();
      dirty.reserve(n);
      saved_arrivals.reserve(n);
      saved_arrivals_pi.reserve(n);
    }
  };
  Scratch scratch_;

  odb::dbMTerm* flop_d_mterm_ = nullptr;
  float flop_d_cap_ = 0.0f;
  int dcalc_ap_ = 0;
  int moves_ = 0;  // slides taken by the last run

  std::vector<odb::dbMaster*> base_cells_;
  CellsOf cells_of_;
  float clock_slew_ = 0.0f;
  double launch_offset_ = 0.0;
  // Per gate.
  std::vector<odb::dbITerm*> latest_output_;
  std::vector<char> unmap_on_slide_;
  std::vector<sta::LibertyCell*> launch_cell_;  // of a new register
  std::vector<double> new_launch_;  // what a new register launches at
  std::vector<double> new_setup_;
  // The charge as an endpoint of the registers on the gate's output and
  // of a primary output it feeds; -inf: none.
  std::vector<double> orig_end_shift_;
  std::vector<double> po_end_shift_;
  // From the gate's output to the worst endpoint behind it, charge
  // included: through its combinational fanout, and from a register put
  // on its output.  -inf: no endpoint.
  std::vector<double> tails_;
  std::vector<double> out_tails_;
};

}  // namespace ret
