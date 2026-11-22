// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Clock.h"
#include "ClockDelayModel.h"
#include "CtsOptions.h"
#include "TreeBuilder.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace utl {
class Logger;
}  // namespace utl

namespace cts {

// Builds a clock tree bottom up, balancing modeled delay instead of wire
// length, so that it stays balanced where the sinks are unevenly spread.
//
// Sinks that are equally late are grouped by nearness under buffers, level by
// level; each buffer is the cell that makes its group as late as the others
// of its level. A sink with a delay behind it, such as a clock gate, joins the
// level whose buffers are as late as it is. The buffers that remain are
// joined two at a time, each joint where the model gives both sides the same
// delay, with repeaters where the wire is long or one side is early. The tree
// is grown with groups that widen level by level and with groups as wide as
// a buffer reaches, and the less late of the two is kept.
class DelayBalancedTreeBuilder : public TreeBuilder
{
 public:
  DelayBalancedTreeBuilder(CtsOptions* options,
                           Clock& clk,
                           TreeBuilder* parent,
                           utl::Logger* logger,
                           odb::dbDatabase* db)
      : TreeBuilder(options, clk, parent, logger, db)
  {
  }

  void run() override;

  void setDelayModel(const ClockDelayModel* model) { model_ = model; }
  // The cell that drives the net, when the clock reaches the net through it.
  void setDriverCell(int cell, odb::dbITerm* input)
  {
    driver_cell_ = cell;
    driver_input_ = input;
  }
  odb::dbITerm* getDriverInput() const { return driver_input_; }
  // Slew at the input of the net's driver, where the timer knows it.
  void setDriverSlew(double slew) { driver_slew_ = slew; }
  // The port the clock comes from, with the timer's slew there, when the
  // net's driver is the only load of the port's net.
  void setSourcePort(const NetPin& port, double slew)
  {
    source_ = port;
    source_slew_ = slew;
    has_source_ = true;
  }
  void setPinModel(odb::dbITerm* pin, const ClockPinModel& model)
  {
    pin_models_[pin] = model;
  }
  // Delay the registers aim at, from the net's driver.
  void setLatency(double latency)
  {
    latency_ = latency;
    has_latency_ = true;
  }
  // A pin that stays on the net beside the tree, such as an output port's.
  void addPortLoad(const NetPin& pin) { port_loads_.push_back(pin); }
  void allowPortBuffer(bool allow) { allow_port_buffer_ = allow; }
  // What the net's driver drives once the tree is built.
  const ClockPinModel& getDriverModel() const { return driver_model_; }

 private:
  struct Node
  {
    // Center of a buffer; the pin of a sink.
    int x = 0;
    int y = 0;
    // Negative for a sink without clock logic behind it.
    int cell = -1;
    double cap = 0.0;
    odb::dbTechLayer* layer = nullptr;
    // Delay of the cell at the nominal input slew, the load on its output,
    // and the delay from the output to the registers.
    double gate = 0.0;
    double load = 0.0;
    double after = 0.0;
    double slew = 0.0;
    double registers = 1.0;
    // Modeled arrival and slew at the input.
    double arrival = 0.0;
    double in_slew = 0.0;
    bool sink = false;
    bool register_clock = false;
    bool slew_ok = true;
    // One of the buffers of a group split by lateness.
    bool part = false;
    int level = 0;
    ClockInst* inst = nullptr;
    std::vector<int> children;
    // Cells that drive the children within the slew limit.
    std::vector<int> options;
    NetWire wire;
  };

  // A rectilinear line through its points.
  struct Path
  {
    std::vector<odb::Point> points;
    double length() const;
    odb::Point at(double distance) const;
  };

  struct JointFit
  {
    double miss = 0.0;
    double place = 0.0;
    int repeaters_a = 0;
    int repeaters_b = 0;
    bool valid = false;
    // How far the first repeater of each side stands off the line.
    double detour_a = 0.0;
    double detour_b = 0.0;
  };

  struct FeedFit
  {
    double miss = 0.0;
    int pads = 0;
    double pitch = 0.0;
    // The first `weak` delaying buffers take the cell `cell`.
    int cell = 0;
    int weak = 0;
  };

  // The way from the net's driver to the top of the tree, as it is planned.
  struct FeedPlan
  {
    int top = 0;
    Path path;
    // Hops beyond what the length asks for, where the buffers of the way
    // stand off it and their wires are longer for it.
    int spare = 0;
    // Whether the first buffer stands beside the driver.
    bool beside = true;
    // How late the registers below the top are, by the slew at its input.
    std::vector<std::pair<double, double>> below;
  };

  // A tree as grown, with the modeled arrival at its registers.
  struct Grown
  {
    std::vector<Node> nodes;
    std::vector<int> roots;
    int levels = 0;
    double mean = 0.0;
    double spread = 0.0;
    // Whether a level above the sinks was looked at with groups narrower
    // than the trunk reaches, so that wide groups may make another tree.
    bool widens = false;
    bool wide = false;
  };

  void initFloor();
  void initSinks(std::vector<int>& pool, std::vector<int>& waiting);
  // Builds the tree over the sinks: levels of groups, then the trunk and the
  // way to the driver. With `wide`, groups above the sinks are as wide as the
  // trunk reaches; otherwise each level starts from twice the diameter of the
  // one below.
  Grown grow(bool wide);
  // A level of buffers over the pool. Above the sinks, a buffer over too few
  // nodes only stands in their way: those nodes are left in `few` for the
  // trunk, which balances them as it joins them. Where the groups are too few
  // to be worth a level, wider ones are tried, as far as the trunk reaches,
  // and `diameter` becomes theirs; no buffers are returned when none are.
  std::vector<int> makeLevel(const std::vector<int>& pool,
                             int level,
                             unsigned fanout,
                             double width,
                             double& diameter,
                             std::vector<int>& few);
  // Joins what the levels leave and leads the net's driver to it; returns
  // what the driver drives. `step` is the delay of the last level.
  std::vector<int> joinTops(std::vector<int> pool, bool fed, double step);
  // The mean, earliest, and latest modeled arrival at the registers.
  void registerArrivals(double& mean, double& earliest, double& latest) const;
  int addBuffer(const odb::Point& at, int cell, int level);

  NetPin inputPin(int node) const;
  NetPin outputPin(int node) const;
  NetPin trunkInput(const odb::Point& at) const;
  NetPin trunkOutput(const odb::Point& at) const;
  double lateAt(int node, double slew) const;
  double late(int node) const { return lateAt(node, slew_); }
  // Lays the wire from the node to its children and times it.
  void settle(int node);
  // Times the node over the wire it has.
  void time(int node);
  void setCell(int node, int cell);
  // Finds the cells that drive the buffer's loads within the slew limit.
  void setOptions(int buffer);
  std::vector<int> optionsOf(int node) const;

  // Groups of items by nearness. The groups aim at one load, counting for
  // each item its pin and the wire to its nearest neighbor, so that they are
  // equally slow; a group that no buffer drives within the slew limit, or
  // that exceeds the diameter or the fanout, is halved.
  std::vector<std::vector<int>> groupsOf(const std::vector<int>& items,
                                         double diameter,
                                         unsigned fanout) const;
  // Splits `items` into `parts` groups of near-equal weight by recursive
  // bisection across the wider extent.
  void bisect(std::vector<int> items,
              int parts,
              const std::vector<double>& weight,
              std::vector<std::vector<int>>& groups) const;
  std::vector<double> spacing(const std::vector<int>& items,
                              double reach) const;
  bool fits(const std::vector<int>& group,
            double diameter,
            unsigned fanout) const;
  double extent(const std::vector<int>& items) const;
  odb::Point centroid(const std::vector<int>& items) const;
  bool anyRegisterClock(const std::vector<int>& items) const;
  NetWire wireOver(const NetPin& driver, const std::vector<int>& items) const;
  // The wire from the net's driver to `items` and the pins that stay on it.
  NetWire driverWire(const std::vector<int>& items) const;
  // The net's driver over `wire`, with the nominal slew at its input, or at
  // its output where it has no cell.
  StageTiming driverStage(const NetWire& wire) const;
  bool drivable(const std::vector<int>& items,
                double diameter,
                unsigned fanout) const;

  // A buffer over each group of the pool, with the cells that may drive it.
  std::vector<int> cluster(const std::vector<int>& pool,
                           double diameter,
                           unsigned fanout,
                           int level,
                           double width);
  // Makes each buffer over one node as late as `target` by its cell and by
  // its distance from the node. Where the node would be later than the others
  // behind any buffer, the buffer is dropped from `buffers` and the node
  // returned, to wait for a later level.
  std::vector<int> padSingles(std::vector<int>& buffers,
                              double target,
                              double width);
  // Gives each load of the buffer the cell that brings the registers below it
  // nearest those below the buffer's other loads, the wires to them counted.
  void even(int buffer);
  // How late the registers below each load of the buffer are, from its
  // output.
  std::vector<double> loadLates(int buffer) const;
  // Gives each buffer the cell that makes it nearest as late as the others
  // and as the waiting sinks about to join them; returns that lateness.
  double balance(const std::vector<int>& buffers,
                 const std::vector<int>& waiting,
                 size_t next,
                 double reference);

  // Joins the items two at a time, each time the two whose joint will be the
  // least late: early items near each other first, so that the items that
  // still need the most delay above them end up deepest, and a joint has
  // little to make up with repeaters.
  int joinAll(const std::vector<int>& items);
  // Joins two nodes under one buffer and returns it. The joint stands where
  // the model gives its two sides the same delay, with the fewest repeaters
  // that keep its wires within a hop and pad its earlier side; where no place
  // does, a side's own buffer takes another of its cells.
  int join(int a, int b);
  // The fewest repeaters, the place on the path, and the detour of a repeater
  // that leave the two sides of a joint equally late.
  JointFit fit(int a, int b, const Path& path) const;
  // The fewest repeaters and the place on the path that leave the two sides
  // nearest. `last` is the last tried, for when none holds the slew limit.
  JointFit fitRepeaters(int a, int b, const Path& path, JointFit& last) const;
  // What no place makes up, a repeater set off from the line does, on the
  // side that is early. A side without a repeater takes one for it only where
  // the sides are far apart, since it delays both.
  JointFit fitDetour(int a,
                     int b,
                     const Path& path,
                     const JointFit& placed) const;
  // The place between `low` and `high` along the path that leaves the two
  // sides nearest, and the `miss` there.
  double placeJoint(int a,
                    int b,
                    const Path& path,
                    int repeaters_a,
                    int repeaters_b,
                    double low,
                    double high,
                    double& miss) const;
  // Lateness of the registers below `b` over those below `a`, with a joint
  // `place` along the path from `a` to `b` and repeaters on each side.
  double lag(int a,
             int b,
             const Path& path,
             double place,
             int repeaters_a,
             int repeaters_b,
             bool* slew_ok = nullptr,
             double detour_a = 0.0,
             double detour_b = 0.0) const;
  // The span of places a joint may have on a path of length `total`.
  void room(double total,
            int repeaters_a,
            int repeaters_b,
            double& low,
            double& high) const;
  // Where the repeaters between a joint at `place` and the `end` of the path
  // stand. The first is set off from the line by `detour`, which makes the
  // wires on either side of it that much longer.
  std::vector<odb::Point> armPoints(const Path& path,
                                    double place,
                                    double end,
                                    int repeaters,
                                    double detour = 0.0) const;
  // Delay from the input of the first of the trunk buffers at `points`,
  // each driving the next and the last driving `end`, to the registers.
  double chainLate(const std::vector<odb::Point>& points,
                   int end,
                   double slew,
                   bool* slew_ok = nullptr) const;
  // The same to the input of `end`; `slew` becomes the slew there. The first
  // `weak` buffers after the first take the cell `cell`.
  double chainDelay(const std::vector<odb::Point>& points,
                    int end,
                    double& slew,
                    bool* slew_ok,
                    int cell = 0,
                    int weak = 0) const;
  // How late the registers below the node are from its input, with `slew`
  // there and the slews that follow from it all the way down.
  double lateBelow(int node, double slew) const;
  // Adds the buffers of a chain and returns the first, or `end` when there
  // are none.
  int chain(const std::vector<odb::Point>& points,
            int end,
            int level,
            int cell = 0,
            int weak = 0);

  // Buffers from the net's driver to `top`, with delaying buffers or a longer
  // way when the registers would otherwise have the clock before the latency
  // they aim at. The first stands beside the driver, unless the driver can
  // take the first wire of the way itself and nothing needs the delay.
  // Returns what the driver drives.
  int feed(int top);
  // The delaying buffers, the way, and the cells that bring the registers
  // nearest the latency they aim at.
  FeedFit fitFeed(FeedPlan& plan) const;
  // The delaying buffers and their distance; returns the most of them that
  // leave the registers early however far apart they stand, or -1.
  int fitPads(const FeedPlan& plan, FeedFit& best) const;
  // A longer way, out along the row and back, for what `under` delaying
  // buffers leave.
  void fitLongerWay(FeedPlan& plan, int under, FeedFit& best) const;
  // Weaker cells for some of the delaying buffers.
  void fitWeakPads(const FeedPlan& plan, int under, FeedFit& best) const;
  int feedHops(const FeedPlan& plan, const Path& way) const;
  // What the driver adds ahead of the first of `points`, and the slew there.
  void driverOver(const FeedPlan& plan,
                  const std::vector<odb::Point>& points,
                  double& before,
                  double& first_slew,
                  bool& slew_ok) const;
  double lateBelowAt(const FeedPlan& plan, double slew) const;
  // How late the registers are from the driver's input over `points`.
  double feedLatency(const FeedPlan& plan,
                     const std::vector<odb::Point>& points,
                     int cell,
                     int weak,
                     bool* chain_ok) const;
  double feedOver(const FeedPlan& plan,
                  const Path& way,
                  int pads,
                  double pitch) const;
  // Delaying buffers stand alternately at the pitch and beside the first
  // buffer, along the path or, where it is short, along the row.
  std::vector<odb::Point> feedPoints(const Path& path,
                                     int hops,
                                     int pads,
                                     double pitch,
                                     bool beside = true) const;
  // Repeaters from the net's driver to `top` where the two are far apart, and
  // a buffer beside the driver where it cannot drive the first of them.
  // Returns what the driver drives.
  int reach(int top);
  // Buffers from the net's driver to the ports of the net, whose wires would
  // otherwise load every register's clock: a trunk wire apart, the last as
  // far from the ports as their load allows, and the first beside the driver
  // where the driver cannot take a trunk wire as well as the tree.
  void planPorts();
  // Buffers from the clock's port to the net's driver where the two are more
  // than a trunk wire apart, and the delay and slew the driver's input has
  // from the port with them.
  void planSource();

  // A shortest line from one point to another on which buffers can stand: of
  // one bend where that is clear of the blockages, else around their corners.
  Path pathBetween(const odb::Point& from, const odb::Point& to) const;
  bool blocked(const odb::Point& at) const;
  // Whether a buffer can stand anywhere on the straight line between two
  // points.
  bool clear(const odb::Point& from, const odb::Point& to) const;
  // The corner of a clear line of one bend between two points, if any.
  bool bend(const odb::Point& from,
            const odb::Point& to,
            odb::Point& corner) const;
  // The point nearest `at` where a buffer stands on a row, inside the core
  // and clear of every blockage.
  odb::Point legal(odb::Point at) const;

  void setDriverModel(const std::vector<int>& roots);
  void emit(const std::vector<int>& roots);
  std::vector<std::string> bufferNames(const std::vector<int>& order,
                                       const std::vector<bool>& leaves) const;
  // Adds the trunk buffers at `points`, each driving the next, to the clock;
  // returns the first and the last.
  std::pair<ClockInst*, ClockInst*> emitChain(
      const std::vector<odb::Point>& points,
      const std::string& kind);
  void report();
  void reportLevels() const;
  void propagate();
  // Sets the modeled clock arrival at the input of `node` and of every node
  // below it, from the input of the net's driver.
  void propagate(int node, double arrival, double slew);

  const ClockDelayModel* model_ = nullptr;
  std::unordered_map<odb::dbITerm*, ClockPinModel> pin_models_;
  ClockPinModel driver_model_;
  std::vector<NetPin> port_loads_;
  bool allow_port_buffer_ = true;
  bool isolates_ports_ = false;
  // Where the buffers from the driver to the net's ports stand.
  std::vector<odb::Point> port_chain_;
  NetPin source_;
  double source_slew_ = 0.0;
  bool has_source_ = false;
  // Where the buffers from the clock's port to the driver stand, and the
  // delay from the port to the driver's input.
  std::vector<odb::Point> source_chain_;
  double source_delay_ = 0.0;
  int driver_cell_ = -1;
  odb::dbITerm* driver_input_ = nullptr;
  double driver_slew_ = -1.0;
  double latency_ = 0.0;
  bool has_latency_ = false;

  std::vector<Node> nodes_;
  // What the net's driver drives.
  std::vector<int> roots_;
  odb::Point driver_;
  NetPin driver_pin_;
  odb::Rect core_;
  int row_height_ = 1;
  int site_width_ = 1;
  int half_width_ = 0;
  int half_height_ = 0;
  int trunk_ = 0;
  // Input slew the delays are quoted at.
  double slew_ = 0.0;
  // Longest trunk wire and least distance between trunk buffers, in DBU.
  double hop_ = 0.0;
  double gap_ = 0.0;
  // Mismatch below which a joint is balanced, and the delay below which a
  // difference is not worth a buffer: a joint that far off takes no repeater
  // for it, and two trees that near are equally late.
  double tolerance_ = 0.0;
  double pad_ = 0.0;
  int joints_ = 0;
  int repeaters_ = 0;
  double worst_miss_ = 0.0;
  double sum_miss_ = 0.0;
};

}  // namespace cts
