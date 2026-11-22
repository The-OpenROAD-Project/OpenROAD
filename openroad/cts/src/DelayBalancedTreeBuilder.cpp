// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "DelayBalancedTreeBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Clock.h"
#include "ClockDelayModel.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace cts {

using utl::CTS;

// Share of the largest load of the strongest buffer that a group aims at.
static constexpr double kLoadShare = 0.85;
// Share of the longest wire within the slew limit that a trunk wire may have.
static constexpr double kHopShare = 0.9;
// Share of a trunk wire that each of a joint's own two wires may have.
static constexpr double kFork = 0.75;
// Largest group diameter, in trunk wires.
static constexpr double kDiameterShare = 1.25;
// Least number of nodes per buffer for another level of groups to be kept.
static constexpr double kReduction = 2.5;
// Share of a level's delay that the loads of one buffer may be apart.
static constexpr double kSpreadShare = 0.2;
// Share of a level's delay by which a sink with clock logic behind it may be
// later than a level and still join it.
static constexpr double kLateShare = 0.25;
// Fewest nodes a buffer above the sinks stands over.
static constexpr size_t kMinGroup = 2;
// Share of a trunk stage's delay by which an item is earlier than the lower
// quartile of the items when the trunk joins it before any other.
static constexpr double kFront = 0.5;
// How far over the least the mismatch of a level may be for weaker cells.
static constexpr double kWeakSlack = 0.15;
// Most repeaters of a joint, most delaying buffers of a feed, and most hops
// beyond what its length asks for on the way from the driver.
static constexpr int kMostRepeaters = 24;
// Most loads a net's own driver is considered for.
static constexpr size_t kMostDriven = 128;
static constexpr int kJointLevel = -1;
static constexpr int kRepeaterLevel = -2;
static constexpr int kFeedLevel = -3;

double DelayBalancedTreeBuilder::Path::length() const
{
  double total = 0.0;
  for (size_t i = 1; i < points.size(); i++) {
    total += odb::Point::manhattanDistance(points[i - 1], points[i]);
  }
  return total;
}

odb::Point DelayBalancedTreeBuilder::Path::at(double distance) const
{
  for (size_t i = 1; i < points.size(); i++) {
    const odb::Point& p = points[i - 1];
    const odb::Point& q = points[i];
    const double step = odb::Point::manhattanDistance(p, q);
    if (distance <= step && step > 0.0) {
      const double t = std::max(distance, 0.0) / step;
      return odb::Point(std::lround(p.x() + t * (q.x() - p.x())),
                        std::lround(p.y() + t * (q.y() - p.y())));
    }
    distance -= step;
  }
  return points.back();
}

void DelayBalancedTreeBuilder::run()
{
  trunk_ = model_->strongestBuffer();
  slew_ = model_->nominalSlew();
  hop_ = kHopShare * model_->maxHop();
  gap_ = model_->master(trunk_)->getWidth();
  const double stage
      = model_->delay(trunk_, slew_, 2.0 * model_->inputCap(trunk_));
  tolerance_ = 0.02 * stage;
  pad_ = 0.3 * stage;
  initFloor();

  std::vector<int> pool;
  std::vector<int> waiting;
  initSinks(pool, waiting);
  if (pool.empty() && waiting.empty()) {
    return;
  }
  const double diameter = options_->getMaxDiameter() * model_->dbuPerMicron();
  const unsigned fanout = options_->getSinkClusteringSize();
  const bool fed = driver_cell_ < 0 || parent_ == nullptr;
  if (fed && allow_port_buffer_ && !port_loads_.empty() && driver_cell_ >= 0) {
    planPorts();
  }
  if (fed && allow_port_buffer_ && has_source_ && driver_cell_ >= 0) {
    planSource();
  }

  if (!fed) {
    std::vector<int> all = pool;
    all.insert(all.end(), waiting.begin(), waiting.end());
    if (drivable(all, diameter, fanout)) {
      debugPrint(logger_,
                 CTS,
                 "delay balance",
                 2,
                 "tree {} left on its driver {}: {} loads, {:.2f} fF of "
                 "{:.2f}, slew {:.2f} ps",
                 clock_.getName(),
                 model_->cellName(driver_cell_),
                 all.size(),
                 driverWire(all).cap[ClockDelayModel::kSlow] * 1e15,
                 model_->maxLoad(driver_cell_) * 1e15,
                 model_->worstSlew(driver_cell_, driverWire(all)) * 1e12);
      roots_ = all;
      setDriverModel(all);
      clock_.setUntouched(true);
      return;
    }
  }

  // The tree is grown twice: with groups above the sinks that widen level
  // by level, and with groups as wide as the trunk reaches from the first.
  // The less late is kept, counting only what is over the latency the
  // registers aim at, and of two as late the one whose registers are nearer
  // one another.
  Grown best = grow(false);
  if (best.widens) {
    Grown wide = grow(true);
    auto over = [&](const Grown& grown) {
      return has_latency_ ? std::max(0.0, grown.mean - latency_) : grown.mean;
    };
    const double more = over(wide) - over(best);
    if (more < -pad_ || (more <= pad_ && wide.spread < best.spread)) {
      best = std::move(wide);
    }
  }
  debugPrint(logger_,
             CTS,
             "delay balance",
             1,
             "tree {} kept with groups {}",
             clock_.getName(),
             best.wide ? "at full reach" : "that widen by level");
  nodes_ = std::move(best.nodes);
  roots_ = std::move(best.roots);
  setDriverModel(roots_);
  emit(roots_);
  clock_.setMaxLevel(best.levels);
  report();
}

DelayBalancedTreeBuilder::Grown DelayBalancedTreeBuilder::grow(const bool wide)
{
  nodes_.clear();
  joints_ = 0;
  repeaters_ = 0;
  worst_miss_ = 0.0;
  sum_miss_ = 0.0;
  std::vector<int> pool;
  std::vector<int> waiting;
  initSinks(pool, waiting);
  double diameter = options_->getMaxDiameter() * model_->dbuPerMicron();
  const unsigned fanout = options_->getSinkClusteringSize();
  const bool fed = driver_cell_ < 0 || parent_ == nullptr;
  bool widens = false;

  // The lateness of the pool and of a level above it.
  double reference = 0.0;
  double step
      = model_->delay(trunk_, slew_, kLoadShare * model_->maxLoad(trunk_));
  size_t next = 0;
  int levels = 0;
  // Nodes left out of the levels: the trunk joins them.
  std::vector<int> tops;
  for (int level = 1;; level++) {
    while (next < waiting.size()
           && late(waiting[next]) <= reference + kLateShare * step) {
      pool.push_back(waiting[next++]);
    }
    if (pool.empty()) {
      if (next == waiting.size()) {
        break;
      }
      reference = late(waiting[next]);
      continue;
    }
    widens = widens || (level > 1 && diameter < kDiameterShare * hop_);
    const bool last = next == waiting.size();
    if (last
        && (pool.size() == 1
            || (!fed && tops.empty()
                && drivable(pool, diameter, level == 1 ? fanout : 0)))) {
      break;
    }
    const double width = kSpreadShare * step;
    std::vector<int> few;
    std::vector<int> buffers
        = makeLevel(pool, level, level == 1 ? fanout : 0, width, diameter, few);
    if (buffers.empty()) {
      break;
    }
    const double target = balance(buffers, waiting, next, reference);
    const size_t made = buffers.size();
    const std::vector<int> early = padSingles(buffers, target, width);
    debugPrint(logger_,
               CTS,
               "delay balance",
               1,
               "tree {} level {}: {} nodes under {} buffers, {} left to the "
               "trunk, lateness {:.2f} ps",
               clock_.getName(),
               level,
               pool.size() - few.size(),
               made - early.size(),
               few.size() + early.size(),
               target * 1e12);
    tops.insert(tops.end(), few.begin(), few.end());
    tops.insert(tops.end(), early.begin(), early.end());
    step = std::max(target - reference, tolerance_);
    reference = target;
    pool = buffers;
    levels = level;
    diameter = wide ? kDiameterShare * hop_
                    : std::min(2.0 * diameter, kDiameterShare * hop_);
  }
  pool.insert(pool.end(), tops.begin(), tops.end());
  pool.insert(pool.end(), waiting.begin() + next, waiting.end());

  roots_ = joinTops(pool, fed, step);
  propagate();
  Grown grown;
  double earliest, latest;
  registerArrivals(grown.mean, earliest, latest);
  grown.spread = latest - earliest;
  grown.nodes = nodes_;
  grown.roots = roots_;
  grown.levels = levels;
  grown.widens = widens;
  grown.wide = wide;
  return grown;
}

std::vector<int> DelayBalancedTreeBuilder::makeLevel(
    const std::vector<int>& pool,
    const int level,
    const unsigned fanout,
    const double width,
    double& diameter,
    std::vector<int>& few)
{
  const size_t first_new = nodes_.size();
  std::vector<int> cells;
  cells.reserve(pool.size());
  for (const int node : pool) {
    cells.push_back(nodes_[node].cell);
  }
  while (true) {
    std::vector<int> buffers = cluster(pool, diameter, fanout, level, width);
    few.clear();
    if (level > 1) {
      std::vector<int> kept;
      for (const int buffer : buffers) {
        std::vector<int>& children = nodes_[buffer].children;
        if (children.size() < kMinGroup && !nodes_[buffer].part) {
          few.insert(few.end(), children.begin(), children.end());
          children.clear();
        } else {
          kept.push_back(buffer);
        }
      }
      buffers = std::move(kept);
    }
    if (!buffers.empty()
        && (level == 1
            || buffers.size() * kReduction <= pool.size() - few.size())) {
      return buffers;
    }
    nodes_.resize(first_new);
    for (size_t i = 0; i < pool.size(); i++) {
      setCell(pool[i], cells[i]);
    }
    const double wider
        = std::min(std::max(2.0 * diameter, gap_), kDiameterShare * hop_);
    if (level == 1 || wider <= diameter) {
      return {};
    }
    diameter = wider;
  }
}

std::vector<int> DelayBalancedTreeBuilder::joinTops(std::vector<int> pool,
                                                    const bool fed,
                                                    const double step)
{
  double earliest = std::numeric_limits<double>::max();
  double latest = std::numeric_limits<double>::lowest();
  for (const int node : pool) {
    earliest = std::min(earliest, late(node));
    latest = std::max(latest, late(node));
  }
  std::vector<int> roots;
  const size_t tops_count = pool.size();
  if (fed) {
    roots.push_back(feed(joinAll(pool)));
  } else if (latest - earliest <= kSpreadShare * step
             && drivable(pool, std::numeric_limits<double>::max(), 0)) {
    roots = std::move(pool);
  } else {
    roots.push_back(reach(joinAll(pool)));
  }
  debugPrint(logger_,
             CTS,
             "delay balance",
             1,
             "tree {} trunk: {} nodes from {:.2f} to {:.2f} ps late under {} "
             "joints with {} repeaters, off balance by {:.2f} ps at most and "
             "{:.2f} ps on average",
             clock_.getName(),
             tops_count,
             earliest * 1e12,
             latest * 1e12,
             joints_,
             repeaters_,
             worst_miss_ * 1e12,
             joints_ > 0 ? sum_miss_ / joints_ * 1e12 : 0.0);
  return roots;
}

void DelayBalancedTreeBuilder::registerArrivals(double& mean,
                                                double& earliest,
                                                double& latest) const
{
  double sum = 0.0;
  double weight = 0.0;
  earliest = std::numeric_limits<double>::max();
  latest = std::numeric_limits<double>::lowest();
  for (int node = 0; node < nodes_.size(); node++) {
    const Node& n = nodes_[node];
    if (!n.sink) {
      continue;
    }
    const double at = n.arrival + lateAt(node, n.in_slew);
    sum += n.registers * at;
    weight += n.registers;
    earliest = std::min(earliest, at);
    latest = std::max(latest, at);
  }
  mean = sum / weight;
}

void DelayBalancedTreeBuilder::initFloor()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  core_ = block->getCoreArea();
  if (core_.area() == 0) {
    core_ = block->computeCoreArea().getEnclosingRect();
  }
  if (core_.area() == 0) {
    core_ = block->getDieArea();
  }
  for (odb::dbRow* row : block->getRows()) {
    row_height_ = row->getSite()->getHeight();
    site_width_ = row->getSite()->getWidth();
    break;
  }
  odb::dbMaster* master = model_->master(trunk_);
  half_width_ = master->getWidth() / 2;
  half_height_ = master->getHeight() / 2;
  driver_ = clock_.getClockPin();
  driver_pin_ = {.x = driver_.x(), .y = driver_.y()};
  odb::dbObject* driver = clock_.getDriverPin();
  if (driver != nullptr && driver->getObjectType() == odb::dbITermObj) {
    driver_pin_.layer = ClockDelayModel::pinLayer(
        static_cast<odb::dbITerm*>(driver)->getMTerm());
  } else if (clock_.getNetObj() != nullptr) {
    driver_pin_.layer
        = ClockDelayModel::pinLayer(clock_.getNetObj()->get1stBTerm());
  }
}

void DelayBalancedTreeBuilder::initSinks(std::vector<int>& pool,
                                         std::vector<int>& waiting)
{
  clock_.forEachSink([&](ClockInst& sink) {
    Node node;
    node.x = sink.getX();
    node.y = sink.getY();
    node.cap = sink.getDbInputPin() != nullptr
                   ? model_->pinCap(sink.getDbInputPin())
                   : sink.getInputCap();
    node.inst = &sink;
    node.sink = true;
    odb::dbITerm* pin = sink.getDbInputPin();
    node.layer
        = pin != nullptr ? ClockDelayModel::pinLayer(pin->getMTerm()) : nullptr;
    node.register_clock = pin != nullptr && model_->isRegisterClock(pin);
    bool waits = false;
    const auto it = pin_models_.find(pin);
    if (it != pin_models_.end()) {
      node.cell = it->second.cell;
      node.gate = it->second.delay;
      node.load = it->second.load;
      node.after = it->second.after;
      node.registers = std::max(1.0, it->second.registers);
      waits = true;
    }
    nodes_.push_back(std::move(node));
    (waits ? waiting : pool).push_back(nodes_.size() - 1);
  });
  std::ranges::stable_sort(
      waiting, [&](const int a, const int b) { return late(a) < late(b); });
}

int DelayBalancedTreeBuilder::addBuffer(const odb::Point& at,
                                        const int cell,
                                        const int level)
{
  Node node;
  node.x = at.x();
  node.y = at.y();
  node.cell = cell;
  node.cap = model_->inputCap(cell);
  node.level = level;
  nodes_.push_back(std::move(node));
  return nodes_.size() - 1;
}

NetPin DelayBalancedTreeBuilder::inputPin(const int node) const
{
  const Node& n = nodes_[node];
  if (n.sink) {
    return {.x = n.x, .y = n.y, .cap = n.cap, .layer = n.layer};
  }
  const odb::Point pin = model_->inputPin(n.cell);
  return {
      .x = n.x + pin.x(),
      .y = n.y + pin.y(),
      .cap = model_->inputCap(n.cell),
      .layer = model_->inputLayer(n.cell),
  };
}

NetPin DelayBalancedTreeBuilder::outputPin(const int node) const
{
  const Node& n = nodes_[node];
  const odb::Point pin = model_->outputPin(n.cell);
  return {
      .x = n.x + pin.x(),
      .y = n.y + pin.y(),
      .cap = 0.0,
      .layer = model_->outputLayer(n.cell),
  };
}

NetPin DelayBalancedTreeBuilder::trunkInput(const odb::Point& at) const
{
  const odb::Point pin = model_->inputPin(trunk_);
  return {
      .x = at.x() + pin.x(),
      .y = at.y() + pin.y(),
      .cap = model_->inputCap(trunk_),
      .layer = model_->inputLayer(trunk_),
  };
}

NetPin DelayBalancedTreeBuilder::trunkOutput(const odb::Point& at) const
{
  const odb::Point pin = model_->outputPin(trunk_);
  return {
      .x = at.x() + pin.x(),
      .y = at.y() + pin.y(),
      .cap = 0.0,
      .layer = model_->outputLayer(trunk_),
  };
}

double DelayBalancedTreeBuilder::lateAt(const int node, const double slew) const
{
  const Node& n = nodes_[node];
  if (n.cell < 0) {
    return n.after;
  }
  return model_->delayAt(n.cell, n.gate, slew, n.load) + n.after;
}

void DelayBalancedTreeBuilder::settle(const int node)
{
  nodes_[node].wire = wireOver(outputPin(node), nodes_[node].children);
  time(node);
}

void DelayBalancedTreeBuilder::time(const int node)
{
  Node& n = nodes_[node];
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, n.cell, slew_, n.wire);
  n.gate = stage.delay;
  n.load = stage.load;
  n.slew = stage.slew;
  double sum = 0.0;
  double weight = 0.0;
  for (int i = 0; i < n.children.size(); i++) {
    const int child = n.children[i];
    sum += nodes_[child].registers
           * (stage.wire_delay[i] + lateAt(child, stage.load_slew[i]));
    weight += nodes_[child].registers;
  }
  n.after = sum / weight;
  n.registers = weight;
  n.cap = model_->inputCap(n.cell);
  n.slew_ok = model_->slewOk(n.cell, n.wire);
}

void DelayBalancedTreeBuilder::setCell(const int node, const int cell)
{
  if (nodes_[node].options.empty() || nodes_[node].cell == cell) {
    return;
  }
  nodes_[node].cell = cell;
  time(node);
}

std::vector<int> DelayBalancedTreeBuilder::optionsOf(const int node) const
{
  if (nodes_[node].options.empty()) {
    return {nodes_[node].cell};
  }
  return nodes_[node].options;
}

double DelayBalancedTreeBuilder::extent(const std::vector<int>& items) const
{
  int x0 = std::numeric_limits<int>::max();
  int y0 = x0;
  int x1 = std::numeric_limits<int>::min();
  int y1 = x1;
  for (const int item : items) {
    x0 = std::min(x0, nodes_[item].x);
    x1 = std::max(x1, nodes_[item].x);
    y0 = std::min(y0, nodes_[item].y);
    y1 = std::max(y1, nodes_[item].y);
  }
  return static_cast<double>(x1 - x0) + (y1 - y0);
}

odb::Point DelayBalancedTreeBuilder::centroid(
    const std::vector<int>& items) const
{
  double x = 0.0;
  double y = 0.0;
  for (const int item : items) {
    x += nodes_[item].x;
    y += nodes_[item].y;
  }
  return odb::Point(std::lround(x / items.size()),
                    std::lround(y / items.size()));
}

bool DelayBalancedTreeBuilder::anyRegisterClock(
    const std::vector<int>& items) const
{
  return std::ranges::any_of(
      items, [&](const int item) { return nodes_[item].register_clock; });
}

NetWire DelayBalancedTreeBuilder::wireOver(const NetPin& driver,
                                           const std::vector<int>& items) const
{
  std::vector<NetPin> loads;
  loads.reserve(items.size());
  for (const int item : items) {
    loads.push_back(inputPin(item));
  }
  return model_->wire(driver, loads, anyRegisterClock(items));
}

NetWire DelayBalancedTreeBuilder::driverWire(
    const std::vector<int>& items) const
{
  std::vector<NetPin> loads;
  loads.reserve(items.size() + port_loads_.size());
  for (const int item : items) {
    loads.push_back(inputPin(item));
  }
  if (isolates_ports_) {
    loads.push_back(trunkInput(port_chain_.front()));
  } else {
    loads.insert(loads.end(), port_loads_.begin(), port_loads_.end());
  }
  return model_->wire(driver_pin_, loads, anyRegisterClock(items));
}

StageTiming DelayBalancedTreeBuilder::driverStage(const NetWire& wire) const
{
  if (driver_cell_ >= 0) {
    return model_->drive(ClockDelayModel::kBalance,
                         driver_cell_,
                         driver_slew_ >= 0.0 ? driver_slew_ : slew_,
                         wire);
  }
  return model_->port(ClockDelayModel::kBalance,
                      driver_slew_ >= 0.0 ? driver_slew_ : slew_,
                      wire);
}

bool DelayBalancedTreeBuilder::drivable(const std::vector<int>& items,
                                        const double diameter,
                                        const unsigned fanout) const
{
  if (driver_cell_ < 0 || (fanout > 0 && items.size() > fanout)
      || items.size() > kMostDriven || extent(items) > diameter) {
    return false;
  }
  const NetWire wire = driverWire(items);
  return wire.cap[ClockDelayModel::kSlow] <= model_->maxLoad(driver_cell_)
         && model_->slewOk(driver_cell_, wire);
}

std::vector<double> DelayBalancedTreeBuilder::spacing(
    const std::vector<int>& items,
    const double reach) const
{
  const int bin = std::clamp(reach, 1.0, 1e9);
  const auto key = [](const int64_t i, const int64_t j) {
    return (static_cast<uint64_t>(i) << 32)
           ^ (static_cast<uint64_t>(j) & 0xffffffffULL);
  };
  std::unordered_map<uint64_t, std::vector<int>> grid;
  for (const int item : items) {
    grid[key(nodes_[item].x / bin, nodes_[item].y / bin)].push_back(item);
  }
  std::vector<double> out;
  out.reserve(items.size());
  for (const int item : items) {
    const Node& n = nodes_[item];
    double nearest = reach;
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        const auto it = grid.find(key(n.x / bin + dx, n.y / bin + dy));
        if (it == grid.end()) {
          continue;
        }
        for (const int other : it->second) {
          if (other != item) {
            nearest = std::min<double>(nearest,
                                       std::abs(nodes_[other].x - n.x)
                                           + std::abs(nodes_[other].y - n.y));
          }
        }
      }
    }
    out.push_back(nearest);
  }
  return out;
}

void DelayBalancedTreeBuilder::bisect(
    std::vector<int> items,
    int parts,
    const std::vector<double>& weight,
    std::vector<std::vector<int>>& groups) const
{
  if (parts <= 1 || items.size() <= 1) {
    groups.push_back(std::move(items));
    return;
  }
  parts = std::min<int>(parts, items.size());
  int x0 = std::numeric_limits<int>::max();
  int y0 = x0;
  int x1 = std::numeric_limits<int>::min();
  int y1 = x1;
  double total = 0.0;
  for (const int item : items) {
    x0 = std::min(x0, nodes_[item].x);
    x1 = std::max(x1, nodes_[item].x);
    y0 = std::min(y0, nodes_[item].y);
    y1 = std::max(y1, nodes_[item].y);
    total += weight[item];
  }
  const bool by_x = x1 - x0 >= y1 - y0;
  std::ranges::sort(items, [&](const int a, const int b) {
    const Node& p = nodes_[a];
    const Node& q = nodes_[b];
    const auto key_p = by_x ? std::pair(p.x, p.y) : std::pair(p.y, p.x);
    const auto key_q = by_x ? std::pair(q.x, q.y) : std::pair(q.y, q.x);
    return key_p != key_q ? key_p < key_q : a < b;
  });
  const int near = parts / 2;
  double want = total * near / parts;
  int cut = 0;
  while (cut < items.size() && want > weight[items[cut]] / 2.0) {
    want -= weight[items[cut]];
    cut++;
  }
  cut = std::clamp<int>(cut, near, items.size() - (parts - near));
  bisect({items.begin(), items.begin() + cut}, near, weight, groups);
  bisect({items.begin() + cut, items.end()}, parts - near, weight, groups);
}

bool DelayBalancedTreeBuilder::fits(const std::vector<int>& group,
                                    const double diameter,
                                    const unsigned fanout) const
{
  if ((fanout > 0 && group.size() > fanout) || extent(group) > diameter) {
    return false;
  }
  const NetWire wire = wireOver(trunkOutput(legal(centroid(group))), group);
  return wire.cap[ClockDelayModel::kSlow] <= model_->maxLoad(trunk_)
         && model_->slewOk(trunk_, wire);
}

std::vector<std::vector<int>> DelayBalancedTreeBuilder::groupsOf(
    const std::vector<int>& items,
    const double diameter,
    const unsigned fanout) const
{
  std::vector<double> weight(nodes_.size(), 0.0);
  const std::vector<double> apart = spacing(items, diameter);
  const bool leaf = anyRegisterClock(items);
  double total = 0.0;
  for (int i = 0; i < items.size(); i++) {
    weight[items[i]] = inputPin(items[i]).cap + model_->wireCap(apart[i], leaf);
    total += weight[items[i]];
  }
  int parts = std::ceil(total / (kLoadShare * model_->maxLoad(trunk_)));
  if (fanout > 0) {
    parts
        = std::max<int>(parts, std::ceil(items.size() / (kLoadShare * fanout)));
  }
  std::vector<std::vector<int>> todo;
  bisect(items, parts, weight, todo);
  std::vector<std::vector<int>> done;
  while (!todo.empty()) {
    std::vector<int> group = std::move(todo.back());
    todo.pop_back();
    if (group.size() > 1 && !fits(group, diameter, fanout)) {
      bisect(std::move(group), 2, weight, todo);
    } else {
      done.push_back(std::move(group));
    }
  }
  return done;
}

std::vector<int> DelayBalancedTreeBuilder::cluster(const std::vector<int>& pool,
                                                   const double diameter,
                                                   const unsigned fanout,
                                                   const int level,
                                                   const double width)
{
  // Nodes that stay alone get a second and a third chance in wider groups:
  // where they are sparse, the wires between them are what a group can
  // afford, and a buffer over each of them is not.
  std::vector<std::vector<int>> near;
  std::vector<int> rest = pool;
  double reach = diameter;
  for (int pass = 0; pass < 3 && !rest.empty(); pass++) {
    std::vector<int> alone;
    for (std::vector<int>& group : groupsOf(rest, reach, fanout)) {
      if (group.size() == 1 && pass < 2) {
        alone.push_back(group.front());
      } else {
        near.push_back(std::move(group));
      }
    }
    rest = std::move(alone);
    reach = std::min(2.0 * reach, std::max(diameter, kDiameterShare * hop_));
  }
  auto make = [&](std::vector<int> group, const bool part) {
    const int buffer = addBuffer(legal(centroid(group)), trunk_, level);
    nodes_[buffer].children = std::move(group);
    nodes_[buffer].part = part;
    settle(buffer);
    even(buffer);
    return buffer;
  };
  // Nothing below a buffer makes up what is left between its loads once
  // each has the cell that suits it best, so a group left more than `width`
  // apart is split by lateness. The buffers of its parts stand together and
  // take the cells that make their registers equally late.
  std::vector<int> buffers;
  const std::vector<double> equal(nodes_.size(), 1.0);
  // The cells its loads took may leave a group more than a buffer drives
  // within the slew limit: its buffer is dropped, and its halves take their
  // turn after it.
  size_t g = 0;
  const auto held = [&](const std::vector<int>& group, const bool part) {
    std::vector<int> cells;
    cells.reserve(group.size());
    for (const int node : group) {
      cells.push_back(nodes_[node].cell);
    }
    const int buffer = make(group, part);
    if (group.size() == 1 || nodes_[buffer].slew_ok) {
      return buffer;
    }
    nodes_.pop_back();
    for (int i = 0; i < group.size(); i++) {
      setCell(group[i], cells[i]);
    }
    std::vector<std::vector<int>> halves;
    bisect(group, 2, equal, halves);
    near.insert(near.begin() + g + 1, halves.begin(), halves.end());
    return -1;
  };
  for (; g < near.size(); g++) {
    // `held` adds to the list, so the group is taken out of it.
    const std::vector<int> group = std::move(near[g]);
    const int whole = held(group, false);
    if (whole < 0) {
      continue;
    }
    const std::vector<double> lates = loadLates(whole);
    const auto [least, most] = std::ranges::minmax_element(lates);
    if (*most - *least <= width) {
      buffers.push_back(whole);
      continue;
    }
    nodes_.pop_back();
    std::vector<int> order(group.size());
    for (int i = 0; i < order.size(); i++) {
      order[i] = i;
    }
    std::ranges::stable_sort(
        order, [&](const int a, const int b) { return lates[a] < lates[b]; });
    for (size_t begin = 0; begin < order.size();) {
      std::vector<int> part;
      size_t end = begin;
      while (end < order.size()
             && lates[order[end]] - lates[order[begin]] <= width) {
        part.push_back(group[order[end++]]);
      }
      const int buffer = held(part, true);
      if (buffer >= 0) {
        buffers.push_back(buffer);
      }
      begin = end;
    }
  }
  for (const int buffer : buffers) {
    setOptions(buffer);
  }
  return buffers;
}

void DelayBalancedTreeBuilder::setOptions(const int buffer)
{
  Node& node = nodes_[buffer];
  for (int cell = 0; cell < model_->numBuffers(); cell++) {
    if (node.wire.cap[ClockDelayModel::kSlow] <= model_->maxLoad(cell)
        && model_->slewOk(cell, node.wire)) {
      node.options.push_back(cell);
    }
  }
  if (node.options.empty()) {
    node.options.push_back(trunk_);
  }
}

std::vector<double> DelayBalancedTreeBuilder::loadLates(const int buffer) const
{
  const Node& n = nodes_[buffer];
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, n.cell, slew_, n.wire);
  std::vector<double> lates;
  lates.reserve(n.children.size());
  for (size_t i = 0; i < n.children.size(); i++) {
    lates.push_back(stage.wire_delay[i]
                    + lateAt(n.children[i], stage.load_slew[i]));
  }
  return lates;
}

void DelayBalancedTreeBuilder::even(const int buffer)
{
  const std::vector<int> children = nodes_[buffer].children;
  if (children.size() < 2) {
    return;
  }
  for (int round = 0; round < 2; round++) {
    const StageTiming stage = model_->drive(ClockDelayModel::kBalance,
                                            nodes_[buffer].cell,
                                            slew_,
                                            nodes_[buffer].wire);
    std::vector<std::pair<double, double>> lates;
    double total = 0.0;
    for (int i = 0; i < children.size(); i++) {
      lates.emplace_back(
          stage.wire_delay[i] + lateAt(children[i], stage.load_slew[i]),
          nodes_[children[i]].registers);
      total += nodes_[children[i]].registers;
    }
    std::ranges::sort(lates);
    double target = lates.back().first;
    double below = 0.0;
    for (const auto& [late, registers] : lates) {
      below += registers;
      if (below >= total / 2.0) {
        target = late;
        break;
      }
    }
    bool changed = false;
    for (int i = 0; i < children.size(); i++) {
      const int child = children[i];
      const int first = nodes_[child].cell;
      int best = first;
      double least = std::numeric_limits<double>::max();
      for (const int cell : optionsOf(child)) {
        setCell(child, cell);
        const double miss = std::abs(
            stage.wire_delay[i] + lateAt(child, stage.load_slew[i]) - target);
        if (miss < least) {
          least = miss;
          best = cell;
        }
      }
      setCell(child, best);
      changed = changed || best != first;
    }
    if (!changed) {
      break;
    }
    settle(buffer);
  }
}

double DelayBalancedTreeBuilder::balance(const std::vector<int>& buffers,
                                         const std::vector<int>& waiting,
                                         const size_t next,
                                         const double reference)
{
  struct Choice
  {
    double late;
    int cell;
  };
  std::vector<std::vector<Choice>> choices(buffers.size());
  std::vector<double> samples;
  // A buffer over one node is there to delay it and takes whatever cell
  // brings it to the lateness of the others.
  std::vector<double> counts(buffers.size());
  for (int i = 0; i < buffers.size(); i++) {
    counts[i] = nodes_[buffers[i]].children.size() > 1
                    ? nodes_[buffers[i]].registers
                    : 0.0;
    for (const int cell : nodes_[buffers[i]].options) {
      setCell(buffers[i], cell);
      choices[i].push_back({.late = late(buffers[i]), .cell = cell});
      if (counts[i] > 0.0) {
        samples.push_back(choices[i].back().late);
      }
    }
  }
  if (samples.empty()) {
    // Every buffer stands over one node: they aim at one another.
    for (int i = 0; i < buffers.size(); i++) {
      counts[i] = nodes_[buffers[i]].registers;
      for (const Choice& choice : choices[i]) {
        samples.push_back(choice.late);
      }
    }
  }
  std::ranges::sort(samples);
  auto cost = [&](const double target) {
    double sum = 0.0;
    for (int i = 0; i < buffers.size(); i++) {
      double miss = std::numeric_limits<double>::max();
      for (const Choice& choice : choices[i]) {
        miss = std::min(miss, std::abs(choice.late - target));
      }
      sum += counts[i] * miss;
    }
    // A sink too late for this level costs a level of its own later.
    const double most = kLateShare * (samples.back() - reference);
    for (size_t i = next; i < waiting.size(); i++) {
      const double sink = late(waiting[i]);
      if (sink > samples.back() + most) {
        break;
      }
      sum += nodes_[waiting[i]].registers
             * std::min(std::abs(sink - target), most);
    }
    return sum;
  };
  // Of the latenesses that cost about the least, the last: it gives the
  // buffers their weakest cells, whose inputs load the level above least.
  std::vector<std::pair<double, double>> costs;
  double least = std::numeric_limits<double>::max();
  const size_t stride = std::max<size_t>(1, samples.size() / 256);
  for (size_t i = 0; i < samples.size(); i += stride) {
    costs.emplace_back(samples[i], cost(samples[i]));
    least = std::min(least, costs.back().second);
  }
  double total = 0.0;
  for (const double count : counts) {
    total += count;
  }
  double target = samples.front();
  for (const auto& [late, sum] : costs) {
    if (sum <= least * (1.0 + kWeakSlack) + total * tolerance_) {
      target = late;
    }
  }
  double sum = 0.0;
  double weight = 0.0;
  for (int i = 0; i < buffers.size(); i++) {
    const Choice* best = &choices[i].front();
    for (const Choice& choice : choices[i]) {
      if (std::abs(choice.late - target) < std::abs(best->late - target)) {
        best = &choice;
      }
    }
    setCell(buffers[i], best->cell);
    sum += counts[i] * best->late;
    weight += counts[i];
  }
  return sum / weight;
}

std::vector<int> DelayBalancedTreeBuilder::padSingles(std::vector<int>& buffers,
                                                      const double target,
                                                      const double width)
{
  std::vector<int> kept;
  std::vector<int> early;
  const odb::Point center = core_.center();
  for (const int buffer : buffers) {
    if (nodes_[buffer].children.size() > 1) {
      kept.push_back(buffer);
      continue;
    }
    const int child = nodes_[buffer].children.front();
    const Path path
        = pathBetween(legal({nodes_[child].x, nodes_[child].y}), center);
    auto at_distance = [&](const double distance) {
      const odb::Point at = legal(path.at(distance));
      nodes_[buffer].x = at.x();
      nodes_[buffer].y = at.y();
      settle(buffer);
      return late(buffer);
    };
    double low = std::min(gap_, path.length());
    double high = std::min(hop_ / 2.0, path.length());
    int best = -1;
    double best_late = 0.0;
    for (const int cell : nodes_[buffer].options) {
      nodes_[buffer].cell = cell;
      const double least = at_distance(low);
      if (least <= target + width / 2.0 && (best < 0 || least > best_late)
          && (nodes_[buffer].slew_ok || cell == trunk_)) {
        best = cell;
        best_late = least;
      }
    }
    if (best < 0) {
      early.push_back(child);
      nodes_[buffer].children.clear();
      continue;
    }
    nodes_[buffer].cell = best;
    if (at_distance(high) > target || !nodes_[buffer].slew_ok) {
      for (int i = 0; i < 20; i++) {
        const double middle = (low + high) / 2.0;
        if (at_distance(middle) > target || !nodes_[buffer].slew_ok) {
          high = middle;
        } else {
          low = middle;
        }
      }
      at_distance(low);
    }
    kept.push_back(buffer);
  }
  buffers = std::move(kept);
  return early;
}

int DelayBalancedTreeBuilder::joinAll(const std::vector<int>& items)
{
  // Delay of a trunk wire per DBU, to weigh a distance against a lateness.
  const NetWire hop = model_->starWire(
      trunkOutput({0, 0}), {trunkInput({static_cast<int>(hop_), 0})}, false);
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, trunk_, slew_, hop);
  const double pace = (stage.delay + stage.wire_delay[0]) / hop_;

  std::vector<int> active = items;
  std::vector<double> lates;
  lates.reserve(active.size());
  for (const int item : active) {
    lates.push_back(late(item));
  }
  // The lateness a joint of two items will have, about.
  auto cost = [&](const int i, const int j) {
    const Node& p = nodes_[active[i]];
    const Node& q = nodes_[active[j]];
    const double wire = pace * (std::abs(p.x - q.x) + std::abs(p.y - q.y));
    const double apart = std::abs(lates[i] - lates[j]);
    return stage.delay + std::max(lates[i], lates[j])
           + std::max(wire, apart) / 2.0;
  };
  std::vector<bool> alive(active.size(), true);
  std::vector<int> partner(active.size(), -1);
  std::vector<double> least(active.size(), 0.0);
  auto find = [&](const int i) {
    partner[i] = -1;
    for (int j = 0; j < active.size(); j++) {
      if (j == i || !alive[j]) {
        continue;
      }
      const double c = cost(i, j);
      if (partner[i] < 0 || c < least[i]) {
        partner[i] = j;
        least[i] = c;
      }
    }
  };
  for (int i = 0; i < active.size(); i++) {
    find(i);
  }
  const double front = kFront * stage.delay;
  for (size_t left = active.size(); left > 1; left--) {
    // An item much earlier than the others only gets further from them as
    // they are joined, so it is joined first.
    std::vector<double> now;
    int a = -1;
    for (int i = 0; i < active.size(); i++) {
      if (alive[i]) {
        now.push_back(lates[i]);
        if (a < 0 || lates[i] < lates[a]) {
          a = i;
        }
      }
    }
    const auto quartile = now.begin() + now.size() / 4;
    std::ranges::nth_element(now, quartile);
    if (lates[a] >= *quartile - front) {
      for (int i = 0; i < active.size(); i++) {
        if (alive[i] && least[i] < least[a]) {
          a = i;
        }
      }
    }
    const int b = partner[a];
    const int joint = join(active[std::min(a, b)], active[std::max(a, b)]);
    alive[a] = false;
    alive[b] = false;
    const int made = active.size();
    active.push_back(joint);
    lates.push_back(late(joint));
    alive.push_back(true);
    partner.push_back(-1);
    least.push_back(0.0);
    find(made);
    for (int i = 0; i < made; i++) {
      if (!alive[i]) {
        continue;
      }
      if (partner[i] == a || partner[i] == b) {
        find(i);
      } else if (cost(i, made) < least[i]) {
        partner[i] = made;
        least[i] = cost(i, made);
      }
    }
  }
  for (int i = 0; i < active.size(); i++) {
    if (alive[i]) {
      return active[i];
    }
  }
  return active.front();
}

void DelayBalancedTreeBuilder::room(const double total,
                                    const int repeaters_a,
                                    const int repeaters_b,
                                    double& low,
                                    double& high) const
{
  const double hops_a = repeaters_a + kFork;
  const double hops_b = repeaters_b + kFork;
  const double gap = std::min(gap_, total / (hops_a + hops_b));
  low = std::max(gap * hops_a, total - hop_ * hops_b);
  high = std::min(hop_ * hops_a, total - gap * hops_b);
}

std::vector<odb::Point> DelayBalancedTreeBuilder::armPoints(
    const Path& path,
    const double place,
    const double end,
    const int repeaters,
    const double detour) const
{
  std::vector<odb::Point> points;
  for (int k = 0; k < repeaters; k++) {
    const double at = place + (end - place) * (kFork + k) / (repeaters + kFork);
    odb::Point point = path.at(at);
    if (k == 0 && detour > 0.0) {
      const odb::Point before = path.at(at - gap_);
      const odb::Point after = path.at(at + gap_);
      const bool along_x = std::abs(after.x() - before.x())
                           >= std::abs(after.y() - before.y());
      const odb::Point middle = core_.center();
      if (along_x) {
        point.setY(point.y() + (point.y() < middle.y() ? detour : -detour));
      } else {
        point.setX(point.x() + (point.x() < middle.x() ? detour : -detour));
      }
    }
    points.push_back(legal(point));
  }
  return points;
}

double DelayBalancedTreeBuilder::chainLate(
    const std::vector<odb::Point>& points,
    const int end,
    double slew,
    bool* slew_ok) const
{
  const double delay = chainDelay(points, end, slew, slew_ok);
  return delay + lateAt(end, slew);
}

double DelayBalancedTreeBuilder::chainDelay(
    const std::vector<odb::Point>& points,
    const int end,
    double& slew,
    bool* slew_ok,
    const int cell,
    const int weak) const
{
  const auto cell_at
      = [&](const int k) { return k >= 1 && k <= weak ? cell : trunk_; };
  const auto pin_of
      = [&](const int k, const odb::Point& pin, const bool input) {
          return NetPin{
              .x = points[k].x() + pin.x(),
              .y = points[k].y() + pin.y(),
              .cap = input ? model_->inputCap(cell_at(k)) : 0.0,
              .layer = input ? model_->inputLayer(cell_at(k))
                             : model_->outputLayer(cell_at(k)),
          };
        };
  double late = 0.0;
  for (int k = 0; k < points.size(); k++) {
    const bool last = k + 1 == points.size();
    const std::vector<NetPin> load{
        last ? inputPin(end)
             : pin_of(k + 1, model_->inputPin(cell_at(k + 1)), true),
    };
    const NetWire wire
        = model_->starWire(pin_of(k, model_->outputPin(cell_at(k)), false),
                           load,
                           last && nodes_[end].register_clock);
    const StageTiming stage
        = model_->drive(ClockDelayModel::kBalance, cell_at(k), slew, wire);
    late += stage.delay + stage.wire_delay[0];
    slew = stage.load_slew[0];
    if (slew_ok != nullptr && *slew_ok) {
      *slew_ok = model_->slewOk(cell_at(k), wire);
    }
  }
  return late;
}

double DelayBalancedTreeBuilder::lateBelow(const int node,
                                           const double slew) const
{
  const Node& n = nodes_[node];
  if (n.sink) {
    return lateAt(node, slew);
  }
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, n.cell, slew, n.wire);
  double sum = 0.0;
  double weight = 0.0;
  for (int i = 0; i < n.children.size(); i++) {
    const Node& child = nodes_[n.children[i]];
    sum += child.registers
           * (stage.wire_delay[i]
              + lateBelow(n.children[i], stage.load_slew[i]));
    weight += child.registers;
  }
  return stage.delay + sum / weight;
}

double DelayBalancedTreeBuilder::lag(const int a,
                                     const int b,
                                     const Path& path,
                                     const double place,
                                     const int repeaters_a,
                                     const int repeaters_b,
                                     bool* slew_ok,
                                     const double detour_a,
                                     const double detour_b) const
{
  const odb::Point joint = legal(path.at(place));
  const std::array<std::vector<odb::Point>, 2> arms{
      armPoints(path, place, 0.0, repeaters_a, detour_a),
      armPoints(path, place, path.length(), repeaters_b, detour_b),
  };
  const std::array<int, 2> ends{a, b};
  std::vector<NetPin> loads;
  bool leaf = false;
  for (int side = 0; side < 2; side++) {
    if (arms[side].empty()) {
      loads.push_back(inputPin(ends[side]));
      leaf = leaf || nodes_[ends[side]].register_clock;
    } else {
      loads.push_back(trunkInput(arms[side].front()));
    }
  }
  const NetWire wire = model_->wire(trunkOutput(joint), loads, leaf);
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, trunk_, slew_, wire);
  std::array<double, 2> late{};
  if (slew_ok != nullptr) {
    *slew_ok = model_->slewOk(trunk_, wire);
  }
  for (int side = 0; side < 2; side++) {
    late[side]
        = stage.wire_delay[side]
          + chainLate(arms[side], ends[side], stage.load_slew[side], slew_ok);
  }
  return late[1] - late[0];
}

double DelayBalancedTreeBuilder::placeJoint(const int a,
                                            const int b,
                                            const Path& path,
                                            const int repeaters_a,
                                            const int repeaters_b,
                                            double low,
                                            double high,
                                            double& miss) const
{
  miss = lag(a, b, path, low, repeaters_a, repeaters_b);
  if (miss < 0.0) {
    return low;
  }
  miss = lag(a, b, path, high, repeaters_a, repeaters_b);
  if (miss > 0.0) {
    return high;
  }
  double place = low;
  for (int i = 0; i < 30; i++) {
    place = (low + high) / 2.0;
    if (lag(a, b, path, place, repeaters_a, repeaters_b) > 0.0) {
      low = place;
    } else {
      high = place;
    }
  }
  miss = lag(a, b, path, place, repeaters_a, repeaters_b);
  return place;
}

DelayBalancedTreeBuilder::JointFit DelayBalancedTreeBuilder::fitRepeaters(
    const int a,
    const int b,
    const Path& path,
    JointFit& last) const
{
  const double total = path.length();
  JointFit best;
  int repeaters_a = 0;
  int repeaters_b = 0;
  while (repeaters_a + repeaters_b < kMostRepeaters) {
    double low, high;
    room(total, repeaters_a, repeaters_b, low, high);
    double place, miss;
    if (low > high) {
      place = total * (repeaters_a + kFork)
              / (repeaters_a + repeaters_b + 2.0 * kFork);
      miss = lag(a, b, path, place, repeaters_a, repeaters_b);
      last = {
          .miss = std::abs(miss),
          .place = place,
          .repeaters_a = repeaters_a,
          .repeaters_b = repeaters_b,
          .valid = true,
      };
    } else {
      place = placeJoint(a, b, path, repeaters_a, repeaters_b, low, high, miss);
      last = {
          .miss = std::abs(miss),
          .place = place,
          .repeaters_a = repeaters_a,
          .repeaters_b = repeaters_b,
          .valid = true,
      };
      // A place that leaves a slew over the limit is no place: the side
      // with the longer wires takes another repeater.
      bool slew_ok = true;
      if (!best.valid || last.miss < best.miss) {
        lag(a, b, path, place, repeaters_a, repeaters_b, &slew_ok);
      }
      if (!slew_ok) {
        const double wire_a = place / (repeaters_a + kFork);
        const double wire_b = (total - place) / (repeaters_b + kFork);
        (wire_a < wire_b ? repeaters_b : repeaters_a)++;
        continue;
      }
      // One more repeater that brings the sides no nearer ends the search:
      // what is left is less than a repeater adds.
      if (best.valid && last.miss >= best.miss) {
        break;
      }
      best = last;
      if (last.miss <= tolerance_) {
        break;
      }
    }
    (miss < 0.0 ? repeaters_b : repeaters_a)++;
  }
  return best;
}

DelayBalancedTreeBuilder::JointFit DelayBalancedTreeBuilder::fitDetour(
    const int a,
    const int b,
    const Path& path,
    const JointFit& placed) const
{
  JointFit best = placed;
  for (const auto& [more_a, more_b] :
       {std::pair(0, 0), std::pair(1, 0), std::pair(0, 1), std::pair(1, 1)}) {
    if (more_a + more_b > 0 && placed.miss <= pad_) {
      break;
    }
    const int repeaters_a = placed.repeaters_a + more_a;
    const int repeaters_b = placed.repeaters_b + more_b;
    double low, high;
    room(path.length(), repeaters_a, repeaters_b, low, high);
    if (low > high) {
      continue;
    }
    double miss;
    const double place
        = placeJoint(a, b, path, repeaters_a, repeaters_b, low, high, miss);
    // A late `b` wants the delay on the side of `a`.
    const bool on_a = miss > 0.0;
    if ((on_a ? repeaters_a : repeaters_b) == 0) {
      continue;
    }
    const auto off = [&](const double detour, bool* slew_ok) {
      return lag(a,
                 b,
                 path,
                 place,
                 repeaters_a,
                 repeaters_b,
                 slew_ok,
                 on_a ? detour : 0.0,
                 on_a ? 0.0 : detour);
    };
    double near = 0.0;
    double far = hop_ / 2.0;
    for (int i = 0; i < 20; i++) {
      const double middle = (near + far) / 2.0;
      ((off(middle, nullptr) > 0.0) == on_a ? near : far) = middle;
    }
    bool slew_ok = true;
    const double left = off(far, &slew_ok);
    if (slew_ok && std::abs(left) < best.miss) {
      best = {
          .miss = std::abs(left),
          .place = place,
          .repeaters_a = repeaters_a,
          .repeaters_b = repeaters_b,
          .valid = true,
          .detour_a = on_a ? far : 0.0,
          .detour_b = on_a ? 0.0 : far,
      };
      if (best.miss <= tolerance_) {
        break;
      }
    }
  }
  return best;
}

DelayBalancedTreeBuilder::JointFit
DelayBalancedTreeBuilder::fit(const int a, const int b, const Path& path) const
{
  JointFit last;
  const JointFit best = fitRepeaters(a, b, path, last);
  if (!best.valid) {
    return last;
  }
  if (best.miss <= tolerance_) {
    return best;
  }
  return fitDetour(a, b, path, best);
}

int DelayBalancedTreeBuilder::chain(const std::vector<odb::Point>& points,
                                    const int end,
                                    const int level,
                                    const int cell,
                                    const int weak)
{
  std::vector<int> made;
  made.reserve(points.size());
  for (int k = 0; k < points.size(); k++) {
    made.push_back(
        addBuffer(points[k], k >= 1 && k <= weak ? cell : trunk_, level));
  }
  for (int k = 0; k < made.size(); k++) {
    nodes_[made[k]].children.push_back(k + 1 < made.size() ? made[k + 1] : end);
  }
  for (int k = made.size() - 1; k >= 0; k--) {
    settle(made[k]);
  }
  return made.empty() ? end : made.front();
}

int DelayBalancedTreeBuilder::join(const int a, const int b)
{
  const Path path
      = pathBetween({nodes_[a].x, nodes_[a].y}, {nodes_[b].x, nodes_[b].y});
  JointFit best = fit(a, b, path);
  const int first_a = nodes_[a].cell;
  const int first_b = nodes_[b].cell;
  int kept_a = first_a;
  int kept_b = first_b;
  // The strongest cells first: a weaker one is taken for what it brings the
  // sides nearer, not for a hair.
  const std::vector<int> options_a = optionsOf(a);
  const std::vector<int> options_b = optionsOf(b);
  for (const int cell_a : std::views::reverse(options_a)) {
    for (const int cell_b : std::views::reverse(options_b)) {
      if (best.miss <= tolerance_ || (cell_a == first_a && cell_b == first_b)) {
        continue;
      }
      setCell(a, cell_a);
      setCell(b, cell_b);
      const JointFit other = fit(a, b, path);
      if (other.miss + tolerance_ < best.miss) {
        best = other;
        kept_a = cell_a;
        kept_b = cell_b;
      }
    }
  }
  setCell(a, kept_a);
  setCell(b, kept_b);
  debugPrint(
      logger_,
      CTS,
      "delay balance",
      2,
      "tree {} joint of {} at ({:.1f} {:.1f}) and {} at ({:.1f} {:.1f}): "
      "{:.2f} and {:.2f} ps late, {:.1f} "
      "um apart, {} and {} repeaters, off the line by {:.1f} and "
      "{:.1f} um, left {:.2f} ps apart",
      clock_.getName(),
      a,
      nodes_[a].x / model_->dbuPerMicron(),
      nodes_[a].y / model_->dbuPerMicron(),
      b,
      nodes_[b].x / model_->dbuPerMicron(),
      nodes_[b].y / model_->dbuPerMicron(),
      late(a) * 1e12,
      late(b) * 1e12,
      path.length() / model_->dbuPerMicron(),
      best.repeaters_a,
      best.repeaters_b,
      best.detour_a / model_->dbuPerMicron(),
      best.detour_b / model_->dbuPerMicron(),
      best.miss * 1e12);
  joints_++;
  repeaters_ += best.repeaters_a + best.repeaters_b;
  worst_miss_ = std::max(worst_miss_, best.miss);
  sum_miss_ += best.miss;

  const int joint = addBuffer(legal(path.at(best.place)), trunk_, kJointLevel);
  const int first_of_a
      = chain(armPoints(path, best.place, 0.0, best.repeaters_a, best.detour_a),
              a,
              kRepeaterLevel);
  const int first_of_b = chain(
      armPoints(
          path, best.place, path.length(), best.repeaters_b, best.detour_b),
      b,
      kRepeaterLevel);
  nodes_[joint].children = {first_of_a, first_of_b};
  settle(joint);
  setOptions(joint);
  return joint;
}

std::vector<odb::Point> DelayBalancedTreeBuilder::feedPoints(
    const Path& path,
    const int hops,
    const int pads,
    const double pitch,
    const bool beside) const
{
  Path pad_path = path;
  if (path.length() < hop_) {
    const odb::Point& from = path.points.front();
    const int reach = from.x() + hop_ <= core_.xMax() ? hop_ : -hop_;
    pad_path.points = {from, {from.x() + reach, from.y()}};
  }
  std::vector<odb::Point> points;
  if (beside) {
    points.push_back(legal(pad_path.at(gap_)));
  }
  for (int k = 0; k < pads; k++) {
    points.push_back(legal(pad_path.at(k % 2 == 0 ? pitch : gap_)));
  }
  for (int k = 1; k < hops; k++) {
    points.push_back(legal(path.at(path.length() * k / hops)));
  }
  return points;
}

int DelayBalancedTreeBuilder::feedHops(const FeedPlan& plan,
                                       const Path& way) const
{
  return std::max<int>(1, std::ceil(way.length() / hop_)) + plan.spare;
}

void DelayBalancedTreeBuilder::driverOver(const FeedPlan& plan,
                                          const std::vector<odb::Point>& points,
                                          double& before,
                                          double& first_slew,
                                          bool& slew_ok) const
{
  std::vector<NetPin> loads{
      points.empty() ? inputPin(plan.top) : trunkInput(points.front()),
  };
  if (isolates_ports_) {
    loads.push_back(trunkInput(port_chain_.front()));
  } else {
    loads.insert(loads.end(), port_loads_.begin(), port_loads_.end());
  }
  const NetWire wire = model_->wire(
      driver_pin_, loads, points.empty() && nodes_[plan.top].register_clock);
  const StageTiming stage = driverStage(wire);
  before = stage.delay + stage.wire_delay[0];
  first_slew = stage.load_slew[0];
  slew_ok = driver_cell_ >= 0 && model_->slewOk(driver_cell_, wire);
}

double DelayBalancedTreeBuilder::lateBelowAt(const FeedPlan& plan,
                                             const double slew) const
{
  size_t k = 1;
  while (k + 1 < plan.below.size() && plan.below[k].first < slew) {
    k++;
  }
  const auto& [s0, l0] = plan.below[k - 1];
  const auto& [s1, l1] = plan.below[k];
  return l0 + (l1 - l0) * (slew - s0) / (s1 - s0);
}

double DelayBalancedTreeBuilder::feedLatency(
    const FeedPlan& plan,
    const std::vector<odb::Point>& points,
    const int cell,
    const int weak,
    bool* chain_ok) const
{
  double before, slew;
  bool slew_ok;
  driverOver(plan, points, before, slew, slew_ok);
  const double delay = chainDelay(points, plan.top, slew, chain_ok, cell, weak);
  return before + delay + lateBelowAt(plan, slew);
}

double DelayBalancedTreeBuilder::feedOver(const FeedPlan& plan,
                                          const Path& way,
                                          const int pads,
                                          const double pitch) const
{
  return feedLatency(
      plan,
      feedPoints(way, feedHops(plan, way), pads, pitch, plan.beside),
      trunk_,
      0,
      nullptr);
}

int DelayBalancedTreeBuilder::fitPads(const FeedPlan& plan, FeedFit& best) const
{
  int under = plan.beside ? -1 : 0;
  bool first = true;
  for (int pads = 0; plan.beside && pads <= kMostRepeaters; pads++) {
    double pitch = gap_;
    double low = gap_;
    double high = hop_;
    if (feedOver(plan, plan.path, pads, low) < latency_) {
      pitch = high;
      if (pads > 0 && latency_ < feedOver(plan, plan.path, pads, high)) {
        for (int i = 0; i < 30; i++) {
          pitch = (low + high) / 2.0;
          (feedOver(plan, plan.path, pads, pitch) < latency_ ? low : high)
              = pitch;
        }
      } else {
        under = pads;
      }
    }
    const double miss
        = std::abs(feedOver(plan, plan.path, pads, pitch) - latency_);
    if (first || miss < best.miss) {
      best = {.miss = miss, .pads = pads, .pitch = pitch};
      first = false;
    }
    if (miss <= tolerance_
        || feedOver(plan, plan.path, pads, gap_) >= latency_) {
      break;
    }
  }
  if (!plan.beside) {
    best = {
        .miss = std::abs(feedOver(plan, plan.path, 0, gap_) - latency_),
        .pads = 0,
        .pitch = gap_,
    };
  }
  return under;
}

void DelayBalancedTreeBuilder::fitLongerWay(FeedPlan& plan,
                                            const int under,
                                            FeedFit& best) const
{
  const double pitch = under > 0 ? hop_ : gap_;
  const odb::Point from = plan.path.points.front();
  const auto longer = [&](const double extra) {
    const int out
        = from.x() + extra / 2.0 <= core_.xMax() ? extra / 2.0 : -extra / 2.0;
    Path way{{from, {from.x() + out, from.y()}}};
    way.points.insert(
        way.points.end(), plan.path.points.begin(), plan.path.points.end());
    return way;
  };
  double low = 0.0;
  double high = 2.0 * hop_;
  for (int i = 0; i < 20; i++) {
    const double middle = (low + high) / 2.0;
    (feedOver(plan, longer(middle), under, pitch) < latency_ ? low : high)
        = middle;
  }
  const Path way = longer(high);
  const double miss = std::abs(feedOver(plan, way, under, pitch) - latency_);
  if (miss < best.miss) {
    best = {.miss = miss, .pads = under, .pitch = pitch};
    plan.path = way;
  }
}

void DelayBalancedTreeBuilder::fitWeakPads(const FeedPlan& plan,
                                           const int under,
                                           FeedFit& best) const
{
  for (int cell = trunk_ - 1; cell >= 0; cell--) {
    for (const int pads : {under, under + 1}) {
      for (int weak = 1; weak <= pads && best.miss > tolerance_; weak++) {
        const auto with = [&](const double pitch, bool* chain_ok) {
          return feedLatency(plan,
                             feedPoints(plan.path,
                                        feedHops(plan, plan.path),
                                        pads,
                                        pitch,
                                        plan.beside),
                             cell,
                             weak,
                             chain_ok);
        };
        double low = gap_;
        double high = hop_;
        double pitch = high;
        if (with(low, nullptr) > latency_) {
          pitch = low;
        } else if (with(high, nullptr) > latency_) {
          for (int i = 0; i < 30; i++) {
            pitch = (low + high) / 2.0;
            (with(pitch, nullptr) < latency_ ? low : high) = pitch;
          }
        }
        bool chain_ok = true;
        const double miss = std::abs(with(pitch, &chain_ok) - latency_);
        if (chain_ok && miss < best.miss) {
          best = {
              .miss = miss,
              .pads = pads,
              .pitch = pitch,
              .cell = cell,
              .weak = weak,
          };
        }
      }
    }
  }
}

DelayBalancedTreeBuilder::FeedFit DelayBalancedTreeBuilder::fitFeed(
    FeedPlan& plan) const
{
  // How late the registers below the top are for a few slews at its input:
  // the delays above are quoted at one slew, and the feed wants what the
  // slews of the tree as built give.
  const double limit = model_->slewLimit();
  double slew = slew_ > 0.0 ? slew_ / 4.0 : limit / 64.0;
  while (slew < 2.0 * limit || plan.below.size() < 2) {
    plan.below.emplace_back(slew, lateBelow(plan.top, slew));
    slew *= 2.0;
  }
  if (!plan.beside && feedOver(plan, plan.path, 0, gap_) < latency_) {
    // The delay is wanted: more of it comes after a buffer beside the
    // driver, less than that buffer adds from a longer way without it.
    plan.beside = true;
    if (feedOver(plan, plan.path, 0, gap_) > latency_) {
      plan.beside = false;
    }
  }
  // The most delaying buffers that leave the registers early however far
  // apart they stand.
  FeedFit best{.miss = 0.0, .pads = 0, .pitch = gap_};
  const int under = fitPads(plan, best);
  // What the delaying buffers leave is made up by a longer way: out along
  // the row and back before it sets off.
  if (best.miss > tolerance_ && under >= 0) {
    fitLongerWay(plan, under, best);
  }
  // And what is still left, by weaker cells for some of the delaying
  // buffers, of which there may then be one more.
  if (plan.beside) {
    fitWeakPads(plan, under, best);
  }
  return best;
}

int DelayBalancedTreeBuilder::feed(const int top)
{
  FeedPlan plan;
  plan.top = top;
  plan.path = pathBetween(legal(driver_), {nodes_[top].x, nodes_[top].y});
  for (; plan.spare < kMostRepeaters; plan.spare++) {
    bool held = true;
    double slew = slew_;
    chainDelay(feedPoints(plan.path, feedHops(plan, plan.path), 0, gap_, true),
               top,
               slew,
               &held);
    if (held) {
      break;
    }
  }
  if (plan.spare == kMostRepeaters) {
    plan.spare = 0;
  }
  double before, first_slew;
  bool bare_ok;
  driverOver(plan,
             feedPoints(plan.path, feedHops(plan, plan.path), 0, gap_, false),
             before,
             first_slew,
             bare_ok);
  plan.beside = !bare_ok;
  FeedFit best{.miss = 0.0, .pads = 0, .pitch = gap_};
  if (has_latency_) {
    best = fitFeed(plan);
  }
  const int hops = feedHops(plan, plan.path);
  debugPrint(logger_,
             CTS,
             "delay balance",
             1,
             "tree {} feed: {:.2f} ps below it, {} buffers over {:.1f} um on "
             "the way{}, {} to delay of which {} weaker, {:.2f} ps from what "
             "the registers aim at",
             clock_.getName(),
             late(top) * 1e12,
             hops - 1,
             plan.path.length() / model_->dbuPerMicron(),
             plan.beside ? " after one beside the driver" : "",
             best.pads,
             best.weak,
             has_latency_ ? best.miss * 1e12 : 0.0);
  return chain(feedPoints(plan.path, hops, best.pads, best.pitch, plan.beside),
               top,
               kFeedLevel,
               best.cell,
               best.weak);
}

int DelayBalancedTreeBuilder::reach(const int top)
{
  const Path path = pathBetween(driver_, {nodes_[top].x, nodes_[top].y});
  // As many hops as the length asks for, and more where the repeaters stand
  // off the way and their wires are longer for it.
  const int least = std::max<int>(1, std::ceil(path.length() / hop_));
  auto points_of = [&](const int hops) {
    std::vector<odb::Point> points;
    for (int k = 1; k < hops; k++) {
      points.push_back(legal(path.at(path.length() * k / hops)));
    }
    return points;
  };
  std::vector<odb::Point> points = points_of(least);
  for (int spare = 0; spare < kMostRepeaters; spare++) {
    const std::vector<odb::Point> tried = points_of(least + spare);
    bool held = true;
    double slew = slew_;
    chainDelay(tried, top, slew, &held);
    if (held) {
      points = tried;
      break;
    }
  }
  int first = chain(points, top, kRepeaterLevel);
  if (!drivable({first}, std::numeric_limits<double>::max(), 0)) {
    first = chain({legal(path.at(gap_))}, first, kRepeaterLevel);
  }
  return first;
}

bool DelayBalancedTreeBuilder::clear(const odb::Point& from,
                                     const odb::Point& to) const
{
  const int x0 = std::min(from.x(), to.x());
  const int x1 = std::max(from.x(), to.x());
  const int y0 = std::min(from.y(), to.y());
  const int y1 = std::max(from.y(), to.y());
  for (const odb::Rect& blockage : blockages_) {
    if (x1 > blockage.xMin() - half_width_ && x0 < blockage.xMax() + half_width_
        && y1 > blockage.yMin() - half_height_
        && y0 < blockage.yMax() + half_height_) {
      return false;
    }
  }
  return true;
}

bool DelayBalancedTreeBuilder::bend(const odb::Point& from,
                                    const odb::Point& to,
                                    odb::Point& corner) const
{
  for (const odb::Point& candidate :
       {odb::Point(to.x(), from.y()), odb::Point(from.x(), to.y())}) {
    if (clear(from, candidate) && clear(candidate, to)) {
      corner = candidate;
      return true;
    }
  }
  return false;
}

DelayBalancedTreeBuilder::Path DelayBalancedTreeBuilder::pathBetween(
    const odb::Point& from,
    const odb::Point& to) const
{
  odb::Point corner(to.x(), from.y());
  if (blockages_.empty() || bend(from, to, corner)) {
    return Path{{from, corner, to}};
  }
  // Shortest way over the corners of the blockages.
  std::vector<odb::Point> stops{from, to};
  for (const odb::Rect& blockage : blockages_) {
    for (const int x : {
             blockage.xMin() - half_width_ - site_width_,
             blockage.xMax() + half_width_ + site_width_,
         }) {
      for (const int y : {
               blockage.yMin() - half_height_ - row_height_,
               blockage.yMax() + half_height_ + row_height_,
           }) {
        const odb::Point stop(x, y);
        if (core_.intersects(stop) && !blocked(stop)) {
          stops.push_back(stop);
        }
      }
    }
  }
  const int count = stops.size();
  std::vector<double> reach(count, std::numeric_limits<double>::max());
  std::vector<int> before(count, -1);
  std::vector<odb::Point> corners(count);
  std::vector<bool> done(count, false);
  reach[0] = 0.0;
  for (int round = 0; round < count; round++) {
    int at = -1;
    for (int i = 0; i < count; i++) {
      if (!done[i] && (at < 0 || reach[i] < reach[at])) {
        at = i;
      }
    }
    if (at < 0 || reach[at] == std::numeric_limits<double>::max() || at == 1) {
      break;
    }
    done[at] = true;
    for (int i = 0; i < count; i++) {
      const double length
          = reach[at] + odb::Point::manhattanDistance(stops[at], stops[i]);
      odb::Point via;
      if (!done[i] && length < reach[i] && bend(stops[at], stops[i], via)) {
        reach[i] = length;
        before[i] = at;
        corners[i] = via;
      }
    }
  }
  if (before[1] < 0) {
    // No clear way, as when an end stands inside a blockage.
    return Path{{from, corner, to}};
  }
  Path path;
  for (int at = 1; at != 0; at = before[at]) {
    path.points.push_back(stops[at]);
    path.points.push_back(corners[at]);
  }
  path.points.push_back(from);
  std::ranges::reverse(path.points);
  return path;
}

bool DelayBalancedTreeBuilder::blocked(const odb::Point& at) const
{
  for (const odb::Rect& blockage : blockages_) {
    if (at.x() > blockage.xMin() - half_width_
        && at.x() < blockage.xMax() + half_width_
        && at.y() > blockage.yMin() - half_height_
        && at.y() < blockage.yMax() + half_height_) {
      return true;
    }
  }
  return false;
}

odb::Point DelayBalancedTreeBuilder::legal(odb::Point at) const
{
  auto on_row = [&](const odb::Point& point) {
    const int x = std::min(std::max(point.x(), core_.xMin() + half_width_),
                           core_.xMax() - half_width_);
    const int rows = std::max(1, core_.dy() / row_height_);
    const int row = std::clamp<int>(
        std::lround(static_cast<double>(point.y() - half_height_ - core_.yMin())
                    / row_height_),
        0,
        rows - 1);
    return odb::Point(x, core_.yMin() + row * row_height_ + half_height_);
  };
  at = on_row(at);
  if (!blocked(at)) {
    return at;
  }
  // Beside each blockage over the point, and over those points in turn where
  // blockages adjoin.
  const odb::Point from = at;
  std::vector<odb::Point> seen{from};
  std::vector<odb::Point> open{from};
  odb::Point best = from;
  int64_t least = std::numeric_limits<int64_t>::max();
  auto distance = [&](const odb::Point& point) {
    return odb::Point::manhattanDistance(point, from);
  };
  while (!open.empty()) {
    const auto nearest = std::ranges::min_element(open, {}, distance);
    at = *nearest;
    open.erase(nearest);
    if (distance(at) >= least) {
      break;
    }
    for (const odb::Rect& blockage : blockages_) {
      if (at.x() <= blockage.xMin() - half_width_
          || at.x() >= blockage.xMax() + half_width_
          || at.y() <= blockage.yMin() - half_height_
          || at.y() >= blockage.yMax() + half_height_) {
        continue;
      }
      for (const odb::Point& candidate : {
               odb::Point(blockage.xMin() - half_width_, at.y()),
               odb::Point(blockage.xMax() + half_width_, at.y()),
               odb::Point(at.x(), blockage.yMin() - half_height_ - row_height_),
               odb::Point(at.x(), blockage.yMax() + half_height_ + row_height_),
           }) {
        const odb::Point point = on_row(candidate);
        if (std::ranges::find(seen, point) != seen.end()) {
          continue;
        }
        seen.push_back(point);
        if (blocked(point)) {
          open.push_back(point);
        } else if (distance(point) < least) {
          least = distance(point);
          best = point;
        }
      }
    }
  }
  return best;
}

void DelayBalancedTreeBuilder::setDriverModel(const std::vector<int>& roots)
{
  const NetWire wire = driverWire(roots);
  const StageTiming stage = driverStage(wire);
  driver_model_.cell = driver_cell_;
  driver_model_.delay = stage.delay;
  driver_model_.load = stage.load;
  double sum = 0.0;
  double weight = 0.0;
  for (int i = 0; i < roots.size(); i++) {
    sum += nodes_[roots[i]].registers
           * (stage.wire_delay[i] + lateAt(roots[i], stage.load_slew[i]));
    weight += nodes_[roots[i]].registers;
  }
  driver_model_.after = sum / weight;
  driver_model_.registers = weight;
}

std::vector<std::string> DelayBalancedTreeBuilder::bufferNames(
    const std::vector<int>& order,
    const std::vector<bool>& leaves) const
{
  std::map<std::string, int> counts;
  std::vector<std::string> names;
  names.reserve(order.size());
  for (size_t i = 0; i < order.size(); i++) {
    const Node& node = nodes_[order[i]];
    std::string kind;
    if (leaves[i]) {
      kind = "leaf";
    } else if (node.level == kJointLevel) {
      kind = "joint";
    } else if (node.level == kRepeaterLevel) {
      kind = "rep";
    } else if (node.level == kFeedLevel) {
      kind = "feed";
    } else {
      kind = std::to_string(node.level);
    }
    names.push_back(kind + "_" + std::to_string(counts[kind]++));
  }
  return names;
}

std::pair<ClockInst*, ClockInst*> DelayBalancedTreeBuilder::emitChain(
    const std::vector<odb::Point>& points,
    const std::string& kind)
{
  ClockInst* first = nullptr;
  ClockInst* before = nullptr;
  odb::dbMaster* master = model_->master(trunk_);
  for (size_t k = 0; k < points.size(); k++) {
    ClockInst& inst
        = clock_.addClockBuffer("clkbuf_" + kind + "_" + std::to_string(k),
                                model_->cellName(trunk_),
                                points[k].x() - master->getWidth() / 2,
                                points[k].y() - master->getHeight() / 2);
    addTreeLevelBuffer(&inst);
    if (before != nullptr) {
      ClockSubNet& net
          = clock_.addSubNet("clknet_" + kind + "_" + std::to_string(k));
      net.addInst(*before);
      net.addInst(inst);
    } else {
      first = &inst;
    }
    before = &inst;
  }
  return {first, before};
}

void DelayBalancedTreeBuilder::emit(const std::vector<int>& roots)
{
  std::vector<int> order;
  std::vector<int> todo(roots.rbegin(), roots.rend());
  while (!todo.empty()) {
    const int node = todo.back();
    todo.pop_back();
    if (nodes_[node].sink) {
      continue;
    }
    order.push_back(node);
    const std::vector<int>& children = nodes_[node].children;
    todo.insert(todo.end(), children.rbegin(), children.rend());
  }
  std::vector<bool> leaves;
  leaves.reserve(order.size());
  for (const int node : order) {
    leaves.push_back(std::ranges::all_of(
        nodes_[node].children,
        [&](const int child) { return nodes_[child].sink; }));
  }
  const std::vector<std::string> names = bufferNames(order, leaves);
  for (size_t i = 0; i < order.size(); i++) {
    Node& node = nodes_[order[i]];
    odb::dbMaster* master = model_->master(node.cell);
    const int left = node.x - master->getWidth() / 2 - core_.xMin();
    const int site = std::lround(static_cast<double>(left) / site_width_);
    ClockInst& inst = clock_.addClockBuffer("clkbuf_" + names[i],
                                            model_->cellName(node.cell),
                                            core_.xMin() + site * site_width_,
                                            node.y - master->getHeight() / 2);
    inst.setInputCap(model_->inputCap(node.cell));
    if (leaves[i]) {
      addFirstLevelSinkDriver(&inst);
    } else {
      addTreeLevelBuffer(&inst);
    }
    node.inst = &inst;
  }
  for (size_t i = 0; i < order.size(); i++) {
    const Node& node = nodes_[order[i]];
    ClockSubNet& net = clock_.addSubNet("clknet_" + names[i]);
    net.addInst(*node.inst);
    for (const int child : node.children) {
      net.addInst(*nodes_[child].inst);
    }
    net.setLeafLevel(leaves[i]);
  }
  std::vector<ClockInst*> loads;
  loads.reserve(roots.size());
  for (const int root : roots) {
    loads.push_back(nodes_[root].inst);
  }
  clock_.setRootLoads(loads);
  const auto [port_first, port_last] = emitChain(port_chain_, "port");
  if (port_first != nullptr) {
    clock_.setPortBuffers(port_first, port_last);
  }
  const auto [source_first, source_last] = emitChain(source_chain_, "source");
  if (source_first != nullptr) {
    clock_.setSourceBuffers(source_first, source_last);
  }
}

void DelayBalancedTreeBuilder::planSource()
{
  int x, y;
  if (!driver_input_->getAvgXY(&x, &y)) {
    driver_input_->getInst()->getLocation(x, y);
  }
  const NetPin input{
      .x = x,
      .y = y,
      .cap = model_->pinCap(driver_input_),
      .layer = ClockDelayModel::pinLayer(driver_input_->getMTerm()),
  };
  const Path path = pathBetween(legal({source_.x, source_.y}), legal({x, y}));
  const int hops = std::max<int>(1, std::ceil(path.length() / hop_));
  for (int k = 1; k < hops; k++) {
    source_chain_.push_back(legal(path.at(path.length() * k / hops)));
  }
  double slew = source_slew_;
  source_delay_ = 0.0;
  for (int k = 0; k <= source_chain_.size(); k++) {
    const bool last = k == source_chain_.size();
    const std::vector<NetPin> load{last ? input : trunkInput(source_chain_[k])};
    StageTiming stage;
    if (k == 0) {
      stage = model_->port(ClockDelayModel::kBalance,
                           slew,
                           model_->starWire(source_, load, false));
    } else {
      stage = model_->drive(
          ClockDelayModel::kBalance,
          trunk_,
          slew,
          model_->starWire(trunkOutput(source_chain_[k - 1]), load, false));
    }
    source_delay_ += stage.delay + stage.wire_delay[0];
    slew = stage.load_slew[0];
  }
  driver_slew_ = slew;
  if (has_latency_) {
    latency_ -= source_delay_;
  }
}

void DelayBalancedTreeBuilder::planPorts()
{
  double x = 0.0;
  double y = 0.0;
  for (const NetPin& pin : port_loads_) {
    x += pin.x;
    y += pin.y;
  }
  const odb::Point ports = legal({static_cast<int>(x / port_loads_.size()),
                                  static_cast<int>(y / port_loads_.size())});
  const odb::Point beside = legal({driver_.x(), driver_.y() + 2 * row_height_});
  Path path = pathBetween(beside, ports);
  std::ranges::reverse(path.points);
  const double total = path.length();
  // Distance from the ports of the last buffer.
  auto last_ok = [&](const double distance) {
    return model_->slewOk(
        trunk_,
        model_->wire(
            trunkOutput(legal(path.at(distance))), port_loads_, false));
  };
  double last = std::min(hop_, total);
  if (!last_ok(last)) {
    double low = 0.0;
    for (int i = 0; i < 12; i++) {
      const double middle = (low + last) / 2.0;
      (last_ok(middle) ? low : last) = middle;
    }
    last = low;
  }
  std::vector<odb::Point> chain;
  if (total > last) {
    const int hops = std::ceil((total - last) / hop_);
    for (int k = hops; k >= 0; k--) {
      chain.push_back(legal(path.at(last + (total - last) * k / hops)));
    }
  } else {
    chain.push_back(beside);
  }
  // The driver takes the wire to the buffer after the one beside it, or to
  // the ports, where its slew allows.
  std::vector<NetPin> loads{trunkInput(beside)};
  if (chain.size() > 1) {
    loads.push_back(trunkInput(chain[1]));
  } else {
    loads.insert(loads.end(), port_loads_.begin(), port_loads_.end());
  }
  if (model_->slewOk(driver_cell_, model_->wire(driver_pin_, loads, false))) {
    chain.erase(chain.begin());
  }
  port_chain_ = std::move(chain);
  isolates_ports_ = !port_chain_.empty();
}

void DelayBalancedTreeBuilder::propagate(const int node,
                                         const double arrival,
                                         const double slew)
{
  Node& n = nodes_[node];
  n.arrival = arrival;
  n.in_slew = slew;
  if (n.sink) {
    return;
  }
  const StageTiming stage
      = model_->drive(ClockDelayModel::kBalance, n.cell, slew, n.wire);
  for (int i = 0; i < n.children.size(); i++) {
    propagate(n.children[i],
              arrival + stage.delay + stage.wire_delay[i],
              stage.load_slew[i]);
  }
}

void DelayBalancedTreeBuilder::propagate()
{
  const StageTiming stage = driverStage(driverWire(roots_));
  for (int i = 0; i < roots_.size(); i++) {
    propagate(roots_[i], stage.delay + stage.wire_delay[i], stage.load_slew[i]);
  }
}

void DelayBalancedTreeBuilder::report()
{
  propagate();
  double mean, earliest, latest;
  registerArrivals(mean, earliest, latest);
  int buffers = 0;
  int over
      = driver_cell_ >= 0 && !model_->slewOk(driver_cell_, driverWire(roots_));
  for (const Node& n : nodes_) {
    if (n.sink || n.inst == nullptr) {
      continue;
    }
    buffers++;
    if (!n.slew_ok) {
      over++;
      debugPrint(logger_,
                 CTS,
                 "delay balance",
                 2,
                 "tree {} level {} buffer {} over {} loads and {:.1f} um: "
                 "slew {:.2f} ps",
                 clock_.getName(),
                 n.level,
                 model_->cellName(n.cell),
                 n.children.size(),
                 n.wire.length / model_->dbuPerMicron(),
                 model_->worstSlew(n.cell, n.wire) * 1e12);
    }
  }
  if (logger_->debugCheck(CTS, "delay balance", 1)) {
    reportLevels();
  }
  logger_->info(CTS,
                303,
                " Delay-balanced tree of \"{}\": {} buffers, modeled "
                "latency {:.1f} ps ({:.1f} to {:.1f}), {} nets over the "
                "slew limit.",
                clock_.getName(),
                buffers,
                mean * 1e12,
                earliest * 1e12,
                latest * 1e12,
                over);
}

void DelayBalancedTreeBuilder::reportLevels() const
{
  // Where the registers' arrivals part: for the buffers of each level, how
  // far the registers below one load are from those below the others.
  std::vector<double> mean(nodes_.size(), 0.0);
  std::map<int, std::array<double, 3>> levels;
  const std::function<void(int)> visit = [&](const int node) {
    const Node& n = nodes_[node];
    if (n.sink) {
      mean[node] = n.arrival + lateAt(node, n.in_slew);
      return;
    }
    double total = 0.0;
    double count = 0.0;
    for (const int child : n.children) {
      visit(child);
      total += nodes_[child].registers * mean[child];
      count += nodes_[child].registers;
    }
    mean[node] = total / count;
    std::array<double, 3>& level = levels[n.level];
    for (const int child : n.children) {
      const double off = mean[child] - mean[node];
      level[0] += nodes_[child].registers * off * off;
      level[1] += nodes_[child].registers;
      level[2] = std::max(level[2], std::abs(off));
    }
  };
  for (const int root : roots_) {
    visit(root);
  }
  for (const auto& [level, sums] : levels) {
    debugPrint(logger_,
               CTS,
               "delay balance",
               1,
               "tree {} level {}: loads apart by {:.2f} ps rms, {:.2f} ps "
               "at most",
               clock_.getName(),
               level,
               std::sqrt(sums[0] / sums[1]) * 1e12,
               sums[2] * 1e12);
  }
}

}  // namespace cts
