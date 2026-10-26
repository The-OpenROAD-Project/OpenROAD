// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "BufferedNet.hh"
#include "OptimizerTypes.hh"
#include "absl/container/flat_hash_map.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/geom.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Scene.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace est {
class EstimateParasitics;
}

namespace rsz {

class Resizer;
class BufferedNet;
class BufferCandidate;
class SetupLegacyBase;
enum class BufferedNetType;
using BufferedNetPtr = std::shared_ptr<BufferedNet>;
using BufferedNetSeq = std::vector<BufferedNetPtr>;

enum class BufferTreeStageType
{
  kWire,
  kVia,
  kBuffer
};

// One wire, via, or buffer encountered on a planned root-to-load path.
struct BufferTreePathStage
{
  BufferTreeStageType type{BufferTreeStageType::kWire};
  const sta::LibertyCell* buffer_cell{nullptr};
  WireRC wire_rc;
  // Capacitance seen below this stage in the proposed tree.
  float downstream_cap{0.0f};
  // True when the stage drives more than one load branch.
  bool drives_distributed_load{false};
};

// Ordered stages from the planned tree root to one load pin.
struct BufferTreeLoadPath
{
  const sta::Pin* load_pin{nullptr};
  std::vector<BufferTreePathStage> stages;

  // True when a buffer stage follows the stage at index.
  bool hasBufferAfter(size_t index) const;
};

// Buffer-tree proposal with the paths and loading needed for MT scoring.
struct RebufferTreePlan
{
  // Planner-owned tree kept alive until the candidate is applied.
  BufferedNetPtr tree;
  sta::Net* flat_net{nullptr};
  sta::Pin* driver_pin{nullptr};
  sta::Instance* parent{nullptr};
  const sta::Pin* target_load_pin{nullptr};
  // Root-to-load paths of every load; the target load's path carries the
  // primary timing estimate, the others the side-load timing.
  std::vector<BufferTreeLoadPath> load_paths;
  float tree_input_cap{0.0f};
  // Driver pin slack of the tree minus that of the current net.
  float driver_slack_gain{0.0f};
  // The anchor tree: the lineage of top-ranked options through the beam
  // rounds, kept in a reserved slot and exempt from the score gates.  Its
  // selector is the top-K one, so it is not the tree the serial Rebuffer would
  // build.
  bool anchor_tree{false};
  int buffer_count{0};
  int load_count{0};

  // The load path of load_pin, or nullptr when the tree does not reach it.
  const BufferTreeLoadPath* loadPath(const sta::Pin* load_pin) const;
  // The load path of the target load, or nullptr when the tree does not reach
  // it.
  const BufferTreeLoadPath* targetPath() const;
};

// Plans and applies timing-driven buffer trees for a driver net.
class Rebuffer : public sta::dbStaState
{
 public:
  Rebuffer(Resizer* resizer);
  ~Rebuffer() override;
  void fullyRebuffer(sta::Pin* user_pin = nullptr);
  void rebufferNet(const sta::Pin* drvr_pin);
  // Prepare makeRebufferTreePlans() on `corner`.  A worker planner gets its own
  // delay calculator, because OpenSTA's keeps per-call scratch state.
  void initForPlanning(sta::Scene* corner, bool worker = false);

  // The warnings a worker planner counted instead of logging them, by
  // message ID (see countWorkerWarning()).
  const std::map<int, int>& workerWarningCounts() const
  {
    return worker_warning_counts_;
  }
  // The driver's net as a BufferedNet on the planner's corner, or nullptr
  // with a warning when the driver cannot be buffered.  Main thread only: the
  // global-route reader behind it is not thread-safe.
  BufferedNetPtr makeBufferedNet(const sta::Pin* drvr_pin);
  // Tree plans over a net from makeBufferedNet().  The caller keeps the
  // required times current; the planner reads them without an STA refresh.
  std::vector<RebufferTreePlan> makeRebufferTreePlans(
      const sta::Pin* drvr_pin,
      BufferedNetPtr net,
      const sta::Pin* target_load_pin,
      int max_plans);
  // Materialize a scored tree in OpenDB and return its inserted-buffer count.
  int applyRebufferTreePlan(const RebufferTreePlan& plan,
                            const char* instance_base_name);

 protected:
  // find_max_wire_length is false for a worker planner (see
  // initForPlanning()).
  void init(bool find_max_wire_length = true);
  void initOnCorner(sta::Scene* corner);

  void annotateLoadSlacks(BufferedNetPtr& tree, sta::Vertex* root_vertex);
  void annotateTiming(const BufferedNetPtr& tree);
  BufferedNetPtr stripTreeBuffers(const BufferedNetPtr& tree);
  BufferedNetPtr resteiner(const BufferedNetPtr& tree);
  BufferedNetPtr bufferForTiming(const BufferedNetPtr& tree,
                                 bool allow_topology_rewrite = true);
  BufferedNetPtr recoverArea(const BufferedNetPtr& root,
                             FixedDelay slack_target,
                             float alpha);

  void setPin(sta::Pin* drvr_pin);

  std::tuple<sta::Delay, sta::Delay, sta::Slew> drvrPinTiming(
      const BufferedNetPtr& bnet);
  FixedDelay slackAtDriverPin(const BufferedNetPtr& bnet);
  // Slack at the driver pin if the driver were resized to `cell`, including
  // the extra delay a larger input capacitance adds to the previous stage.
  // Empty when `cell` has no equivalent arc or violates slew limits.
  std::optional<FixedDelay> slackWithDriverCell(const BufferedNetPtr& bnet,
                                                sta::LibertyCell* cell);
  // False when resizing the driver to `cell` would slow the previous stage
  // enough to push one of its other loads below both its current slack and
  // `slack_limit`.
  bool upstreamSideLoadsOk(sta::LibertyCell* cell, FixedDelay slack_limit);
  // Pick the smallest (tree + driver) area among the candidate trees and the
  // driver's swappable cells whose slack meets `slack_target`, the target the
  // tree search used with the current driver.  Returns the chosen tree and
  // sets `chosen_cell`; returns nullptr when no combination meets the target.
  BufferedNetPtr chooseTreeAndDriver(const std::vector<BufferedNetPtr>& trees,
                                     sta::LibertyCell* current_cell,
                                     FixedDelay slack_target,
                                     sta::LibertyCell*& chosen_cell);
  // With -rebuffer_size_driver, choose between `area_tree` and alternative
  // trees derived from `timing_tree`, each with every size of the driver of
  // `drvr_pin`, and resize the driver when that wins.  Returns the tree to
  // export.
  BufferedNetPtr sizeDriverWithTree(const sta::Pin* drvr_pin,
                                    const BufferedNetPtr& timing_tree,
                                    const BufferedNetPtr& area_tree,
                                    FixedDelay slack_target);
  std::optional<FixedDelay> evaluateOption(const BufferedNetPtr& option,
                                           // Only used for debug print.
                                           int index);

  float maxSlewMargined(float max_slew);
  float maxCapMargined(float max_cap);

  bool loadSlewSatisfactory(sta::LibertyPort* driver,
                            const BufferedNetPtr& bnet);

  FixedDelay bufferDelay(sta::LibertyCell* cell,
                         const sta::RiseFallBoth* rf,
                         float load_cap);

  BufferedNetPtr addWire(const BufferedNetPtr& p,
                         odb::Point wire_end,
                         int wire_layer,
                         int level = -1);

  BufferedNetPtr importBufferTree(const sta::Pin* drvr_pin,
                                  const sta::Scene* corner);
  BufferedNetPtr cloneBufferedNetTree(const BufferedNetPtr& tree);
  // Appends the stages from the root to every load of tree.
  void buildLoadPaths(const BufferedNetPtr& tree,
                      std::vector<BufferTreePathStage>& prefix,
                      std::vector<BufferTreeLoadPath>& load_paths);
  // Structural identity of a buffer tree: pre-order, type-tagged encoding
  // where pointers identify pins and cells within one planning call.
  using TreeSignature = std::vector<uint64_t>;

  // Encode the tree structure and referenced cells/pins for deduplication.
  TreeSignature treeSignature(const BufferedNetPtr& tree);
  // Append one subtree to that structural encoding.
  void appendTreeSignature(const BufferedNetPtr& tree,
                           TreeSignature& signature);
  // Gate delays of one buffer output port driving a lumped load, memoized on
  // every gateDelays() input (port, scene, load, target slews). The memo is
  // bypassed while annotated input slews are active and cleared by setPin()
  // for each driver.
  const std::array<sta::ArcDelay, sta::RiseFall::index_count>&
  gateDelaysForLoad(sta::LibertyPort* output,
                    const sta::Scene* scene,
                    float load_cap);

  // === Messages =============================================================
  // The messages the serial and the RebufferMt planning share, each with one
  // ID.  The warnings of tree planning may come from a worker planner, which
  // counts them instead.
  void warnTopPortDriver(const sta::Pin* drvr_pin);
  void warnBufferedNetFailed(const sta::Pin* drvr_pin);
  void warnPathTraceFailed(const sta::Pin* load_pin);
  void warnNoDesignRuleSolution();
  void warnNoViableBuffering(const sta::Pin* drvr_pin, int rounds);
  void errorAreaRecoveryFailed();
  void errorAreaRecoveryUnreproducible();
  // True when a worker planner counted the warning `id` instead of logging
  // it, which would make the log depend on the thread schedule.
  bool countWorkerWarning(int id);
  std::map<int, int> worker_warning_counts_;

  // === Option ranking =======================================================
  // A beam option with the measures the rankings compare.
  struct RankedOption
  {
    BufferedNetPtr option;
    // The driver slack evaluateOption() found when the option was selected;
    // the beam keeps it instead of evaluating the option again.
    FixedDelay slack{};
    // No slack transition: the tree is unconstrained and ranks first.
    bool unconstrained{false};
    // Area recovery: the option meets the slack target, and keeps the best
    // slack within slackLossFloor().
    bool meets_target{false};
    bool preserves_best_slack{false};
    TreeSignature signature{};
    // Position in the generated options: the tie-break, so fully tied trees
    // keep their generation order instead of their heap order.
    int index{0};
  };
  // Timing order: unconstrained, slack, cap, area, buffer count.
  static bool timingOrder(const RankedOption& lhs, const RankedOption& rhs);
  // Area-recovery order: meets the target, preserves the best slack, slack
  // among the others, area among the rest, cap, buffer count, slack, index.
  static bool areaOrder(const RankedOption& lhs, const RankedOption& rhs);
  // The slack an option may fall to below best_slack and still preserve it:
  // 1 ps under a negative best slack, 20 ps under a positive one.
  FixedDelay slackLossFloor(FixedDelay best_slack) const;
  // The timing options ranked by driver slack, each with the slack it was
  // evaluated at: the single best when max_plans is 1, as the serial Rebuffer
  // picks, else the max_plans best.
  std::vector<RankedOption> selectTimingOptions(const BufferedNetSeq& top_opts,
                                                int max_plans);
  std::vector<RankedOption> bufferForTimingTopK(const BufferedNetPtr& tree,
                                                bool allow_topology_rewrite,
                                                int max_plans);
  // The max_plans best area-recovered options of root that meet
  // slack_target, else the one with the best slack.
  std::vector<RankedOption> recoverAreaTopK(const BufferedNetPtr& root,
                                            FixedDelay slack_target,
                                            float alpha,
                                            int max_plans,
                                            bool strict_assured = true);
  // Keep the anchor in a beam of selected options: it replaces its
  // structural twin, else the last slot or a free one.
  static void keepAnchorTree(const std::vector<RankedOption>& ranked_options,
                             const RankedOption& anchor,
                             const TreeSignature& anchor_signature,
                             int plan_limit,
                             std::vector<RankedOption>& selected_options);

  // === RebufferMt tree planning (makeRebufferTreePlans) =====================
  // What one call plans for; the beam helpers read it when they turn a tree
  // into a RebufferTreePlan.
  struct PlanRequest
  {
    const sta::Pin* drvr_pin{nullptr};
    const sta::Pin* target_load_pin{nullptr};
    sta::Net* flat_net{nullptr};
    sta::Instance* parent{nullptr};
    FixedDelay original_slack{};
  };

  // Convert a buffered-net tree to an estimate-ready plan.
  RebufferTreePlan makePlanFromTree(const BufferedNetPtr& tree,
                                    const PlanRequest& request);
  // Drop trees that repeat an earlier tree's structure and record the
  // signature of the others.
  std::vector<RankedOption> uniqueTreeOptions(
      std::vector<RankedOption> options);
  // The plan_limit best trees by driver slack; the anchor keeps a slot.
  std::vector<RankedOption> limitTimingOptions(
      std::vector<RankedOption> options,
      const RankedOption& anchor,
      int plan_limit);
  // The plan_limit best area-recovered trees that meet slack_target; the
  // anchor keeps a slot.
  std::vector<RankedOption> limitAreaOptions(std::vector<RankedOption> options,
                                             FixedDelay slack_target,
                                             const RankedOption& anchor,
                                             int plan_limit);
  // The slack a tree may give back to area recovery.
  FixedDelay areaRecoveryTarget(const BufferedNetPtr& tree);
  // Three rounds of timing-driven buffering over the beam.  Returns false
  // when a round finds no viable tree.  anchor follows the top-ranked option
  // of the previous anchor's expansion.
  bool runTimingBeam(const BufferedNetPtr& bnet,
                     const sta::Pin* drvr_pin,
                     bool allow_topology_rewrite,
                     int plan_limit,
                     std::vector<RankedOption>& beam,
                     RankedOption& anchor);
  // Five rounds of area recovery over the beam, the anchor tree recovered
  // against its own target.
  void runAreaRecoveryBeam(int plan_limit,
                           std::vector<RankedOption>& beam,
                           RankedOption& anchor);
  // Convert the surviving beam options to scored tree proposals.
  std::vector<RebufferTreePlan> plansFromBeam(
      const std::vector<RankedOption>& beam,
      const RankedOption& anchor,
      const PlanRequest& request,
      int plan_limit);
  // Annotate every node of root with its arrival delay from the driver and
  // return the slack target area recovery works against: -INF when the tree
  // is unconstrained.
  FixedDelay annotateArrivalDelays(const BufferedNetPtr& root,
                                   FixedDelay slack_target);
  // Every tree that drops or downsizes buffers of root while each node keeps
  // the slack alpha sets between its current slack and slack_target.  With
  // strict_assured, a node without such an option is an error.  Otherwise the
  // tree yields no option.
  BufferedNetSeq areaRecoveryOptions(const BufferedNetPtr& root,
                                     FixedDelay slack_target,
                                     float alpha,
                                     bool strict_assured);

  bool insertBufferOptions(BufferedNetSeq& opts,
                           int level,
                           int next_segment_wl = 0,
                           bool area_oriented = false,
                           FixedDelay slack_threshold = FixedDelay::ZERO,
                           BufferedNet* exemplar = nullptr,
                           bool strict_assured = true);

  void insertAssuredOption(BufferedNetSeq& opts,
                           BufferedNetPtr assured_opt,
                           int level);

  std::vector<sta::Instance*> collectImportedTreeBufferInstances(
      sta::Pin* drvr_pin,
      const BufferedNetPtr& imported_tree);
  int exportBufferTree(const BufferedNetPtr& choice,
                       sta::Net* net,  // output of buffer.
                       int level,
                       sta::Instance* parent_in,
                       const char* instance_base_name);

  void printProgress(int iteration, bool force, bool end, int remaining) const;

  int fanout(sta::Vertex* vertex) const;
  int wireLengthLimitImpliedByLoadSlew(sta::LibertyCell*);
  int wireLengthLimitImpliedByMaxCap(sta::LibertyCell*);
  int wireLengthStepForLayer(int layer);

  BufferedNetPtr attemptTopologyRewrite(const BufferedNetPtr& node,
                                        const BufferedNetPtr& left,
                                        const BufferedNetPtr& right,
                                        float best_cap);

  float findBufferLoadLimitImpliedByDriverSlew(sta::LibertyCell* cell);
  void characterizeBuffers();

  struct BufferSize
  {
    sta::LibertyCell* cell;
    FixedDelay intrinsic_delay;
    float margined_max_cap;
    float driver_resistance;

    struct Asymptotics
    {
      // asymptotic parameters when buffering long wires
      float buffer_spacing;
      float delay_per_meter;
      float delay_per_farad;
      float input_cap;
    };
    // -1 for `set_wire_rc` values
    std::map<int, Asymptotics> long_wire_asymptotics;
  };

  sta::Delay characterizationDelay(BufferSize& size, float load_cap);
  void findLongWireAsymptotics(int layer, Rebuffer::BufferSize& size);

  bool bufferSizeCanDriveLoad(const BufferSize& size,
                              const BufferedNetPtr& bnet,
                              int extra_wire_length = 0);

  bool hasTopLevelOutputPort(sta::Net* net);
  int rebufferPin(const sta::Pin* drvr_pin);

  utl::Logger* logger_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  Resizer* resizer_ = nullptr;
  est::EstimateParasitics* estimate_parasitics_ = nullptr;

  std::vector<BufferSize> buffer_sizes_;
  std::map<sta::LibertyCell*, BufferSize*> buffer_sizes_index_;

  // Electrical inputs that uniquely identify a cached buffer delay lookup.
  struct BufferDelayKey
  {
    const sta::LibertyPort* port{nullptr};
    const sta::Scene* scene{nullptr};
    float load_cap{0.0f};
    float rise_slew{0.0f};
    float fall_slew{0.0f};
    bool operator==(const BufferDelayKey& other) const = default;
  };

  // Hashes the complete buffer-delay lookup key.
  struct BufferDelayKeyHash
  {
    size_t operator()(const BufferDelayKey& key) const;
  };

  // Reuses gate delays at identical scene, load, and input-slew conditions.
  using BufferDelayCache = absl::flat_hash_map<
      BufferDelayKey,
      std::array<sta::ArcDelay, sta::RiseFall::index_count>,
      BufferDelayKeyHash>;
  BufferDelayCache buffer_delay_cache_;
  std::array<sta::ArcDelay, sta::RiseFall::index_count> uncached_gate_delays_;

  sta::Pin* pin_ = nullptr;
  float fanout_limit_ = 0.0f;
  float drvr_pin_max_slew_ = 0.0f;
  float drvr_load_high_water_mark_ = 0.0f;
  const sta::Scene* corner_ = nullptr;
  sta::LibertyPort* drvr_port_ = nullptr;
  sta::Path* arrival_paths_[sta::RiseFall::index_count] = {nullptr};

  // A worker planner's private delay calculator (see initForPlanning); every
  // gate delay this planner computes goes through delayCalc().
  std::unique_ptr<sta::ArcDelayCalc> owned_delay_calc_;
  sta::ArcDelayCalc* delayCalc() const
  {
    return owned_delay_calc_ != nullptr ? owned_delay_calc_.get()
                                        : arc_delay_calc_;
  }

  int resizer_max_wire_length_ = 0;
  double initial_design_area_ = 0.0;
  int print_interval_ = 0;
  int removed_count_ = 0;
  int inserted_count_ = 0;

  // margins in percent
  float slew_margin_ = 20.0f;
  float cap_margin_ = 20.0f;

  const sta::MinMax* min_ = sta::MinMax::min();
  const sta::MinMax* max_ = sta::MinMax::max();

  // Elmore factor for 20-80% slew thresholds.
  static constexpr float elmore_skew_factor_ = 1.39;
  static constexpr float relaxation_factor_ = 0.01;

  double long_wire_stepping_runtime_ = 0;

  // Choose the driver size together with the buffer tree in fullyRebuffer
  // (set_opt_config -rebuffer_size_driver).
  bool size_driver_ = false;
  int driver_resize_count_ = 0;
  double size_ra_runtime_ = 0;
  double size_eval_runtime_ = 0;

  friend class rsz::BufferCandidate;
  friend class rsz::SetupLegacyBase;
  friend class Resizer;
};

};  // namespace rsz
