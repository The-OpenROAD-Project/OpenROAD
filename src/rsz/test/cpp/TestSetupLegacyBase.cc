// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// Pre-include all headers that transitively pull in standard-library types
// (e.g. <sstream> via ODB → Boost → <complex>) before opening private access.
// Doing so ensures include-guards fire first and the macros never corrupt
// standard-library class layouts.
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "RepairSetupContext.hh"
#include "db_sta/dbSta.hh"
#include "gtest/gtest.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "tst/IntegratedFixture.h"

// Open protected/private access so rankPathDrivers (protected) is callable
// from the test body without a subclass override.
#define private public
#define protected public
#include "SetupLegacyBase.hh"
#undef protected
#undef private

namespace rsz {

class TestSetupLegacyBase : public tst::IntegratedFixture
{
 public:
  TestSetupLegacyBase()
      : tst::IntegratedFixture(tst::IntegratedFixture::Technology::kNangate45,
                               "_main/src/rsz/test/")
  {
  }

 protected:
  void SetUp() override
  {
    readVerilogAndSetup("TestSetupLegacyBase_StageDelay.v");

    // Minimal placement: both instances near the origin inside a small die.
    // Nangate45: 2000 database units per µm.
    block_->setDieArea(odb::Rect(0, 0, 200000, 200000));
    placeInst("stage1", 0, 0);
    placeInst("stage2", 4000, 0);

    ep_.estimateWireParasitics();
    sta_->updateTiming(/*full=*/true);

    resizer_.runRepairSetupPreamble();
  }

  void placeInst(const char* inst_name, int x, int y)
  {
    odb::dbInst* inst = block_->findInst(inst_name);
    ASSERT_NE(inst, nullptr) << "Instance not found: " << inst_name;
    inst->setLocation(x, y);
    inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }

  sta::Vertex* findEndpointVertex(const char* port_name) const
  {
    for (sta::Vertex* v : sta_->endpoints()) {
      const sta::Pin* pin = (v != nullptr) ? v->pin() : nullptr;
      if (pin != nullptr
          && std::string(sta_->network()->pathName(pin)) == port_name) {
        return v;
      }
    }
    return nullptr;
  }

  // Build a policy object with the given use_stage_delay_ranking setting.
  // Callers must keep committer and ctx alive as long as the policy is used.
  std::unique_ptr<SetupLegacyBase> makePolicy(MoveCommitter& committer,
                                              RepairSetupContext& ctx,
                                              const bool use_stage_delay)
  {
    OptimizerRunConfig config;
    config.use_stage_delay_ranking = use_stage_delay;
    auto policy
        = std::make_unique<SetupLegacyBase>(resizer_, committer, ctx, config);
    policy->start();
    return policy;
  }

  // Manually compute the wire delay on the edge leaving driver at path index i
  // in the expanded path.  Returns 0 when there is no wire edge at i+1.
  static sta::Delay wireDelayAfterDriver(const sta::PathExpanded& expanded,
                                         const int driver_index,
                                         const sta::Scene* corner,
                                         sta::Graph* graph,
                                         sta::dbSta* sta)
  {
    const int path_length = expanded.size();
    if (driver_index + 1 >= path_length) {
      return 0.0;
    }
    const sta::Path* next_node = expanded.path(driver_index + 1);
    sta::Edge* net_edge = next_node->prevEdge(sta);
    const sta::TimingArc* net_arc = next_node->prevArc(sta);
    if (net_edge == nullptr || !net_edge->isWire() || net_arc == nullptr) {
      return 0.0;
    }
    return graph->arcDelay(
        net_edge, net_arc, corner->dcalcAnalysisPtIndex(sta::MinMax::max()));
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// StageDelayEqualsLoadPlusWireDelay
//
// For every driver stage in the path:
//   stage_delay[i] == load_delay[i] + wire_delay_after_driver[i]
//
// where wire_delay_after_driver is computed independently from the STA graph.
// This directly pins the formula introduced in rankPathDrivers.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TestSetupLegacyBase, StageDelayEqualsLoadPlusWireDelay)
{
  sta::Vertex* endpoint = findEndpointVertex("out");
  ASSERT_NE(endpoint, nullptr) << "endpoint 'out' not found in STA";

  sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
  ASSERT_NE(path, nullptr) << "no worst-slack path to 'out'";

  sta::PathExpanded expanded(path, sta_.get());
  const sta::Scene* corner = path->scene(sta_.get());
  const int lib_ap = static_cast<int>(corner->libertyIndex(sta::MinMax::max()));

  MoveCommitter committer(resizer_);
  RepairSetupContext ctx(resizer_);

  auto load_policy = makePolicy(committer, ctx, /*use_stage_delay=*/false);
  auto stage_policy = makePolicy(committer, ctx, /*use_stage_delay=*/true);

  const auto load_delays
      = load_policy->rankPathDrivers(expanded, corner, lib_ap);
  const auto stage_delays
      = stage_policy->rankPathDrivers(expanded, corner, lib_ap);

  // Both modes must surface the same driver stages.
  ASSERT_EQ(load_delays.size(), stage_delays.size());
  ASSERT_FALSE(load_delays.empty()) << "expected at least one driver on path";

  // Build index→delay maps for easy comparison.
  std::unordered_map<int, sta::Delay> load_by_idx;
  for (const auto& [idx, delay] : load_delays) {
    load_by_idx[idx] = delay;
  }

  sta::Graph* graph = sta_->graph();

  for (const auto& [idx, stage_delay] : stage_delays) {
    ASSERT_TRUE(load_by_idx.count(idx))
        << "stage_delays has index " << idx << " not present in load_delays";
    const sta::Delay ld = load_by_idx.at(idx);
    const sta::Delay wd
        = wireDelayAfterDriver(expanded, idx, corner, graph, sta_.get());

    // Core formula: stage_delay = load_delay + wire_delay.
    EXPECT_FLOAT_EQ(stage_delay, ld + wd)
        << "at path index " << idx << " load_delay=" << ld
        << " wire_delay=" << wd;

    // Wire delay is always non-negative.
    EXPECT_GE(stage_delay, ld)
        << "stage delay must not be less than load delay";
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// BothModesReturnResultsSortedDescending
//
// rankPathDrivers guarantees descending sort (highest delay first).
// Verify the invariant holds for both ranking modes.
// ─────────────────────────────────────────────────────────────────────────────
TEST_F(TestSetupLegacyBase, BothModesReturnResultsSortedDescending)
{
  sta::Vertex* endpoint = findEndpointVertex("out");
  ASSERT_NE(endpoint, nullptr);
  sta::Path* path = sta_->vertexWorstSlackPath(endpoint, sta::MinMax::max());
  ASSERT_NE(path, nullptr);

  sta::PathExpanded expanded(path, sta_.get());
  const sta::Scene* corner = path->scene(sta_.get());
  const int lib_ap = static_cast<int>(corner->libertyIndex(sta::MinMax::max()));

  MoveCommitter committer(resizer_);
  RepairSetupContext ctx(resizer_);
  auto load_policy = makePolicy(committer, ctx, /*use_stage_delay=*/false);
  auto stage_policy = makePolicy(committer, ctx, /*use_stage_delay=*/true);

  const auto load_delays
      = load_policy->rankPathDrivers(expanded, corner, lib_ap);
  const auto stage_delays
      = stage_policy->rankPathDrivers(expanded, corner, lib_ap);

  auto isSortedDescending
      = [](const std::vector<std::pair<int, sta::Delay>>& v) {
          for (size_t i = 1; i < v.size(); ++i) {
            // Primary key: delay descending.  Tie-break: index descending.
            if (v[i - 1].second < v[i].second) {
              return false;
            }
            if (v[i - 1].second == v[i].second && v[i - 1].first < v[i].first) {
              return false;
            }
          }
          return true;
        };

  EXPECT_TRUE(isSortedDescending(load_delays))
      << "load-delay mode result is not sorted descending";
  EXPECT_TRUE(isSortedDescending(stage_delays))
      << "stage-delay mode result is not sorted descending";
}

}  // namespace rsz
