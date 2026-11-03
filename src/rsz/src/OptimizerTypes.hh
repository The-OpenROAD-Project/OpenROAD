// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "OptimizationPolicyConfig.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"

namespace sta {
class dbSta;
class Instance;
class LibertyCell;
class LibertyPort;
class MinMax;
class Net;
class Path;
class PathExpanded;
class Pin;
class Pvt;
class RiseFall;
class Scene;
class StaState;
class TimingArc;
class TimingArcSet;
class Vertex;
}  // namespace sta

namespace rsz {
class BufferedNet;
class MoveCandidate;
class Resizer;
class SequentialTimingEstimator;
struct SizeDownFanoutMtLoad;
struct SplitLoadMtInput;
struct SwapPinsMtInput;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
}  // namespace rsz

namespace rsz {

// === Optimizer message IDs ================================================

// Message IDs shared across multiple call sites
inline constexpr int kMsgCloneOutputPinMissing = 100;
inline constexpr int kMsgRepairSetupExpectedMaxPath = 500;
// The serial and Mt pin-swap generators simulate cell functions the same way.
inline constexpr int kMsgUnknownFuncExprOp = 91;

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
  kMissingCurrentTimingArc,

  // Candidate timing context.
  kMissingCandidatePort,
  kMissingCandidateTimingArc,

  // Buffer insertion timing context.
  kInvalidBufferInsertion,
  kMissingBufferTimingArc,

  // A modeled side path pushed below the prepared global worst slack.
  kSideSlackFloorViolation,

  // The opposite output transition of the target would fall below the floor.
  kDualRfNonImproving
};

// === Fail-reason labels =====================================================

const char* failReasonName(FailReason reason);

// === Timing arc matching ====================================================

// kExact            : require full conditional/default/mode equivalence
//                     (current cell, fanin/fanout lookups, strict path data).
// kRelaxedCandidate : fall back to same port-name and RF matching when no
//                     exact arc exists.  For sizing/VtSwap target candidate
//                     analysis only.
enum class ArcMatchMode : uint8_t
{
  kExact,
  kRelaxedCandidate
};

enum class ArcMatchType : uint8_t
{
  kNone,
  kExact,
  kRelaxed
};

ArcMatchType matchTimingArc(const sta::TimingArc* reference,
                            const sta::TimingArc* candidate_arc,
                            ArcMatchMode match_mode);

const sta::TimingArc* findMatchingTimingArc(const sta::TimingArc* reference,
                                            const sta::TimingArcSet* candidate,
                                            ArcMatchMode match_mode,
                                            ArcMatchType* match_type);

// True when candidate drive resistance is at most the current value on both
// rise and fall.
bool driveResNoHigherOnBothRf(const sta::LibertyPort* current,
                              const sta::LibertyPort* candidate);

// === Target preparation flags ==============================================

// The target-level data the generators read, prepared for every target of a
// prepare stage.  A generator's own inputs come from its prepareInputs().
using PrepareCacheMask = uint32_t;

inline constexpr PrepareCacheMask kNoPrepareCache = 0;
inline constexpr PrepareCacheMask kEstimatorContextCache = 1u << 0;
// The flip-flop input-side checks a cell swap of the target may disturb
// (SequentialTimingEstimator); needs kEstimatorContextCache.
inline constexpr PrepareCacheMask kSequentialEstimatorCache = 1u << 2;

// The functionally equivalent input ports of a cell by input port.  The
// library stays fixed within a run, so the policy shares one cache across
// the targets of SwapPinsMt.
using EquivPinCache = std::unordered_map<const sta::LibertyPort*,
                                         std::vector<sta::LibertyPort*>>;

// === Prepared timing data ===================================================

// Identifies one Liberty timing arc on the current worst path, together with
// the analysis corner, PVT condition, and min/max sense needed to evaluate the
// NLDM/CCS delay table.
struct SelectedArc
{
  const sta::Scene* scene{nullptr};
  const sta::MinMax* min_max{nullptr};
  const sta::Pvt* pvt{nullptr};
  const sta::TimingArc* ref_arc{nullptr};

  const sta::LibertyPort* inputPort() const;
  const sta::LibertyPort* outputPort() const;
  const sta::RiseFall* inputRiseFall() const;
  const sta::RiseFall* outputRiseFall() const;
  sta::LibertyCell* currentCell() const;
};

const sta::TimingArc* findMatchingTimingArc(const sta::TimingArc* reference,
                                            const sta::TimingArcSet* candidate);

// Return the latch D-side path attached to an expanded latch-through path.
const sta::Path* latchDataPath(const sta::PathExpanded& expanded);

// Walk the chained latch D-side fanin segments of an expanded path.  Each
// visitor PathExpanded reference is valid only during that visitor call.
// A visitor returns true to stop walking and false to continue.
bool visitLatchFaninSegments(
    const sta::PathExpanded& expanded,
    const sta::StaState* sta,
    const std::function<bool(const sta::Path*, sta::PathExpanded&)>& visitor);

// Visit the main expanded path and every chained latch D-side fanin segment.
// A visitor returns true to stop walking and false to continue.
bool visitPathSegments(
    const sta::Path* path,
    sta::PathExpanded& expanded,
    const sta::StaState* sta,
    const std::function<bool(const sta::Path*, sta::PathExpanded&)>& visitor);

// One timing arc that can contribute to the driver's output slew merge for the
// selected output transition.  STA merges output slew across all such arcs
// before the next stage consumes it.
struct OutputSlewMergeArc
{
  const sta::TimingArc* ref_arc{nullptr};
  float input_slew{0.0f};
  float current_model_slew{0.0f};
};

// Three-point load-dependent STA driver waveform slew correction for one
// table-worst output-slew arc.  This is prepared on the main thread and then
// interpolated by worker threads.
struct SlewBiasSample
{
  float load_cap{0.0f};
  float table_slew{0.0f};
  float sta_slew{0.0f};
};

// Three sampled corrections from Liberty output slew to current STA slew.
struct SlewBiasModel
{
  // False when no stable correction could be sampled.
  bool valid{false};
  // Arc that controls the merged output slew at the sampled operating point.
  const sta::TimingArc* table_worst_arc{nullptr};
  float input_slew{0.0f};
  std::array<SlewBiasSample, 3> samples;
};

// A target cell arc that drives the opposite output transition through the
// selected input.  Workers use it to charge a candidate that speeds up one
// transition but slows the other.
struct OppositeRfArc
{
  const sta::TimingArc* ref_arc{nullptr};
  float input_slew{0.0f};
  float current_model_delay{0.0f};
};

// Snapshot of one timing stage's electrical state needed for table-model delay
// estimation.  MT policies prepare this on the main thread and then worker
// threads read it from Target without touching STA analysis state.
struct DelayStageState
{
  SelectedArc arc;
  sta::Pin* driver_pin{nullptr};
  float input_slew{0.0f};
  float load_cap{0.0f};
  float current_model_delay{0.0f};  // Table-model cell delay baseline
  float current_delay{0.0f};        // STA graph cell delay baseline
  float current_wire_delay{0.0f};   // Selected path wire delay after driver
  float current_model_slew{0.0f};   // Table-model merged slew baseline
  float current_slew{0.0f};         // STA graph merged slew baseline
  std::vector<OutputSlewMergeArc> output_slew_merge_arcs;
  SlewBiasModel sta_slew_bias;
  // Target stage only: the opposite-transition arcs, empty when dual_rf_eval
  // is off.
  std::vector<OppositeRfArc> opposite_rf_arcs;
  int path_index{-1};
};

// One load pin whose slack pays for the delay a candidate adds to it: a load
// on a target fanin net, or a load a split moves to a new driver.  weight
// scales the charged loss: a pin on a violating worst path moves its endpoint
// one for one, other pins only lose slack locally.
struct SideLoad
{
  const sta::Pin* pin{nullptr};
  float slack{0.0f};
  float weight{1.0f};
};

// One input net of the target instance.  A new cell changes every input pin
// capacitance, so each fanin driver slows its other loads.  The selected-arc
// net takes the window's exact delta, the others the drive resistance.
struct FaninSideNet
{
  const sta::LibertyPort* input_port{nullptr};
  float input_cap{0.0f};
  float delay_per_cap{0.0f};
  bool selected_arc{false};
  std::vector<SideLoad> loads;
};

// Prepared delay-estimator context for a target stage and its optional
// N-level fanin/fanout timing window.  path_stages always contains at least
// one entry (the target stage) after a successful buildContext; consumers
// access electrical state via target() rather than duplicated top-level
// fields.
struct EstimatorContext
{
  std::vector<DelayStageState> path_stages;
  int target_stage_index{0};
  int delay_estimation_levels{0};

  // The target's fanin nets and their other loads, empty when fanin_side_loss
  // is off.
  std::vector<FaninSideNet> fanin_side_nets;
  // The worst slack at prepare time.  A side path may fall below it by at most
  // side_floor_margin times the candidate's gain, or the checkpoint would fail.
  float side_slack_floor{-std::numeric_limits<float>::infinity()};
  float side_floor_margin{1.0f};
  // Weight of the summed side loss before it is subtracted from the gain.
  float fanin_side_loss_weight{1.0f};
  // Worst slack of the opposite output transition at the target driver; the
  // dual rise/fall check treats that transition as one more side path.
  float opposite_rf_slack{std::numeric_limits<float>::infinity()};
  // Cached sum of stage.current_delay across path_stages; populated once at
  // build time so per-candidate scoring does not re-walk the window.
  float current_total_delay{0.0f};

  const DelayStageState& target() const
  {
    return path_stages[target_stage_index];
  }
  // The lowest slack a side path may fall to for a candidate that gains gain.
  float sideSlackFloor(const float gain) const
  {
    return side_slack_floor - side_floor_margin * std::max(gain, 0.0f);
  }
};

// Raw delay-estimator result before a candidate converts it into a
// policy-level Estimate.
struct DelayEstimate
{
  bool legal{false};
  float candidate_delay{0.0f};
  float arrival_impr{0.0f};
  int side_paths_evaluated{0};
  // The weighted slack loss of the side paths in the negative-slack region,
  // zero when none.
  float side_loss{0.0f};
  FailReason reason{FailReason::kNone};
};

// Lumped resistance and capacitance of one estimated wire segment.
struct WireRC
{
  float length{0.0f};
  float res{0.0f};
  float cap{0.0f};
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

// Current slack and driver-to-load wire delay of one load on a RebufferMt
// target net, with the weight the side-load model gives it.
struct RebufferLoadTiming
{
  float slack{0.0f};
  float wire_delay{0.0f};
  float weight{1.0f};
};

using RebufferLoadTimingMap
    = std::unordered_map<const sta::Pin*, RebufferLoadTiming>;

inline constexpr TargetViewMask kPathDriverView = 1u << 0;
inline constexpr TargetViewMask kInstanceView = 1u << 1;

// Repairable instance or path driver with timing context prepared for moves.
struct Target
{
  // === Target identity ======================================================
  TargetViewMask views{kPathDriverView};

  // Output-side pin that identifies the move target.
  // - kPathDriverView: target driver output pin
  // - kInstanceView: target instance output pin
  sta::Pin* driver_pin{nullptr};

  // The current path-segment endpoint survives STA refresh; Path* cannot.
  const sta::Pin* endpoint_pin{nullptr};

  // Original failing endpoint that caused this target to be collected.  This
  // differs from endpoint_pin for latch D-side fanin segments.  The endpoint
  // collector sets it; the LEGACY policies leave it null.
  const sta::Pin* violated_endpoint_pin{nullptr};
  // Transition of the selected driver path.
  const sta::RiseFall* driver_rf{nullptr};

  // Endpoint-side timing path handle.  Expanding this path exposes the full
  // startpoint-to-endpoint chain.
  const sta::Path* endpoint_path{nullptr};
  int path_index{-1};

  // Selected driver stage on `path`.  inputPath() and prevDriverPath() walk
  // backward from this stage with Path::prevPath().
  const sta::Path* driver_path{nullptr};
  // The load the driver feeds on the path, captured while the path is valid:
  // a through-filtered path end does not survive the next search.  Null when
  // the path ends at the driver.
  const sta::Pin* load_pin{nullptr};
  int fanout{0};
  sta::Slack slack{0.0};
  // scene and min_max are captured at construction (while endpoint_path is
  // valid) so later moves in a repair sequence can read them after earlier
  // commits have invalidated endpoint_path.  Access via activeScene()/minMax().
  const sta::Scene* scene{nullptr};
  const sta::MinMax* min_max{nullptr};

  // === Prepared data for MT generation/estimation ==========================
  std::optional<EstimatorContext> estimator_context;
  // Prepared flip-flop input-side checks for candidate Q-path swaps.
  std::shared_ptr<const SequentialTimingEstimator> sequential_timing_estimator;

  // === Generation inputs (MoveGenerator::prepareInputs()) ===================
  // Fanin max-cap headroom of the target instance (the cell-swap
  // generators); the STA capacitance check behind it is main-thread only.
  std::optional<std::vector<Resizer::InputCapBudget>> input_cap_budgets;
  // RebufferMt: the driver net as a buffered-net tree and, with the
  // side-load model, the current timing of its loads (empty without it).
  BufferedNetPtr rebuffer_net;
  RebufferLoadTimingMap rebuffer_load_timing;
  // SplitLoadMt: the split prepareInputs() chose from live STA, null when it
  // has none.  SwapPinsMt: the arc delays prepareInputs() measured through
  // the equivalent inputs, from which generate() picks the fastest; null when
  // the gate has none.
  // UnbufferMt: the delay that removing the target's buffer is estimated to
  // save, unset when the buffer cannot be removed.
  // SizeDownFanoutMt: the measured loads generate() decides from, empty when no
  // load has a smaller cell.  The candidates share these.
  std::shared_ptr<const SplitLoadMtInput> split_load_input;
  std::shared_ptr<const SwapPinsMtInput> swap_pins_input;
  std::optional<float> unbuffer_gain;
  std::vector<std::shared_ptr<const SizeDownFanoutMtLoad>>
      size_down_fanout_loads;

  // === Field validation ====================================================
  bool canBePathDriver() const;
  bool canBeInstance() const;

  // === Lazy STA/OpenDB resolution ==========================================
  std::string name(const Resizer& resizer) const;
  const sta::Pin* endpointPin(const Resizer& resizer) const;
  const sta::Pin* violatedEndpointPin(const Resizer& resizer) const;
  sta::Pin* resolvedPin(const Resizer& resizer) const;
  sta::Instance* inst(const Resizer& resizer) const;
  sta::Vertex* vertex(const Resizer& resizer) const;
  const sta::Scene* activeScene(const Resizer& resizer) const;
  const sta::Mode* mode(const Resizer& resizer) const;
  const sta::MinMax* minMax(const Resizer& resizer) const;
  const sta::Path* driverPath(const Resizer& resizer) const;

  // input stage of the driver_path
  const sta::Path* inputPath(const Resizer& resizer) const;

  // previous driver of the driver_path
  const sta::Path* prevDriverPath(const Resizer& resizer) const;
};

// What a candidate's ECO writes and what its timing reads, from prepared data.
// The batch selector rejects write conflicts and candidates whose timing
// depends on a batch member.
struct MoveImpact
{
  // False when an estimator could not enumerate all reads and writes.
  bool complete{false};
  // Database objects mutated by applying the candidate.
  std::vector<sta::Instance*> write_instances;
  std::vector<sta::Net*> write_nets;
  // Connectivity changed by a structural move.
  std::vector<const sta::Pin*> reconnect_pins;
  std::vector<sta::Instance*> reconnect_instances;
  // Objects whose timing the candidate can alter after application.
  std::vector<sta::Instance*> timing_effect_instances;
  std::vector<sta::Net*> timing_effect_nets;
  // Objects whose timing the candidate estimate read before application.
  std::vector<sta::Instance*> timing_dependency_instances;
  std::vector<sta::Net*> timing_dependency_nets;
};

Target makePathDriverTarget(const sta::Path* endpoint_path,
                            const sta::PathExpanded& expanded,
                            int path_index,
                            sta::Slack slack,
                            const Resizer& resizer,
                            const sta::Pin* violated_endpoint_pin = nullptr);

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
  // Unscaled arrival_impr in seconds; absent if estimation did not complete.
  std::optional<float> raw_gain{};
  // Non-owning static label: failReasonName() or a candidate's own literal
  // such as "side_loss_non_improving" or "min_score".
  std::string_view reason{};
  // A reject by a modelled side cost alone while the own path gains.  The WNS
  // tail may admit it and let the batch STA judge the side cost.  The moves
  // set it for their side-loss, sequential-check and score-floor rejects;
  // only SizeUpMt sets it for an opposite-transition reject.
  bool tail_admissible{false};
  // Opposite-side constraint loss stays separate from primary timing gain.
  std::optional<float> sequential_penalty{};
  std::optional<float> sequential_side_loss{};
  // Estimated side-path slack loss charged against the local gain.
  std::optional<float> side_loss{};
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
  // Worker-local profiling time, merged only after all estimates complete.
  double worker_elapsed{0.0};

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
  bool skip_size_down_fanout{false};
  bool skip_buffering{false};
  bool skip_buffer_removal{false};
  bool skip_buffer_to_inverters{false};
  bool skip_last_gasp{false};
  bool skip_vt_swap{false};
  bool skip_crit_vt_swap{false};
  // Stop-rule preset of PARALLEL_EVAL; the other policies ignore it.
  RepairEffort effort{RepairEffort::kHigh};
  // Setup repair profiler: 0 off, 1 timing samples, 2 adds the batch and
  // pass events, 3 adds the per-move analysis.  Files go to profile_dir, or
  // "repair_setup_profile" when empty, with one run_N per call.
  int profile_mode{0};
  std::string profile_dir;
  std::vector<MoveType> sequence;
  std::string phases;
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
    case MoveType::kSizeDownFanout:
      return "SizeDownFanoutMove";
    case MoveType::kSwapPins:
      return "SwapPinsMove";
    case MoveType::kVtSwap:
      return "VtSwapMove";
    case MoveType::kUnbuffer:
      return "UnbufferMove";
    case MoveType::kSplitLoad:
      return "SplitLoadMove";
    case MoveType::kReroute:
      return "RerouteMove";
    case MoveType::kBufferToInverters:
      return "BufferToInvertersMove";
    case MoveType::kCount:
      break;
  }
  throw std::runtime_error("Unhandled MoveType in moveName()");
}

}  // namespace rsz
