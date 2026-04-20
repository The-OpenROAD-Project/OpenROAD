// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "CloneGenerator.hh"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "CloneCandidate.hh"
#include "MoveCandidate.hh"
#include "MoveCommitter.hh"
#include "OptimizerTypes.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

constexpr int kCloneMinFanout = 8;

using FanoutSlack = std::pair<sta::Vertex*, sta::Slack>;

bool resolveDriverTarget(Resizer& resizer,
                         const Target& target,
                         sta::Pin*& drvr_pin,
                         sta::Instance*& drvr_inst,
                         sta::Instance*& parent,
                         sta::LibertyCell*& original_cell,
                         sta::Vertex*& drvr_vertex)
{
  // Keep cloning limited to high-fanout combinational drivers that can absorb
  // the split.
  if (!target.isKind(TargetKind::kPathDriver)
      || target.endpoint_path == nullptr) {
    return false;
  }

  drvr_vertex = target.vertex(resizer);
  if (drvr_vertex == nullptr) {
    return false;
  }

  drvr_pin = target.resolvedPin(resizer);
  if (drvr_pin == nullptr) {
    return false;
  }
  if (target.fanout <= kCloneMinFanout) {
    debugPrint(resizer.logger(),
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Fanout {} <= {} min fanout",
               resizer.network()->pathName(drvr_pin),
               target.fanout,
               kCloneMinFanout);
    return false;
  }
  if (!resizer.okToBufferNet(drvr_pin)) {
    debugPrint(resizer.logger(),
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Not OK to buffer net",
               resizer.network()->pathName(drvr_pin));
    return false;
  }

  drvr_inst = resizer.dbNetwork()->instance(drvr_pin);
  if (drvr_inst == nullptr) {
    return false;
  }
  if (!resizer.isSingleOutputCombinational(drvr_inst)) {
    debugPrint(resizer.logger(),
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Not single output combinational",
               resizer.network()->pathName(drvr_pin));
    return false;
  }

  parent = resizer.dbNetwork()->getOwningInstanceParent(drvr_pin);
  original_cell = resizer.network()->libertyCell(drvr_inst);
  return parent != nullptr && original_cell != nullptr;
}

std::vector<FanoutSlack> collectFanoutSlacks(Resizer& resizer,
                                             const Target& target,
                                             sta::Vertex* drvr_vertex)
{
  std::vector<FanoutSlack> fanout_slacks;
  // Rank fanouts by slack delta so the clone steals the easiest half of the
  // load set.
  const sta::Path* driver_path = target.driverPath(resizer);
  const sta::RiseFall* rf = driver_path != nullptr
                                ? driver_path->transition(resizer.staState())
                                : nullptr;
  if (rf == nullptr) {
    return fanout_slacks;
  }

  const sta::Slack drvr_slack
      = resizer.sta()->slack(drvr_vertex, rf, resizer.maxAnalysisMode());
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, resizer.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    sta::Vertex* fanout_vertex = edge->to(resizer.graph());
    const sta::Slack fanout_slack
        = resizer.sta()->slack(fanout_vertex, rf, resizer.maxAnalysisMode());
    fanout_slacks.emplace_back(fanout_vertex, fanout_slack - drvr_slack);
  }

  std::ranges::sort(fanout_slacks,
                    [&resizer](const FanoutSlack& lhs, const FanoutSlack& rhs) {
                      return lhs.second > rhs.second
                             || (lhs.second == rhs.second
                                 && resizer.network()->pathNameLess(
                                     lhs.first->pin(), rhs.first->pin()));
                    });
  return fanout_slacks;
}

sta::LibertyCell* chooseCloneCell(Resizer& resizer,
                                  sta::LibertyCell* original_cell)
{
  sta::LibertyCell* clone_cell = resizer.halfDrivingPowerCell(original_cell);
  return clone_cell != nullptr ? clone_cell : original_cell;
}

std::vector<sta::Pin*> selectMovedLoads(
    Resizer& resizer,
    const std::vector<FanoutSlack>& fanout_slacks)
{
  std::vector<sta::Pin*> moved_loads;
  // Move the less critical half of the loads and keep top-level ports on the
  // original driver.
  const int split_index = fanout_slacks.size() / 2;
  moved_loads.reserve(split_index);
  for (int i = 0; i < split_index; ++i) {
    sta::Pin* load_pin = fanout_slacks[i].first->pin();
    if (!resizer.network()->isTopLevelPort(load_pin)) {
      moved_loads.push_back(load_pin);
    }
  }
  return moved_loads;
}

odb::Point computeCloneLocation(Resizer& resizer,
                                sta::Pin* drvr_pin,
                                const std::vector<FanoutSlack>& fanout_slacks)
{
  int count = 1;
  int centroid_x = resizer.dbNetwork()->location(drvr_pin).getX();
  int centroid_y = resizer.dbNetwork()->location(drvr_pin).getY();
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; ++i) {
    sta::Pin* load_pin = fanout_slacks[i].first->pin();
    centroid_x += resizer.dbNetwork()->location(load_pin).getX();
    centroid_y += resizer.dbNetwork()->location(load_pin).getY();
    ++count;
  }
  return {centroid_x / count, centroid_y / count};
}

}  // namespace

CloneGenerator::CloneGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

bool CloneGenerator::isApplicable(const Target& target) const
{
  return target.isKind(TargetKind::kPathDriver)
         && target.endpoint_path != nullptr;
}

std::vector<std::unique_ptr<MoveCandidate>> CloneGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = nullptr;
  sta::Instance* drvr_inst = nullptr;
  sta::Instance* parent = nullptr;
  sta::LibertyCell* original_cell = nullptr;
  sta::Vertex* drvr_vertex = nullptr;
  if (!resolveDriverTarget(resizer_,
                           target,
                           drvr_pin,
                           drvr_inst,
                           parent,
                           original_cell,
                           drvr_vertex)) {
    return candidates;
  }

  if (committer_.hasPendingMoves(MoveType::kBuffer, drvr_inst)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Has pending BufferMove",
               resizer_.network()->pathName(drvr_pin));
    return candidates;
  }
  if (committer_.hasPendingMoves(MoveType::kSplitLoad, drvr_inst)) {
    debugPrint(resizer_.logger(),
               RSZ,
               "clone_move",
               2,
               "REJECT CloneMove {}: Has pending SplitLoadMove",
               resizer_.network()->pathName(drvr_pin));
    return candidates;
  }

  std::vector<FanoutSlack> fanout_slacks
      = collectFanoutSlacks(resizer_, target, drvr_vertex);
  if (fanout_slacks.empty()) {
    return candidates;
  }

  std::vector<sta::Pin*> moved_loads
      = selectMovedLoads(resizer_, fanout_slacks);

  sta::LibertyCell* clone_cell = chooseCloneCell(resizer_, original_cell);

  const odb::Point clone_loc
      = computeCloneLocation(resizer_, drvr_pin, fanout_slacks);
  candidates.push_back(
      std::make_unique<CloneCandidate>(resizer_,
                                       target,
                                       drvr_pin,
                                       drvr_inst,
                                       parent,
                                       original_cell,
                                       clone_cell,
                                       clone_loc,
                                       std::move(moved_loads)));
  return candidates;
}

}  // namespace rsz
