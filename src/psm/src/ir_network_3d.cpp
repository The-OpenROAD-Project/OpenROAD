// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ir_network_3d.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir_network.h"
#include "node.h"
#include "odb/db.h"
#include "shape.h"
#include "utl/Logger.h"

namespace psm {

IRNetwork3D::IRNetwork3D(odb::dbChipNet* chip_net,
                         utl::Logger* logger,
                         bool floorplanning)
    : chip_net_(chip_net), logger_(logger), floorplanning_(floorplanning)
{
}

IRNetwork3D::~IRNetwork3D() = default;

void IRNetwork3D::construct()
{
  interdie_connections_.clear();
  networks_.clear();

  for (uint32_t index = 0; index < chip_net_->getNumBumpInsts(); index++) {
    std::vector<odb::dbChipInst*> path;
    odb::dbChipBumpInst* bump_inst = chip_net_->getBumpInst(index, path);
    if (bump_inst == nullptr || path.size() != 1) {
      logger_->error(utl::PSM,
                     94,
                     "dbChipNet {} requires a flat assembly with one chip "
                     "instance per bump path.",
                     chip_net_->getName());
    }

    odb::dbChipInst* chip_inst = path.back();
    odb::dbNet* net = bump_inst->getChipBump()->getNet();
    if (net == nullptr) {
      logger_->error(utl::PSM,
                     95,
                     "A bump on chiplet {} has no internal dbNet.",
                     chip_inst->getName());
    }

    const auto exists = std::ranges::find_if(
        networks_, [chip_inst, net](const ChipletNetwork& candidate) {
          return candidate.chip_inst == chip_inst && candidate.net == net;
        });
    if (exists == networks_.end()) {
      auto network = std::make_unique<IRNetwork>(net, logger_, floorplanning_);
      networks_.push_back({chip_inst, net, std::move(network)});
    }
  }

  for (odb::dbChipRSeg* rseg : chip_net_->getChipRSegs()) {
    Node* source = findEndpoint(rseg->getSourceCapNode());
    Node* target = findEndpoint(rseg->getTargetCapNode());
    odb::dbChipBumpInst* source_bump
        = rseg->getSourceCapNode()->getChipBumpInst();
    odb::dbChipBumpInst* target_bump
        = rseg->getTargetCapNode()->getChipBumpInst();
    odb::dbChipInst* source_chip
        = source_bump->getChipRegionInst()->getChipInst();
    odb::dbChipInst* target_chip
        = target_bump->getChipRegionInst()->getChipInst();
    if (source_chip == target_chip) {
      logger_->error(utl::PSM,
                     106,
                     "dbChipRSeg on {} does not cross between chiplets.",
                     chip_net_->getName());
    }
    odb::dbNet* source_net = source_bump->getChipBump()->getNet();
    odb::dbNet* target_net = target_bump->getChipBump()->getNet();
    if (source_net->getSigType() != target_net->getSigType()) {
      logger_->error(utl::PSM,
                     166,
                     "Inter-chip bond on {} shorts {}/{} ({}) to {}/{} ({}).",
                     chip_net_->getName(),
                     source_chip->getName(),
                     source_net->getName(),
                     source_net->getSigType().getString(),
                     target_chip->getName(),
                     target_net->getName(),
                     target_net->getSigType().getString());
    }
    if (!std::isfinite(rseg->getResistance()) || rseg->getResistance() <= 0.0) {
      logger_->error(utl::PSM,
                     96,
                     "dbChipRSeg on {} must have positive resistance.",
                     chip_net_->getName());
    }
    interdie_connections_.push_back(std::make_unique<FixedResistanceConnection>(
        source, target, rseg->getResistance()));
  }
  bond_state_ = getBondState();
}

std::vector<IRNetwork3D::BondState> IRNetwork3D::getBondState() const
{
  std::vector<BondState> state;
  for (auto* bond : chip_net_->getChipRSegs()) {
    auto* source = bond->getSourceCapNode();
    auto* target = bond->getTargetCapNode();
    auto* source_bump = source->getChipBumpInst();
    auto* target_bump = target->getChipBumpInst();
    state.push_back({bond->getId(),
                     source->getId(),
                     target->getId(),
                     source_bump == nullptr ? 0 : source_bump->getId(),
                     target_bump == nullptr ? 0 : target_bump->getId(),
                     bond->getResistance()});
  }
  std::ranges::sort(state, {}, &BondState::id);
  return state;
}

bool IRNetwork3D::hasSameBonds() const
{
  // Bond create/destroy/setResistance do not notify dbBlock observers.
  // Never dereference a bond pointer saved before a possible deletion.
  return getBondState() == bond_state_;
}

odb::PtrMap<odb::dbInst, Node::NodeSet> IRNetwork3D::getInstanceNodeMapping(
    odb::dbNet* net) const
{
  for (const auto& chiplet : networks_) {
    if (chiplet.net == net) {
      return chiplet.network->getInstanceNodeMapping();
    }
  }
  return {};
}

bool IRNetwork3D::isConnected() const
{
  return getNodeCount() != 0 && getDisconnectedNodes().empty();
}

IRNetwork3D::Nodes IRNetwork3D::getDisconnectedNodes() const
{
  const Nodes nodes = getNodes();
  if (nodes.empty()) {
    return {};
  }

  std::unordered_map<Node*, std::vector<Node*>> adjacency;
  for (Connection* connection : getConnections()) {
    Node* node0 = connection->getNode0();
    Node* node1 = connection->getNode1();
    adjacency[node0].push_back(node1);
    adjacency[node1].push_back(node0);
  }

  // Visit across both chiplet-local edges and inter-die connections.
  std::unordered_set<Node*> visited;
  std::queue<Node*> pending;
  Node* start = nullptr;
  for (const auto& chiplet : networks_) {
    for (const auto& [layer, layer_nodes] : chiplet.network->getNodes()) {
      if (!layer_nodes.empty()) {
        start = layer_nodes.front().get();
        break;
      }
    }
    if (start != nullptr) {
      break;
    }
  }
  if (start == nullptr) {
    start = *nodes.begin();
  }
  visited.insert(start);
  pending.push(start);
  while (!pending.empty()) {
    Node* node = pending.front();
    pending.pop();
    for (Node* neighbor : adjacency[node]) {
      if (visited.insert(neighbor).second) {
        pending.push(neighbor);
      }
    }
  }
  Nodes disconnected;
  for (Node* node : nodes) {
    if (!visited.contains(node)) {
      disconnected.insert(node);
    }
  }
  return disconnected;
}

IRNetwork3D::ChipletNetwork* IRNetwork3D::findNetwork(
    odb::dbChipBumpInst* bump_inst)
{
  if (bump_inst == nullptr) {
    logger_->error(
        utl::PSM, 97, "dbChipCapNode on {} has no bump.", chip_net_->getName());
  }

  odb::dbChipInst* chip_inst = bump_inst->getChipRegionInst()->getChipInst();
  odb::dbNet* net = bump_inst->getChipBump()->getNet();
  const auto network = std::ranges::find_if(
      networks_, [chip_inst, net](const ChipletNetwork& candidate) {
        return candidate.chip_inst == chip_inst && candidate.net == net;
      });
  if (network == networks_.end()) {
    logger_->error(utl::PSM,
                   98,
                   "Cannot find the chiplet PDN for an endpoint on {}.",
                   chip_net_->getName());
  }
  return &*network;
}

Node* IRNetwork3D::findEndpoint(odb::dbChipCapNode* cap_node)
{
  if (cap_node == nullptr) {
    logger_->error(utl::PSM,
                   99,
                   "dbChipRSeg on {} has no cap node.",
                   chip_net_->getName());
  }

  ChipletNetwork* chiplet = findNetwork(cap_node->getChipBumpInst());
  // The bond lands on the bump's PAD, not the chip's BTerm.
  odb::dbInst* bump = cap_node->getChipBumpInst()->getChipBump()->getInst();
  Node* node = nullptr;
  for (const auto& candidate : chiplet->network->getITermNodes()) {
    if (candidate->getITerm()->getInst() == bump) {
      if (node != nullptr) {
        logger_->error(utl::PSM,
                       141,
                       "Bump {} has more than one terminal on net {}.",
                       bump->getName(),
                       chiplet->net->getName());
      }
      node = candidate.get();
    }
  }
  if (node == nullptr) {
    logger_->error(utl::PSM,
                   100,
                   "Cannot map bump {} on chiplet {} into its PDN.",
                   bump->getName(),
                   chiplet->chip_inst->getName());
  }
  return node;
}

std::size_t IRNetwork3D::getNodeCount() const
{
  std::size_t count = 0;
  for (const ChipletNetwork& chiplet : networks_) {
    count += chiplet.network->getNodeCount(true);
    count += chiplet.network->getBPinNodes().size();
  }
  return count;
}

std::size_t IRNetwork3D::getConnectionCount() const
{
  std::size_t count = interdie_connections_.size();
  for (const ChipletNetwork& chiplet : networks_) {
    count += chiplet.network->getConnections().size();
  }
  return count;
}

IRNetwork3D::Nodes IRNetwork3D::getNodes() const
{
  Nodes nodes;
  // Include isolated nodes too: collecting only edge endpoints hides opens.
  for (const ChipletNetwork& chiplet : networks_) {
    for (const auto& [layer, layer_nodes] : chiplet.network->getNodes()) {
      for (const auto& node : layer_nodes) {
        nodes.insert(node.get());
      }
    }
    for (const auto& node : chiplet.network->getITermNodes()) {
      nodes.insert(node.get());
    }
    for (const auto& node : chiplet.network->getBPinNodes()) {
      nodes.insert(node.get());
    }
  }
  return nodes;
}

std::vector<Connection*> IRNetwork3D::getConnections() const
{
  std::vector<Connection*> connections;
  connections.reserve(getConnectionCount());
  for (const ChipletNetwork& chiplet : networks_) {
    for (const auto& connection : chiplet.network->getConnections()) {
      connections.push_back(connection.get());
    }
  }
  for (const auto& connection : interdie_connections_) {
    connections.push_back(connection.get());
  }
  return connections;
}

odb::PtrSet<odb::dbTechLayer> IRNetwork3D::getLayers() const
{
  odb::PtrSet<odb::dbTechLayer> layers;
  for (const ChipletNetwork& chiplet : networks_) {
    const auto chiplet_layers = chiplet.network->getLayers();
    layers.insert(chiplet_layers.begin(), chiplet_layers.end());
  }
  return layers;
}

Node* IRNetwork3D::findTerminalNode(odb::dbChipInst* chip_inst,
                                    const std::string& terminal) const
{
  Node* result = nullptr;
  for (const ChipletNetwork& chiplet : networks_) {
    if (chiplet.chip_inst != chip_inst) {
      continue;
    }
    for (const auto& node : chiplet.network->getBPinNodes()) {
      if (node->getBPin()->getBTerm()->getName() == terminal) {
        if (result != nullptr) {
          logger_->error(
              utl::PSM,
              142,
              "Port {} has multiple shapes; choose a single-shape source port.",
              terminal);
        }
        result = node.get();
      }
    }
    for (const auto& node : chiplet.network->getITermNodes()) {
      auto* iterm = node->getITerm();
      const std::string name = std::string(iterm->getInst()->getName()) + "/"
                               + iterm->getMTerm()->getName();
      if (name == terminal) {
        if (result != nullptr) {
          logger_->error(utl::PSM, 143, "Terminal {} is ambiguous.", terminal);
        }
        result = node.get();
      }
    }
  }
  return result;
}

std::vector<odb::dbNet*> IRNetwork3D::getNets() const
{
  std::vector<odb::dbNet*> nets;
  for (const ChipletNetwork& chiplet : networks_) {
    nets.push_back(chiplet.net);
  }
  return nets;
}

std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>>
IRNetwork3D::getChipletNets() const
{
  std::vector<std::pair<odb::dbChipInst*, odb::dbNet*>> result;
  for (const auto& chiplet : networks_) {
    result.emplace_back(chiplet.chip_inst, chiplet.net);
  }
  return result;
}

std::vector<std::pair<odb::dbChipInst*, ITermNode*>>
IRNetwork3D::getITermNodes() const
{
  std::vector<std::pair<odb::dbChipInst*, ITermNode*>> result;
  for (const auto& chiplet : networks_) {
    for (const auto& node : chiplet.network->getITermNodes()) {
      result.emplace_back(chiplet.chip_inst, node.get());
    }
  }
  return result;
}

std::map<Node*, odb::dbChipInst*> IRNetwork3D::getNodeChiplets() const
{
  std::map<Node*, odb::dbChipInst*> result;
  for (const auto& chiplet : networks_) {
    for (const auto& [layer, nodes] : chiplet.network->getNodes()) {
      for (const auto& node : nodes) {
        result[node.get()] = chiplet.chip_inst;
      }
    }
    for (const auto& node : chiplet.network->getITermNodes()) {
      result[node.get()] = chiplet.chip_inst;
    }
    for (const auto& node : chiplet.network->getBPinNodes()) {
      result[node.get()] = chiplet.chip_inst;
    }
  }
  return result;
}

}  // namespace psm
