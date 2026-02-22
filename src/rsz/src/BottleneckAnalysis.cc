// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2026, The OpenROAD Authors

#include "BottleneckAnalysis.hh"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"
#include "sta/Corner.hh"
#include "sta/DcalcAnalysisPt.hh"
#include "sta/Delay.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/MinMax.hh"
#include "sta/NetworkClass.hh"
#include "sta/Path.hh"
#include "sta/PathAnalysisPt.hh"
#include "sta/PortDirection.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/SearchPred.hh"
#include "sta/TimingArc.hh"
#include "sta/TimingRole.hh"
#include "utl/Logger.h"

using namespace sta;  // NOLINT

using utl::RSZ;

namespace rsz {

// The code can compute a "bottleneck metric" on each driving pin
// in the design.
//
// The metric is defined as
//
//   bottleneck(pin) = log[ sum exp(-p_slack / alpha) ]
//
// where the sum is over all paths going through `pin`, `p_slack` is the slack
// on each path, and `alpha` is a parameter. The motivation is that if a pin
// is a bottleneck, there will be many paths going through the pin each with
// a low value of slack.
//
// Directly applying the above formula would be intractable for large designs.
// If we simplify the timing model such that there's a single arrival value
// for each vertex (no rise/fall, clock domain or false path state distinction),
// then we can compute the bottleneck metric on all pins in time linear to
// the size of the design.
//
// The algorithm is the following: for every pin we define
//
//  fwd(pin) = sum1 exp(p_arrival / alpha)
//  bwd(pin) = sum2 exp(-p_required / alpha)
//
// where sum1 is over paths starting at any proper startpoint and terminating
// in `pin`, and sum2 is over paths starting at `pin` and terminating in
// a proper endpoint. `p_arrival` is the arrival time at the end of a path and
// `p_required` is the required time at the start of a path.
//
// fwd(pin) can be computed for all pins in a single forward pass by looking
// at the fwd() value on fanins. Similarly bwd(pin) can be computed in a single
// backward pass by looking at bwd() of fanouts. Finally the bottleneck metric
// is computed with
//
//   bottleneck(pin) = log[ fwd(pin) * bwd(pin) ]
//
// Notes:
//
//  * To apply the linear algorithm we synthesize a simplified timing view
//    where each vertex has a single arrival time. We attempt to do this with
//    minimal loss of information and the details are in the code. The
//    simplified view should be strictly pessimistic: On any path, compared to
//    full-fledged STA, the simplified view should always give the same or worse
//    slack, and on the very worst path the slacks should be matching. This
//    holds up in the presence of multiple clock domains and false paths.
//
//  * We work with "log fwd(pin)" and "log bwd(pin)" for improved numerical
//    stability.

BottleneckAnalysis::BottleneckAnalysis(Resizer* resizer) : resizer_(resizer)
{
}

BottleneckAnalysis::~BottleneckAnalysis() = default;

bool BottleneckAnalysis::isTimingUnconstrained(Vertex* vertex)
{
  return sta_->vertexSlack(vertex, sta::MinMax::max()) >= sta::INF / 2;
}

bool BottleneckAnalysis::launchesPath(Vertex* drvr)
{
  Pin* drvr_pin = drvr->pin();
  LibertyCell* cell = network_->libertyCell(network_->instance(drvr_pin));
  return drvr->isDriver(network_) && (!cell || cell->hasSequentials());
}

Arrival BottleneckAnalysis::referenceTime(Path* path)
{
  if (!launchesPath(path->vertex(sta_))) {
    return (path->arrival() + path->required()) / 2;
  }
  return path->arrival();
}

Edge* BottleneckAnalysis::prevEdge(Vertex* vertex)
{
  VertexInEdgeIterator edge_iter(vertex, graph_);
  return edge_iter.hasNext() ? edge_iter.next() : nullptr;
}

void BottleneckAnalysis::encodePropagation(Vertex* drvr)
{
  DriverPinData driver_pin;

  if (isTimingUnconstrained(drvr)) {
    return;
  }

  std::map<int, int> fanin_cache;

  SearchPredNonLatch2 search_pred(sta_);
  VertexInEdgeIterator edge_iter(drvr, graph_);
  while (edge_iter.hasNext()) {
    Edge* edge = edge_iter.next();

    if (!search_pred.searchThru(edge)) {
      continue;
    }

    if (isTimingUnconstrained(edge->from(graph_))) {
      continue;
    }

    // Workaround for asap7 output->output edges
    if (edge->from(graph_)->isDriver(network_)) {
      continue;
    }

    const TimingRole* role = edge->timingArcSet()->role();
    Edge* prev_edge = prevEdge(edge->from(graph_));
    Vertex* prev_drvr = prev_edge->from(graph_);

    Delay max_delay = -sta::INF;

    if (!role->isTimingCheck() && role != TimingRole::tristateDisable()
        && role != TimingRole::tristateEnable()
        && role != TimingRole::clockTreePathMin()
        && role != TimingRole::clockTreePathMax() && prev_drvr != nullptr) {
      VertexPathIterator prev_drvr_path_iter(prev_drvr, sta_);
      while (prev_drvr_path_iter.hasNext()) {
        Path* prev_drvr_path = prev_drvr_path_iter.next();
        Tag* prev_drvr_tag = prev_drvr_path->tag(sta_);
        PathAnalysisPt* path_ap = prev_drvr_path->pathAnalysisPt(sta_);
        DcalcAnalysisPt* dcalc_ap = prev_drvr_path->dcalcAnalysisPt(sta_);

        TimingArcSet* wire_arc_set = TimingArcSet::wireTimingArcSet();
        TimingArc* prev_arc = wire_arc_set->arcs()[TimingArcSet::wireArcIndex(
            prev_drvr_path->transition(sta_))];
        ArcDelay prev_arc_delay
            = graph_->arcDelay(prev_edge, prev_arc, dcalc_ap->index());

        Tag* from_tag = search_->thruTag(prev_drvr_tag,
                                         prev_edge,
                                         prev_drvr_path->transition(sta_),
                                         prev_drvr_path->minMax(sta_),
                                         path_ap,
                                         nullptr);
        Path* from_path
            = from_tag ? Path::vertexPath(edge->from(graph_), from_tag, sta_)
                       : nullptr;

        if (from_path && dcalc_ap->delayMinMax() == sta::MinMax::max()) {
          for (TimingArc* arc : edge->timingArcSet()->arcs()) {
            if (arc->fromEdge()->asRiseFall() == from_path->transition(sta_)) {
              Tag* to_tag = search_->thruTag(from_tag,
                                             edge,
                                             arc->toEdge()->asRiseFall(),
                                             from_path->minMax(sta_),
                                             path_ap,
                                             nullptr);

              if (to_tag) {
                Path* to_path
                    = Path::vertexPath(edge->to(graph_), to_tag, sta_);
                if (to_path) {
                  ArcDelay arc_delay
                      = graph_->arcDelay(edge, arc, dcalc_ap->index());
                  max_delay
                      = std::max(max_delay,
                                 referenceTime(prev_drvr_path) + prev_arc_delay
                                     + arc_delay - referenceTime(to_path));
                }
              }
            }
          }
        }
      }
    }

    debugPrint(logger_,
               RSZ,
               "bottleneck",
               7,
               "{}->{}->{} max_delay {}",
               prev_drvr->name(network_),
               edge->from(graph_)->name(network_),
               edge->to(graph_)->name(network_),
               max_delay);

    int fi = drvr_indices_.at(prev_drvr);
    if (fanin_cache.contains(fi)) {
      double& max_delay2 = driver_pin.fanins[fanin_cache.at(fi)].second;
      max_delay2 = std::max(max_delay2, (double) max_delay);
    } else {
      fanin_cache[fi] = driver_pin.fanins.size();
      driver_pin.fanins.emplace_back(fi, max_delay);
    }
  }

  driver_pin.pin = drvr->pin();
  drvr_indices_[drvr] = driver_pins_.size();
  driver_pins_.push_back(driver_pin);
}

void BottleneckAnalysis::encodeLaunching(Vertex* drvr)
{
  DriverPinData driver_pin;
  driver_pin.launcher = true;
  driver_pin.pin = drvr->pin();
  drvr_indices_[drvr] = driver_pins_.size();
  driver_pins_.push_back(driver_pin);
}

void BottleneckAnalysis::encodeSampling(Vertex* vertex)
{
  if (sta_->vertexSlack(vertex, sta::MinMax::max()) >= sta::INF / 2) {
    return;
  }

  Edge* edge = prevEdge(vertex);
  Delay required = sta::INF;

  if (edge) {
    Vertex* drvr = edge->from(graph_);
    VertexPathIterator drvr_path_iter(drvr, sta_);
    while (drvr_path_iter.hasNext()) {
      Path* drvr_path = drvr_path_iter.next();
      Tag* drvr_tag = drvr_path->tag(sta_);
      PathAnalysisPt* path_ap = drvr_path->pathAnalysisPt(sta_);
      DcalcAnalysisPt* dcalc_ap = drvr_path->dcalcAnalysisPt(sta_);
      TimingArcSet* wire_arc_set = TimingArcSet::wireTimingArcSet();
      TimingArc* arc = wire_arc_set->arcs()[TimingArcSet::wireArcIndex(
          drvr_path->transition(sta_))];
      ArcDelay arc_delay = graph_->arcDelay(edge, arc, dcalc_ap->index());

      Tag* to_tag = search_->thruTag(drvr_tag,
                                     edge,
                                     drvr_path->transition(sta_),
                                     drvr_path->minMax(sta_),
                                     path_ap,
                                     nullptr);
      Path* to_path
          = to_tag ? Path::vertexPath(edge->to(graph_), to_tag, sta_) : nullptr;

      if (to_path && to_path->minMax(sta_) == sta::MinMax::max()) {
        required = std::min(
            required,
            to_path->required() - referenceTime(drvr_path) - arc_delay);
      }
    }

    debugPrint(logger_,
               RSZ,
               "bottleneck",
               7,
               "adding required {} on {}",
               drvr->name(network_),
               required);

    endpoints_.push_back(EndpointData{
        .fanin_index = drvr_indices_.at(edge->from(graph_)),
        .required = required,
    });
  }
}

static float logsumexp(float a, float b)
{
  if (b > a) {
    std::swap(a, b);
  }
  return a + log2(1.0 + exp2(b - a));
}

void BottleneckAnalysis::analyze(double alpha, int npins, bool verbose)
{
  init();
  initOnCorner(sta_->cmdCorner());
  resizer_->level_drvr_vertices_valid_ = false;
  resizer_->ensureLevelDrvrVertices();

  driver_pins_.clear();
  endpoints_.clear();
  drvr_indices_.clear();

  for (auto drvr : resizer_->level_drvr_vertices_) {
    if (!launchesPath(drvr)) {
      encodePropagation(drvr);
    } else {
      encodeLaunching(drvr);
    }
  }

  std::unique_ptr<sta::LeafInstanceIterator> inst_iter(
      network_->leafInstanceIterator());
  while (inst_iter->hasNext()) {
    auto* inst = inst_iter->next();
    LibertyCell* cell = network_->libertyCell(inst);
    if (cell && cell->hasSequentials()) {
      std::unique_ptr<InstancePinIterator> port_iter(
          network_->pinIterator(inst));
      while (port_iter->hasNext()) {
        Pin* pin = port_iter->next();
        if (network_->direction(pin)->isAnyInput()) {
          Vertex* load = graph_->pinLoadVertex(pin);
          encodeSampling(load);
        }
      }
    }
  }

  std::unique_ptr<InstancePinIterator> port_iter(
      network_->pinIterator(network_->topInstance()));
  while (port_iter->hasNext()) {
    Pin* pin = port_iter->next();
    if (network_->direction(pin)->isAnyOutput()) {
      Vertex* load = graph_->pinDrvrVertex(pin);
      encodeSampling(load);
    }
  }

  alpha /= std::numbers::ln2;

  // Foward pass
  for (auto& pin : driver_pins_) {
    pin.bwd_accumulator = -INF;
    if (pin.launcher) {
      pin.fwd_accumulator = 0;
    } else {
      pin.fwd_accumulator = -INF;
      for (auto [fi, arc_delay] : pin.fanins) {
        double fanin_fwd_accumulator
            = fi >= 0 ? driver_pins_[fi].fwd_accumulator : 1.0;
        pin.fwd_accumulator = logsumexp(
            pin.fwd_accumulator, fanin_fwd_accumulator + arc_delay / alpha);
      }
    }
  }

  // Backward pass
  for (auto& endp : endpoints_) {
    if (endp.fanin_index >= 0) {
      driver_pins_[endp.fanin_index].bwd_accumulator
          = logsumexp(driver_pins_[endp.fanin_index].bwd_accumulator,
                      -endp.required / alpha);
    }
  }
  for (int i = (int) driver_pins_.size() - 1; i >= 0; i--) {
    DriverPinData& pin = driver_pins_[i];
    for (auto [fi, arc_delay] : pin.fanins) {
      if (fi >= 0) {
        auto& fanin = driver_pins_[fi];
        fanin.bwd_accumulator = logsumexp(
            fanin.bwd_accumulator, pin.bwd_accumulator + arc_delay / alpha);
      }
    }
  }

  if (verbose) {
    std::vector<int> ordering;
    ordering.reserve(driver_pins_.size());
    for (int i = 0; i < driver_pins_.size(); i++) {
      ordering.push_back(i);
    }
    std::ranges::sort(ordering, [&](int ai, int bi) {
      auto& a = driver_pins_[ai];
      auto& b = driver_pins_[bi];
      return a.fwd_accumulator + a.bwd_accumulator
             > b.fwd_accumulator + b.bwd_accumulator;
    });

    logger_->report("Top {} timing bottlenecks:", npins);
    for (int i = 0; i < std::min((int) ordering.size(), npins); i++) {
      auto& pin = driver_pins_[ordering[i]];
      float value = pin.fwd_accumulator + pin.bwd_accumulator;
      logger_->report(" - {} (metric {})",
                      network_->name(pin.pin),
                      delayAsString(value * alpha, this));
    }
  }
}

float BottleneckAnalysis::pinValue(Pin* pin)
{
  Vertex* pin_vertex = graph_->pinDrvrVertex(pin);
  auto it = drvr_indices_.find(pin_vertex);
  if (it != drvr_indices_.end()) {
    auto& pin = driver_pins_[it->second];
    return pin.fwd_accumulator + pin.bwd_accumulator;
  }
  return 0.0;
}

void BottleneckAnalysis::pinRemoved(Pin* pin)
{
  Vertex* pin_vertex = graph_->pinDrvrVertex(pin);
  auto it = drvr_indices_.find(pin_vertex);
  if (it != drvr_indices_.end()) {
    auto& pin = driver_pins_[it->second];
    drvr_indices_.erase(it);
    pin.pin = nullptr;
  }
}

void BottleneckAnalysis::init()
{
  logger_ = resizer_->logger_;
  dbStaState::init(resizer_->sta_);
  db_network_ = resizer_->db_network_;
  estimate_parasitics_ = resizer_->estimate_parasitics_;
}

void BottleneckAnalysis::initOnCorner(Corner* corner)
{
  corner_ = corner;
}

};  // namespace rsz
