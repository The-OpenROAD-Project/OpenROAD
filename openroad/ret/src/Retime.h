// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <compare>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyCell;
class Pin;
class RiseFall;
class Vertex;
}  // namespace sta
namespace utl {
class Logger;
}

namespace ret {

// Forward retiming on a delay model read from STA.  A slide r[v] -= 1 moves
// the registers on the inputs of gate v to its output.  The search takes the
// worst failing gate whose slide lowers neither the total negative slack nor
// the worst slack and raises the slack through the gate or the total.  Its
// slides are ranked: a path is a slide no other slide needs with the slides
// it needs, and the paths the model gains most from come first, so any
// prefix is a retiming.
class Retime
{
 public:
  using CellsOf = std::function<std::vector<odb::dbMaster*>(odb::dbMaster*)>;
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMTerm* flop_d_mterm,
         CellsOf cells_of);

  // max_moves <= 0: unlimited.  margin: a fraction of the clock period.
  void run(int max_moves, double margin);
  const std::vector<int>& getRetiming() const { return r_; }
  int getMoves() const { return static_cast<int>(slides_.size()); }
  const std::vector<int>& getSlides() const { return slides_; }

 private:
  struct FaninEdge
  {
    int source;
    int weight;
    float wire;
    float arc_comb;  // target's arc when its output is combinational
    float arc_reg;   // target's arc when its output drives a register
    odb::dbITerm* sink;
    float launch;  // STA arrival at the sink
    const EdgePhys* phys;
    // STA arrival at the edge's driver pin minus that at the source gate's
    // latest output.
    float driver_shift;
    // Added once a slide takes the edge's registers away: the source gate
    // gets a new load and the gate a new input slew.
    float cross_extra;
  };

  // The slack into and out of the register a slide across gate v leaves
  // behind it, and what the slide adds to the total negative slack.
  struct SlideEffect
  {
    double into = 0.0;
    double out = 0.0;
    double total_change = 0.0;
  };

  struct FailingGate
  {
    double slack;
    int gate;
    auto operator<=>(const FailingGate&) const = default;
  };

  struct Scratch
  {
    std::vector<int> queue;
    std::vector<int> local_indeg;
    std::vector<char> in_dirty;
    std::vector<int> dirty;
    std::vector<double> saved_arrivals;
    void init(int n);
  };

  // The arrival at a gate no path of that root reaches.
  static constexpr double kUnreached = -1e30;
  static constexpr double kNegInf = -std::numeric_limits<double>::infinity();

  // The search (Retime.cpp).
  void search(int max_moves, double margin);
  // -1: none.  gain: what the slide adds to the total negative slack.
  int pickSlide(double margin, double& gain);
  void rankSlides();
  // Per slide, the earlier slides it needs: the one before it across the
  // same gate, and those that put a register on an input of its gate.
  std::vector<std::vector<int>> slideNeeds() const;
  static std::vector<int> pathOf(int end,
                                 const std::vector<std::vector<int>>& needs);
  // Worst first.
  std::vector<FailingGate> failingGates() const;
  SlideEffect slideEffect(int v);  // the slide is undone
  double worstSlack() const;
  double negativeSlackAt(int t) const;
  double slackThrough(int v) const;
  // Infinite when no constrained path runs there.
  double slackOf(double arrival, double tail) const;
  double requiredTime() const;
  // What an endpoint at v's output is charged on top of v's arrival.
  double endShift(int v) const;
  bool forwardMovable(int v) const;
  bool outputRegistered(int v) const;
  // The registers on an edge under the slides taken.
  int registersOn(int u, const Edge& e) const
  {
    return e.weight + r_[e.target] - r_[u];
  }
  int registersOn(int t, const FaninEdge& fe) const
  {
    return fe.weight + r_[t] - r_[fe.source];
  }
  bool isCombinational(int u, const Edge& e) const;
  bool isCombinational(int t, const FaninEdge& fe) const;

  // A slide across v changes the arrivals of v, of its fanins (their output
  // may stop being registered), of its fanouts (they may become roots), and
  // of everything downstream of those.
  void applySlide(int v);
  void undoSlide(int v);
  void computeArrivals();
  // The arrivals of the marked gates; the old ones are saved for undoSlide.
  void propagateArrivals();
  void setArrival(int t);
  double launchArrival(int t, const FaninEdge& fe) const;
  double crossingDelay(const FaninEdge& fe) const;

  void computeTails();
  // A slide across v changes the tails of v, of its fanins, and of
  // everything upstream of those.
  void updateTails(int v);
  // The tails of the marked gates.  False when a combinational loop left a
  // gate unvisited.
  bool propagateTails();
  void setTail(int u);

  void mark(int t);
  void markAll();
  void markDownstream();
  void markUpstream();
  // Each marked gate after the marked gates it depends on.  A gate on a
  // combinational loop is not visited and stays marked.
  void visitMarked(bool forward, const std::function<void(int)>& visit);

  // The delay model (RetimeModel.cpp), read from STA.
  void buildModel();
  void resolveFlopDCap();

  void computeWireArcs();
  // The wire delay of an edge once no register is left on it: the wire into
  // each of its registers, then the wire from the last one to the sink.
  float wireDelay(const EdgePhys& phys) const;
  float pinArrival(odb::dbITerm* db_pin) const;
  float arrivalAt(sta::Vertex* vertex, const sta::RiseFall* rf) const;
  float getGateArc(odb::dbITerm* input_pin) const;

  void computePinnedFloors();

  void computeLatestOutputs();
  odb::dbITerm* latestOutput(int v) const;

  void computeGateArcs();
  // forced_slew < 0: the input slew STA has.  out_slew: the output slew of
  // the slowest arc.
  float characterizeArc(int v,
                        odb::dbITerm* sink,
                        float load,
                        float fallback,
                        float forced_slew = -1.0f,
                        float* out_slew = nullptr) const;
  float pinCap(odb::dbITerm* pin) const;
  void calibrateArcs(int t);
  double modeledOutputArrival(int t, bool registered) const;
  std::optional<float> staArc(odb::dbITerm* input, odb::dbITerm* output) const;

  void computeCrossExtra();
  // The pin of the source gate driving the edge's first net; null when the
  // net is not driven by that gate.
  odb::dbITerm* edgeSource(const FaninEdge& fe) const;
  // The source gate's load once the gate behind the edge's registers reads
  // its net directly.
  float sourceLoadAfterSlide(const FaninEdge& fe, odb::dbITerm* source) const;
  float loadCap(odb::dbITerm* driver) const;
  // How much later gate u's output gets at `new_load`; `slew`: its output
  // slew then, negative without an arc.
  float slowdownAtLoad(int u, float load, float new_load, float& slew) const;
  float slowdownAtSlew(int t, odb::dbITerm* sink, float slew) const;

  void computeLaunch();
  // launch_offset_ (the clock arrival): median over the registers of STA's
  // launch minus the table clock-to-Q; clock_slew_: the last one's clock slew.
  void sampleLaunchOffset();
  void sampleLaunch(odb::dbInst* flop, std::vector<float>& offsets);
  odb::dbITerm* firstOutput(int v) const;
  float pickLaunchCell(int u, float load);
  std::vector<sta::LibertyCell*> newRegisterCells(int u) const;
  std::vector<odb::dbInst*> registersInto(int t) const;

  void computeDriverShifts();

  void computeEndpoints();
  double registerEndShift(int v, double arrival) const;
  double fixedEndShift(int v, double arrival) const;
  // What the endpoint at `pin` is charged on top of `arrival`; -inf when
  // it is no endpoint to STA or STA does not constrain it.
  double endShiftAt(const sta::Pin* pin, double arrival) const;

  const RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  std::vector<int> r_;
  std::vector<double> arrivals_;
  // A pinned gate's latest arrival through its inputs the graph leaves out.
  std::vector<double> pinned_floor_;
  std::vector<std::vector<FaninEdge>> fanin_;
  // Per gate: its outgoing edges as (target, index into fanin_[target]).
  std::vector<std::vector<EdgeRef>> fanout_;
  Scratch scratch_;

  odb::dbMTerm* flop_d_mterm_ = nullptr;
  float flop_d_cap_ = 0.0f;
  int dcalc_ap_ = 0;
  std::vector<int> slides_;
  std::vector<double> gains_;

  CellsOf cells_of_;
  std::vector<odb::dbMaster*> base_cells_;  // what a new register is built from
  float clock_slew_ = 0.0f;
  double launch_offset_ = 0.0;
  std::vector<odb::dbITerm*> latest_output_;
  std::vector<sta::LibertyCell*> launch_cell_;  // of a new register
  std::vector<double> new_launch_;  // what a new register launches at
  std::vector<double> new_setup_;
  // The charge as an endpoint of the registers on the gate's edges, and of
  // the endpoints no slide moves: a port or a pin outside the graph (a
  // control pin, a register whose chain reaches no gate) that reads the
  // gate's output.  -inf: none.
  std::vector<double> orig_end_shift_;
  std::vector<double> fixed_end_shift_;
  // From the gate's output to the worst endpoint behind it, charge
  // included: through its combinational fanout, and from a register put
  // on its output.  -inf: no endpoint.
  std::vector<double> tails_;
  std::vector<double> out_tails_;
  bool loops_ = false;  // a combinational loop: the tails are computed whole
};

}  // namespace ret
