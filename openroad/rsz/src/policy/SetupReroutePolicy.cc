// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "SetupReroutePolicy.hh"

#include <algorithm>
#include <utility>
#include <vector>

#include "OptimizerTypes.hh"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/TimingArc.hh"
#include "utl/Logger.h"

namespace rsz {

using std::pair;
using utl::RSZ;

void SetupReroutePolicy::buildMainMoveSequence(const bool log_sequence)
{
  move_sequence_.clear();
  move_sequence_.push_back(MoveType::kReroute);
  activateMoveSequence(log_sequence);
}

bool SetupReroutePolicy::repairPath(sta::Path* path,
                                    const sta::Slack path_slack,
                                    const bool force_single_repair)
{
  static_cast<void>(force_single_repair);

  sta::PathExpanded expanded(path, sta_);
  if (expanded.size() <= 1) {
    return false;
  }

  const sta::Scene* corner = path->scene(sta_);
  if (path->minMax(sta_) != max_) {
    logger_->error(utl::RSZ,
                   kMsgRepairSetupExpectedMaxPath,
                   "repairSetup expects max delay path");
    return false;
  }

  std::vector<pair<int, sta::Delay>> wire_delays;
  for (int i = expanded.startIndex(); i < expanded.size(); ++i) {
    const sta::Path* path_node = expanded.path(i);
    sta::Vertex* path_vertex = path_node->vertex(sta_);
    const sta::Pin* path_pin = path_node->pin(sta_);
    if (i <= 0 || !path_vertex->isDriver(network_)
        || network_->isTopLevelPort(path_pin) || i + 1 >= expanded.size()) {
      continue;
    }

    const sta::Path* next_node = expanded.path(i + 1);
    sta::Edge* net_edge = next_node->prevEdge(sta_);
    const sta::TimingArc* net_arc = next_node->prevArc(sta_);
    if (net_edge == nullptr || !net_edge->isWire()) {
      continue;
    }

    const sta::Delay delay = graph_->arcDelay(
        net_edge, net_arc, corner->dcalcAnalysisPtIndex(max_));
    if (delay > 0.0) {
      wire_delays.emplace_back(i, delay);
    }
    debugPrint(logger_,
               RSZ,
               "repair_setup",
               3,
               "{} wire delay = {}",
               path_vertex->name(network_),
               delayAsString(delay, 3, sta_));
  }

  std::ranges::sort(
      wire_delays,
      [](const pair<int, sta::Delay>& lhs, const pair<int, sta::Delay>& rhs) {
        return lhs.second > rhs.second
               || (lhs.second == rhs.second && lhs.first > rhs.first);
      });

  debugPrint(logger_,
             RSZ,
             "repair_setup",
             3,
             "Reroute wire pass: delays: {}, path slack: {}",
             wire_delays.size(),
             delayAsString(path_slack, 3, sta_));

  int changed = 0;
  for (const pair<int, sta::Delay>& wire_delay : wire_delays) {
    Target target;
    makePathDriverTarget(path, expanded, wire_delay.first, path_slack, target);
    if (tryRepairPathTarget(
            target, path_slack, /*repairs_per_pass=*/1, changed)) {
      return true;
    }
  }
  return false;
}

}  // namespace rsz
