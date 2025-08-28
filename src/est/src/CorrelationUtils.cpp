// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/EstimateParasitics.h"
#include "sta/Corner.hh"
#include "sta/Parasitics.hh"

using sta::Corner;
using sta::Parasitic;
using sta::ParasiticAnalysisPt;
using sta::ParasiticNodeResistorMap;
using sta::ParasiticResistor;
using utl::EST;

namespace est {

static std::optional<float> walkResistorsToNode(
    Logger* logger,
    Parasitics* parasitics,
    ParasiticNodeResistorMap& map,
    ParasiticNode* upstream_node,
    ParasiticNode* node,
    ParasiticNode* target,
    std::set<ParasiticNode*> on_stack)
{
  if (node == target) {
    // We've hit the target. The callers on stack will add resistance values on
    // the path
    return {0};
  }

  for (ParasiticResistor* resistor : map[node]) {
    ParasiticNode* other_node = parasitics->otherNode(resistor, node);

    if (other_node != upstream_node) {
      if (on_stack.count(other_node)) {
        // For now we abort on loops as we don't support parallel resistance.
        logger->error(
            EST,
            8,
            "conductive loop detected when walking to parasitic node {}",
            parasitics->name(target));
      }

      on_stack.insert(other_node);
      auto result = walkResistorsToNode(
          logger, parasitics, map, node, other_node, target, on_stack);
      on_stack.erase(other_node);

      if (result) {
        return result.value() + parasitics->value(resistor);
      }
    }
  }

  return {};
}

std::optional<float> EstimateParasitics::sumPointToPointResist(Pin* point1,
                                                               Pin* point2)
{
  Corner* corner = sta::Sta::sta()->cmdCorner();
  const ParasiticAnalysisPt* ap
      = corner->findParasiticAnalysisPt(sta::MinMax::max());

  if (network_->net(point1) != network_->net(point2)) {
    logger_->error(EST,
                   9,
                   "cannot compute resistance between pins {} and {} which are "
                   "not connected",
                   network_->name(point1),
                   network_->name(point2));
  }

  Parasitic* parasitic
      = parasitics_->findParasiticNetwork(network_->net(point1), ap);

  if (!parasitic) {
    return {};
  }

  // We assume the network has no conductive loops, and any point is reachable
  // from any other point by walking over resistive elements
  auto node1 = parasitics_->findParasiticNode(parasitic, point1);
  auto node2 = parasitics_->findParasiticNode(parasitic, point2);
  if (!node1 || !node2) {
    return {};
  }

  auto map = parasitics_->parasiticNodeResistorMap(parasitic);
  // `on_stack` is used for loop detection
  std::set<ParasiticNode*> on_stack;
  return walkResistorsToNode(
      logger_, parasitics_, map, nullptr, node1, node2, on_stack);
}

}  // namespace est
