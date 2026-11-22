// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <unordered_set>
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
  // flop_d_mterm is the data pin of the cell new registers are built from.
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMTerm* flop_d_mterm);

  using CellsOf = std::function<std::vector<odb::dbMaster*>(odb::dbMaster*)>;
  // Judges a slide by its slack in the detailed delay model
  // (RetimeModel.cpp).  base_cells: what a new register can be built from.
  // cells_of: what a register of a given cell can be rebuilt from.
  // fastest_cell: a new register is priced with its fastest cell, else with
  // its slowest.
  void enableSlackSearch(std::vector<odb::dbMaster*> base_cells,
                         CellsOf cells_of,
                         bool fastest_cell);
  void setQuiet(bool quiet) { quiet_ = quiet; }
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
    // The rest is read by the slack search only.
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
  // The slide without the pass over all gates for the worst arrival.
  void applySlide(int v);
  void undoSlide(int v);
  double worstArrival();
  void markCriticalCone(double worst);
  double excess(double arrival) const;
  double arrivalAfterSlide(int v) const;
  void collectSlideCone(int v);
  void sweepSlideCone(int v);
  double estimateSlideDelta(int v);
  bool forwardMovable(int v) const;
  void reportSlide(int v, int moves);
  void reportResult(int moves, int pi_rejects);

  // The slack search (RetimeModel.cpp).
  struct ModelSlack
  {
    double worst = 0.0;
    double total = 0.0;
  };
  // The slack through gate v before a slide across it, into and out of
  // the register the slide leaves behind v, and what the slide adds to the
  // model's total negative slack.
  struct SlideEffect
  {
    double before = 0.0;
    double into = 0.0;
    double out = 0.0;
    double total_change = 0.0;
  };
  struct SlackTally
  {
    int for_spot = 0;
    int for_total = 0;
    int refused_for_total = 0;
  };
  struct Pick
  {
    int gate = -1;         // -1: no slide left
    bool skipped = false;  // a refused spot was passed over
  };
  void runBySlack(int max_moves);
  // First the worst failing spot whose slide raises the slack through it
  // by more than `margin` without lowering the total negative slack; else
  // the first whose slide raises the total without lowering the worst
  // slack.  A spot neither rule takes is marked in `refused`.
  Pick pickSlide(double margin, std::vector<char>& refused, SlackTally& tally);
  // The gates a slide can cross whose worst path fails, worst first.
  std::vector<std::pair<double, int>> failingSpots(
      const std::vector<char>& refused,
      bool& skipped) const;
  SlideEffect slideEffect(int v);  // the slide is undone
  void updateTails(std::vector<char>& refused);
  void reopenAround(const std::vector<int>& gates,
                    std::vector<char>& refused) const;
  void reportSlackSearch(const ModelSlack& start, const SlackTally& tally);
  ModelSlack modelSlack() const;
  // The arrival that leaves no slack before an endpoint's charge.
  double requiredTime() const;
  // Infinite when no constrained path runs there.
  double slackOf(double arrival, double tail) const;
  double slackThrough(int v) const;
  // The negative slack of the path ending at t's output; zero when it is
  // no endpoint or meets timing.
  double negativeSlackAt(int t) const;
  // What an endpoint at v's output is charged on top of v's arrival.
  double endShift(int v) const;
  // The arrival a registered edge into t starts from.
  double launchArrival(int t, const FaninEdge& fe) const;
  // What a slide that took the registers off the edge left on it.
  double crossingDelay(int t, const FaninEdge& fe) const;
  void computeTails();

  void computeSlackModel();
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
  // Sets launch_cell_[u]; returns its clock-to-Q at the load.
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

  // `set_debug_level RET why 1`: where STA's failing endpoints are, and
  // what stops a slide at the first gate of the worst path into each
  // failing endpoint of the model.
  enum class SlideStop
  {
    kStartsAtPinned,
    kWouldHelp,
    kRefusedForTotal,
    kLengthenedPathLimits,
    kShortenedPathGainsLittle,
    kPinnedGate,
    kRegisteredOutput,
    kUnregisteredInput,
    kNoFanin,
    kNoPath,
  };
  static constexpr int kSlideStops = 10;
  void reportFailingEndpoints() const;
  std::unordered_set<const sta::Pin*> modeledEndpoints() const;
  void reportStops(double margin);
  SlideStop slideStop(int s, double margin);
  // The gate behind the register that launches the worst path into t; -1
  // when the model traces none.
  int launchGate(int t) const;
  // The gate the worst pinned-rooted path into t starts at.
  int pinnedStart(int t) const;

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

  odb::dbMTerm* flop_d_mterm_ = nullptr;
  float flop_d_cap_ = 0.0f;
  double critical_threshold_ = 0.0;
  double worst_pi_ = 0.0;  // worst pinned-rooted arrival of the last sweep
  double pi_limit_ = 0.0;  // pinned-rooted arrivals may not grow past this
  int dcalc_ap_ = 0;
  bool quiet_ = false;
  int moves_ = 0;  // slides accepted by the last run

  bool slack_search_ = false;
  std::vector<odb::dbMaster*> base_cells_;
  CellsOf cells_of_;
  bool fastest_cell_ = false;
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
