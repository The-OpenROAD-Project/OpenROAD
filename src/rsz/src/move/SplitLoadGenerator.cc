// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SplitLoadGenerator.hh"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "MoveCandidate.hh"
#include "MoveGenerator.hh"
#include "OptimizerTypes.hh"
#include "SplitLoadCandidate.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/Transition.hh"
#include "utl/Logger.h"

namespace rsz {

using utl::RSZ;

namespace {

constexpr int kSplitLoadMinFanout = 8;
using FanoutSlack = std::pair<sta::Vertex*, sta::Slack>;

bool resolveDriverPin(Resizer& resizer,
                      const Target& target,
                      sta::Pin*& drvr_pin,
                      sta::Vertex*& drvr_vertex)
{
  if (!target.canBePathDriver()) {
    return false;
  }

  drvr_pin = target.resolvedPin(resizer);
  if (drvr_pin == nullptr) {
    return false;
  }
  if (target.fanout <= kSplitLoadMinFanout) {
    debugPrint(resizer.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Fanout {} <= {} min fanout",
               resizer.network()->pathName(drvr_pin),
               target.fanout,
               kSplitLoadMinFanout);
    return false;
  }
  if (!resizer.okToBufferNet(drvr_pin)) {
    debugPrint(resizer.logger(),
               RSZ,
               "split_load_move",
               2,
               "REJECT SplitLoadMove {}: Not OK to buffer net",
               resizer.network()->pathName(drvr_pin));
    return false;
  }

  drvr_vertex = target.vertex(resizer);
  return drvr_vertex != nullptr;
}

std::vector<FanoutSlack> collectRankedFanoutSlacks(Resizer& resizer,
                                                   const Target& target,
                                                   sta::Vertex* drvr_vertex)
{
  std::vector<FanoutSlack> fanout_slacks;
  const sta::Path* driver_path = target.driverPath(resizer);
  const sta::RiseFall* rf = driver_path != nullptr
                                ? driver_path->transition(resizer.staState())
                                : nullptr;
  if (rf == nullptr) {
    return fanout_slacks;
  }

  const sta::Slack drvr_slack
      = resizer.sta()->slack(drvr_vertex, resizer.maxAnalysisMode());
  sta::VertexOutEdgeIterator edge_iter(drvr_vertex, resizer.graph());
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    if (!edge->isWire()) {
      continue;
    }

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

std::unique_ptr<sta::PinSet> chooseSplitLoads(
    Resizer& resizer,
    const std::vector<FanoutSlack>& fanout_slacks)
{
  auto load_pins = std::make_unique<sta::PinSet>(resizer.network());
  const int split_index = fanout_slacks.size() / 2;
  for (int i = 0; i < split_index; ++i) {
    sta::Pin* split_pin = fanout_slacks[i].first->pin();
    if (!resizer.network()->isTopLevelPort(split_pin)) {
      load_pins->insert(split_pin);
    }
  }
  return load_pins;
}

}  // namespace

SplitLoadGenerator::SplitLoadGenerator(const GeneratorContext& context)
    : MoveGenerator(context)
{
}

std::vector<std::unique_ptr<MoveCandidate>> SplitLoadGenerator::generate(
    const Target& target)
{
  std::vector<std::unique_ptr<MoveCandidate>> candidates;
  sta::Pin* drvr_pin = nullptr;
  sta::Vertex* drvr_vertex = nullptr;
  if (!resolveDriverPin(resizer_, target, drvr_pin, drvr_vertex)) {
    return candidates;
  }

  std::vector<FanoutSlack> fanout_slacks
      = collectRankedFanoutSlacks(resizer_, target, drvr_vertex);
  if (fanout_slacks.empty()) {
    return candidates;
  }

  std::unique_ptr<sta::PinSet> load_pins
      = chooseSplitLoads(resizer_, fanout_slacks);
  if (load_pins->empty()) {
    return candidates;
  }

  sta::LibertyCell* buffer_cell = resizer_.lowestDriveBufferCell();
  odb::dbNet* db_drvr_net = nullptr;
  odb::dbModNet* db_mod_drvr_net = nullptr;
  resizer_.dbNetwork()->net(drvr_pin, db_drvr_net, db_mod_drvr_net);
  sta::Net* drvr_net = resizer_.dbNetwork()->dbToSta(db_drvr_net);
  if (buffer_cell == nullptr || drvr_net == nullptr) {
    return candidates;
  }

  const odb::Point drvr_loc = resizer_.dbNetwork()->location(drvr_pin);
  candidates.push_back(std::make_unique<SplitLoadCandidate>(
      resizer_, target, drvr_net, buffer_cell, drvr_loc, std::move(load_pins)));
  return candidates;
}

}  // namespace rsz
