// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "clock_tree_report.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "sta/Clock.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"

namespace web {

const char* ClockTreeNode::typeToString(Type t)
{
  switch (t) {
    case ROOT:
      return "root";
    case BUFFER:
      return "buffer";
    case INVERTER:
      return "inverter";
    case CLOCK_GATE:
      return "clock_gate";
    case REGISTER:
      return "register";
    case MACRO:
      return "macro";
    default:
      return "unknown";
  }
}

// ---- Internal tree structure (mirrors gui::ClockTree) ----

namespace {

struct TreeNode
{
  TreeNode* parent = nullptr;
  sta::Net* net = nullptr;
  sta::dbNetwork* network = nullptr;
  int level = 0;

  std::map<const sta::Pin*, sta::Delay> drivers;
  std::map<const sta::Pin*, sta::Delay> child_sinks;
  std::map<const sta::Pin*, sta::Delay> leaves;
  std::vector<std::unique_ptr<TreeNode>> fanout;

  sta::Net* getNet(const sta::Pin* pin) const
  {
    sta::Term* term = network->term(pin);
    sta::Net* n = term ? network->net(term) : network->net(pin);
    return network->findFlatNet(n);
  }

  bool isLeaf(const sta::Pin* pin) const
  {
    return network->isRegClkPin(pin) || network->isLatchData(pin);
  }

  bool addVertex(sta::Vertex* vertex, sta::Delay delay)
  {
    const sta::Pin* pin = vertex->pin();
    if (isLeaf(pin)) {
      leaves[pin] = delay;
      return false;
    }
    if (vertex->isDriver(network)) {
      drivers[pin] = delay;
    } else {
      child_sinks[pin] = delay;
    }
    return true;
  }

  TreeNode* findChild(sta::Net* target)
  {
    for (auto& child : fanout) {
      if (child->net == target) {
        return child.get();
      }
    }
    return nullptr;
  }

  TreeNode* getOrCreateChild(sta::Net* target)
  {
    TreeNode* existing = findChild(target);
    if (existing) {
      return existing;
    }
    auto child = std::make_unique<TreeNode>();
    child->parent = this;
    child->net = target;
    child->network = network;
    child->level = level + 1;
    TreeNode* ptr = child.get();
    fanout.push_back(std::move(child));
    return ptr;
  }

  void addPath(sta::PathExpanded& path, int idx, const sta::StaState* sta)
  {
    if (idx == static_cast<int>(path.size())) {
      return;
    }
    const sta::Path* ref = path.path(idx);
    sta::Vertex* vertex = ref->vertex(sta);
    sta::Pin* pin = vertex->pin();
    sta::Net* pin_net = getNet(pin);

    TreeNode* target = getOrCreateChild(pin_net);
    if (target->addVertex(vertex, ref->arrival())) {
      target->addPath(path, idx + 1, sta);
    }
  }

  void addPath(sta::PathExpanded& path, const sta::StaState* sta)
  {
    const sta::Path* start = path.startPath();
    if (start->clkEdge(sta)->transition() != sta::RiseFall::rise()) {
      return;
    }
    if (start->minMax(sta) != sta::MinMax::max()) {
      return;
    }
    if (getNet(start->pin(sta)) != net) {
      return;
    }
    addPath(path, 0, sta);
  }

  int getTotalFanout() const
  {
    int total = 0;
    if (!leaves.empty()) {
      total = 1;
    }
    for (const auto& child : fanout) {
      total += child->getTotalFanout();
    }
    return total;
  }

  template <typename Cmp>
  sta::Delay getArrivalBound(Cmp cmp, sta::Delay init) const
  {
    sta::Delay result = init;
    for (const auto& [pin, arr] : drivers) {
      result = cmp(result, arr);
    }
    for (const auto& [pin, arr] : leaves) {
      result = cmp(result, arr);
    }
    for (const auto& child : fanout) {
      result = cmp(result, child->getArrivalBound(cmp, init));
    }
    return result;
  }

  sta::Delay getMinArrival() const
  {
    return getArrivalBound([](auto a, auto b) { return std::min(a, b); },
                           std::numeric_limits<sta::Delay>::max());
  }

  sta::Delay getMaxArrival() const
  {
    return getArrivalBound([](auto a, auto b) { return std::max(a, b); },
                           std::numeric_limits<sta::Delay>::lowest());
  }
};

ClockTreeNode::Type classifyDriver(const sta::Pin* pin, sta::dbNetwork* network)
{
  sta::Instance* inst = network->instance(pin);
  if (!inst) {
    return ClockTreeNode::ROOT;
  }
  sta::LibertyCell* cell = network->libertyCell(inst);
  if (!cell) {
    return ClockTreeNode::UNKNOWN;
  }
  if (cell->isClockGate()) {
    return ClockTreeNode::CLOCK_GATE;
  }
  if (cell->isInverter()) {
    return ClockTreeNode::INVERTER;
  }
  if (cell->isBuffer()) {
    return ClockTreeNode::BUFFER;
  }
  return ClockTreeNode::UNKNOWN;
}

struct ResolvedPin
{
  odb::dbITerm* iterm = nullptr;
  odb::dbBTerm* bterm = nullptr;
};

static ResolvedPin resolveStaPin(const sta::Pin* pin, sta::dbNetwork* network)
{
  ResolvedPin rp;
  odb::dbModITerm* moditerm;
  network->staToDb(pin, rp.iterm, rp.bterm, moditerm);
  return rp;
}

ClockTreeNode::Type classifyLeaf(const sta::Pin* pin, sta::dbNetwork* network)
{
  sta::Instance* inst = network->instance(pin);
  if (!inst) {
    return ClockTreeNode::UNKNOWN;
  }
  auto [iterm, bterm] = resolveStaPin(pin, network);
  if (iterm) {
    odb::dbInst* db_inst = iterm->getInst();
    if (db_inst->getMaster()->getType().isBlock()) {
      return ClockTreeNode::MACRO;
    }
  }
  return ClockTreeNode::REGISTER;
}

void getPinLocation(const sta::Pin* pin,
                    sta::dbNetwork* network,
                    int& x,
                    int& y)
{
  x = 0;
  y = 0;
  auto [iterm, bterm] = resolveStaPin(pin, network);
  if (iterm) {
    odb::dbInst* inst = iterm->getInst();
    int lx, ly;
    inst->getLocation(lx, ly);
    odb::Rect bbox = inst->getBBox()->getBox();
    x = (bbox.xMin() + bbox.xMax()) / 2;
    y = (bbox.yMin() + bbox.yMax()) / 2;
  } else if (bterm) {
    for (odb::dbBPin* bpin : bterm->getBPins()) {
      odb::Rect bbox = bpin->getBBox();
      x = (bbox.xMin() + bbox.xMax()) / 2;
      y = (bbox.yMin() + bbox.yMax()) / 2;
      break;
    }
  }
}

std::string getPinName(const sta::Pin* pin, sta::dbNetwork* network)
{
  auto [iterm, bterm] = resolveStaPin(pin, network);
  if (iterm) {
    return iterm->getName();
  }
  if (bterm) {
    return bterm->getName();
  }
  return "";
}

std::string getInstName(const sta::Pin* pin, sta::dbNetwork* network)
{
  auto [iterm, bterm] = resolveStaPin(pin, network);
  if (iterm) {
    return iterm->getInst()->getName();
  }
  if (bterm) {
    return bterm->getName();
  }
  return "";
}

// Flatten one TreeNode level into the output array.
// Each TreeNode can have multiple driver pins; we emit one output node
// per unique instance.  The root TreeNode typically has no drivers
// (they live in the first child), so we handle that case by emitting
// a synthetic root node and recursing directly into children.
void flattenNode(const TreeNode* tree,
                 int parent_id,
                 sta::dbNetwork* network,
                 float time_scale,
                 const std::string& clock_name,
                 ClockTreeData& data)
{
  // Driverless TreeNode: the tree has alternating "sink" and "driver"
  // levels per net hop.  Sink-only levels are pass-throughs — skip them
  // and recurse into children with the same parent_id.
  // Exception: the absolute root (parent_id == -1) emits a synthetic node.
  if (tree->drivers.empty()) {
    int effective_parent = parent_id;

    if (parent_id == -1) {
      // Top-level root — emit a synthetic root node
      ClockTreeNode root_node;
      root_node.id = static_cast<int>(data.nodes.size());
      root_node.parent_id = -1;
      root_node.name = clock_name;
      root_node.type = ClockTreeNode::ROOT;
      root_node.level = 0;
      root_node.fanout
          = static_cast<int>(tree->fanout.size() + tree->leaves.size());
      effective_parent = root_node.id;
      data.nodes.push_back(root_node);
    }

    for (const auto& child : tree->fanout) {
      flattenNode(
          child.get(), effective_parent, network, time_scale, clock_name, data);
    }
    for (const auto& [leaf_pin, leaf_arrival] : tree->leaves) {
      ClockTreeNode leaf;
      leaf.id = static_cast<int>(data.nodes.size());
      leaf.parent_id = effective_parent;
      leaf.name = getInstName(leaf_pin, network);
      leaf.pin_name = getPinName(leaf_pin, network);
      leaf.type = classifyLeaf(leaf_pin, network);
      leaf.arrival = static_cast<float>(leaf_arrival / time_scale);
      leaf.level = tree->level + 1;
      leaf.fanout = 0;
      getPinLocation(leaf_pin, network, leaf.dbu_x, leaf.dbu_y);
      data.nodes.push_back(leaf);
    }
    return;
  }

  // Emit driver nodes
  std::set<std::string> emitted_insts;
  for (const auto& [pin, arrival] : tree->drivers) {
    std::string inst_name = getInstName(pin, network);
    if (!emitted_insts.insert(inst_name).second) {
      continue;
    }

    ClockTreeNode node;
    node.id = static_cast<int>(data.nodes.size());
    node.parent_id = parent_id;
    node.name = std::move(inst_name);
    node.pin_name = getPinName(pin, network);
    node.type = classifyDriver(pin, network);
    node.fanout = static_cast<int>(tree->fanout.size() + tree->leaves.size());
    node.level = tree->level;
    getPinLocation(pin, network, node.dbu_x, node.dbu_y);

    // Position the node at its INPUT arrival (from parent's child_sinks)
    // and compute delay as the cell delay (output - input).
    // For the root (no parent), use the driver arrival directly.
    node.arrival = static_cast<float>(arrival / time_scale);
    if (tree->parent) {
      sta::Instance* inst = network->instance(pin);
      if (inst) {
        for (const auto& [sink_pin, sink_arr] : tree->parent->child_sinks) {
          if (network->instance(sink_pin) == inst) {
            node.arrival = static_cast<float>(sink_arr / time_scale);
            node.delay = static_cast<float>((arrival - sink_arr) / time_scale);
            break;
          }
        }
      }
    }

    int this_id = node.id;
    data.nodes.push_back(std::move(node));

    // Recurse into children
    for (const auto& child : tree->fanout) {
      flattenNode(child.get(), this_id, network, time_scale, clock_name, data);
    }

    // Emit leaf nodes
    for (const auto& [leaf_pin, leaf_arrival] : tree->leaves) {
      ClockTreeNode leaf;
      leaf.id = static_cast<int>(data.nodes.size());
      leaf.parent_id = this_id;
      leaf.name = getInstName(leaf_pin, network);
      leaf.pin_name = getPinName(leaf_pin, network);
      leaf.type = classifyLeaf(leaf_pin, network);
      leaf.arrival = static_cast<float>(leaf_arrival / time_scale);
      leaf.level = tree->level + 1;
      leaf.fanout = 0;
      getPinLocation(leaf_pin, network, leaf.dbu_x, leaf.dbu_y);
      data.nodes.push_back(std::move(leaf));
    }
  }
}

}  // namespace

ClockTreeReport::ClockTreeReport(sta::dbSta* sta) : sta_(sta)
{
}

std::vector<ClockTreeData> ClockTreeReport::getReport() const
{
  std::vector<ClockTreeData> result;
  if (!sta_) {
    return result;
  }

  sta_->ensureGraph();
  sta_->searchPreamble();
  sta::Scene* scene = sta_->cmdScene();
  sta_->ensureClkNetwork(scene->mode());
  sta_->ensureClkArrivals();

  sta::dbNetwork* network = sta_->getDbNetwork();
  const float time_scale = sta_->units()->timeUnit()->scale();
  const std::string time_suffix
      = sta_->units()->timeUnit()->scaleAbbrevSuffix();

  // Build internal trees per clock
  std::map<const sta::Clock*, std::unique_ptr<TreeNode>> roots;
  std::vector<const sta::Clock*> clock_order;

  for (sta::Clock* clk : scene->sdc()->clocks()) {
    auto root = std::make_unique<TreeNode>();
    root->network = network;
    root->level = 0;
    // Get the root net from the clock's first pin
    const sta::PinSet& pins = clk->pins();
    if (pins.empty()) {
      continue;
    }
    const sta::Pin* first_pin = *pins.begin();
    sta::Term* term = network->term(first_pin);
    sta::Net* net = term ? network->net(term) : network->net(first_pin);
    root->net = network->findFlatNet(net);

    clock_order.push_back(clk);
    roots[clk] = std::move(root);
  }

  // Populate trees from STA clock paths
  sta::Graph* graph = sta_->graph();
  for (sta::Vertex* src_vertex : graph->regClkVertices()) {
    sta::VertexPathIterator path_iter(src_vertex, sta_);
    while (path_iter.hasNext()) {
      sta::Path* path = path_iter.next();
      if (path->scene(sta_) != scene) {
        continue;
      }
      sta::PathExpanded expand(path, sta_);
      const sta::Clock* clock = path->clock(sta_);
      if (clock) {
        auto it = roots.find(clock);
        if (it != roots.end()) {
          it->second->addPath(expand, sta_);
        }
      }
    }
  }

  // Flatten each tree into ClockTreeData
  for (const sta::Clock* clk : clock_order) {
    const auto& root = roots[clk];
    if (root->drivers.empty() && root->fanout.empty()) {
      continue;  // virtual clock or empty tree
    }

    ClockTreeData data;
    data.clock_name = clk->name();
    data.time_unit = time_suffix;

    flattenNode(root.get(), -1, network, time_scale, data.clock_name, data);

    if (!data.nodes.empty()) {
      sta::Delay min_arr = root->getMinArrival();
      sta::Delay max_arr = root->getMaxArrival();
      data.min_arrival = static_cast<float>(min_arr / time_scale);
      data.max_arrival = static_cast<float>(max_arr / time_scale);
    }

    result.push_back(std::move(data));
  }

  return result;
}

}  // namespace web
