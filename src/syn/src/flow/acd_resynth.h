// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "flow/acd.h"
#include "flow/target_index.h"

namespace odb {
class dbInst;
class dbITerm;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace rsz {
class Resizer;
}

namespace syn {

class Synthesis;

namespace acd {

struct PinIdLess
{
  sta::dbNetwork* network = nullptr;

  bool operator()(const sta::Pin* a, const sta::Pin* b) const
  {
    return network->id(a) < network->id(b);
  }
};

// The drivers a cut is built out of
using ConePins = std::set<const sta::Pin*, PinIdLess>;

class Resynthesis : public sta::dbStaState
{
 public:
  Resynthesis(utl::Logger* logger,
              sta::dbSta* sta,
              rsz::Resizer* resizer,
              Synthesis* synthesis);
  ~Resynthesis();
  void init();

  void resynthesize(int max_leaves,
                    int max_intermediate_leaves,
                    int max_cells,
                    int max_outerfans,
                    bool exclude_buffers,
                    bool allow_lateral,
                    float timing_opt_effort,
                    bool apply);
  void resynthesizePin(const sta::Pin* head);

  // Put the substitutions found in place of the logic they stand for.
  void commit();

  // Print where the work went
  void reportStats();

 private:
  // Number of pins loading the net driven by `pin`.
  int nusers(const sta::Pin* pin);

  // View `pin` as an instance output, or null if it is a top-level port.
  odb::dbITerm* asDriverITerm(const sta::Pin* pin);

  // The sole driver of the net attached to `input`, or null if the net is
  // absent or has no single driver.
  const sta::Pin* driverOf(odb::dbITerm* input);

  // The second output of `driver`'s instance, or null if it has just the one.
  const sta::Pin* siblingOutput(odb::dbITerm* driver);
  bool isMultiOutput(const sta::Pin* pin);

  // Is `pin` expanded into the cone with its whole fanout inside the mffc?
  bool containedInMffc(
      const ConePins& drivers_in_cone,
      const std::unordered_map<const sta::Pin*, int>& mffc_users,
      const sta::Pin* pin);

  // Would `pin`'s gate be left dead behind by the cut?
  bool entersMffc(const ConePins& drivers_in_cone,
                  const std::unordered_map<const sta::Pin*, int>& mffc_users,
                  const sta::Pin* pin);

  // Pull `pin`'s gate into the mffc, and recursively any of its fanins which
  // are only kept alive by it.
  void refMffc(const ConePins& drivers_in_cone,
               int& mffc_size,
               std::unordered_map<const sta::Pin*, int>& mffc_users,
               const sta::Pin* pin);

  bool expandableDriver(odb::dbITerm* driver);

  // Take in gates below the cut for as long as that brings its output count
  // down.  Grows the cone; leaves and outputs move with it.
  void expandForward(std::vector<const sta::Pin*>& leaves,
                     ConePins& drivers_in_cone,
                     std::unordered_map<const sta::Pin*, int>& cut_users,
                     int& ngates,
                     int& nouterfans);
  bool takeInOneGate(std::vector<const sta::Pin*>& leaves,
                     ConePins& drivers_in_cone,
                     std::unordered_map<const sta::Pin*, int>& cut_users,
                     int& ngates,
                     int& nouterfans);

  // Take `g` in if it accounts for two or more of the cut's outputs.
  bool takeInGate(odb::dbInst* g,
                  std::vector<const sta::Pin*>& leaves,
                  ConePins& drivers_in_cone,
                  std::unordered_map<const sta::Pin*, int>& cut_users,
                  int& ngates,
                  int& nouterfans);

  // Recover the cone instances in a deterministic order, pick out the drivers
  // which are outputs of the cut, and describe the cut as a `GateNetwork` over
  // `leaves`.
  void collectCone(const std::vector<const sta::Pin*>& leaves,
                   const ConePins& drivers_in_cone,
                   const std::unordered_map<const sta::Pin*, int>& cut_users,
                   std::vector<odb::dbInst*>& cone,
                   std::vector<const sta::Pin*>& roots,
                   GateNetwork& net);

  void expandCut(int wedge,
                 std::vector<const sta::Pin*> leaves,
                 int ngates,
                 int mffc_size,
                 int nouterfans,
                 ConePins drivers_in_cone,
                 std::unordered_map<const sta::Pin*, int> cut_users,
                 std::unordered_map<const sta::Pin*, int> mffc_users);

  // Total cell area `net` is built from.
  float networkArea(const GateNetwork& net);

  // Render `net` onto a single line for debug output.
  std::string formatGateNetwork(const GateNetwork& net,
                                const std::vector<const sta::Pin*>& leaves);

  void evaluateCut(const std::vector<const sta::Pin*>& roots,
                   const std::vector<const sta::Pin*>& leaves,
                   const std::vector<odb::dbInst*>& cone,
                   const GateNetwork& net,
                   int mffc_size);

  struct Substitution
  {
    std::vector<const sta::Pin*> leaves;
    GateNetwork original;
    GateNetwork replacement;
    float gain;
  };

  // Build `s.replacement` into the netlist and note the drivers it obsoletes.
  bool applySubstitution(const Substitution& s, ConePins& obsoleted);
  // Best over the cuts enumerated for the pin in hand
  std::optional<Substitution> pin_best_;
  // One per pin that had anything worth doing, over the whole run
  std::vector<Substitution> substitutions_;

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  rsz::Resizer* resizer_ = nullptr;
  Synthesis* synthesis_ = nullptr;

  // Pin the current enumeration is rooted at
  const sta::Pin* head_ = nullptr;

  const sta::Scene* corner_ = nullptr;
  // Input slew delay estimation assumes, indexed by sta::RiseFall
  float fixed_slew_[2] = {0.0f, 0.0f};
  // Cells `synthesize` gets to build out of
  cm::TargetIndex index_;
  // Kept across `synthesize` calls: the matches carry over between cuts
  std::optional<MatchCache> match_cache_;
  DelayEstimationParameters dparams_;
  // Slack has to improve by at least this much to be worth anything
  float epsilon_ = 0.0f;
  // Worst slack in the design, which criticality is measured against
  float wns_ = 0.0f;

  // Counts up to keep the names of what we build apart
  int made_ = 0;

  // Where the work goes.  Reported at the end of a `resynthesize` call.
  struct Stats
  {
    long long heads = 0;
    long long expansions = 0;     // expandCut calls
    long long cuts = 0;           // cuts that reached evaluateCut
    long long syntheses = 0;      // synthesize calls
    long long forward_scans = 0;  // takeInOneGate calls
    long long taken_in = 0;       // gates taken in from below
    long long explores = 0;       // branch-and-bound nodes across all syntheses
    long long worst_explore = 0;  // most any one synthesize took
    double t_sta = 0;
    double t_enumerate = 0;
    double t_forward = 0;
    double t_cut_timing = 0;
    double t_synthesize = 0;
    double t_slack = 0;
    double t_commit = 0;
  };
  Stats stats_;

  // Control for area/slack trade-off; determines how much area is a unit of
  // slack improvement worth
  float timing_opt_effort_ = 0;
  // Set for the duration of a `resynthesize` call
  int max_leaves_ = 0;
  int max_intermediate_leaves_ = 0;
  int max_cells_ = 0;
  // Outputs a cut may have on the way, before taking gates in from below
  // brings the count down.  The final cut still has to be within two.
  int max_outerfans_ = 0;
  bool exclude_buffers_ = false;
  // Let the search factor out intermediates which don't reduce the variable
  // count.  Finds decompositions nothing else will, at a steep price: it is
  // the bulk of the search's work.
  bool allow_lateral_ = false;
};

}  // namespace acd
}  // namespace syn
