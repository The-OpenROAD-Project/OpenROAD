// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "Rebuffer.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "BufferMove.hh"
#include "BufferedNet.hh"
#include "UnbufferMove.hh"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "est/EstimateParasitics.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/ArcDelayCalc.hh"
#include "sta/Delay.hh"
#include "sta/Fuzzy.hh"
#include "sta/Graph.hh"
#include "sta/GraphClass.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/StaState.hh"
#include "sta/TimingArc.hh"
#include "sta/Transition.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace rsz {

using namespace sta;  // NOLINT

using odb::dbSigType;
using std::make_shared;
using utl::RSZ;

using BnetType = BufferedNetType;
using BnetSeq = BufferedNetSeq;
using BnetPtr = BufferedNetPtr;
using BnetMetrics = BufferedNet::Metrics;

Rebuffer::Rebuffer(Resizer* resizer) : resizer_(resizer)
{
}

void Rebuffer::annotateLoadSlacks(BnetPtr& tree, sta::Vertex* root_vertex)
{
  for (auto rf_index : sta::RiseFall::rangeIndex()) {
    arrival_paths_[rf_index] = nullptr;
  }

  visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> int {
        switch (node->type()) {
          case BnetType::via:
          case BnetType::wire:
          case BnetType::buffer:
            return recurse(node->ref());
          case BnetType::junction:
            return recurse(node->ref()) + recurse(node->ref2());
          case BnetType::load: {
            const sta::Pin* load_pin = node->loadPin();
            sta::Vertex* vertex = graph_->pinLoadVertex(load_pin);
            sta::Path* req_path
                = sta_->vertexWorstSlackPath(vertex, sta::MinMax::max());
            sta::Path* arrival_path = req_path;

            while (req_path && arrival_path->vertex(sta_) != root_vertex) {
              arrival_path = arrival_path->prevPath();
              if (!arrival_path) {
                logger_->warn(
                    RSZ,
                    2006,
                    "failed to trace timing path for load {} when buffering {}",
                    network_->name(load_pin),
                    network_->name(pin_));
                break;
              }
            }

            if (!arrival_path) {
              node->setSlackTransition(nullptr);
              node->setSlack(FixedDelay::INF);
            } else {
              const sta::RiseFall* rf = req_path->transition(sta_);
              node->setSlackTransition(rf->asRiseFallBoth());
              node->setSlack(FixedDelay(
                  req_path->required() - arrival_path->arrival(), resizer_));

              if (arrival_paths_[rf->index()] == nullptr) {
                arrival_paths_[rf->index()] = arrival_path;
              }
            }
            return 1;
          }
          default:
            logger_->critical(RSZ, 1001, "unhandled BufferedNet type");
        }
      },
      tree);
}

BnetPtr Rebuffer::stripTreeBuffers(const BnetPtr& tree)
{
  return visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> BnetPtr {
        switch (node->type()) {
          case BnetType::via:
            return make_shared<BufferedNet>(BnetType::via,
                                            node->location(),
                                            node->layer(),
                                            node->refLayer(),
                                            recurse(node->ref()),
                                            corner_,
                                            resizer_);
          case BnetType::wire:
            return make_shared<BufferedNet>(BnetType::wire,
                                            node->location(),
                                            node->layer(),
                                            recurse(node->ref()),
                                            corner_,
                                            resizer_,
                                            estimate_parasitics_);
          case BnetType::junction:
            return make_shared<BufferedNet>(BnetType::junction,
                                            node->location(),
                                            recurse(node->ref()),
                                            recurse(node->ref2()),
                                            resizer_);
          case BnetType::load: {
            return node;
          }
          case BnetType::buffer: {
            return recurse(node->ref());
          }
          default:
            logger_->critical(RSZ, 1002, "unhandled BufferedNet type");
        }
      },
      tree);
}

BnetPtr Rebuffer::resteiner(const BnetPtr& tree)
{
  std::vector<BnetPtr> sinks;
  visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> int {
        switch (node->type()) {
          case BnetType::wire:
          case BnetType::via:
            return recurse(node->ref());
          case BnetType::junction:
            return recurse(node->ref()) + recurse(node->ref2());
          case BnetType::load:
            sinks.push_back(node);
            return 1;
          case BnetType::buffer: {
            sinks.push_back(make_shared<BufferedNet>(BnetType::buffer,
                                                     node->location(),
                                                     node->bufferCell(),
                                                     resteiner(node->ref()),
                                                     corner_,
                                                     resizer_,
                                                     estimate_parasitics_));
            return 1;
          }
          default:
            logger_->critical(RSZ, 1006, "unhandled BufferedNet type");
        }
      },
      tree);

  return resizer_->makeBufferedNetSteinerOverBnets(
      tree->location(), sinks, corner_);
}

// Returns: (gate delay, slack correction to account for changed gate pin load,
// gate pin slew)
std::tuple<sta::Delay, sta::Delay, sta::Slew> Rebuffer::drvrPinTiming(
    const BnetPtr& bnet)
{
  if (bnet->slackTransition() == nullptr) {
    return {0, 0, 0};
  }

  sta::Delay delay = 0, correction = INF;
  sta::Slew slew = 0;
  for (auto rf : bnet->slackTransition()->range()) {
    const sta::Path* arrival_path = arrival_paths_[rf->index()];
    const sta::Path* driver_path = arrival_path->prevPath();
    const TimingArc* driver_arc = arrival_path->prevArc(resizer_);
    const Edge* driver_edge = arrival_path->prevEdge(resizer_);

    sta::Delay rf_delay, rf_correction;
    sta::Slew rf_slew = 0;

    if (driver_path) {
      sta::LoadPinIndexMap load_pin_index_map(network_);
      sta::Slew slew = graph_delay_calc_->edgeFromSlew(
          driver_path->vertex(sta_),
          driver_arc->fromEdge()->asRiseFall(),
          driver_edge,
          driver_path->scene(sta_),
          driver_path->minMax(sta_));

      auto dcalc_result
          = arc_delay_calc_->gateDelay(nullptr,
                                       driver_arc,
                                       slew,
                                       bnet->cap() + drvr_port_->capacitance(),
                                       nullptr,
                                       load_pin_index_map,
                                       driver_path->scene(sta_),
                                       driver_path->minMax(sta_));
      rf_delay = dcalc_result.gateDelay();

      Arrival prev_arrival = driver_path->isClock(sta_)
                                 ? search_->clkPathArrival(driver_path)
                                 : driver_path->arrival();
      rf_correction
          = arrival_path->arrival() - (prev_arrival + dcalc_result.gateDelay());
      rf_slew = dcalc_result.drvrSlew();
    } else {
      rf_delay = 0;
      rf_correction = 0;
    }

    delay = std::max(rf_delay, delay);
    correction = std::min(rf_correction, correction);
    slew = std::max(rf_slew, slew);
  }

  return {delay, correction, slew};
}

bool Rebuffer::loadSlewSatisfactory(sta::LibertyPort* driver,
                                    const BnetPtr& bnet)
{
  double wire_res, wire_cap;
  estimate_parasitics_->wireSignalRC(corner_, wire_res, wire_cap);
  float r_drvr = driver->driveResistance();
  float load_slew
      = (r_drvr + resizer_->dbuToMeters(bnet->maxLoadWireLength()) * wire_res)
        * bnet->cap() * elmore_skew_factor_;

  return load_slew <= maxSlewMargined(bnet->maxLoadSlew());
}

FixedDelay Rebuffer::slackAtDriverPin(const BufferedNetPtr& bnet)
{
  sta::Delay correction;
  std::tie(std::ignore, correction, std::ignore) = drvrPinTiming(bnet);
  return bnet->slack() + FixedDelay(correction, resizer_);
}

std::optional<FixedDelay> Rebuffer::evaluateOption(const BnetPtr& option,
                                                   // Only used for debug print.
                                                   int index)
{
  sta::Delay correction;
  sta::Slew slew;
  std::tie(std::ignore, correction, slew) = drvrPinTiming(option);
  FixedDelay slack = option->slack() + FixedDelay(correction, resizer_);

  if (!loadSlewSatisfactory(drvr_port_, option)) {
    return {};
  }

  // Refuse buffering option if we are violating max slew on the driver pin.
  // There's one exception: If we previously observed satisfactory slew with
  // an even higher load, ficticiously assume this load satisfies max slew.
  // This is a precaution against non-monotonic slew vs load data which would
  // cause inconsistencies in the algorithm.
  if (slew > drvr_pin_max_slew_ && option->cap() > drvr_load_high_water_mark_) {
    return {};
  }

  drvr_load_high_water_mark_
      = std::max(option->cap(), drvr_load_high_water_mark_);

  debugPrint(logger_,
             RSZ,
             "rebuffer",
             2,
             "option {:3d}: {:2d} buffers {:.2f} area slack {} cap {}",
             index,
             option->bufferCount(),
             option->area(),
             delayAsString(slack.toSeconds(), this, 3),
             units_->capacitanceUnit()->asString(option->cap()));
  return slack;
}

static BnetPtr stripWireOnBnet(BnetPtr ptr)
{
  while (ptr->type() == BnetType::wire || ptr->type() == BnetType::via) {
    ptr = ptr->ref();
  }
  return ptr;
}

static BnetPtr stripWiresAndBuffersOnBnet(BnetPtr ptr)
{
  while (ptr->type() == BnetType::wire || ptr->type() == BnetType::buffer
         || ptr->type() == BnetType::via) {
    ptr = ptr->ref();
  }
  return ptr;
}

static const RiseFallBoth* combinedTransition(const RiseFallBoth* a,
                                              const RiseFallBoth* b)
{
  if (a == b) {
    return a;
  }
  if (a == nullptr) {
    return b;
  }
  if (b == nullptr) {
    return a;
  }
  return RiseFallBoth::riseFall();
}

static BufferedNetPtr createBnetJunction(Resizer* resizer,
                                         const BufferedNetPtr& p,
                                         const BufferedNetPtr& q,
                                         odb::Point location)
{
  BufferedNetPtr junc = make_shared<BufferedNet>(
      BufferedNetType::junction, location, p, q, resizer);
  junc->setSlackTransition(
      combinedTransition(p->slackTransition(), q->slackTransition()));
  junc->setSlack(std::min(p->slack(), q->slack()));
  return junc;
}

float Rebuffer::maxSlewMargined(float max_slew)
{
  return max_slew * (1.0 - slew_margin_ / 100.0);
}

float Rebuffer::maxCapMargined(float max_cap)
{
  return max_cap * (1.0 - cap_margin_ / 100.0);
}

bool Rebuffer::bufferSizeCanDriveLoad(const BufferSize& size,
                                      const BnetPtr& bnet,
                                      int extra_wire_length)
{
  double wire_res, wire_cap;
  sta::LibertyPort *inp, *outp;
  estimate_parasitics_->wireSignalRC(corner_, wire_res, wire_cap);
  size.cell->bufferPorts(inp, outp);

  const float extra_cap = resizer_->dbuToMeters(extra_wire_length) * wire_cap
                          + outp->capacitance();

  const float load_slew
      = (size.driver_resistance
         + resizer_->dbuToMeters(bnet->maxLoadWireLength() + extra_wire_length)
               * wire_res)
        * (bnet->cap() + extra_cap) * elmore_skew_factor_;

  bool load_slew_satisfied = load_slew <= maxSlewMargined(bnet->maxLoadSlew());
  bool max_cap_satisfied = (bnet->cap() + extra_cap) <= size.margined_max_cap;
  return load_slew_satisfied && max_cap_satisfied;
}

int Rebuffer::wireLengthLimitImpliedByLoadSlew(sta::LibertyCell* cell)
{
  sta::LibertyPort *in, *out;
  cell->bufferPorts(in, out);

  const float r_drvr = out->driveResistance();
  double wire_res, wire_cap;
  estimate_parasitics_->wireSignalRC(corner_, wire_res, wire_cap);

  const float max_slew = maxSlewMargined(resizer_->maxInputSlew(in, corner_));

  const double a = wire_res * wire_cap;
  const double b = wire_res * in->capacitance() + r_drvr * wire_cap;
  const double c = r_drvr * in->capacitance() - max_slew / elmore_skew_factor_;
  const double D = b * b - 4 * a * c;

  if (D < 0) {
    logger_->error(RSZ,
                   2010,
                   "cannot determine wire length limit implied by load slew on "
                   "cell {} in corner {}",
                   cell->name(),
                   corner_->name());
  }

  const double meters = (-b + std::sqrt(D)) / (2 * a);

  if (meters > 1) {
    // no limit
    return std::numeric_limits<int>::max();
  }

  if (meters < 0 || resizer_->metersToDbu(meters) == 0) {
    logger_->error(RSZ,
                   2011,
                   "cannot determine wire length limit implied by load slew on "
                   "cell {} in corner {}",
                   cell->name(),
                   corner_->name());
  }

  return resizer_->metersToDbu(meters);
}

int Rebuffer::wireLengthLimitImpliedByMaxCap(sta::LibertyCell* cell)
{
  sta::LibertyPort *in, *out;
  cell->bufferPorts(in, out);

  bool cap_limit_exists;
  float cap_limit;
  out->capacitanceLimit(max_, cap_limit, cap_limit_exists);

  if (cap_limit_exists) {
    double wire_res, wire_cap;
    estimate_parasitics_->wireSignalRC(corner_, wire_res, wire_cap);
    double slack = maxCapMargined(cap_limit) - in->capacitance();
    return std::max(0, resizer_->metersToDbu(slack / wire_cap));
  }

  return std::numeric_limits<int>::max();
}

BnetPtr Rebuffer::attemptTopologyRewrite(const BnetPtr& node,
                                         const BnetPtr& left,
                                         const BnetPtr& right,
                                         float best_cap)
{
  FixedDelay junc_slack = std::min(left->slack(), right->slack());

  // Diversify topology and provide shortcut for critical paths.
  //
  // If we encounter:
  //
  //     +-----[buffer*]- a
  //     |
  // ----+
  //     |   +-[buffer*]- b
  //     |   |
  //     +---+
  //         |
  //         +---------- (critical)
  //
  // with at least one [buffer*] site occupied, we rewrite to:
  //
  //     +-------------- (critical)
  //     |
  // ----+
  //     |           +-- b
  //     |           |
  //     +-[buffer]--+
  //                 |
  //                 +-- a
  //
  // provided some additional criteria are met (see code).
  //
  BnetPtr crit1, aux1;
  if (left->slack() < right->slack()) {
    crit1 = stripWireOnBnet(left);
    aux1 = stripWireOnBnet(right);
  } else {
    crit1 = stripWireOnBnet(right);
    aux1 = stripWireOnBnet(left);
  }
  if (crit1->type() == BnetType::junction) {
    BnetPtr crit2 = crit1->ref(), aux2 = crit1->ref2();
    if (crit2->slack() > aux2->slack()) {
      std::swap(crit2, aux2);
    }
    aux2 = stripWireOnBnet(aux2);
    if (aux1->type() == BnetType::buffer || aux2->type() == BnetType::buffer) {
      aux1 = stripWiresAndBuffersOnBnet(aux1);
      aux2 = stripWiresAndBuffersOnBnet(aux2);
      crit2 = stripWiresAndBuffersOnBnet(crit2);

      const BnetPtr in1 = addWire(aux1, node->location(), -1);
      const BnetPtr in2 = addWire(aux2, node->location(), -1);
      const BnetPtr junc1
          = addWire(createBnetJunction(resizer_, in1, in2, node->location()),
                    node->location(),
                    -1);
      const BnetPtr in3 = addWire(crit2, node->location(), -1);

      // Find a buffer size with smallest input pin capacitance such that
      // crit2 stays being the critical branch; if none can be found
      // abort rewriting
      for (BufferSize size : buffer_sizes_) {
        sta::LibertyPort *in, *out;
        size.cell->bufferPorts(in, out);

        if (fuzzyGreaterEqual(in->capacitance() + in3->cap(), best_cap)
            || fuzzyGreaterEqual(in->capacitance() + in3->cap(),
                                 left->cap() + right->cap())
            || junc1->slack() - size.intrinsic_delay < junc_slack) {
          break;
        }

        const FixedDelay buffer_delay
            = bufferDelay(size.cell,
                          junc1->slackTransition(),
                          junc1->cap() + out->capacitance());
        const FixedDelay buffer_slack = junc1->slack() - buffer_delay;

        if (buffer_slack >= junc_slack && bufferSizeCanDriveLoad(size, junc1)) {
          // We are comitting to the rewrite
          BnetPtr buffer = make_shared<BufferedNet>(BnetType::buffer,
                                                    node->location(),
                                                    size.cell,
                                                    junc1,
                                                    corner_,
                                                    resizer_,
                                                    estimate_parasitics_);
          buffer->setSlack(buffer_slack);
          buffer->setSlackTransition(junc1->slackTransition());
          buffer->setDelay(buffer_delay);
          return createBnetJunction(resizer_, buffer, in3, node->location());
        }
      }
    }
  }

  return {};
}

static std::optional<int> findWireLayer(BnetPtr node)
{
  while (node->type() != BnetType::wire && node->type() != BnetType::load
         && node->type() != BnetType::junction) {
    node = node->ref();
  }
  if (node->type() == BnetType::wire) {
    return {node->layer()};
  }
  return {};
}

// Find initial timing-optimized buffering choice over the provided tree
BnetPtr Rebuffer::bufferForTiming(const BnetPtr& tree,
                                  bool allow_topology_rewrite)
{
  sta::LibertyPort* strong_driver;
  {
    sta::LibertyPort* dummy;
    buffer_sizes_.back().cell->bufferPorts(dummy, strong_driver);
  }

  BnetSeq top_opts = visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> BnetSeq {
        switch (node->type()) {
          case BnetType::via:
          case BnetType::buffer:
          case BnetType::wire: {
            int layer = -1;
            if (auto wire_layer = findWireLayer(node)) {
              layer = wire_layer.value();
            }
            BnetSeq opts = recurse(stripWiresAndBuffersOnBnet(node->ref()));
            odb::Point location
                = stripWiresAndBuffersOnBnet(node->ref())->location();

            const int full_wl
                = odb::Point::manhattanDistance(node->location(), location);
            if (full_wl > wire_length_step_ / 2) {
              debugPrint(logger_,
                         RSZ,
                         "rebuffer",
                         4,
                         "{:{}s}inserting prebuffers",
                         "",
                         level);
              // This is a long wire, allow for insertion of buffers at the
              // farther end
              insertBufferOptions(
                  opts, level, std::min(full_wl, wire_length_step_));
            } else {
              BnetSeq opts1 = opts;
              for (BnetPtr& opt : opts1) {
                opt = addWire(opt, node->location(), layer, level);
              }
              insertBufferOptions(opts1, level, 0);
              if (opts1.empty()) {
                // if generated options empty, start again but allow for
                // insertion of buffers at the farther end
                opts1 = opts;
                insertBufferOptions(opts1, level, full_wl);
                for (BnetPtr& opt : opts1) {
                  opt = addWire(opt, node->location(), layer, level);
                }
                insertBufferOptions(opts1, level, 0);
              }
              if (opts1.empty()) {
                // if generated options still empty, this is an internal error
                // of the algorithm (wire_length_step_ should have been chosen
                // to always allow a minimal size buffer to drive itself without
                // ERC)
                logger_->critical(RSZ,
                                  2008,
                                  "buffering pin {}: wire step options empty",
                                  network_->name(pin_));
              }
              return opts1;
            }

            utl::DebugScopedTimer timer(long_wire_stepping_runtime_);
            int round = 0;
            while (location != node->location()) {
              debugPrint(logger_,
                         RSZ,
                         "rebuffer",
                         4,
                         "{:{}s}round {} no of options {}",
                         "",
                         level,
                         round,
                         opts.size());

              const int step = wire_length_step_;

              // move `location` towards `node->location()` by `step`
              int dx = node->location().x() - location.x();
              int dy = node->location().y() - location.y();

              if (abs(dx) + abs(dy) >= step) {
                const float ratio
                    = (float) abs(dx) / (float) (abs(dx) + abs(dy));
                const int dx_abs = std::min((int) (ratio * step), step);
                const int dy_abs = step - dx_abs;
                dx = dx > 0 ? dx_abs : -dx_abs;
                dy = dy > 0 ? dy_abs : -dy_abs;
              }

              location.addX(dx);
              location.addY(dy);

              const int remaining_wl
                  = odb::Point::manhattanDistance(node->location(), location);

              for (BnetPtr& opt : opts) {
                opt = addWire(opt, location, layer, level);
              }
              insertBufferOptions(opts, level, std::min(remaining_wl, step));

              if (opts.empty()) {
                logger_->critical(RSZ,
                                  2007,
                                  "buffering pin {}: wire step options empty",
                                  network_->name(pin_));
              }
              round++;
            }
            return opts;
          }

          case BnetType::junction: {
            const BnetSeq& opts_left = recurse(node->ref());
            const BnetSeq& opts_right = recurse(node->ref2());

            BnetSeq opts;
            opts.reserve(std::max(opts_left.size(), opts_right.size()));
            float best_cap = INF;

            auto li = opts_left.rbegin(), lend = opts_left.rend();
            auto ri = opts_right.rbegin(), rend = opts_right.rend();

            while (li != lend && ri != rend) {
              while (li + 1 != lend && (*(li + 1))->slack() >= (*ri)->slack()) {
                li++;
              }
              while (ri + 1 != rend && (*(ri + 1))->slack() >= (*li)->slack()) {
                ri++;
              }

              bool rewrote = false;
              BnetPtr junc;

              if (allow_topology_rewrite) {
                junc = attemptTopologyRewrite(node, *li, *ri, best_cap);
                if (junc) {
                  rewrote = true;
                }
              }

              if (!rewrote) {
                junc = createBnetJunction(resizer_, *li, *ri, node->location());
              }

              if (junc->fanout() <= fanout_limit_) {
                debugPrint(logger_,
                           RSZ,
                           "rebuffer",
                           3,
                           "{:{}s}{}{}",
                           "",
                           level,
                           rewrote ? "(rewritten) " : "",
                           junc->to_string(resizer_));

                best_cap = junc->cap();
                opts.push_back(std::move(junc));
              }

              while (true) {
                // increment either li or ri, whichever leads to smaller slack
                // decrease
                FixedDelay next_li_slack = (li + 1 != lend)
                                               ? (*(li + 1))->slack()
                                               : -FixedDelay::INF;
                FixedDelay next_ri_slack = (ri + 1 != rend)
                                               ? (*(ri + 1))->slack()
                                               : -FixedDelay::INF;

                if (next_li_slack > next_ri_slack) {
                  li++;
                } else {
                  ri++;
                }

                if (li == lend || ri == rend
                    || (*li)->cap() + (*ri)->cap() < best_cap) {
                  break;
                }
              }
            }
            std::ranges::reverse(opts);
            return opts;
          }

          case BufferedNetType::load: {
            debugPrint(logger_,
                       RSZ,
                       "rebuffer",
                       3,
                       "{:{}s}{}",
                       "",
                       level,
                       node->to_string(resizer_));
            return {node};
          }
          default:
            logger_->error(RSZ, 1004, "unhandled BufferedNet type");
        }
      },
      tree);

  if (top_opts.empty()) {
    logger_->critical(RSZ, 2009, "buffering pin {}: no options produced");
  }

  FixedDelay best_slack = -FixedDelay::INF;
  BufferedNetPtr best_option = nullptr;
  int best_index = 0;
  int i = 1;
  debugPrint(logger_, RSZ, "rebuffer", 2, "timing-optimized options");
  for (const BufferedNetPtr& p : top_opts) {
    // Find slack for drvr_pin into option.
    std::optional<FixedDelay> slack = evaluateOption(p, i);

    if (!slack) {
      // ignore this option as it doesn't pass ERC
      continue;
    }

    if (best_option == nullptr
        || p->slackTransition() == nullptr /* buffer tree unconstrained */
        || slack.value() > best_slack) {
      best_slack = slack.value();
      best_option = p;
      best_index = i;
    }
    i++;
  }
  if (best_option) {
    debugPrint(logger_, RSZ, "rebuffer", 2, "best option {}", best_index);
  } else {
    debugPrint(logger_, RSZ, "rebuffer", 2, "no available option");
  }

  return best_option;
}

static void pruneCapVsAreaOptions(sta::StaState* sta, BufferedNetSeq& options)
{
  std::ranges::sort(
      options,
      [](const BufferedNetPtr& option1, const BufferedNetPtr& option2) {
        return std::make_tuple(option1->area(), option1->cap())
               < std::make_tuple(option2->area(), option2->cap());
      });

  if (options.empty()) {
    return;
  }

  float lowest_cap_seen = options[0]->cap();
  size_t si = 1;
  for (size_t pi = si; pi < options.size(); pi++) {
    const BufferedNetPtr& p = options[pi];
    float cap = p->cap();
    // If cap is the same or worse than lowest_cap_seen, remove solution p.
    if (fuzzyLess(cap, lowest_cap_seen)) {
      // Otherwise copy the survivor down.
      options[si++] = p;
      lowest_cap_seen = cap;
    }
  }
  options.resize(si);
  std::ranges::reverse(options);
}

// Used in area recovery: we need to make sure that there will always be
// one good option passing all ERCs and other constraints. To that end
// we may need to insert an "assured" option, i.e. an option which is no
// worse (in the sense of `BufferedNet::metrics()`) than the option selected,
// on the current node, in the previous pass over the tree. This is supposed
// to be a rare occurrence and when it happens, we use this helper.
void Rebuffer::insertAssuredOption(BnetSeq& opts,
                                   BnetPtr assured_opt,
                                   int level)
{
  auto it = std::ranges::find_if(opts, [&](const BnetPtr& opt) {
    return opt->cap() >= assured_opt->cap();
  });
  debugPrint(logger_,
             RSZ,
             "rebuffer",
             3,
             "{:{}s}assured fixup: {}",
             "",
             level,
             assured_opt->to_string(resizer_));
  opts.insert(it, std::move(assured_opt));
}

// Recover area on a rebuffering choice without regressing timing
BufferedNetPtr Rebuffer::recoverArea(const BufferedNetPtr& root,
                                     FixedDelay slack_target,
                                     float alpha)
{
  sta::Delay slack_correction;
  std::tie(std::ignore, slack_correction, std::ignore) = drvrPinTiming(root);

  if (!root->slackTransition()) {
    // zero out correction if tree has unconstrained timing
    // (all slacks are FixedDelay::INF)
    slack_correction = 0;
    slack_target = -FixedDelay::INF;
  }

  // spread down arrival delay from the root of the tree to each
  // point on the tree
  visitTree(
      [](auto& recurse, int level, const BnetPtr& node, FixedDelay arrival)
          -> int {
        node->setArrivalDelay(arrival);
        switch (node->type()) {
          case BnetType::via:
          case BnetType::wire:
          case BnetType::buffer:
            recurse(node->ref(), arrival + node->delay());
            break;
          case BnetType::junction:
            recurse(node->ref(), arrival);
            recurse(node->ref2(), arrival);
            break;
          case BnetType::load:
            break;
        }
        return 0;
      },
      root,
      -FixedDelay(slack_correction, resizer_));

  BnetSeq top_opts = visitTree(
      [&](auto& recurse, int level, const BnetPtr& node, int upstream_wl)
          -> BufferedNetSeq {
        switch (node->type()) {
          case BnetType::buffer:
          case BnetType::wire: {
            const BnetPtr& inner
                = (node->type() == BnetType::buffer) ? node->ref() : node;

            BnetSeq opts;
            if (inner->type() == BnetType::wire) {
              opts = recurse(inner->ref(), inner->length());
              for (BnetPtr& opt : opts) {
                opt = addWire(opt, inner->location(), inner->layer(), level);
              }
            } else {
              opts = recurse(inner, 0);
            }

            FixedDelay threshold = FixedDelay::lerp(
                node->slack(), slack_target + node->arrivalDelay(), alpha);
            insertBufferOptions(opts,
                                level,
                                /*next_segment_wl=*/upstream_wl,
                                /*area_oriented=*/true,
                                threshold,
                                /*exemplar=*/node.get());
            return opts;
          }
          case BnetType::junction: {
            const BnetSeq& left_opts = recurse(node->ref(), upstream_wl);
            const BnetSeq& right_opts = recurse(node->ref2(), upstream_wl);

            FixedDelay threshold = FixedDelay::lerp(
                node->slack(), slack_target + node->arrivalDelay(), alpha);
            BnetMetrics assured_envelope = node->metrics().withSlack(threshold);
            BnetPtr assured_fallback;

            BnetSeq opts;
            opts.reserve(left_opts.size() * right_opts.size());
            for (const BnetPtr& left : left_opts) {
              for (const BnetPtr& right : right_opts) {
                BnetPtr junc = createBnetJunction(
                    resizer_, left, right, node->location());
                if (!assured_fallback && junc->fitsEnvelope(assured_envelope)) {
                  assured_fallback = junc;
                }

                if (junc->fanout() <= fanout_limit_) {
                  opts.push_back(std::move(junc));
                }
              }
            }

            pruneCapVsAreaOptions(sta_, opts);
            debugPrint(logger_,
                       RSZ,
                       "rebuffer",
                       3,
                       "{:{}s}junction: {} options",
                       "",
                       level,
                       opts.size());
            for (const auto& opt : opts) {
              debugPrint(logger_,
                         RSZ,
                         "rebuffer",
                         3,
                         "{:{}s} - {}",
                         "",
                         level,
                         opt->to_string(resizer_));
            }

            bool assured_found = false;
            for (const BnetPtr& opt : opts) {
              if (opt->fitsEnvelope(assured_envelope)) {
                assured_found = true;
                break;
              }
            }

            if (!assured_found) {
              insertAssuredOption(opts, assured_fallback, level);
            }
            return opts;
          }
          case BnetType::load: {
            debugPrint(logger_,
                       RSZ,
                       "rebuffer",
                       3,
                       "{:{}s}{}",
                       "",
                       level,
                       node->to_string(resizer_));
            return {node};
          }
          default:
            logger_->error(RSZ, 1005, "unhandled BufferedNet type");
        }
      },
      root,
      0);

  FixedDelay best_slack = -FixedDelay::INF;
  float best_area = std::numeric_limits<float>::max();
  BufferedNetPtr best_slack_option = nullptr, best_area_option = nullptr;
  int best_slack_index = 0, best_area_index = 0;
  int i = 1;
  debugPrint(logger_, RSZ, "rebuffer", 2, "area-optimized options");
  for (const BufferedNetPtr& p : top_opts) {
    // Find slack for drvr_pin into option.
    std::optional<FixedDelay> slack = evaluateOption(p, i);

    if (!slack) {
      // ignore this option as it doesn't pass ERC
      continue;
    }

    if (best_slack_option == nullptr
        || p->slackTransition() == nullptr /* buffer tree unconstrained */
        || slack.value() > best_slack) {
      best_slack = slack.value();
      best_slack_option = p;
      best_slack_index = i;
    }
    if ((slack.value() >= slack_target
         || p->slackTransition() == nullptr /* buffer tree unconstrained */)
        && (best_area_option == nullptr || fuzzyLess(p->area(), best_area))) {
      best_area = p->area();
      best_area_option = p;
      best_area_index = i;
    }
    i++;
  }

  if (best_area_option) {
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               2,
               "best option {} (area optimized)",
               best_area_index);
    return best_area_option;
  }
  if (best_slack_option) {
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               2,
               "best option {} (closest to meeting timing target)",
               best_slack_index);
    return best_slack_option;
  }
  debugPrint(logger_, RSZ, "rebuffer", 2, "no available option");
  return nullptr;
}

void Rebuffer::annotateTiming(const BnetPtr& tree)
{
  visitTree(
      [&](auto& recurse, int level, const BnetPtr& bnet) -> int {
        switch (bnet->type()) {
          case BnetType::load:
            return 0;
          case BnetType::junction: {
            int ret = recurse(bnet->ref()) + recurse(bnet->ref2());
            const BnetPtr& p = bnet->ref();
            const BnetPtr& q = bnet->ref2();
            bnet->setSlackTransition(
                combinedTransition(p->slackTransition(), q->slackTransition()));
            bnet->setSlack(std::min(p->slack(), q->slack()));
            bnet->setCapacitance(p->cap() + q->cap());
            return ret;
          }
          case BnetType::wire: {
            int ret = recurse(bnet->ref());
            BnetPtr p = bnet->ref();
            double layer_res, layer_cap;
            bnet->wireRC(
                corner_, resizer_, estimate_parasitics_, layer_res, layer_cap);
            double wire_length = resizer_->dbuToMeters(bnet->length());
            double wire_res = wire_length * layer_res;
            double wire_cap = wire_length * layer_cap;
            FixedDelay wire_delay
                = FixedDelay(wire_res * (wire_cap / 2 + p->cap()), resizer_);
            if (bnet->length() == 0) {
              wire_res = 0;
              wire_cap = 0;
              wire_delay = FixedDelay::ZERO;
            }
            bnet->setDelay(wire_delay);
            bnet->setSlack(p->slack() - wire_delay);
            bnet->setSlackTransition(p->slackTransition());
            return ret;
          }
          case BnetType::buffer: {
            int ret = recurse(bnet->ref());
            BnetPtr p = bnet->ref();
            sta::LibertyPort *in, *out;
            bnet->bufferCell()->bufferPorts(in, out);
            FixedDelay buffer_delay
                = bufferDelay(bnet->bufferCell(),
                              p->slackTransition(),
                              p->cap() + out->capacitance());
            bnet->setDelay(buffer_delay);
            bnet->setSlack(p->slack() - buffer_delay);
            bnet->setSlackTransition(p->slackTransition());
            return ret;
          }
          default:
            abort();
        }
      },
      tree);
}

FixedDelay Rebuffer::bufferDelay(sta::LibertyCell* cell,
                                 const RiseFallBoth* rf,
                                 float load_cap)
{
  FixedDelay delay = FixedDelay::ZERO;

  if (rf) {
    for (auto rf1 : rf->range()) {
      const Scene* scene = arrival_paths_[rf1->index()]->scene(sta_);
      LibertyPort *input, *output;
      cell->bufferPorts(input, output);
      ArcDelay gate_delays[RiseFall::index_count];
      Slew slews[RiseFall::index_count];
      resizer_->gateDelays(output, load_cap, scene, max_, gate_delays, slews);
      delay = std::max<FixedDelay>(
          delay, FixedDelay(gate_delays[rf1->index()], resizer_));
    }
  }

  return delay;
}

BnetPtr Rebuffer::addWire(const BnetPtr& p,
                          odb::Point wire_end,
                          int wire_layer,
                          int level)
{
  BnetPtr z = make_shared<BufferedNet>(BnetType::wire,
                                       wire_end,
                                       wire_layer,
                                       p,
                                       corner_,
                                       resizer_,
                                       estimate_parasitics_);

  double layer_res, layer_cap;
  z->wireRC(corner_, resizer_, estimate_parasitics_, layer_res, layer_cap);
  double wire_length = resizer_->dbuToMeters(z->length());
  double wire_res = wire_length * layer_res;
  double wire_cap = wire_length * layer_cap;
  FixedDelay wire_delay
      = FixedDelay(wire_res * (wire_cap / 2 + p->cap()), resizer_);

  // account for wire delay
  z->setDelay(wire_delay);
  z->setSlack(p->slack() - wire_delay);
  z->setSlackTransition(p->slackTransition());

  if (level != -1) {
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               3,
               "{:{}s}wire wl {} {}",
               "",
               level,
               z->length(),
               z->to_string(resizer_));
  }

  return z;
}

void Rebuffer::insertBufferOptions(
    BufferedNetSeq& opts,
    int level,
    int next_segment_wl,
    bool area_oriented /*=false*/,
    FixedDelay slack_threshold /*=0; used for area mode only*/,
    BufferedNet* exemplar /*=nullptr; used for area mode only*/)
{
  if (opts.empty()) {
    return;
  }

  BufferSize& strong_driver = buffer_sizes_.back();

  BnetMetrics assured_envelope
      = area_oriented ? exemplar->metrics().withSlack(slack_threshold)
                      : BnetMetrics{};
  bool assured_satisfied = !area_oriented;

  float best_area = INF;
  FixedDelay best_slack = -FixedDelay::INF;

  // both `opts` and `buffer_sizes_` are ordered by ascending input
  // capacitance
  BufferedNetSeq new_opts;
  new_opts.reserve(opts.size() * 2);
  auto opts_iter = opts.begin();

  auto pass_through = [&](float threshold_cap) {
    // pass through non-redundant options with cap below `threshold_cap`
    for (; opts_iter != opts.end() && (*opts_iter)->cap() <= threshold_cap;
         opts_iter++) {
      BnetPtr& opt = *opts_iter;

      bool keep = area_oriented ? (fuzzyLess(opt->area(), best_area)
                                   && opt->slack() >= slack_threshold)
                                : (opt->slack() > best_slack);

      if (!bufferSizeCanDriveLoad(strong_driver, opt, next_segment_wl)) {
        keep = false;
      }

      if (keep) {
        new_opts.push_back(opt);

        debugPrint(logger_,
                   RSZ,
                   "rebuffer",
                   3,
                   "{:{}s}{}",
                   "",
                   level,
                   opt->to_string(resizer_));

        if (!assured_satisfied && opt->fitsEnvelope(assured_envelope)) {
          assured_satisfied = true;
        }
        best_slack = opt->slack();
        best_area = opt->area();
      }
    }
  };

  for (BufferSize buffer_size : buffer_sizes_) {
    sta::LibertyCell* buffer_cell = buffer_size.cell;
    sta::LibertyPort *in, *out;
    buffer_cell->bufferPorts(in, out);
    pass_through(in->capacitance());

    BnetPtr load_opt;
    FixedDelay load_opt_buffer_delay = FixedDelay::ZERO;
    auto it = (new_opts.empty() && opts_iter == opts.end()
               && opts_iter > opts.begin())
                  ? (opts_iter - 1)
                  : opts_iter;
    for (; it != opts.end(); it++) {
      BnetPtr& opt = *it;

      if ((area_oriented
               ? (opt->slack() - buffer_size.intrinsic_delay >= slack_threshold
                  && fuzzyLess(opt->area() + buffer_cell->area(), best_area))
               : (opt->slack() - buffer_size.intrinsic_delay) > best_slack)
          && bufferSizeCanDriveLoad(buffer_size, opt)) {
        // this is a candidate, make the detailed delay calculation
        const FixedDelay buffer_delay
            = bufferDelay(buffer_cell,
                          opt->slackTransition(),
                          opt->cap() + out->capacitance());
        const FixedDelay slack = opt->slack() - buffer_delay;

        if (area_oriented ? slack >= slack_threshold : slack > best_slack) {
          load_opt = opt;
          load_opt_buffer_delay = buffer_delay;
          best_slack = slack;
          best_area = load_opt->area() + buffer_cell->area();
        }
      }
    }

    if (load_opt) {
      BnetPtr z = make_shared<BufferedNet>(BnetType::buffer,
                                           load_opt->location(),
                                           buffer_cell,
                                           load_opt,
                                           corner_,
                                           resizer_,
                                           estimate_parasitics_);
      z->setSlack(best_slack);
      z->setSlackTransition(load_opt->slackTransition());
      z->setDelay(load_opt_buffer_delay);

      if (!assured_satisfied && z->fitsEnvelope(assured_envelope)) {
        assured_satisfied = true;
      }

      debugPrint(logger_,
                 RSZ,
                 "rebuffer",
                 3,
                 "{:{}s}buffer {} load {} delay {}: {}",
                 "",
                 level,
                 buffer_cell->name(),
                 units_->capacitanceUnit()->asString(load_opt->cap()),
                 delayAsString(load_opt_buffer_delay.toSeconds(), this),
                 z->to_string(resizer_));
      new_opts.push_back(std::move(z));
    }
  }
  pass_through(INF);

  if (!assured_satisfied) {
    assert(exemplar != nullptr);

    if (exemplar && exemplar->type() == BnetType::buffer) {
      sta::LibertyCell* buffer_cell = exemplar->bufferCell();
      sta::LibertyPort *in, *out;
      buffer_cell->bufferPorts(in, out);

      float best_area = INF;
      BnetPtr best_option;
      for (const BnetPtr& load_opt : opts) {
        if (load_opt->area() >= best_area) {
          continue;
        }

        const FixedDelay buffer_delay
            = bufferDelay(buffer_cell,
                          load_opt->slackTransition(),
                          load_opt->cap() + out->capacitance());
        if (bufferSizeCanDriveLoad(*buffer_sizes_index_.at(buffer_cell),
                                   load_opt)
            && load_opt->slack() - buffer_delay >= slack_threshold) {
          BnetPtr z = make_shared<BufferedNet>(BnetType::buffer,
                                               load_opt->location(),
                                               buffer_cell,
                                               load_opt,
                                               corner_,
                                               resizer_,
                                               estimate_parasitics_);
          z->setSlack(load_opt->slack() - buffer_delay);
          z->setSlackTransition(load_opt->slackTransition());
          z->setDelay(buffer_delay);
          if (z->fitsEnvelope(assured_envelope)) {
            best_area = load_opt->area();
            best_option = z;
          }
        }
      }

      if (best_option) {
        insertAssuredOption(new_opts, best_option, level);
        assured_satisfied = true;
      }
    } else {
      for (const BnetPtr& opt : opts) {
        if (opt->fitsEnvelope(assured_envelope)) {
          insertAssuredOption(new_opts, opt, level);
          assured_satisfied = true;
          break;
        }
      }
    }

    if (!assured_satisfied) {
      logger_->error(
          RSZ,
          501,
          "buffering pin {} failed: area recovery cannot reproduce solution",
          network_->name(pin_),
          delayAsString(slack_threshold.toSeconds(), this, 3));
    }
  }

  new_opts.swap(opts);
}

static float bufferCin(const sta::LibertyCell* cell)
{
  sta::LibertyPort *a, *y;
  cell->bufferPorts(a, y);
  return a->capacitance();
}

void Rebuffer::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
  resizer_max_wire_length_
      = resizer_->metersToDbu(resizer_->findMaxWireLength());
  sta_->checkCapacitancesPreamble(sta_->scenes());
  sta_->checkSlewsPreamble();
  sta_->checkFanoutPreamble();

  resizer_->findFastBuffers();
  buffer_sizes_.clear();

  for (auto cell : resizer_->buffer_fast_sizes_) {
    sta::LibertyPort *in, *out;
    cell->bufferPorts(in, out);
    buffer_sizes_.push_back(BufferSize{
        cell,
        FixedDelay(out->intrinsicDelay(sta_), resizer_),
        /*margined_max_cap=*/0.0f,
        out->driveResistance(),
    });
  }

  std::ranges::sort(buffer_sizes_, [=](BufferSize a, BufferSize b) {
    return bufferCin(a.cell) < bufferCin(b.cell);
  });

  buffer_sizes_index_.clear();
  for (auto& size : buffer_sizes_) {
    buffer_sizes_index_[size.cell] = &size;
  }
}

void Rebuffer::initOnCorner(Scene* corner)
{
  corner_ = corner;
  wire_length_step_
      = std::min({resizer_max_wire_length_,
                  wireLengthLimitImpliedByLoadSlew(buffer_sizes_.front().cell),
                  wireLengthLimitImpliedByMaxCap(buffer_sizes_.front().cell)});
  characterizeBufferLimits();
}

float Rebuffer::findBufferLoadLimitImpliedByDriverSlew(sta::LibertyCell* cell)
{
  sta::LibertyPort *inp, *outp;
  cell->bufferPorts(inp, outp);

  float max_slew;
  bool max_slew_exists = false;
  sta_->findSlewLimit(
      outp, corner_, sta::MinMax::max(), max_slew, max_slew_exists);
  max_slew = maxSlewMargined(max_slew);
  float in_slew = maxSlewMargined(resizer_->maxInputSlew(inp, corner_));

  auto objective = [&](float load_cap) {
    sta::Slew slew = -INF;
    for (TimingArcSet* arc_set : cell->timingArcSets()) {
      if (!arc_set->role()->isTimingCheck()) {
        for (TimingArc* arc : arc_set->arcs()) {
          sta::LoadPinIndexMap load_pin_index_map(network_);
          ArcDcalcResult dcalc_result
              = arc_delay_calc_->gateDelay(nullptr,
                                           arc,
                                           in_slew,
                                           load_cap,
                                           nullptr,
                                           load_pin_index_map,
                                           corner_,
                                           max_);
          const Slew& drvr_slew = dcalc_result.drvrSlew();
          slew = std::max(slew, drvr_slew);
        }
      }
    }
    return slew - max_slew;
  };

  double drvr_res = outp->driveResistance();
  if (drvr_res == 0.0) {
    return INF;
  }

  // cap1 lower bound
  // cap2 upper bound
  double cap1 = 0.0;
  double cap2 = max_slew / drvr_res * 2;
  double tol = .01;  // 1%
  double diff1 = objective(cap2);
  // binary search for diff = 0.
  while (abs(cap1 - cap2) > std::max(cap1, cap2) * tol) {
    if (diff1 < 0.0) {
      cap1 = cap2;
      cap2 *= 2;
      diff1 = objective(cap2);
    } else {
      double cap3 = (cap1 + cap2) / 2.0;
      double diff2 = objective(cap3);
      if (diff2 < 0.0) {
        cap1 = cap3;
      } else {
        cap2 = cap3;
        diff1 = diff2;
      }
    }
  }
  return cap1;
}

// Needs to be called when the margins (slew_margin_, cap_margin_) change
void Rebuffer::characterizeBufferLimits()
{
  for (auto& size : buffer_sizes_) {
    sta::LibertyPort *in, *out;
    size.cell->bufferPorts(in, out);

    bool cap_limit_exists;
    float cap_limit;
    out->capacitanceLimit(max_, cap_limit, cap_limit_exists);

    size.margined_max_cap
        = std::min(cap_limit_exists ? maxCapMargined(cap_limit) : INF,
                   findBufferLoadLimitImpliedByDriverSlew(size.cell));
  }
}

static bool isPortBuffer(sta::dbNetwork* network, sta::Instance* inst)
{
  if (network->libertyCell(inst) && network->libertyCell(inst)->isBuffer()) {
    odb::dbInst* db_inst = network->staToDb(inst);
    for (odb::dbITerm* iterm : db_inst->getITerms()) {
      if (iterm->getNet() && iterm->getNet()->getITerms().size() == 1
          && !iterm->getNet()->getBTerms().empty()) {
        return true;
      }
    }
  }

  return false;
}

BnetPtr Rebuffer::importBufferTree(const sta::Pin* drvr_pin,
                                   const sta::Scene* corner)
{
  BufferedNetPtr tree = resizer_->makeBufferedNet(drvr_pin, corner);
  if (!tree) {
    return nullptr;
  }

  return visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> BnetPtr {
        switch (node->type()) {
          case BnetType::wire: {
            auto inner = recurse(node->ref());
            if (inner) {
              return make_shared<BufferedNet>(BnetType::wire,
                                              node->location(),
                                              node->layer(),
                                              inner,
                                              corner,
                                              resizer_,
                                              estimate_parasitics_);
            }
            return nullptr;
          }
          case BnetType::junction: {
            auto left = recurse(node->ref());
            auto right = recurse(node->ref2());
            if (left && right) {
              return make_shared<BufferedNet>(
                  BnetType::junction, node->location(), left, right, resizer_);
            }
            return nullptr;
          }
          case BnetType::load: {
            auto pin = node->loadPin();
            auto port = network_->libertyPort(pin);
            if (!port) {
              return node;
            }
            auto cell = port->libertyCell();
            if (!cell->isBuffer() || !port->direction()->isInput()) {
              return node;
            }

            sta::Instance* inst = network_->instance(pin);
            if (!resizer_->isLogicStdCell(inst)
                || isPortBuffer(db_network_, inst)
                || !resizer_->unbuffer_move_->canRemoveBuffer(inst, true)) {
              return node;
            }

            sta::LibertyPort *in_port, *out_port;
            cell->bufferPorts(in_port, out_port);
            auto buffer_drvr_pin
                = network_->findPin(network_->instance(pin), out_port);

            if (!buffer_drvr_pin) {
              return node;
            }

            auto inner_bnet = importBufferTree(buffer_drvr_pin, corner);
            sta::Vertex* buffer_drvr = graph_->pinDrvrVertex(buffer_drvr_pin);

            if (!inner_bnet) {
              if (fanout(buffer_drvr) != 0) {
                logger_->error(RSZ,
                               2002,
                               "failed bnet construction for {}",
                               network_->name(pin));
              }
              return node;
            }

            return make_shared<BufferedNet>(BnetType::buffer,
                                            node->location(),
                                            cell,
                                            inner_bnet,
                                            corner,
                                            resizer_,
                                            estimate_parasitics_);
          }
          default:
            logger_->critical(RSZ, 1003, "unhandled BufferedNet type");
        }
      },
      tree);
}

static FixedDelay criticalPathDelay(utl::Logger* logger, const BnetPtr& root)
{
  FixedDelay worst_load_slack = FixedDelay::INF;
  visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> int {
        switch (node->type()) {
          case BnetType::wire:
          case BnetType::buffer:
            return recurse(node->ref());
          case BnetType::junction:
            return recurse(node->ref()) + recurse(node->ref2());
          case BnetType::load:
            if (node->slack() < worst_load_slack) {
              worst_load_slack = node->slack();
            }
            return 1;
          default:
            logger->critical(RSZ, 1007, "unhandled BufferedNet type");
        }
      },
      root);
  return worst_load_slack - root->slack();
}

std::vector<sta::Instance*> Rebuffer::collectImportedTreeBufferInstances(
    sta::Pin* drvr_pin,
    const BnetPtr& imported_tree)
{
  std::set<const sta::Pin*> imported_as_loads;
  visitTree(
      [&](auto& recurse, int level, const BnetPtr& node) -> int {
        switch (node->type()) {
          case BnetType::wire:
          case BnetType::buffer:
            return recurse(node->ref());
          case BnetType::junction:
            return recurse(node->ref()) + recurse(node->ref2());
          case BnetType::load:
            imported_as_loads.insert(node->loadPin());
            return 1;
          default:
            abort();
        }
      },
      imported_tree);

  std::vector<sta::Instance*> insts;
  sta::Net* net = network_->net(drvr_pin);
  if (!net) {
    return {};
  }
  std::vector<sta::Net*> queue = {net};

  while (!queue.empty()) {
    sta::Net* net_ = queue.back();
    sta::NetConnectedPinIterator* pin_iter
        = network_->connectedPinIterator(net_);
    queue.pop_back();

    while (pin_iter->hasNext()) {
      const sta::Pin* pin = pin_iter->next();
      const sta::LibertyPort* port = network_->libertyPort(pin);
      if (port && port->libertyCell()->isBuffer()) {
        sta::LibertyPort *in, *out;
        port->libertyCell()->bufferPorts(in, out);

        if (port == in && !imported_as_loads.contains(pin)) {
          sta::Instance* inst = network_->instance(pin);
          insts.push_back(inst);
          const sta::Pin* out_pin = network_->findPin(inst, out);
          if (out_pin) {
            queue.push_back(network_->net(out_pin));
          }
        }
      }
    }
    delete pin_iter;
  }

  return insts;
}

int Rebuffer::exportBufferTree(const BufferedNetPtr& choice,
                               sta::Net* net,  // Original Driver Net (flat)
                               int level,
                               sta::Instance* parent_in,
                               const char* instance_base_name)
{
  // Algorithm: Bottom-Up Buffer Tree Insertion
  //
  // This recursive lambda traverses the BufferedNet tree in a bottom-up manner
  // to insert buffers while handling hierarchical connections properly.
  //
  // The algorithm works as follows:
  // 1. For leaf nodes (load), collect the physical terminal
  // (odb::dbITerm/dbBTerm)
  //    and return it as a load to the parent.
  // 2. For wire/junction nodes, recursively process children and propagate
  //    their loads upward.
  // 3. For buffer nodes:
  //    a) First, recursively collect all loads from the subtree (child_loads)
  //    b) Insert a buffer that drives those child_loads using
  //       insertBufferBeforeLoads()
  //    c) Return the buffer's input terminal as a load to the parent,
  //       effectively "intercepting" the connection
  //
  // The key insight is that we build from leaves to root, so when we insert
  // a buffer, all its downstream loads are already identified. The buffer's
  // input then becomes a load for the next level up, creating a chain of
  // buffers and hierarchical connections as specified by the BufferedNet tree.
  //
  // Return: A set of dbObjects (odb::dbITerm, dbBTerm, or dbModITerm)
  // representing
  //         the terminals that this subtree drives (i.e., the "loads" seen
  //         from the parent's perspective).
  std::function<void(const BufferedNetPtr&, sta::PinSet&, int&)> insertBuffers;

  insertBuffers = [&](const BufferedNetPtr& node,
                      sta::PinSet& current_loads,
                      int& count) {
    switch (node->type()) {
      case BufferedNetType::via:
      case BufferedNetType::wire: {
        insertBuffers(node->ref(), current_loads, count);
        break;
      }

      case BufferedNetType::junction: {
        insertBuffers(node->ref(), current_loads, count);
        insertBuffers(node->ref2(), current_loads, count);
        break;
      }

      case BufferedNetType::load: {
        const sta::Pin* load_pin = node->loadPin();

        // Skip DontTouch pins
        if (resizer_->dontTouch(load_pin)) {
          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     3,
                     "exportBufferTree: skipped load {} due to dont touch",
                     sdc_network_->pathName(load_pin));
          return;
        }

        // Collect physical terminal
        if (load_pin != nullptr) {
          assert(db_network_->staToDb(load_pin) != nullptr);
          current_loads.insert(load_pin);
        }
        break;
      }

      case BufferedNetType::buffer: {
        // 1. Collect loads from children first (Bottom-Up)
        sta::PinSet child_loads(network_);
        insertBuffers(node->ref(), child_loads, count);

        if (child_loads.empty()) {
          return;
        }

        // 2. Insert Buffer
        // Note: We always pass 'net' (the original driver net) because
        // as we build bottom-up, the 'input' of the subtree we just processed
        // is attached to the original driver net until this buffer intercepts
        // it.
        sta::LibertyCell* buffer_cell = node->bufferCell();

        // In this rebuffer logic, target loads can be on different dbNets.
        // So we pass 'true' to 'loads_on_diff_nets' argument.
        odb::Point buffer_loc = node->location();
        odb::dbInst* buf_inst = db_network_->staToDb(
            resizer_->insertBufferBeforeLoads(net,
                                              &child_loads,
                                              buffer_cell,
                                              &buffer_loc,
                                              instance_base_name,
                                              kDefaultNetBaseName,
                                              odb::dbNameUniquifyType::ALWAYS,
                                              true /*loads_on_diff_nets*/));

        if (buf_inst) {
          count++;
          resizer_->level_drvr_vertices_valid_ = false;

          sta::LibertyPort *input, *output;
          buffer_cell->bufferPorts(input, output);

          // 3. The input of this new buffer becomes the load for the parent
          // node
          odb::dbITerm* input_term = buf_inst->findITerm(input->name());
          if (input_term) {
            sta::Pin* sta_input_pin = db_network_->dbToSta(input_term);
            current_loads.insert(sta_input_pin);
          }

          debugPrint(logger_,
                     RSZ,
                     "rebuffer",
                     3,
                     "insert {} ({}) -> {} loads",
                     buf_inst->getName(),
                     buffer_cell->name(),
                     child_loads.size());

          // Print each load pin's full hierarchical name
          if (logger_->debugCheck(RSZ, "rebuffer", 3)) {
            for (const sta::Pin* load_pin : child_loads) {
              if (load_pin) {
                debugPrint(logger_,
                           RSZ,
                           "rebuffer",
                           3,
                           "  load pin: {}",
                           sdc_network_->pathName(load_pin));
              }
            }
          }
        } else {
          // No need to issue a WARNING or ERROR because a proper
          // WARNING or ERROR has been issued already by insertBuffer API.

          // If insertion failed, pass the child loads up so connectivity isn't
          // lost
          current_loads.insert(child_loads.begin(), child_loads.end());
        }
        break;
      }
    }
  };

  int inserted_count = 0;
  sta::PinSet top_loads(network_);

  // Start the bottom-up traversal
  insertBuffers(choice, top_loads, inserted_count);

  return inserted_count;
}

void Rebuffer::printProgress(int iteration,
                             bool force,
                             bool end,
                             int remaining) const
{
  const bool start = iteration == 0;

  if (start && !end) {
    logger_->report("   Iter   |    Area   | Removed | Inserted |   Pins");
    logger_->report("          |           | Buffers | Buffers  | Remaining");
    logger_->report("-------------------------------------------------------");
  }

  if (iteration % print_interval_ == 0 || force || end) {
    const double design_area = resizer_->computeDesignArea();
    const double area_growth = design_area - initial_design_area_;
    double area_growth_percent = std::numeric_limits<double>::infinity();
    if (std::abs(initial_design_area_) > 0.0) {
      area_growth_percent = area_growth / initial_design_area_ * 100.0;
    }

    logger_->report("{: >9s} | {: >+8.1f}% | {: >7d} | {: >8d} | {: >9d}",
                    end ? "final" : std::to_string(iteration),
                    area_growth_percent,
                    removed_count_,
                    inserted_count_,
                    remaining);
  }

  if (end) {
    logger_->report("-------------------------------------------------------");
  }
}

int Rebuffer::fanout(sta::Vertex* vertex) const
{
  int fanout = 0;
  VertexOutEdgeIterator edge_iter(vertex, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();
    // Disregard output->output timing arcs
    if (edge->isWire()) {
      fanout++;
    }
  }
  return fanout;
}

void Rebuffer::setPin(sta::Pin* drvr_pin)
{
  // set rebuffering globals
  pin_ = drvr_pin;
  drvr_port_ = network_->libertyPort(drvr_pin);
  drvr_load_high_water_mark_ = 0;

  {
    float fanout, max_fanout, fanout_slack;
    sta_->checkFanout(
        drvr_pin, corner_->mode(), max_, fanout, max_fanout, fanout_slack);
    if (max_fanout > 0.0) {
      fanout_limit_ = max_fanout;
    } else {
      fanout_limit_ = INF;
    }

    float max_slew;
    bool max_slew_exists = false;
    sta_->findSlewLimit(
        drvr_port_, corner_, sta::MinMax::max(), max_slew, max_slew_exists);
    if (max_slew_exists) {
      drvr_pin_max_slew_ = maxSlewMargined(max_slew);
    } else {
      drvr_pin_max_slew_ = INF;
    }
  }
}

void Rebuffer::fullyRebuffer(sta::Pin* user_pin)
{
  double sta_runtime = 0, bft_runtime = 0, ra_runtime = 0;
  long_wire_stepping_runtime_ = 0;

  init();
  resizer_->ensureLevelDrvrVertices();

  std::vector<sta::Pin*> filtered_pins;
  for (auto drvr : resizer_->level_drvr_vertices_) {
    sta::Pin* drvr_pin = drvr->pin();
    sta::Net* net = nullptr;
    odb::dbNet* net_db = nullptr;

    // Get the flat net safely
    if (network_->isTopLevelPort(drvr_pin)) {
      net = network_->net(network_->term(drvr_pin));
      net_db = db_network_->flatNet(network_->term(drvr_pin));
    } else {
      net_db = db_network_->flatNet(drvr_pin);
      net = db_network_->dbToSta(net_db);
    }

    if (net && !resizer_->dontTouch(net) && !net_db->isConnectedByAbutment()
        && !net_db->isSpecial() && net_db->getSigType() == dbSigType::SIGNAL
        && !sta_->isClock(drvr_pin, sta_->cmdMode())
        && !sta_->isClockSrc(drvr_pin, sta_->cmdMode()->sdc())
        // Exclude tie hi/low cells and supply nets.
        && !sta_->isConstant(drvr_pin, sta_->cmdMode())
        && !resizer_->isTristateDriver(drvr_pin)) {
      sta::Instance* inst = network_->instance(drvr_pin);

      if (inst && network_->libertyCell(inst)
          && (!network_->libertyCell(inst)->isBuffer()
              || !resizer_->isLogicStdCell(inst)
              || isPortBuffer(db_network_, inst))) {
        filtered_pins.push_back(drvr_pin);
      }
    }
  }

  std::ranges::reverse(filtered_pins);

  if (user_pin) {
    filtered_pins = {user_pin};
  }
  sta_->findRequireds();

  initial_design_area_ = resizer_->computeDesignArea();
  print_interval_ = std::max((int) filtered_pins.size() / 10, 1);
  removed_count_ = 0;
  inserted_count_ = 0;

  if (print_interval_ >= 1000) {
    print_interval_ = (print_interval_ / 100) * 100;
  } else if (print_interval_ >= 100) {
    print_interval_ = (print_interval_ / 10) * 10;
  }

  initOnCorner(sta_->cmdScene());
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);

  for (auto iter = 0; iter < filtered_pins.size(); iter++) {
    printProgress(iter, false, false, filtered_pins.size() - iter);

    sta::Pin* drvr_pin = filtered_pins[iter];

    odb::dbNet* db_net = nullptr;
    if (network_->isTopLevelPort(drvr_pin)) {
      db_net = db_network_->flatNet(network_->term(drvr_pin));
    } else {
      db_net = db_network_->flatNet(drvr_pin);
    }

    sta::Vertex* drvr = graph_->pinDrvrVertex(drvr_pin);

    {
      utl::DebugScopedTimer timer(sta_runtime);
      search_->findRequireds(drvr->level());
    }

    // set rebuffering globals
    setPin(drvr_pin);

    debugPrint(logger_,
               RSZ,
               "rebuffer",
               1,
               "[iter {}] buffering pin {} max_fanout={} max_slew={}",
               iter,
               network_->name(drvr_pin),
               fanout_limit_,
               delayAsString(drvr_pin_max_slew_, this, 3));

    BnetPtr original_tree = importBufferTree(drvr_pin, corner_);
    if (!original_tree) {
      if (fanout(drvr) != 0) {
        logger_->error(RSZ,
                       2001,
                       "failed bnet construction for {}",
                       network_->name(drvr_pin));
      }
      continue;
    }

    bool debug = (drvr_pin == resizer_->debug_pin_);
    if (debug) {
      logger_->setDebugLevel(RSZ, "rebuffer", 4);
    }

    annotateLoadSlacks(original_tree, drvr);
    annotateTiming(original_tree);

    sta::Slack slack = slackAtDriverPin(original_tree).toSeconds();
    sta::Path* req_path = sta_->vertexWorstSlackPath(drvr, sta::MinMax::max());
    sta::Slack sta_slack
        = req_path ? (req_path->required() - req_path->arrival()) : INF;

    // The slack estimation error as observed on the original tree: we have both
    // the full STA figure and our timing model estimate and we can compare
    sta::Delay original_tree_slack_error = slack - sta_slack;

    BnetPtr unbuffered_tree = resteiner(stripTreeBuffers(original_tree));
    BnetPtr timing_tree = unbuffered_tree;

    {
      utl::DebugScopedTimer timer(bft_runtime);
      for (int i = 0; i < 3; i++) {
        timing_tree = bufferForTiming(timing_tree);
        if (!timing_tree) {
          logger_->warn(
              RSZ,
              2005,
              "cannot find a viable buffering solution on pin {} after {} "
              "rounds of buffering (no solution meets design rules)",
              network_->name(drvr_pin),
              i + 1);
          break;
        }
      }
    }

    if (!timing_tree) {
      continue;
    }

    sta::Delay drvr_gate_delay;
    std::tie(drvr_gate_delay, std::ignore, std::ignore)
        = drvrPinTiming(timing_tree);

    // At this point `timing_tree` holds a timing-optimized solution. Now we
    // move onto area recovery. We compute `relaxation` which is the total
    // amount by which the slack is allowed to decrease as we are searching for
    // lower area solutions.
    //
    // There are two contributions:
    //
    //  - If we are projecting positive slack, we trade away quarter of it for
    //    area. If we observed on `original_tree` that our timing model was too
    //    optimistic compared to full STA, we compensate and reduce the
    //    relaxation by the error amount.
    //
    //  - No matter the projected slack, we always add a small amount
    //    (`relaxation_factor_`) of the overall projected critical path delay.
    //    This helps with not introducing buffers for marginal benefit, or to no
    //    actual benefit due to tiny timing model discrepancy. Esp. if we are
    //    keeping buffers from previous rounds of rebuffering this helps with
    //    edge cases where we cyclically insert buffers on non-critical
    //    branches. (Because the load from a newly inserted buffer and its
    //    connecting wire is evaluated as smaller by a minuscule amount from
    //    that of an existing buffer/wire. This can happen because, in the
    //    computation of wire load, we are using pin position for the existing
    //    buffer, but instance position for the new buffer.)
    //

    FixedDelay target_slack = -FixedDelay::INF;

    // Check the tree isn't fully unconstrained
    if (timing_tree->slackTransition() != nullptr) {
      sta::Delay relaxation
          = std::max<float>(0.0f,
                            ((slackAtDriverPin(timing_tree).toSeconds())
                             - std::min(original_tree_slack_error, 0.0f)))
                / 4.0f
            + (std::max(drvr_gate_delay, 0.0f)
               + criticalPathDelay(logger_, timing_tree).toSeconds())
                  * relaxation_factor_;
      target_slack
          = slackAtDriverPin(timing_tree) - FixedDelay(relaxation, resizer_);
    }

    BnetPtr area_opt_tree = timing_tree;
    {
      utl::DebugScopedTimer timer(ra_runtime);
      for (int i = 0; i < 5 && area_opt_tree; i++) {
        area_opt_tree
            = recoverArea(area_opt_tree, target_slack, ((float) (1 + i)) / 5);
      }
    }

    if (!area_opt_tree) {
      printProgress(iter, true, false, filtered_pins.size() - iter - 1);
      logger_->error(RSZ, 2004, "failed area recovery");
      break;
    }

    sta::Instance* parent = db_network_->getOwningInstanceParent(drvr_pin);
    odb::dbITerm* drvr_op_iterm = nullptr;
    odb::dbBTerm* drvr_op_bterm = nullptr;
    odb::dbModITerm* drvr_op_moditerm = nullptr;
    db_network_->staToDb(
        drvr_pin, drvr_op_iterm, drvr_op_bterm, drvr_op_moditerm);

    auto insts = collectImportedTreeBufferInstances(drvr_pin, unbuffered_tree);

    inserted_count_ += exportBufferTree(
        area_opt_tree, db_network_->dbToSta(db_net), 1, parent, "place");

    for (auto* inst : insts) {
      resizer_->unbuffer_move_->removeBuffer(inst);
      removed_count_++;
    }

    sta_->ensureLevelized();
    sta::Level max_level = 0;
    visitTree(
        [&](auto& recurse, int level, const BnetPtr& bnet) -> int {
          switch (bnet->type()) {
            case BnetType::wire:
              return recurse(bnet->ref());
            case BnetType::junction:
              return recurse(bnet->ref()) + recurse(bnet->ref2());
            case BnetType::load:
              max_level = std::max(
                  max_level, graph_->pinDrvrVertex(bnet->loadPin())->level());
              return 1;
            case BnetType::buffer: {
              return recurse(bnet->ref());
            }
            default:
              abort();
          }
        },
        unbuffered_tree);

    estimate_parasitics_->updateParasitics();

    {
      utl::DebugScopedTimer timer(sta_runtime);
      sta_->findDelays(max_level);
      search_->findArrivals(max_level);
    }

    debugPrint(logger_, RSZ, "rebuffer", 2, "-------------------------------");

    if (debug) {
      logger_->setDebugLevel(RSZ, "rebuffer", 0);
    }
  }

  printProgress(filtered_pins.size(), false, true, 0);
  resizer_->level_drvr_vertices_valid_ = false;

  debugPrint(logger_, RSZ, "rebuffer", 1, "Time spent");
  debugPrint(logger_, RSZ, "rebuffer", 1, "----------");
  debugPrint(logger_, RSZ, "rebuffer", 1, "STA {:.2f}", sta_runtime);
  debugPrint(
      logger_, RSZ, "rebuffer", 1, "Buffer for timing {:.2f}", bft_runtime);
  debugPrint(logger_,
             RSZ,
             "rebuffer",
             1,
             "  of which long wire stepping {:.2f}",
             long_wire_stepping_runtime_);
  debugPrint(logger_, RSZ, "rebuffer", 1, "Recover area {:.2f}", ra_runtime);
}

bool Rebuffer::hasTopLevelOutputPort(sta::Net* net)
{
  sta::NetConnectedPinIterator* pin_iter = network_->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const sta::Pin* pin = pin_iter->next();
    if (network_->isTopLevelPort(pin) && network_->direction(pin)->isOutput()) {
      delete pin_iter;
      return true;
    }
  }
  delete pin_iter;
  return false;
}

int Rebuffer::rebufferPin(const sta::Pin* drvr_pin)
{
  if (network_->isTopLevelPort(drvr_pin)) {
    logger_->warn(RSZ,
                  2020,
                  "rebuffering does not support top port as the driver pin: {}",
                  network_->name(drvr_pin));
    return 0;
  }

  sta::Net* const net = network_->net(drvr_pin);
  odb::dbNet* const db_net = db_network_->flatNet(drvr_pin);

  drvr_port_ = network_->libertyPort(drvr_pin);
  if (net && drvr_port_ &&
      // Verilog connects by net name, so there is no way to distinguish the
      // net from the port.
      !hasTopLevelOutputPort(net)) {
    setPin(const_cast<sta::Pin*>(drvr_pin));
    BufferedNetPtr bnet = resizer_->makeBufferedNet(drvr_pin, corner_);

    if (!bnet) {
      logger_->warn(RSZ,
                    75,
                    "makeBufferedNet failed for driver {}",
                    network_->pathName(drvr_pin));
      return 0;
    }

    const bool debug = (drvr_pin == resizer_->debug_pin_);
    if (debug) {
      logger_->setDebugLevel(RSZ, "rebuffer", 4);
    }
    debugPrint(logger_,
               RSZ,
               "rebuffer",
               2,
               "driver {}",
               sdc_network_->pathName(drvr_pin));

    sta::Vertex* drvr = graph_->pinDrvrVertex(drvr_pin);

    sta_->findRequireds();
    annotateLoadSlacks(bnet, drvr);

    const bool allow_topology_rewrite
        = (estimate_parasitics_->getParasiticsSrc()
           == est::ParasiticsSrc::placement);

    for (int i = 0; i < 3; i++) {
      bnet = bufferForTiming(bnet, allow_topology_rewrite);
      if (!bnet) {
        logger_->warn(
            RSZ,
            2021,
            "cannot find a viable buffering solution on pin {} after {} "
            "rounds of buffering (no solution meets design rules)",
            network_->name(drvr_pin),
            i + 1);
        break;
      }
    }

    if (!bnet) {
      return 0;
    }

    sta::Delay drvr_gate_delay;
    std::tie(drvr_gate_delay, std::ignore, std::ignore) = drvrPinTiming(bnet);
    sta::Delay relaxation = (std::max(drvr_gate_delay, 0.0f)
                             + criticalPathDelay(logger_, bnet).toSeconds())
                            * relaxation_factor_;

    FixedDelay target
        = slackAtDriverPin(bnet) - FixedDelay(relaxation, resizer_);

    for (int i = 0; i < 5 && bnet; i++) {
      bnet = recoverArea(bnet, target, ((float) (1 + i)) / 5);
    }

    if (!bnet) {
      logger_->error(RSZ, 2022, "failed area recovery");
      return 0;
    }

    sta::Instance* parent
        = db_network_->getOwningInstanceParent(const_cast<sta::Pin*>(drvr_pin));
    odb::dbITerm* drvr_op_iterm = nullptr;
    odb::dbBTerm* drvr_op_bterm = nullptr;
    odb::dbModITerm* drvr_op_moditerm = nullptr;
    db_network_->staToDb(
        drvr_pin, drvr_op_iterm, drvr_op_bterm, drvr_op_moditerm);

    int inserted_count;
    inserted_count = exportBufferTree(
        bnet, db_network_->dbToSta(db_net), 1, parent, "rebuffer");

    if (inserted_count > 0) {
      resizer_->level_drvr_vertices_valid_ = false;
    }

    debugPrint(logger_, RSZ, "rebuffer", 2, "-------------------------------");

    return inserted_count;
  }

  return 0;
}

// Return inserted buffer count.
int BufferMove::rebuffer(const sta::Pin* drvr_pin)
{
  return resizer_->rebuffer_->rebufferPin(drvr_pin);
}

// For testing.
void BufferMove::rebufferNet(const sta::Pin* drvr_pin)
{
  auto& rebuffer = resizer_->rebuffer_;
  rebuffer->init();
  rebuffer->initOnCorner(sta_->cmdScene());
  est::IncrementalParasiticsGuard guard(estimate_parasitics_);
  int inserted_buffer_count = rebuffer->rebufferPin(drvr_pin);
  logger_->report("Inserted {} buffers.", inserted_buffer_count);
}

};  // namespace rsz
