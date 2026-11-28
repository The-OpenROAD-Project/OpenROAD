// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <compare>
#include <functional>
#include <limits>
#include <optional>
#include <set>
#include <vector>

#include "RegisterTiming.h"
#include "RegisterUtils.h"
#include "RetimingGraph.h"
#include "odb/db.h"

namespace sta {
class dbSta;
class LibertyCell;
class Pin;
class RiseFall;
class Scene;
class Vertex;
}  // namespace sta
namespace utl {
class Logger;
}

namespace ret {

// Forward retiming on a delay model read from STA.  A slide r[v] -= 1 moves
// the registers on the inputs of gate v to its output.
class Retime
{
 public:
  using CellsOf = std::function<std::vector<odb::dbMaster*>(odb::dbMaster*)>;
  Retime(const RetimingGraph* graph,
         sta::dbSta* sta,
         utl::Logger* logger,
         odb::dbMTerm* flop_d_mterm,
         CellsOf cells_of);

  // max_moves <= 0: unlimited.  margin_fraction: of the clock period.
  void run(int max_moves, double margin_fraction);
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
    float driver_shift;  // the driver's STA arrival minus the source gate's
    float cross_extra;   // added once a slide takes the edge's registers away
  };

  struct Slide
  {
    int gate;
    double gain;  // what it adds to the total negative slack
  };

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

  struct FailingGates
  {
    std::set<FailingGate> order;
    std::vector<double> slacks;  // per gate; 0: not in the order
    void init(int n);
    void set(int gate, double slack);
  };

  struct Scratch
  {
    std::vector<int> queue;
    std::vector<int> marked;
    std::vector<char> is_marked;
    std::vector<int> pending;  // per gate: the marked gates it waits for
    std::vector<double> saved_arrivals;
    void init(int n);
  };

  static constexpr double kUnreached = -1e30;
  static constexpr double kNegInf = -std::numeric_limits<double>::infinity();

  // The search (Retime.cpp).
  void search(int max_moves, double margin_fraction);
  std::optional<Slide> pickSlide(double margin);
  void rankSlides();
  std::vector<std::vector<int>> slideNeeds() const;
  static std::vector<int> pathOf(int end,
                                 const std::vector<std::vector<int>>& needs);
  void updateFailing();
  SlideEffect slideEffect(int v);  // the slide is undone
  double negativeSlackAt(int t) const;
  double slackThrough(int v) const;
  double slackOf(double arrival, double tail) const;
  double requiredTime() const;
  // What an endpoint at v's output is charged on top of v's arrival.
  double endShift(int v) const;
  bool forwardMovable(int v) const;
  bool outputRegistered(int v) const;
  bool outputRegisteredInGraph(int v) const;
  bool hasEndpoint(int v) const;
  int registersOn(int u, const Edge& e) const
  {
    return ret::registersOn(r_, u, e);
  }
  int registersOn(int t, const FaninEdge& fe) const
  {
    return fe.weight + r_[t] - r_[fe.source];
  }
  bool isCombinational(int u, const Edge& e) const;
  bool isCombinational(int t, const FaninEdge& fe) const;

  void applySlide(int v);
  void undoSlide(int v);
  void computeArrivals();
  void propagateArrivals();
  void setArrival(int t);
  double launchArrival(int t, const FaninEdge& fe) const;

  void computeTails();
  void updateTails(int v);
  // False when a combinational loop left a gate unvisited.
  bool propagateTails();
  void setTail(int u);

  void mark(int t);
  void markAll();
  void markWithFanins(int v);
  void markDownstream();
  void markUpstream();
  // A gate on a combinational loop is not visited and stays marked.
  void visitMarked(bool forward, const std::function<void(int)>& visit);
  int markedDependencies(int t, bool forward) const;

  // The delay model (RetimeModel.cpp), read from STA.
  void buildModel();
  void resolveFlopDCap();

  void buildEdges();
  float wireDelay(const EdgePhys& phys) const;
  float pinArrival(odb::dbITerm* db_pin) const;
  float arrivalAt(sta::Vertex* vertex, const sta::RiseFall* rf) const;
  float latestArrival(sta::Vertex* vertex) const;
  float slowestStaArc(odb::dbITerm* input_pin) const;

  void computePinnedFloors();

  void computeLatestOutputs();
  odb::dbITerm* latestOutput(int v) const;

  void computeGateArcs();
  // nullopt without a table arc.  forced_slew: nullopt is STA's input slew.
  std::optional<TableArc> characterizeArc(
      int v,
      odb::dbITerm* input,
      float load,
      std::optional<float> forced_slew = std::nullopt) const;
  float pinCap(odb::dbITerm* pin) const;
  void calibrateArcs(int t);
  double modeledOutputArrival(int t, bool registered) const;
  std::optional<float> staArc(odb::dbITerm* input, odb::dbITerm* output) const;

  void computeCrossExtra();
  float crossExtra(int t,
                   const FaninEdge& fe,
                   float source_load,
                   float new_load) const;
  // nullptr when the source gate does not drive the edge's first net.
  odb::dbITerm* edgeSource(const FaninEdge& fe) const;
  struct NetReaders
  {
    float wire = 0.0f;  // STA's wire capacitance of the net
    int count = 0;
  };
  NetReaders netReaders(odb::dbNet* net) const;
  float sourceLoadAfterSlide(const FaninEdge& fe,
                             float source_load,
                             const NetReaders& readers) const;
  float loadCap(odb::dbITerm* driver) const;
  // slew: gate u's output slew at new_load, negative without an arc.
  float slowdownAtLoad(int u, float load, float new_load, float& slew) const;
  float slowdownAtSlew(int t, odb::dbITerm* sink, float slew) const;

  void computeLaunch();
  void sampleLaunchOffset();
  void sampleLaunch(odb::dbInst* flop, std::vector<float>& offsets);
  odb::dbITerm* firstOutput(int v) const;
  struct LaunchCell
  {
    sta::LibertyCell* cell = nullptr;
    float clock_to_q = 0.0f;
  };
  LaunchCell fastestLaunchCell(int u, float load) const;
  std::vector<sta::LibertyCell*> newRegisterCells(int u) const;
  std::vector<odb::dbInst*> registersInto(int t) const;

  void computeDriverShifts();

  void computeEndpoints();
  double registerEndShift(int v, double arrival) const;
  double fixedEndShift(int v, double arrival) const;
  // -inf when STA does not constrain the pin.
  double endShiftAt(const sta::Pin* pin, double arrival) const;

  const RetimingGraph* graph_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;

  // The search.
  std::vector<int> r_;
  std::vector<int> slides_;
  std::vector<double> gains_;  // per slide
  std::vector<double> arrivals_;
  // To the worst endpoint behind the gate; -inf: none.
  std::vector<double> tails_;
  std::vector<double> out_tails_;
  bool loops_ = false;  // a combinational loop
  Scratch scratch_;
  FailingGates failing_;   // movable gates, by the slack through them
  FailingGates negative_;  // gates, by the slack of their endpoint

  // The delay model.
  odb::dbMTerm* flop_d_mterm_ = nullptr;
  float flop_d_cap_ = 0.0f;
  std::vector<sta::Scene*> scenes_;  // the command scene alone
  int dcalc_ap_ = 0;
  CellsOf cells_of_;
  mutable NetDrivers drivers_;
  std::vector<odb::dbMaster*> base_cells_;
  std::vector<std::vector<FaninEdge>> fanin_;
  // Per gate: its edges as (target, index into fanin_[target]).
  std::vector<std::vector<EdgeRef>> fanout_;
  // A pinned gate's latest arrival through the inputs the graph leaves out.
  std::vector<double> pinned_floor_;
  float clock_slew_ = 0.0f;
  double launch_offset_ = 0.0;
  std::vector<odb::dbITerm*> latest_output_;
  std::vector<sta::LibertyCell*> launch_cell_;  // of a new register
  std::vector<double> new_launch_;
  std::vector<double> new_setup_;
  std::vector<double> orig_end_shift_;   // of the registers on the edges
  std::vector<double> fixed_end_shift_;  // of the endpoints no slide moves
};

// The slides in ranked order: best path first, the earlier one on a tie.
// paths: each a sorted list of slides.  gains: per slide.
std::vector<int> rankPaths(const std::vector<std::vector<int>>& paths,
                           const std::vector<double>& gains);

}  // namespace ret
