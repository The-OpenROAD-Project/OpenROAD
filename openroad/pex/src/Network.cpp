// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Network.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "pex/Resistance.h"

namespace pex {

NetPlace Network::place(const int shape_id, const odb::Point point) const
{
  const auto it = shapes.find(shape_id);
  if (it == shapes.end()) {
    throw std::out_of_range("no wire shape " + std::to_string(shape_id));
  }
  const ShapePlace& s = it->second;
  if (s.resistor < 0) {
    return {.resistor = -1, .node = s.node, .t = 0};
  }
  const int length
      = std::abs(s.end.x() - s.start.x()) + std::abs(s.end.y() - s.start.y());
  double f = 0.5;
  if (length > 0) {
    const int along = s.end.x() != s.start.x() ? point.x() - s.start.x()
                                               : point.y() - s.start.y();
    const int toward = s.end.x() != s.start.x() ? s.end.x() - s.start.x()
                                                : s.end.y() - s.start.y();
    f = std::clamp(static_cast<double>(along) / toward, 0.0, 1.0);
  }
  return {.resistor = s.resistor, .node = -1, .t = s.t0 + (f * (s.t1 - s.t0))};
}

namespace {

class Builder
{
 public:
  Builder(odb::dbNet* net, const Resistance& resistance, const double max_res)
      : net_(net), resistance_(resistance), max_res_(max_res)
  {
  }

  Network build();

 private:
  struct Pending
  {
    int shape;
    double resistance;
    odb::Point start;
    odb::Point end;
  };

  // The junctions a branch or a short leaves from, which must be nodes,
  // and the shorts' forwarding to the junction they join.
  void findJunctions(odb::dbWire* wire);
  void walk(odb::dbWire* wire);
  double shapeResistance(const odb::dbWirePathShape& shape,
                         odb::Point previous) const;

  int resolve(const int junction) const
  {
    const auto it = forward_.find(junction);
    return it == forward_.end() ? junction : it->second;
  }

  // The node of a junction and the terminals on it, made when new. A
  // terminal arriving at a plain junction's node turns it into the
  // terminal's; a second terminal there gets its own node, joined to the
  // first by a resistor of 0 ohms.
  int node(odb::dbITerm* iterm,
           odb::dbBTerm* bterm,
           int junction,
           odb::Point point);
  // The node of a terminal at the node `at` (-1 for none yet): the
  // terminal's own when it has one, else `at` taken over when a plain
  // junction, else a new node.
  int attach(NetNode::Kind kind,
             int id,
             std::unordered_map<int, int>& terms,
             odb::Point point,
             int at);
  // Ends the shapes pending since the source in a resistor to `target`.
  void close(int target);

  odb::dbNet* net_;
  const Resistance& resistance_;
  const double max_res_;
  Network network_;
  std::set<int> required_;
  std::unordered_map<int, int> forward_;    // a short's junction to its own
  std::unordered_map<int, int> junctions_;  // node by junction id
  std::vector<Pending> pending_;
  double sum_ = 0;
  int source_ = -1;
};

Network Builder::build()
{
  odb::dbWire* wire = net_->getWire();
  if (wire == nullptr) {
    return {};
  }
  findJunctions(wire);
  walk(wire);
  return std::move(network_);
}

void Builder::findJunctions(odb::dbWire* wire)
{
  odb::dbWirePathItr paths;
  odb::dbWirePath path;
  odb::dbWirePathShape shape;
  for (paths.begin(wire); paths.getNextPath(path);) {
    if (path.is_short) {
      required_.insert(path.short_junction);
      forward_[path.junction_id] = resolve(path.short_junction);
    } else if (path.is_branch) {
      required_.insert(path.junction_id);
    }
    while (paths.getNextShape(shape)) {
    }
  }
}

void Builder::walk(odb::dbWire* wire)
{
  odb::dbWirePathItr paths;
  odb::dbWirePath path;
  for (paths.begin(wire); paths.getNextPath(path);) {
    source_
        = node(path.iterm, path.bterm, resolve(path.junction_id), path.point);
    odb::Point previous = path.point;
    int last = -1;
    odb::dbWirePathShape shape;
    while (paths.getNextShape(shape)) {
      const double r = shapeResistance(shape, previous);
      if (max_res_ > 0 && !pending_.empty() && sum_ + r > max_res_) {
        close(node(nullptr, nullptr, last, previous));
      }
      pending_.push_back({
          .shape = shape.junction_id,
          .resistance = r,
          .start = previous,
          .end = shape.point,
      });
      sum_ += r;
      if (shape.iterm != nullptr || shape.bterm != nullptr
          || required_.contains(shape.junction_id) || max_res_ <= 0) {
        close(node(shape.iterm, shape.bterm, shape.junction_id, shape.point));
      }
      previous = shape.point;
      last = shape.junction_id;
    }
    if (!pending_.empty()) {
      close(node(nullptr, nullptr, last, previous));
    }
  }
}

double Builder::shapeResistance(const odb::dbWirePathShape& shape,
                                const odb::Point previous) const
{
  if (shape.shape.isVia()) {
    return resistance_.via(shape.shape);
  }
  const odb::Rect box = shape.shape.getBox();
  const int dx = std::abs(shape.point.x() - previous.x());
  const int dy = std::abs(shape.point.y() - previous.y());
  if (dx + dy == 0) {
    return 0;
  }
  const int width = dx > 0 ? box.dy() : box.dx();
  return resistance_.wire(shape.shape.getTechLayer(), width, dx + dy);
}

int Builder::node(odb::dbITerm* iterm,
                  odb::dbBTerm* bterm,
                  const int junction,
                  const odb::Point point)
{
  const auto at = junctions_.find(junction);
  int result = at == junctions_.end() ? -1 : at->second;
  if (iterm != nullptr) {
    result = attach(
        NetNode::Kind::kITerm, iterm->getId(), network_.iterms, point, result);
  }
  if (bterm != nullptr) {
    result = attach(
        NetNode::Kind::kBTerm, bterm->getId(), network_.bterms, point, result);
  }
  if (result < 0) {
    result = static_cast<int>(network_.nodes.size());
    network_.nodes.push_back(
        {.kind = NetNode::Kind::kJunction, .id = junction, .point = point});
  }
  junctions_.try_emplace(junction, result);
  return result;
}

int Builder::attach(const NetNode::Kind kind,
                    const int id,
                    std::unordered_map<int, int>& terms,
                    const odb::Point point,
                    const int at)
{
  const auto it = terms.find(id);
  int own = it == terms.end() ? -1 : it->second;
  if (own < 0 && at >= 0
      && network_.nodes[at].kind == NetNode::Kind::kJunction) {
    network_.nodes[at].kind = kind;
    network_.nodes[at].id = id;
    own = at;
  } else if (own < 0) {
    own = static_cast<int>(network_.nodes.size());
    network_.nodes.push_back({.kind = kind, .id = id, .point = point});
  }
  terms.emplace(id, own);
  if (at >= 0 && at != own) {
    network_.resistors.push_back(
        {.source = at, .target = own, .resistance = 0});
  }
  return own;
}

void Builder::close(const int target)
{
  const int resistor
      = target == source_ ? -1 : static_cast<int>(network_.resistors.size());
  if (resistor < 0) {
    network_.loops++;
  } else {
    network_.resistors.push_back(
        {.source = source_, .target = target, .resistance = sum_});
  }
  double before = 0;
  for (size_t i = 0; i < pending_.size(); i++) {
    const Pending& p = pending_[i];
    ShapePlace place{
        .resistor = resistor,
        .node = resistor < 0 ? source_ : -1,
        .start = p.start,
        .end = p.end,
    };
    if (sum_ > 0) {
      place.t0 = before / sum_;
      place.t1 = (before + p.resistance) / sum_;
    } else {
      place.t0 = static_cast<double>(i) / pending_.size();
      place.t1 = static_cast<double>(i + 1) / pending_.size();
    }
    before += p.resistance;
    network_.shapes[p.shape] = place;
  }
  pending_.clear();
  sum_ = 0;
  source_ = target;
}

}  // namespace

Network buildNetwork(odb::dbNet* net,
                     const Resistance& resistance,
                     const double max_res)
{
  return Builder(net, resistance, max_res).build();
}

}  // namespace pex
