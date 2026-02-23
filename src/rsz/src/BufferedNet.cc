// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "BufferedNet.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "est/EstimateParasitics.h"
#include "est/SteinerTree.h"
#include "grt/GRoute.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Delay.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/Scene.hh"
#include "sta/Transition.hh"
#include "stt/SteinerTreeBuilder.h"
// Use spdlog fmt::format until c++20 that supports std::format.
#include "spdlog/fmt/fmt.h"
#include "sta/Fuzzy.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

// Resizer::makeBufferedNetGroute
#include "db_sta/dbNetwork.hh"
#include "grt/GlobalRouter.h"
#include "grt/PinGridLocation.h"
#include "grt/RoutePt.h"
#include "sta/Hash.hh"

namespace sta {
class Port;
class Pin;
}  // namespace sta

namespace rsz {

using sta::MinMax;
using sta::Pin;
using sta::Port;
using sta::Scene;
using std::make_shared;
using std::max;
using std::min;
using utl::RSZ;

static const char* to_string(BufferedNetType type);

////////////////////////////////////////////////////////////////

const FixedDelay FixedDelay::INF = FixedDelay(100.0f, nullptr);
const FixedDelay FixedDelay::ZERO = FixedDelay(0.0f, nullptr);

FixedDelay::FixedDelay() : value_fs_(0)
{
}

FixedDelay::FixedDelay(sta::Delay float_value, Resizer* resizer)
{
  if (resizer
      && (float_value > (FixedDelay::INF).toSeconds()
          || float_value < (-FixedDelay::INF).toSeconds())) {
    resizer->logger()->error(RSZ,
                             1008,
                             "FixedDelay conversion out of range: {}",
                             delayAsString(float_value, resizer));
  }

  value_fs_ = float_value * second_;
}

////////////////////////////////////////////////////////////////

// load
BufferedNet::BufferedNet(const BufferedNetType type,
                         const odb::Point& location,
                         const sta::Pin* load_pin,
                         const sta::Scene* corner,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::load) {
    resizer->logger()->critical(
        RSZ, 78, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::load;
  location_ = location;
  load_pin_ = load_pin;
  corner_ = corner;

  sta::Network* network = resizer->network();
  sta::LibertyPort* load_port = network->libertyPort(load_pin);
  if (load_port) {
    cap_ = resizer->portCapacitance(load_port, corner);
    fanout_ = resizer->portFanoutLoad(load_port);
    max_load_slew_ = resizer->maxInputSlew(load_port, corner);
  } else if (network->isTopLevelPort(load_pin)) {
    Port* port = network->port(load_pin);
    for (auto rf : sta::RiseFall::range()) {
      float pin_cap, wire_cap;
      int fanout;
      bool has_pin_cap, has_wire_cap, has_fanout;
      corner->sdc()->portExtCap(port,
                                rf,
                                MinMax::max(),
                                pin_cap,
                                has_pin_cap,
                                wire_cap,
                                has_wire_cap,
                                fanout,
                                has_fanout);
      if (has_pin_cap) {
        cap_ = std::max(cap_, pin_cap);
      }
    }
  }
}

// junc
BufferedNet::BufferedNet(const BufferedNetType type,
                         const odb::Point& location,
                         const BufferedNetPtr& ref,
                         const BufferedNetPtr& ref2,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::junction) {
    resizer->logger()->critical(
        RSZ, 79, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::junction;
  location_ = location;
  layer_ = null_layer;
  ref_ = ref;
  ref2_ = ref2;
  corner_ = ref->corner();

  cap_ = ref->cap() + ref2->cap();
  fanout_ = ref->fanout() + ref2->fanout();
  max_load_slew_ = min(ref->maxLoadSlew(), ref2->maxLoadSlew());

  area_ = ref->area() + ref2->area();
}

// wire
BufferedNet::BufferedNet(const BufferedNetType type,
                         const odb::Point& location,
                         const int layer,
                         const BufferedNetPtr& ref,
                         const sta::Scene* corner,
                         const Resizer* resizer,
                         const est::EstimateParasitics* estimate_parasitics)
{
  if (type != BufferedNetType::wire) {
    resizer->logger()->critical(
        RSZ, 80, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::wire;
  location_ = location;
  layer_ = layer;
  ref_ = ref;
  corner_ = corner;

  double wire_res, wire_cap;
  wireRC(corner, resizer, estimate_parasitics, wire_res, wire_cap);
  cap_ = ref->cap() + resizer->dbuToMeters(length()) * wire_cap;
  fanout_ = ref->fanout();
  max_load_slew_ = ref->maxLoadSlew();

  area_ = ref->area();
}

// via
BufferedNet::BufferedNet(const BufferedNetType type,
                         const odb::Point& location,
                         const int layer,
                         const int ref_layer,
                         const BufferedNetPtr& ref,
                         const Scene* corner,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::via) {
    resizer->logger()->critical(
        RSZ, 87, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::via;
  location_ = location;
  layer_ = layer;
  ref_layer_ = ref_layer;
  ref_ = ref;
  corner_ = corner;

  cap_ = ref->cap();
  fanout_ = ref->fanout();
  max_load_slew_ = ref->maxLoadSlew();

  area_ = ref->area();
}

// buffer
BufferedNet::BufferedNet(const BufferedNetType type,
                         const odb::Point& location,
                         sta::LibertyCell* buffer_cell,
                         const BufferedNetPtr& ref,
                         const sta::Scene* corner,
                         const Resizer* resizer,
                         const est::EstimateParasitics* estimate_parasitics)
{
  if (type != BufferedNetType::buffer) {
    resizer->logger()->critical(
        RSZ, 81, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::buffer;
  location_ = location;
  buffer_cell_ = buffer_cell;
  layer_ = null_layer;
  ref_ = ref;
  corner_ = corner;

  sta::LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  cap_ = resizer->portCapacitance(input, corner);
  fanout_ = resizer->portFanoutLoad(input);
  max_load_slew_ = resizer->maxInputSlew(input, corner);

  area_ = ref->area() + buffer_cell_->area();
}

void BufferedNet::reportTree(const Resizer* resizer) const
{
  reportTree(0, resizer);
}

void BufferedNet::reportTree(const int level, const Resizer* resizer) const
{
  resizer->logger()->report("{:{}s}{}", "", level, to_string(resizer));
  switch (type_) {
    case BufferedNetType::load:
      break;
    case BufferedNetType::buffer:
    case BufferedNetType::wire:
    case BufferedNetType::via:
      ref_->reportTree(level + 1, resizer);
      break;
    case BufferedNetType::junction:
      ref_->reportTree(level + 1, resizer);
      ref2_->reportTree(level + 1, resizer);
      break;
  }
}

std::string BufferedNet::to_string(const Resizer* resizer) const
{
  sta::Network* sdc_network = resizer->sdcNetwork();
  sta::Units* units = resizer->units();
  sta::Unit* dist_unit = units->distanceUnit();
  const char* x = dist_unit->asString(resizer->dbuToMeters(location_.x()), 2);
  const char* y = dist_unit->asString(resizer->dbuToMeters(location_.y()), 2);
  const char* cap = units->capacitanceUnit()->asString(cap_);

  switch (type_) {
    case BufferedNetType::load:
      // {:{}s} format indents level spaces.
      return fmt::format("load {} ({}, {}) cap {} slack {} load sl {}",
                         sdc_network->pathName(load_pin_),
                         x,
                         y,
                         cap,
                         delayAsString(slack().toSeconds(), resizer),
                         delayAsString(maxLoadSlew(), resizer));
    case BufferedNetType::wire:
      return fmt::format("wire ({}, {}) cap {} slack {} buffers {} load sl {}",
                         x,
                         y,
                         cap,
                         delayAsString(slack().toSeconds(), resizer),
                         bufferCount(),
                         delayAsString(maxLoadSlew(), resizer));
    case BufferedNetType::via:
      return fmt::format(
          "via ({}, {}) layer {} -> {}", x, y, layer_, ref_layer_);
    case BufferedNetType::buffer:
      return fmt::format(
          "buffer ({}, {}) {} cap {} slack {} buffers {} load sl {}",
          x,
          y,
          buffer_cell_->name(),
          cap,
          delayAsString(slack().toSeconds(), resizer),
          bufferCount(),
          delayAsString(maxLoadSlew(), resizer));
    case BufferedNetType::junction:
      return fmt::format(
          "junction ({}, {}) cap {} slack {} buffers {} load sl {}",
          x,
          y,
          cap,
          delayAsString(slack().toSeconds(), resizer),
          bufferCount(),
          delayAsString(maxLoadSlew(), resizer));
  }
  // suppress gcc warning
  return "";
}

int BufferedNet::length() const
{
  return odb::Point::manhattanDistance(location_, ref_->location());
}

void BufferedNet::setCapacitance(float cap)
{
  cap_ = cap;
}

void BufferedNet::setFanout(float fanout)
{
  fanout_ = fanout;
}

void BufferedNet::setMaxLoadSlew(float max_slew)
{
  max_load_slew_ = max_slew;
}

void BufferedNet::setSlackTransition(const sta::RiseFallBoth* transitions)
{
  slack_transitions_ = transitions;
}

void BufferedNet::setSlack(FixedDelay slack)
{
  slack_ = slack;
}

void BufferedNet::setDelay(FixedDelay delay)
{
  delay_ = delay;
}

void BufferedNet::setArrivalDelay(FixedDelay delay)
{
  arrival_delay_ = delay;
}

int BufferedNet::bufferCount() const
{
  switch (type_) {
    case BufferedNetType::buffer:
      return ref_->bufferCount() + 1;
    case BufferedNetType::wire:
    case BufferedNetType::via:
      return ref_->bufferCount();
    case BufferedNetType::junction:
      return ref_->bufferCount() + ref2_->bufferCount();
    case BufferedNetType::load:
      return 0;
  }
  return 0;
}

int BufferedNet::loadCount() const
{
  switch (type_) {
    case BufferedNetType::buffer:
      return ref_->loadCount();
    case BufferedNetType::wire:
    case BufferedNetType::via:
      return ref_->loadCount();
    case BufferedNetType::junction:
      return ref_->loadCount() + ref2_->loadCount();
    case BufferedNetType::load:
      return 1;
  }
  return 0;
}

int BufferedNet::maxLoadWireLength() const
{
  switch (type_) {
    case BufferedNetType::wire:
      return length() + ref_->maxLoadWireLength();
    case BufferedNetType::via:
      return ref_->maxLoadWireLength();
    case BufferedNetType::junction:
      return max(ref_->maxLoadWireLength(), ref2_->maxLoadWireLength());
    case BufferedNetType::load:
      return 0;
    case BufferedNetType::buffer:
      return 0;
  }
  return 0;
}

void BufferedNet::wireRC(const sta::Scene* corner,
                         const Resizer* resizer,
                         const est::EstimateParasitics* estimate_parasitics,
                         // Return values.
                         double& res,
                         double& cap)
{
  if (type_ != BufferedNetType::wire) {
    resizer->logger()->critical(RSZ, 82, "wireRC called for non-wire");
  }
  if (layer_ == BufferedNet::null_layer) {
    if (length() == 0) {
      res = 0;
      cap = 0;
    } else {
      const double dx
          = resizer->dbuToMeters(std::abs(location_.x() - ref_->location().x()))
            / resizer->dbuToMeters(length());
      const double dy
          = resizer->dbuToMeters(std::abs(location_.y() - ref_->location().y()))
            / resizer->dbuToMeters(length());
      res = dx * estimate_parasitics->wireSignalHResistance(corner)
            + dy * estimate_parasitics->wireSignalVResistance(corner);
      cap = dx * estimate_parasitics->wireSignalHCapacitance(corner)
            + dy * estimate_parasitics->wireSignalVCapacitance(corner);
    }
  } else {
    odb::dbTech* tech = resizer->db_->getTech();
    estimate_parasitics->layerRC(
        tech->findRoutingLayer(layer_), corner, res, cap);
  }
}

double BufferedNet::viaResistance(
    const Scene* corner,
    const Resizer* resizer,
    const est::EstimateParasitics* estimate_parasitics)
{
  if (type_ != BufferedNetType::via) {
    resizer->logger()->critical(RSZ, 92, "viaResistance called for non-via");
  }
  double total_resist = 0;
  odb::dbTech* tech = resizer->db_->getTech();
  for (int i = std::min(layer_, ref_layer_); i < std::max(layer_, ref_layer_);
       i++) {
    odb::dbTechLayer* tech_layer = tech->findRoutingLayer(i);
    odb::dbTechLayer* cut_layer
        = tech_layer ? tech_layer->getUpperLayer() : nullptr;
    if (cut_layer) {
      double res = 0.0;
      double cap = 0.0;
      estimate_parasitics->layerRC(cut_layer, corner, res, cap);
      if (res == 0.0) {
        res = cut_layer->getResistance();
      }
      total_resist += res;
    } else {
      resizer->logger()->error(
          RSZ,
          93,
          "cannot find resistance of via layer above routing layer {}",
          i);
    }
  }
  return total_resist;
}

static const char* to_string(const BufferedNetType type)
{
  switch (type) {
    case BufferedNetType::load:
      return "load";
    case BufferedNetType::junction:
      return "junction";
    case BufferedNetType::wire:
      return "wire";
    case BufferedNetType::via:
      return "via";
    case BufferedNetType::buffer:
      return "buffer";
  }
  return "??";
}

////////////////////////////////////////////////////////////////

BufferedNetPtr Resizer::makeBufferedNet(const sta::Pin* drvr_pin,
                                        const sta::Scene* corner)
{
  switch (estimate_parasitics_->getParasiticsSrc()) {
    case est::ParasiticsSrc::placement:
      return makeBufferedNetSteiner(drvr_pin, corner);
    case est::ParasiticsSrc::global_routing:
    case est::ParasiticsSrc::detailed_routing:
      return makeBufferedNetGroute(drvr_pin, corner);
    case est::ParasiticsSrc::none:
      return nullptr;
  }
  return nullptr;
}

using SteinerPtAdjacents = std::vector<std::vector<SteinerPt>>;
using SteinerPtPinVisited
    = std::unordered_set<odb::Point, est::PointHash, est::PointEqual>;

static BufferedNetPtr makeBufferedNetFromTree(
    const est::SteinerTree* tree,
    const est::SteinerPt from,
    const est::SteinerPt to,
    const SteinerPtAdjacents& adjacents,
    const int level,
    SteinerPtPinVisited& pins_visited,
    const sta::Scene* corner,
    const Resizer* resizer,
    const est::EstimateParasitics* estimate_parasitics,
    utl::Logger* logger,
    const sta::Network* network)
{
  BufferedNetPtr bnet = nullptr;
  const sta::PinSeq* pins = tree->pins(to);
  const odb::Point to_loc = tree->location(to);
  // If there is more than one node at a location we don't want to
  // add the pins repeatedly.  The first node wins and the rest are skipped.
  if (pins && pins_visited.find(to_loc) == pins_visited.end()) {
    pins_visited.insert(to_loc);
    for (const sta::Pin* pin : *pins) {
      if (network->isLoad(pin)) {
        BufferedNetPtr bnet1 = make_shared<BufferedNet>(
            BufferedNetType::load, tree->location(to), pin, corner, resizer);
        if (bnet1) {
          debugPrint(logger,
                     RSZ,
                     "make_buffered_net",
                     4,
                     "{:{}s}{}",
                     "",
                     level,
                     bnet1->to_string(resizer));
          if (bnet) {
            bnet = make_shared<BufferedNet>(BufferedNetType::junction,
                                            tree->location(to),
                                            bnet,
                                            bnet1,
                                            resizer);
          } else {
            bnet = std::move(bnet1);
          }
        }
      }
    }
  }
  // Steiner pt.
  for (const int adj : adjacents[to]) {
    if (adj != from) {
      BufferedNetPtr bnet1 = makeBufferedNetFromTree(tree,
                                                     to,
                                                     adj,
                                                     adjacents,
                                                     level + 1,
                                                     pins_visited,
                                                     corner,
                                                     resizer,
                                                     estimate_parasitics,
                                                     logger,
                                                     network);
      if (bnet1) {
        if (bnet) {
          bnet = make_shared<BufferedNet>(BufferedNetType::junction,
                                          tree->location(to),
                                          bnet,
                                          bnet1,
                                          resizer);
        } else {
          bnet = std::move(bnet1);
        }
      }
    }
  }
  if (bnet && from != est::SteinerTree::null_pt
      && tree->location(to) != tree->location(from)) {
    bnet = make_shared<BufferedNet>(BufferedNetType::wire,
                                    tree->location(from),
                                    BufferedNet::null_layer,
                                    bnet,
                                    corner,
                                    resizer,
                                    estimate_parasitics);
  }
  return bnet;
}

// Make BufferedNet from steiner tree.
BufferedNetPtr Resizer::makeBufferedNetSteiner(const sta::Pin* drvr_pin,
                                               const sta::Scene* corner)
{
  BufferedNetPtr bnet;
  est::SteinerTree* tree = estimate_parasitics_->makeSteinerTree(drvr_pin);
  if (tree) {
    const SteinerPt drvr_pt = tree->drvrPt();
    if (drvr_pt != est::SteinerTree::null_pt) {
      const int branch_count = tree->branchCount();
      SteinerPtAdjacents adjacents(branch_count);
      for (int i = 0; i < branch_count; i++) {
        const stt::Branch& branch_pt = tree->branch(i);
        const SteinerPt j = branch_pt.n;
        if (j != i) {
          adjacents[i].push_back(j);
          adjacents[j].push_back(i);
        }
      }
      SteinerPtPinVisited pins_visited;
      bnet = rsz::makeBufferedNetFromTree(tree,
                                          est::SteinerTree::null_pt,
                                          drvr_pt,
                                          adjacents,
                                          0,
                                          pins_visited,
                                          corner,
                                          this,
                                          estimate_parasitics_,
                                          logger_,
                                          network_);
    }
    delete tree;
  }
  return bnet;
}

// helper for makeBufferedNetSteinerOverBnets
static BufferedNetPtr makeBufferedNetFromTree2(
    const est::SteinerTree* tree,
    const est::SteinerPt from,
    const est::SteinerPt to,
    const SteinerPtAdjacents& adjacents,
    const int level,
    SteinerPtPinVisited& pins_visited,
    const sta::Scene* corner,
    const Resizer* resizer,
    const est::EstimateParasitics* estimate_parasitics,
    utl::Logger* logger,
    const sta::Network* network,
    std::map<odb::Point, std::vector<BufferedNetPtr>>& sink_map)
{
  BufferedNetPtr bnet = nullptr;
  const odb::Point to_loc = tree->location(to);
  // If there is more than one node at a location we don't want to
  // add the pins repeatedly.  The first node wins and the rest are skipped.
  if (sink_map.contains(to_loc)
      && pins_visited.find(to_loc) == pins_visited.end()) {
    pins_visited.insert(to_loc);
    for (BufferedNetPtr sink : sink_map[to_loc]) {
      if (bnet) {
        bnet = make_shared<BufferedNet>(
            BufferedNetType::junction, to_loc, bnet, sink, resizer);
      } else {
        bnet = std::move(sink);
      }
    }
  }
  // Steiner pt.
  for (int adj : adjacents[to]) {
    if (adj != from) {
      BufferedNetPtr bnet1 = makeBufferedNetFromTree2(tree,
                                                      to,
                                                      adj,
                                                      adjacents,
                                                      level + 1,
                                                      pins_visited,
                                                      corner,
                                                      resizer,
                                                      estimate_parasitics,
                                                      logger,
                                                      network,
                                                      sink_map);
      if (bnet1) {
        if (bnet) {
          bnet = make_shared<BufferedNet>(BufferedNetType::junction,
                                          tree->location(to),
                                          bnet,
                                          bnet1,
                                          resizer);
        } else {
          bnet = std::move(bnet1);
        }
      }
    }
  }
  if (bnet && from != est::SteinerTree::null_pt
      && tree->location(to) != tree->location(from)) {
    bnet = make_shared<BufferedNet>(BufferedNetType::wire,
                                    tree->location(from),
                                    BufferedNet::null_layer,
                                    bnet,
                                    corner,
                                    resizer,
                                    estimate_parasitics);
  }
  return bnet;
}

////////////////////////////////////////////////////////////////

// Make BufferedNet from Steiner tree. This is similar to
// makeBufferedNetSteiner but supports sinks of type BufferedNetPtr
BufferedNetPtr Resizer::makeBufferedNetSteinerOverBnets(
    odb::Point root,
    const std::vector<BufferedNetPtr>& sinks,
    const sta::Scene* corner)
{
  BufferedNetPtr bnet = nullptr;
  std::vector<odb::Point> sink_points;
  std::map<odb::Point, std::vector<BufferedNetPtr>> sink_map;
  for (const auto& sink : sinks) {
    sink_points.push_back(sink->location());
    sink_map[sink->location()].push_back(sink);
  }
  est::SteinerTree* tree
      = estimate_parasitics_->makeSteinerTree(root, sink_points);
  if (tree) {
    SteinerPt drvr_pt = tree->drvrPt();
    if (drvr_pt != est::SteinerTree::null_pt) {
      int branch_count = tree->branchCount();
      SteinerPtAdjacents adjacents(branch_count);
      for (int i = 0; i < branch_count; i++) {
        stt::Branch& branch_pt = tree->branch(i);
        SteinerPt j = branch_pt.n;
        if (j != i) {
          adjacents[i].push_back(j);
          adjacents[j].push_back(i);
        }
      }
      SteinerPtPinVisited pins_visited;
      bnet = rsz::makeBufferedNetFromTree2(tree,
                                           est::SteinerTree::null_pt,
                                           drvr_pt,
                                           adjacents,
                                           0,
                                           pins_visited,
                                           corner,
                                           this,
                                           estimate_parasitics_,
                                           logger_,
                                           network_,
                                           sink_map);
    }
    delete tree;
  }
  return bnet;
}

////////////////////////////////////////////////////////////////

BufferedNetPtr Resizer::stitchTrees(const BufferedNetPtr& outer_tree,
                                    Pin* stitching_load,
                                    const BufferedNetPtr& inner_tree)
{
  using BnetType = BufferedNetType;
  using BnetPtr = BufferedNetPtr;
  return visitTree(
      [this, stitching_load, inner_tree](
          auto& recurse, int level, const BnetPtr& node) -> BufferedNetPtr {
        if (!node) {
          return nullptr;
        }
        switch (node->type()) {
          case BnetType::via: {
            BnetPtr new_ref = recurse(node->ref());
            if (!new_ref) {
              return nullptr;
            }
            if (new_ref == node->ref()) {
              // Optimization: do not create a new node if it would be
              // equivalent to the existing one; analogously below for wire and
              // junction
              return node;
            }
            return std::make_shared<BufferedNet>(BnetType::via,
                                                 node->location(),
                                                 node->layer(),
                                                 node->refLayer(),
                                                 new_ref,
                                                 node->corner(),
                                                 this);
          }
          case BnetType::wire: {
            BnetPtr new_ref = recurse(node->ref());
            if (!new_ref) {
              return nullptr;
            }
            if (new_ref == node->ref()) {
              return node;
            }
            return std::make_shared<BufferedNet>(
                BnetType::wire,
                node->location(),
                node->layer(),
                new_ref,  // Fixed: was recurse(node->ref())
                node->corner(),
                this,
                estimate_parasitics_);
          }
          case BnetType::junction: {
            BnetPtr new_ref = recurse(node->ref());
            BnetPtr new_ref2 = recurse(node->ref2());
            if (!new_ref || !new_ref2) {
              return nullptr;
            }
            if (new_ref == node->ref() && new_ref2 == node->ref2()) {
              return node;
            }
            return std::make_shared<BufferedNet>(
                BnetType::junction, node->location(), new_ref, new_ref2, this);
          }
          case BnetType::load: {
            if (node->loadPin() == stitching_load) {
              // This is the buffer input pin we want to replace
              // Connect buffer input location to buffer output location with
              // wire
              odb::Point buffer_in_loc = node->location();
              int buffer_in_layer = node->layer();
              odb::Point buffer_out_loc = inner_tree->location();
              int buffer_out_layer = inner_tree->layer();

              // Check if we need to add connection
              if (buffer_in_loc == buffer_out_loc
                  && buffer_in_layer == buffer_out_layer) {
                // Same location and layer - just splice in inner_tree
                return inner_tree;
              }
              if (buffer_in_layer == buffer_out_layer) {
                // Same layer but different location - add wire segment
                debugPrint(logger_,
                           RSZ,
                           "buffer_removal",
                           2,
                           "Adding wire from buffer input ({}, {}) to output "
                           "({}, {}) on layer {}",
                           buffer_in_loc.getX(),
                           buffer_in_loc.getY(),
                           buffer_out_loc.getX(),
                           buffer_out_loc.getY(),
                           buffer_in_layer);

                return std::make_shared<BufferedNet>(BnetType::wire,
                                                     buffer_in_loc,
                                                     buffer_in_layer,
                                                     inner_tree,
                                                     node->corner(),
                                                     this,
                                                     estimate_parasitics_);
              }
              // Different layers - need via(s) + wire
              debugPrint(logger_,
                         RSZ,
                         "buffer_removal",
                         2,
                         "Adding via+wire from buffer input ({}, {}, layer "
                         "{}) to output ({}, {}, layer {})",
                         buffer_in_loc.getX(),
                         buffer_in_loc.getY(),
                         buffer_in_layer,
                         buffer_out_loc.getX(),
                         buffer_out_loc.getY(),
                         buffer_out_layer);

              // Validate layer range
              if (buffer_in_layer < 1 || buffer_out_layer < 1) {
                // layer assignment is incomplete
                return inner_tree;
              }

              // Build connection: start from inner_tree and work backwards
              BnetPtr current = inner_tree;

              // Add via stack from buffer_out_layer to buffer_in_layer
              int layer_step = (buffer_in_layer > buffer_out_layer) ? 1 : -1;
              odb::Point current_loc = buffer_out_loc;

              debugPrint(
                  logger_,
                  RSZ,
                  "buffer_removal",
                  1,
                  "Creating via stack: from_layer={} to_layer={} step={}",
                  buffer_out_layer,
                  buffer_in_layer,
                  layer_step);
              for (int layer = buffer_out_layer; layer != buffer_in_layer;
                   layer += layer_step) {
                int next_layer = layer + layer_step;
                debugPrint(logger_,
                           RSZ,
                           "buffer_removal",
                           2,
                           "Creating via from layer {} to layer {}",
                           layer,
                           next_layer);
                current = std::make_shared<BufferedNet>(BnetType::via,
                                                        current_loc,
                                                        layer,
                                                        next_layer,
                                                        current,
                                                        node->corner(),
                                                        this);
              }
              // Add wire if locations differ
              if (buffer_in_loc != buffer_out_loc) {
                current = std::make_shared<BufferedNet>(BnetType::wire,
                                                        buffer_in_loc,
                                                        buffer_in_layer,
                                                        current,
                                                        node->corner(),
                                                        this,
                                                        estimate_parasitics_);
              }

              return current;
            }
            // Not the stitching load, return as-is
            return node;
          }
          default:
            logger_->critical(RSZ, 130, "unhandled BufferedNet type");
        }
      },
      outer_tree);
}

////////////////////////////////////////////////////////////////

using grt::RoutePt;

class RoutePtHash
{
 public:
  size_t operator()(const RoutePt& pt) const;
};

class RoutePtEqual
{
 public:
  bool operator()(const RoutePt& pt1, const RoutePt& pt2) const;
};

using GRoutePtAdjacents
    = std::unordered_map<RoutePt, std::vector<RoutePt>, RoutePtHash>;

size_t RoutePtHash::operator()(const RoutePt& pt) const
{
  size_t hash = sta::hash_init_value;
  sta::hashIncr(hash, pt.x());
  sta::hashIncr(hash, pt.y());
  sta::hashIncr(hash, pt.layer());
  return hash;
}

bool RoutePtEqual::operator()(const RoutePt& pt1, const RoutePt& pt2) const
{
  return pt1.x() == pt2.x() && pt1.y() == pt2.y() && pt1.layer() == pt2.layer();
}

static const RoutePt route_pt_null(0, 0, 0);

using RoutePtSet = std::unordered_set<RoutePt, RoutePtHash, RoutePtEqual>;
using RoutePtPinMap = std::unordered_map<RoutePt, sta::PinSeq, RoutePtHash>;

static BufferedNetPtr makeBufferedNet(
    RoutePt& from,
    RoutePt& to,
    GRoutePtAdjacents& adjacents,
    RoutePtPinMap& loc_pin_map,
    int level,
    const sta::Scene* corner,
    const Resizer* resizer,
    const est::EstimateParasitics* estimate_parasitics,
    utl::Logger* logger,
    sta::dbNetwork* db_network,
    RoutePtSet& visited)
{
  if (visited.find(to) != visited.end()) {
    debugPrint(logger, RSZ, "groute_bnet", 2, "Loop found in groute");
    return nullptr;
  }
  visited.insert(to);

  odb::Point from_pt(from.x(), from.y());
  odb::Point to_pt(to.x(), to.y());

  BufferedNetPtr bnet = nullptr;
  const sta::PinSeq& pins = loc_pin_map[to];
  for (const sta::Pin* pin : pins) {
    if (db_network->isLoad(pin)) {
      auto load_bnet = make_shared<BufferedNet>(
          BufferedNetType::load, to_pt, pin, corner, resizer);
      debugPrint(logger,
                 RSZ,
                 "groute_bnet",
                 2,
                 "{:{}s}{}",
                 "",
                 level,
                 load_bnet->to_string(resizer));
      if (bnet) {
        bnet = make_shared<BufferedNet>(
            BufferedNetType::junction, to_pt, bnet, load_bnet, resizer);
      } else {
        bnet = std::move(load_bnet);
      }
    }
  }

  for (RoutePt& adj : adjacents[to]) {
    if (adj != from) {
      BufferedNetPtr bnet1 = makeBufferedNet(to,
                                             adj,
                                             adjacents,
                                             loc_pin_map,
                                             level + 1,
                                             corner,
                                             resizer,
                                             estimate_parasitics,
                                             logger,
                                             db_network,
                                             visited);
      if (bnet1) {
        if (bnet) {
          bnet = make_shared<BufferedNet>(
              BufferedNetType::junction, to_pt, bnet, bnet1, resizer);
        } else {
          bnet = std::move(bnet1);
        }
      }
    }
  }

  if (bnet && from != route_pt_null) {
    if (from_pt != to_pt) {
      bnet = make_shared<BufferedNet>(BufferedNetType::wire,
                                      from_pt,
                                      to.layer(),
                                      bnet,
                                      corner,
                                      resizer,
                                      estimate_parasitics);
    } else {
      bnet = make_shared<BufferedNet>(BufferedNetType::via,
                                      from_pt,
                                      from.layer(),
                                      to.layer(),
                                      bnet,
                                      corner,
                                      resizer);
    }
  }
  return bnet;
}

BufferedNetPtr Resizer::makeBufferedNetGroute(const sta::Pin* drvr_pin,
                                              const sta::Scene* corner)
{
  odb::dbNet* db_net = db_network_->findFlatDbNet(drvr_pin);
  const sta::Net* net = db_network_->dbToSta(db_net);
  assert(db_net != nullptr);

  std::vector<grt::PinGridLocation> pin_grid_locs
      = global_router_->getPinGridPositions(db_net);

  bool found_drvr_route_pt = false;
  RoutePt drvr_route_pt;
  RoutePtPinMap loc_pin_map;

  for (grt::PinGridLocation& pin_loc : pin_grid_locs) {
    sta::Pin* pin = pin_loc.iterm ? db_network_->dbToSta(pin_loc.iterm)
                                  : db_network_->dbToSta(pin_loc.bterm);
    RoutePt pin_route_pt(
        pin_loc.grid_pt.getX(), pin_loc.grid_pt.getY(), pin_loc.conn_layer);
    debugPrint(logger_,
               RSZ,
               "repair_net",
               2,
               "pin {}{} grid ({} {}) layer {}",
               network_->pathName(pin),
               (pin == drvr_pin) ? " drvr" : "",
               pin_route_pt.x(),
               pin_route_pt.y(),
               pin_route_pt.layer());
    loc_pin_map[pin_route_pt].push_back(pin);
    if (pin == drvr_pin) {
      drvr_route_pt = pin_route_pt;
      found_drvr_route_pt = true;
    }
  }

  if (found_drvr_route_pt) {
    grt::NetRouteMap& route_map = global_router_->getRoutes();
    grt::GRoute& route = route_map[db_network_->staToDb(net)];
    GRoutePtAdjacents adjacents(route.size());
    for (grt::GSegment& seg : route) {
      RoutePt from(seg.init_x, seg.init_y, seg.init_layer);
      RoutePt to(seg.final_x, seg.final_y, seg.final_layer);
      debugPrint(logger_,
                 RSZ,
                 "repair_net",
                 2,
                 "route {} {} {} -> {} {} {}",
                 seg.init_x,
                 seg.init_y,
                 seg.init_layer,
                 seg.final_x,
                 seg.final_y,
                 seg.final_layer);
      adjacents[from].push_back(to);
      adjacents[to].push_back(from);
    }
    RoutePt null_route_pt(0, 0, 0);
    RoutePtSet visited;
    BufferedNetPtr bnet = rsz::makeBufferedNet(null_route_pt,
                                               drvr_route_pt,
                                               adjacents,
                                               loc_pin_map,
                                               0,
                                               corner,
                                               this,
                                               estimate_parasitics_,
                                               logger_,
                                               db_network_,
                                               visited);
    if (bnet) {
      if (bnet->loadCount() != pin_grid_locs.size() - 1) {
        // we are subtracting one to account for driver at the root of the
        // tree
        logger_->error(RSZ,
                       74,
                       "Failed to build tree from global routes for pin '{}' "
                       "and net '{}' at grid ({}, {}): found route to {} "
                       "pins, expected {}",
                       db_network_->pathName(drvr_pin),
                       db_net->getName(),
                       drvr_route_pt.x(),
                       drvr_route_pt.y(),
                       bnet->loadCount(),
                       pin_grid_locs.size() - 1);
        return nullptr;
      }
    }
    return bnet;
  }
  logger_->warn(RSZ,
                73,
                "driver pin {} not found in global routes",
                db_network_->pathName(drvr_pin));
  return nullptr;
}

bool BufferedNet::fitsEnvelope(Metrics target)
{
  return maxLoadWireLength() <= target.max_load_wl && slack() >= target.slack
         && sta::fuzzyLessEqual(cap(), target.cap)
         && sta::fuzzyGreaterEqual(maxLoadSlew(), target.max_load_slew)
         && sta::fuzzyLessEqual(fanout(), target.fanout);
}

}  // namespace rsz
