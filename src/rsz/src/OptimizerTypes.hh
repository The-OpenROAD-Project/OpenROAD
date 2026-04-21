// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "rsz/Resizer.hh"
#include "sta/Delay.hh"

namespace sta {
class dbSta;
class Instance;
class LibertyCell;
class LibertyPort;
class MinMax;
class Path;
class PathExpanded;
class Pin;
class Pvt;
class RiseFall;
class Scene;
class Vertex;
}  // namespace sta

namespace rsz {
class MoveCandidate;
class Resizer;
}  // namespace rsz

namespace rsz {

// === Optimizer message IDs ================================================

// Message IDs shared across multiple call sites
inline constexpr int kMsgCloneOutputPinMissing = 100;
inline constexpr int kMsgRepairSetupExpectedMaxPath = 500;
inline constexpr int kMsgPolicyCommittedMoves = 2023;

enum class FailReason : uint8_t
{
  // General status.
  kNone,
  kEstimateLegal,
  kEstimateNonImproving,

  // Target identity and move eligibility.
  kMissingTargetInstance,

  // Path and current timing-arc context.
  kPathIndexOutOfRange,
  kMissingPrevTimingArc,
  kMissingArcTransition,
  kMissingSceneOrMinMax,
  kMissingOutputPort,
  kMissingCurrentPortMap,
  kInvalidSelectedArc,
  kMissingInputSlewArc,
  kMissingCurrentTimingArc,

  // Prepared arc delay state.
  kInvalidContext,

  // Candidate timing context.
  kMissingCandidatePort,
  kMissingCandidateTimingArc
};

// === Fail-reason labels =====================================================

const char* failReasonName(FailReason reason);

// === Target preparation flags ==============================================

using PrepareCacheMask = uint32_t;

inline constexpr PrepareCacheMask kNoPrepareCache = 0;
inline constexpr PrepareCacheMask kArcDelayStateCache = 1u << 0;

// === Prepared timing data ===================================================

// Identifies one Liberty timing arc (input->output, rise/fall) on the current
// worst path, together with the analysis corner, PVT condition, and min/max
// sense needed to evaluate the NLDM/CCS delay table.
struct SelectedArc
{
  const sta::Scene* scene{nullptr};
  const sta::MinMax* min_max{nullptr};
  const sta::Pvt* pvt{nullptr};
  const sta::LibertyPort* input_port{nullptr};
  const sta::LibertyPort* output_port{nullptr};
  const sta::RiseFall* in_rf{nullptr};
  const sta::RiseFall* out_rf{nullptr};
};

// Snapshot of one timing arc's electrical state needed for table-model delay
// estimation.  MT policies prepare this on the main thread and then worker
// threads read it from Target without touching STA analysis state.
struct ArcDelayState
{
  bool isValid() const;

  SelectedArc arc;
  float input_slew{0.0f};
  float load_cap{0.0f};
  float current_delay{0.0f};  // Original delay before optimization
};

// Raw delay-estimator result before a candidate converts it into a
// policy-level Estimate.
struct DelayEstimate
{
  bool legal{false};
  float candidate_delay{0.0f};
  float arrival_impr{0.0f};
  FailReason reason{FailReason::kNone};
};

// === Optimization target model =============================================

// One optimization target selected by a policy.  A Target is the handle a
// generator/candidate pair uses to locate the instance and surrounding arc
// that a move should modify.
//
// Why lazy resolution: STA objects (Vertex/Instance) become stale every
// time an ECO is journaled, committed, or restored (pointer invalidation on
// delete/re-create, re-levelization on buffer insertion, etc.).  The Target
// keeps a policy-facing identity (views + path + driver_pin + slack) that
// survives those rebuilds; STA handles are resolved on demand through
// resolvedPin() / inst() / vertex().  The driver path is the selected stage
// point on `path`; inputPath() and prevDriverPath() walk backward from it when
// needed.
//
// Target view bits identify each way a target can be consumed by move
// generators:
//   - kPathDriverView : driver of one stage on a timing path (most moves)
//   - kInstanceView   : target instance output pin, usable by instance-level
//   moves
// A single target may provide multiple views, for example a path-driver target
// can also expose the instance reached through driver_pin.
using TargetViewMask = uint8_t;

inline constexpr TargetViewMask kPathDriverView = 1u << 0;
inline constexpr TargetViewMask kInstanceView = 1u << 1;

struct Target
{
  // === Target identity ======================================================
  TargetViewMask views{kPathDriverView};

  // Output-side pin that identifies the move target.
  // - kPathDriverView: target driver output pin
  // - kInstanceView: target instance output pin
  sta::Pin* driver_pin{nullptr};

  // Endpoint-side timing path handle.  Expanding this path exposes the full
  // startpoint-to-endpoint chain.
  const sta::Path* endpoint_path{nullptr};
  int path_index{-1};

  // Selected driver stage on `path`.  inputPath() and prevDriverPath() walk
  // backward from this stage with Path::prevPath().
  const sta::Path* driver_path{nullptr};
  int fanout{0};
  sta::Slack slack{0.0};
  const sta::Scene* scene{nullptr};

  // === Prepared data for MT generation/estimation ==========================
  std::optional<ArcDelayState> arc_delay;

  // === Field validation ====================================================
  bool canBePathDriver() const;
  bool canBeInstance() const;
  // Check whether the per-target fields required by a prepare mask are set.
  bool isPrepared(PrepareCacheMask mask) const;

  // === Lazy STA/OpenDB resolution ==========================================
  std::string name(const Resizer& resizer) const;
  const sta::Pin* endpointPin(const Resizer& resizer) const;
  sta::Pin* resolvedPin(const Resizer& resizer) const;
  sta::Instance* inst(const Resizer& resizer) const;
  sta::Vertex* vertex(const Resizer& resizer) const;
  const sta::Scene* activeScene(const Resizer& resizer) const;
  const sta::MinMax* minMax(const Resizer& resizer) const;
  const sta::Path* driverPath(const Resizer& resizer) const;

  // input stage of the driver_path
  const sta::Path* inputPath(const Resizer& resizer) const;

  // previous driver of the driver_path
  const sta::Path* prevDriverPath(const Resizer& resizer) const;
};

Target makePathDriverTarget(const sta::Path* endpoint_path,
                            const sta::PathExpanded& expanded,
                            int path_index,
                            sta::Slack slack,
                            const Resizer& resizer);

// === Estimation inputs and results =========================================

// Input bundle for legacy slack-estimation helpers (e.g., Rebuffer-based
// unbuffer/splitload flows) that compare a proposed topology change against
// the setup slack margin on one analysis corner.
//
// Only the members actually needed for one helper are populated -- other
// fields stay null.  `scene` is the single analysis view the caller is
// evaluating on; callers that need multi-scene slack loop over scenes and
// build one SlackEstimatorParams per iteration.
struct SlackEstimatorParams
{
  SlackEstimatorParams(float margin, const sta::Scene* scene)
      : setup_slack_margin(margin), scene(scene)
  {
  }

  sta::Pin* driver_pin{nullptr};
  sta::Pin* prev_driver_pin{nullptr};
  sta::Pin* driver_input_pin{nullptr};
  sta::Instance* driver{nullptr};
  const sta::Path* driver_path{nullptr};
  const sta::Path* prev_driver_path{nullptr};
  sta::LibertyCell* driver_cell{nullptr};
  const float setup_slack_margin;
  const sta::Scene* scene;
};

// Move-local score reported by MoveCandidate::estimate() before an ECO is
// committed.
//
// Contract:
//   - legal==false : the candidate is infeasible/non-improving for this
//                    target; policies must skip it regardless of `score`.
//   - legal==true  : score is a move-type-normalized cost where *higher is
//                    better* (larger arrival improvement / larger slack win).
//                    Policies compare scores across candidates of possibly
//                    different types; policies keep scores comparable across
//                    the move types they evaluate together.
struct Estimate
{
  bool legal{false};
  float score{0.0f};
};

// Result of applying one candidate ECO (MoveCandidate::apply()).  The
// committer consumes this to keep move accounting and tracker reports in sync
// with what actually changed on OpenDB/STA.
//
// Fields:
//   accepted           : true if the ECO was applied successfully.  On
//                        rejection the committer rolls the journal back.
//   type               : MoveType value that produced this result.
//   move_count         : number of primary-type moves this result should
//                        add to pending counters (usually 1; unbuffer/clone
//                        may credit more).
//   touched_instances  : instances the ECO mutated; the committer marks them
//                        for tracker re-visit logic and conflict detection.
struct MoveResult
{
  bool accepted{false};
  MoveType type{MoveType::kCount};
  int move_count{0};
  std::vector<sta::Instance*> touched_instances;
};

// === Candidate evaluation data =============================================

using CandidateVector = std::vector<std::unique_ptr<MoveCandidate>>;

// Worker-produced result for one prepared target.  Policies fill candidates
// and estimates in matching order, then keep a raw pointer to the candidate
// they will commit from the `candidates` owner vector.  Ranking policies choose
// the best score; legacy-order policies can choose the first legal candidate.
struct TargetEvaluation
{
  CandidateVector candidates;
  std::vector<Estimate> estimates;
  MoveCandidate* best_candidate{nullptr};
  Estimate best_estimate;

  bool hasBestCandidate() const { return best_candidate != nullptr; }
  MoveCandidate& bestCandidate() const { return *best_candidate; }
};

// === Run configuration ======================================================

// Full repair_setup run configuration captured from the Tcl/API entry point
// (Optimizer::repairSetup) and frozen for the lifetime of one run.
//
// Every policy and generator receives this via start()/GeneratorContext and
// must treat it as read-only.  Mutable per-iteration state lives in the
// policy's *RepairState structs, not here.  `sequence` and `phases` together
// describe the user-specified move schedule; the `skip_*` flags suppress
// individual move types without having to modify `sequence`.
struct OptimizerRunConfig
{
  double setup_slack_margin{0.0};
  double repair_tns_end_percent{1.0};
  int max_iterations{0};
  int max_passes{100};
  int max_repairs_per_pass{1};
  bool match_cell_footprint{false};
  bool verbose{false};
  bool skip_pin_swap{false};
  bool skip_gate_cloning{false};
  bool skip_size_down{false};
  bool skip_buffering{false};
  bool skip_buffer_removal{false};
  bool skip_last_gasp{false};
  bool skip_vt_swap{false};
  bool skip_crit_vt_swap{false};
  std::vector<MoveType> sequence;
  std::string phases;
};

// Narrow immutable option view for repair-setup helper phases (last-gasp,
// directional repair, VT-swap of critical cells) that only need the
// slack-margin plus skip flags, not the full OptimizerRunConfig surface.
// All fields are const so helpers cannot silently mutate per-run options.
struct RepairSetupParams
{
  const float setup_slack_margin;
  const bool verbose;
  const bool skip_pin_swap;
  const bool skip_gate_cloning;
  const bool skip_size_down;
  const bool skip_buffering;
  const bool skip_buffer_removal;
  const bool skip_vt_swap;
};

// Policy-to-generator configuration channel for tunable knobs that do not
// belong in the OptimizerRunConfig.
struct OptPolicyConfig
{
  // Cap on how many per-target candidates a generator expands.
  int max_candidate_generation{0};

  // Experimental. Hard cap on accepted moves for policies that support bounded
  // commits. A value of 0 means unlimited.
  int max_committed_moves{0};
};

// === Move type labels ======================================================

inline const char* moveName(const MoveType move_type)
{
  switch (move_type) {
    case MoveType::kBuffer:
      return "BufferMove";
    case MoveType::kClone:
      return "CloneMove";
    case MoveType::kSizeUp:
      return "SizeUpMove";
    case MoveType::kSizeUpMatch:
      return "SizeUpMatchMove";
    case MoveType::kSizeDown:
      return "SizeDownMove";
    case MoveType::kSwapPins:
      return "SwapPinsMove";
    case MoveType::kVtSwap:
      return "VtSwapMove";
    case MoveType::kUnbuffer:
      return "UnbufferMove";
    case MoveType::kSplitLoad:
      return "SplitLoadMove";
    case MoveType::kCount:
      break;
  }
  throw std::runtime_error("Unhandled MoveType in moveName()");
}

}  // namespace rsz
