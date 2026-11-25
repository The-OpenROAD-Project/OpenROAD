// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyCell;
class RiseFall;
class Vertex;
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
  // flop_d_mterm: the data pin of the base cell.  cells_of: what a register
  // of a given cell can be rebuilt from.
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMTerm* flop_d_mterm,
         CellsOf cells_of);

  void run(int max_moves);  // <= 0: unlimited
  const std::vector<int>& getRetiming() const { return r_; }
  int getMoves() const { return moves_; }

 private:
  // The arrival at a gate no path of that root reaches.
  static constexpr double kUnreached = -1e30;
  static constexpr double kNegInf = -std::numeric_limits<double>::infinity();

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

  float pinArrival(odb::dbITerm* db_pin) const;
  float arrivalAt(sta::Vertex* vertex, const sta::RiseFall* rf) const;
  float getGateArc(odb::dbITerm* input_pin) const;
  // The wire delay of an edge once no register is left on it: the wire into
  // each of its registers, then the wire from the last one to the sink.
  float wireDelay(const EdgePhys& phys) const;

  void computeArcDelays();
  void resolveFlopDCap();
  void computeWireArcs();
  // After computeWireArcs: the modeled inputs are the fanin edges' sinks.
  void computePinnedFloors();
  void computeGateArcs();
  // forced_slew < 0: the input slew STA has.  out_slew: the output slew of
  // the slowest arc.
  float characterizeArc(int v,
                        odb::dbITerm* sink,
                        float load,
                        float fallback,
                        float forced_slew = -1.0f,
                        float* out_slew = nullptr) const;

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

  // The slack into and out of the register a slide across gate v leaves
  // behind it, and what the slide adds to the total negative slack.
  struct SlideEffect
  {
    double into = 0.0;
    double out = 0.0;
    double total_change = 0.0;
  };
  struct Pick
  {
    int gate = -1;         // -1: no slide left
    bool skipped = false;  // a refused gate was passed over
  };
  void search(int max_moves);
  // A gate neither rule of the search takes is marked in `refused`.
  Pick pickSlide(double margin, std::vector<char>& refused);
  // The gates a slide can cross whose worst path fails, worst first.
  std::vector<std::pair<double, int>> failingGates(
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
  // The pin of gate fe.u driving the edge's first net; null when not on fe.u.
  odb::dbITerm* edgeSource(const FaninEdge& fe) const;
  odb::dbITerm* latestOutput(int v) const;
  odb::dbITerm* firstOutput(int v) const;
  float pinCap(odb::dbITerm* pin) const;
  float loadCap(odb::dbITerm* driver) const;
  // The source gate's load once the gate behind the edge's registers reads
  // its net directly.
  float sourceLoadAfterSlide(const FaninEdge& fe, odb::dbITerm* source) const;
  // How much later gate u's output gets at `new_load`; `slew`: its output
  // slew then, negative without an arc.
  float slowdownAtLoad(int u, float load, float new_load, float& slew) const;
  float slowdownAtSlew(int t, odb::dbITerm* sink, float slew) const;
  std::optional<float> staArc(odb::dbITerm* input,
                              odb::dbITerm* output) const;
  // STA's input arrivals through the model's arcs.
  double modeledOutputArrival(int t, bool registered) const;
  // launch_offset_ (the clock arrival): median over sampled registers of STA's
  // launch minus the table clock-to-Q; clock_slew_: the last one's clock slew.
  void sampleLaunchOffset();
  void sampleLaunch(odb::dbInst* flop, std::vector<float>& offsets);
  std::vector<sta::LibertyCell*> newRegisterCells(int u) const;
  // Sets launch_cell_[u] to the fastest new cell; returns its clock-to-Q.
  float pickLaunchCell(int u, float load);
  // `regs`: the registers a first slide across t moves.
  bool slideKeepsClass(int t, std::vector<odb::dbInst*>& regs) const;
  float typicalGateDelay() const;
  // The gates unmapping the edge's registers leaves on the path: one for an
  // inverting register, two for another class that is not plain.
  int unmappedGates(const FaninEdge& fe) const;
  double registerEndShift(int v, double arrival) const;
  double portEndShift(int v, double arrival) const;

  const RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<int> r_;
  std::vector<double> gate_floor_;
  std::vector<double> arrivals_;     // register-rooted
  std::vector<double> arrivals_pinned_;  // pinned-rooted
  // A pinned gate's latest arrival through its inputs the graph leaves out.
  std::vector<double> pinned_floor_;
  std::vector<std::vector<FaninEdge>> fanin_;

  struct Scratch
  {
    std::vector<int> queue;
    std::vector<int> local_indeg;
    std::vector<char> in_dirty;
    std::vector<int> dirty;
    std::vector<double> saved_arrivals;
    std::vector<double> saved_arrivals_pinned;
    void init(int n);
  };
  Scratch scratch_;

  odb::dbMTerm* flop_d_mterm_ = nullptr;
  float flop_d_cap_ = 0.0f;
  int dcalc_ap_ = 0;
  int moves_ = 0;  // slides taken by the last run

  CellsOf cells_of_;
  std::vector<odb::dbMaster*> base_cells_;  // what a new register is built from
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
