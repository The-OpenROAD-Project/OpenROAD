// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#include "web/clockTree.h"

#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "colorGenerator.h"
#include "dbDescriptors.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathExpanded.hh"
#include "sta/Scene.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/Transition.hh"
#include "web/core.h"

namespace web {

ClockTree::ClockTree(ClockTree* parent, sta::Net* net)
    : parent_(parent),
      clock_(parent->clock_),
      network_(parent->network_),
      net_(net),
      level_(parent_->level_ + 1),
      subtree_visibility_(true)
{
}

ClockTree::ClockTree(sta::Clock* clock, sta::dbNetwork* network)
    : parent_(nullptr),
      clock_(clock),
      network_(network),
      net_(nullptr),
      level_(0),
      subtree_visibility_(true)
{
  // A virtual clock has no pins; net_ stays null and ClockWidget::populate
  // skips the tree. Dereferencing pins().begin() here would be UB.
  if (!clock_->pins().empty()) {
    net_ = getNet(*clock_->pins().begin());
  }
}

std::set<const sta::Pin*> ClockTree::getDrivers(bool visibility) const
{
  std::set<const sta::Pin*> drivers;
  if (!visibility or isVisible()) {
    for (const auto& [driver, arrival] : drivers_) {
      drivers.insert(driver);
    }
  }
  return drivers;
}

ClockTree* ClockTree::getTree(sta::Net* net)
{
  ClockTree* tree = findTree(net, false);
  if (tree != nullptr) {
    return tree;
  }

  auto child = std::make_unique<ClockTree>(this, net);
  tree = child.get();
  fanout_.push_back(std::move(child));
  return tree;
}

ClockTree* ClockTree::findTree(odb::dbNet* net, bool include_children)
{
  return findTree(network_->dbToSta(net), include_children);
}

ClockTree* ClockTree::findTree(sta::Net* net, bool include_children)
{
  if (net == net_) {
    return this;
  }

  ClockTree* tree = nullptr;
  for (const auto& fanout : fanout_) {
    if (fanout->getNet() == net) {
      return fanout.get();
    }

    if (include_children) {
      tree = fanout->findTree(net);

      if (tree != nullptr) {
        break;
      }
    }
  }

  return tree;
}

// Change its own visibility and subtree visibility
void ClockTree::setSubtreeVisibility(bool visibility)
{
  subtree_visibility_ = visibility;
  for (const auto& fanout : fanout_) {
    fanout->setSubtreeVisibility(subtree_visibility_);
  }
}

int ClockTree::getSinkCount() const
{
  return leaves_.size() + fanout_.size();
}

int ClockTree::getTotalLeaves(bool visibility) const
{
  if (visibility and !subtree_visibility_) {
    return 0;
  }

  int total = leaves_.size();

  for (const auto& fanout : fanout_) {
    total += fanout->getTotalLeaves(visibility);
  }

  return total;
}

int ClockTree::getTotalFanout(bool visibility) const
{
  if (visibility and !subtree_visibility_) {
    return 1;
  }

  int total = 0;
  if (!leaves_.empty()) {
    total = 1;
  }

  for (const auto& fanout : fanout_) {
    total += fanout->getTotalFanout(visibility);
  }

  return total;
}

int ClockTree::getMaxLeaves(bool visibility) const
{
  if (visibility and !subtree_visibility_) {
    return 0;
  }

  int width = leaves_.size();

  for (const auto& fanout : fanout_) {
    width = std::max(width, fanout->getMaxLeaves(visibility));
  }

  return width;
}

sta::Delay ClockTree::getMinimumArrival(bool visibility) const
{
  sta::Delay minimum = std::numeric_limits<float>::max();
  if (!visibility or isVisible()) {
    for (const auto& [driver, arrival] : drivers_) {
      minimum = std::min(minimum, arrival);
    }
  }

  if (!visibility or subtree_visibility_) {
    for (const auto& [leaf, arrival] : leaves_) {
      minimum = std::min(minimum, arrival);
    }

    for (const auto& fanout : fanout_) {
      minimum = std::min(minimum, fanout->getMinimumArrival(visibility));
    }
  }

  return minimum;
}

sta::Delay ClockTree::getMaximumArrival(bool visibility) const
{
  sta::Delay maximum = std::numeric_limits<float>::min();
  if (!visibility or isVisible()) {
    for (const auto& [driver, arrival] : drivers_) {
      maximum = std::max(maximum, arrival);
    }
  }

  if (!visibility or subtree_visibility_) {
    for (const auto& [leaf, arrival] : leaves_) {
      maximum = std::max(maximum, arrival);
    }

    for (const auto& fanout : fanout_) {
      maximum = std::max(maximum, fanout->getMaximumArrival(visibility));
    }
  }

  return maximum;
}

sta::Delay ClockTree::getMinimumDriverDelay(bool visibility) const
{
  sta::Delay minimum = std::numeric_limits<float>::max();
  if ((!visibility or isVisible()) && parent_ != nullptr) {
    for (const auto& [driver, arrival] : drivers_) {
      const auto& [parent_sink, time] = parent_->getPairedSink(driver);
      minimum = std::min(minimum, sta::Delay(arrival - time));
    }
  }

  if (!visibility or subtree_visibility_) {
    for (const auto& fanout : fanout_) {
      minimum = std::min(minimum, fanout->getMinimumDriverDelay(visibility));
    }
  }

  return minimum;
}

odb::PtrSet<odb::dbNet> ClockTree::getNets(bool visibility) const
{
  odb::PtrSet<odb::dbNet> nets;

  if (!visibility or subtree_visibility_) {
    if (net_ != nullptr) {
      nets.insert(network_->staToDb(net_));
    }

    for (const auto& fanout : fanout_) {
      const auto fanout_nets = fanout->getNets(visibility);
      nets.insert(fanout_nets.begin(), fanout_nets.end());
    }
  }

  return nets;
}

void ClockTree::addPath(sta::PathExpanded& path,
                        int idx,
                        const sta::StaState* sta)
{
  if (idx == path.size()) {
    return;
  }

  const sta::Path* ref = path.path(idx);
  sta::Vertex* vertex = ref->vertex(sta);
  sta::Pin* pin = vertex->pin();
  sta::Net* net = getNet(pin);

  ClockTree* add_to_tree = getTree(net);
  if (add_to_tree->addVertex(vertex, ref->arrival())) {
    add_to_tree->addPath(path, idx + 1, sta);
  }
}

void ClockTree::addPath(sta::PathExpanded& path, const sta::StaState* sta)
{
  const sta::Path* start = path.startPath();
  if (start->clkEdge(sta)->transition() != sta::RiseFall::rise()) {
    // only populate with rising edges
    return;
  }

  if (start->minMax(sta) != sta::MinMax::max()) {
    // only populate with max delay
    return;
  }

  if (getNet(start->pin(sta)) != net_) {
    // dont add paths that do not share a net at the root
    return;
  }

  addPath(path, 0, sta);
}

sta::Net* ClockTree::getNet(const sta::Pin* pin) const
{
  sta::Term* term = network_->term(pin);
  sta::Net* net = term ? network_->net(term) : network_->net(pin);
  return network_->findFlatNet(net);
}

bool ClockTree::isLeaf(const sta::Pin* pin) const
{
  return network_->isRegClkPin(pin) || network_->isLatchData(pin);
}

bool ClockTree::addVertex(sta::Vertex* vertex, sta::Delay delay)
{
  const sta::Pin* pin = vertex->pin();

  if (isLeaf(pin)) {
    leaves_[pin] = delay;
    return false;
  }
  if (vertex->isDriver(network_)) {
    drivers_[pin] = delay;
  } else {
    child_sinks_[pin] = delay;
  }
  return true;
}

std::pair<const sta::Pin*, sta::Delay> ClockTree::getPairedSink(
    const sta::Pin* paired_pin) const
{
  sta::Instance* inst = network_->instance(paired_pin);

  if (inst == nullptr) {
    return {nullptr, 0.0};
  }

  for (const auto& [sink, delay] : child_sinks_) {
    if (network_->instance(sink) == inst) {
      return {sink, delay};
    }
  }

  return {nullptr, 0.0};
}

std::vector<std::pair<const sta::Pin*, const sta::Pin*>> ClockTree::findPathTo(
    const sta::Pin* pin) const
{
  auto pin_map = getPinMapping();

  std::vector<std::pair<const sta::Pin*, const sta::Pin*>> path;

  if (drivers_.empty()) {
    // No driver, so there is no root to walk towards.
    return path;
  }

  // looking for path to root
  const sta::Pin* root = drivers_.begin()->first;

  const sta::Pin* search_pin = pin;
  while (search_pin != root) {
    const auto& connections = pin_map[search_pin];

    if (connections.empty()) {
      // search_pin is not connected to anything in this tree, so the walk
      // cannot reach the root.
      break;
    }

    for (const sta::Pin* connect : connections) {
      path.emplace_back(connect, search_pin);
    }

    if (connections.contains(root)) {
      break;
    }

    search_pin = *connections.begin();
  }

  return path;
}

std::map<const sta::Pin*, std::set<const sta::Pin*>> ClockTree::getPinMapping()
    const
{
  std::map<const sta::Pin*, std::set<const sta::Pin*>> pins;

  const auto drivers = getDrivers(false);
  for (const auto& [leaf, arrival] : leaves_) {
    pins[leaf].insert(drivers.begin(), drivers.end());
  }

  for (const auto& [sink, arrival] : child_sinks_) {
    pins[sink].insert(drivers.begin(), drivers.end());
  }

  if (parent_ != nullptr) {
    for (const sta::Pin* driver : drivers) {
      const auto& [parent_sink, time] = parent_->getPairedSink(driver);
      pins[driver].insert(parent_sink);
    }
  }

  for (const auto& fanout : fanout_) {
    for (const auto& [pin, connections] : fanout->getPinMapping()) {
      pins[pin].insert(connections.begin(), connections.end());
    }
  }

  return pins;
}

std::vector<std::unique_ptr<ClockTree>> buildClockTrees(sta::dbSta* sta,
                                                        sta::Scene* scene,
                                                        const sta::Clock* only)
{
  sta->ensureGraph();
  sta->searchPreamble();
  sta->ensureClkNetwork(scene->mode());
  sta->ensureClkArrivals();

  std::vector<std::unique_ptr<ClockTree>> trees;
  std::map<const sta::Clock*, ClockTree*> roots;
  for (sta::Clock* clk : scene->sdc()->clocks()) {
    if (only != nullptr && clk != only) {
      continue;
    }
    auto root = std::make_unique<ClockTree>(clk, sta->getDbNetwork());
    roots[clk] = root.get();
    trees.push_back(std::move(root));
  }

  sta::Graph* graph = sta->graph();
  for (sta::Vertex* src_vertex : graph->regClkVertices()) {
    sta::VertexPathIterator path_iter(src_vertex, sta);
    while (path_iter.hasNext()) {
      sta::Path* path = path_iter.next();

      if (path->scene(sta) != scene) {
        continue;
      }

      const auto root = roots.find(path->clock(sta));
      if (root == roots.end()) {
        continue;
      }
      sta::PathExpanded expand(path, sta);
      root->second->addPath(expand, sta);
    }
  }

  return trees;
}

namespace {

constexpr int kClockTreePenWidth = 2;

void setClockTreePen(Painter& painter, const Painter::Color& color)
{
  painter.setPenAndBrush(color, true, Painter::kSolid, kClockTreePenWidth);
}

void paintTree(const ClockTree& tree,
               int depth,
               const int max_depth,
               const Descriptor* descriptor,
               ColorGenerator& colors,
               Painter& painter)
{
  odb::dbNet* net = tree.getNetwork()->staToDb(tree.getNet());
  descriptor->highlight(net, painter);

  const Painter::Color pen_color = painter.getPenColor();
  const bool change_color = depth < max_depth && tree.getSinkCount() > 1;
  if (change_color) {
    depth++;
  }
  for (const auto& fanout : tree.getFanout()) {
    if (change_color) {
      setClockTreePen(painter, colors.getColor());
    }
    paintTree(*fanout, depth, max_depth, descriptor, colors, painter);
  }

  setClockTreePen(painter, pen_color);
}

}  // namespace

Painter::Color clockTreeTrunkColor(const Painter::Color& background)
{
  const double luminance
      = 0.299 * background.r + 0.587 * background.g + 0.114 * background.b;
  return luminance > 127 ? Painter::kBlack : Painter::kWhite;
}

void paintClockTree(const ClockTree& tree,
                    const int max_depth,
                    const Painter::Color& trunk,
                    Painter& painter)
{
  const Descriptor* descriptor = Gui::get()->getDescriptor<odb::dbNet*>();
  if (descriptor == nullptr || tree.getNet() == nullptr) {
    return;
  }
  ColorGenerator colors;
  setClockTreePen(painter, trunk);
  paintTree(tree, 0, max_depth, descriptor, colors, painter);
}

}  // namespace web
