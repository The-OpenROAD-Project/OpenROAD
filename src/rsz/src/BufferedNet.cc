/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "BufferedNet.hh"

#include <algorithm>
#include <memory>

#include "rsz/Resizer.hh"
// Use spdlog fmt::format until c++20 that supports std::format.
#include <spdlog/fmt/fmt.h>

#include "sta/Liberty.hh"
#include "sta/Units.hh"
#include "utl/Logger.h"

// Resizer::makeBufferedNetSteiner
#include "SteinerTree.hh"

// Resizer::makeBufferedNetGroute
#include "db_sta/dbNetwork.hh"
#include "grt/GlobalRouter.h"
#include "grt/RoutePt.h"
#include "sta/Hash.hh"

namespace rsz {

using std::make_shared;
using std::max;
using std::min;

using sta::INF;

using utl::RSZ;

static const char* to_string(BufferedNetType type);

////////////////////////////////////////////////////////////////

// load
BufferedNet::BufferedNet(const BufferedNetType type,
                         const Point& location,
                         const Pin* load_pin,
                         const Corner* corner,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::load) {
    resizer->logger()->critical(
        RSZ, 78, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::load;
  location_ = location;
  load_pin_ = load_pin;
  buffer_cell_ = nullptr;
  layer_ = null_layer;
  ref_ = nullptr;
  ref2_ = nullptr;

  LibertyPort* load_port = resizer->network()->libertyPort(load_pin);
  if (load_port) {
    cap_ = resizer->portCapacitance(load_port, corner);
    fanout_ = resizer->portFanoutLoad(load_port);
    max_load_slew_ = resizer->maxInputSlew(load_port, corner);
  } else {
    cap_ = 0.0;
    fanout_ = 1;
    max_load_slew_ = INF;
  }

  required_path_.init();
  required_delay_ = 0.0;
}

// junc
BufferedNet::BufferedNet(const BufferedNetType type,
                         const Point& location,
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
  load_pin_ = nullptr;
  buffer_cell_ = nullptr;
  layer_ = null_layer;
  ref_ = ref;
  ref2_ = ref2;

  cap_ = ref->cap() + ref2->cap();
  fanout_ = ref->fanout() + ref2->fanout();
  max_load_slew_ = min(ref->maxLoadSlew(), ref2->maxLoadSlew());

  required_path_.init();
  required_delay_ = 0.0;
}

// wire
BufferedNet::BufferedNet(const BufferedNetType type,
                         const Point& location,
                         const int layer,
                         const BufferedNetPtr& ref,
                         const Corner* corner,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::wire) {
    resizer->logger()->critical(
        RSZ, 80, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::wire;
  location_ = location;
  load_pin_ = nullptr;
  buffer_cell_ = nullptr;
  layer_ = layer;
  ref_ = ref;
  ref2_ = nullptr;

  double wire_res, wire_cap;
  wireRC(corner, resizer, wire_res, wire_cap);
  cap_ = ref->cap() + resizer->dbuToMeters(length()) * wire_cap;
  fanout_ = ref->fanout();
  max_load_slew_ = ref->maxLoadSlew();

  required_path_.init();
  required_delay_ = 0.0;
}

// buffer
BufferedNet::BufferedNet(const BufferedNetType type,
                         const Point& location,
                         LibertyCell* buffer_cell,
                         const BufferedNetPtr& ref,
                         const Corner* corner,
                         const Resizer* resizer)
{
  if (type != BufferedNetType::buffer) {
    resizer->logger()->critical(
        RSZ, 81, "incorrect BufferedNet type {}", rsz::to_string(type));
  }
  type_ = BufferedNetType::buffer;
  location_ = location;
  load_pin_ = nullptr;
  buffer_cell_ = buffer_cell;
  layer_ = null_layer;
  ref_ = ref;
  ref2_ = nullptr;

  LibertyPort *input, *output;
  buffer_cell->bufferPorts(input, output);
  cap_ = resizer->portCapacitance(input, corner);
  fanout_ = resizer->portFanoutLoad(input);
  max_load_slew_ = resizer->maxInputSlew(input, corner);

  required_path_.init();
  required_delay_ = 0.0;
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
      ref_->reportTree(level + 1, resizer);
      break;
    case BufferedNetType::junction:
      ref_->reportTree(level + 1, resizer);
      ref2_->reportTree(level + 1, resizer);
      break;
  }
}

string BufferedNet::to_string(const Resizer* resizer) const
{
  Network* sdc_network = resizer->sdcNetwork();
  Units* units = resizer->units();
  Unit* dist_unit = units->distanceUnit();
  const char* x = dist_unit->asString(resizer->dbuToMeters(location_.x()), 2);
  const char* y = dist_unit->asString(resizer->dbuToMeters(location_.y()), 2);
  const char* cap = units->capacitanceUnit()->asString(cap_);

  switch (type_) {
    case BufferedNetType::load:
      // {:{}s} format indents level spaces.
      return fmt::format("load {} ({}, {}) cap {} req {}",
                         sdc_network->pathName(load_pin_),
                         x,
                         y,
                         cap,
                         delayAsString(required(resizer), resizer));
    case BufferedNetType::wire:
      return fmt::format("wire ({}, {}) cap {} req {} buffers {}",
                         x,
                         y,
                         cap,
                         delayAsString(required(resizer), resizer),
                         bufferCount());
    case BufferedNetType::buffer:
      return fmt::format("buffer ({}, {}) {} cap {} req {} buffers {}",
                         x,
                         y,
                         buffer_cell_->name(),
                         cap,
                         delayAsString(required(resizer), resizer),
                         bufferCount());
    case BufferedNetType::junction:
      return fmt::format("junction ({}, {}) cap {} req {} buffers {}",
                         x,
                         y,
                         cap,
                         delayAsString(required(resizer), resizer),
                         bufferCount());
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

void BufferedNet::setRequiredPath(const PathRef& path_ref)
{
  required_path_ = path_ref;
}

Required BufferedNet::required(const StaState* sta) const
{
  if (required_path_.isNull()) {
    return INF;
  }
  return required_path_.required(sta) - required_delay_;
}

void BufferedNet::setRequiredDelay(Delay delay)
{
  required_delay_ = delay;
}

int BufferedNet::bufferCount() const
{
  switch (type_) {
    case BufferedNetType::buffer:
      return ref_->bufferCount() + 1;
    case BufferedNetType::wire:
      return ref_->bufferCount();
    case BufferedNetType::junction:
      return ref_->bufferCount() + ref2_->bufferCount();
    case BufferedNetType::load:
      return 0;
  }
  return 0;
}

int BufferedNet::maxLoadWireLength() const
{
  switch (type_) {
    case BufferedNetType::wire:
      return length() + ref_->maxLoadWireLength();
    case BufferedNetType::junction:
      return max(ref_->maxLoadWireLength(), ref2_->maxLoadWireLength());
    case BufferedNetType::load:
      return 0;
    case BufferedNetType::buffer:
      return 0;
  }
  return 0;
}

void BufferedNet::wireRC(const Corner* corner,
                         const Resizer* resizer,
                         // Return values.
                         double& res,
                         double& cap)
{
  if (type_ != BufferedNetType::wire) {
    resizer->logger()->critical(RSZ, 82, "wireRC called for non-wire");
  }
  if (layer_ == BufferedNet::null_layer) {
    double dx
        = resizer->dbuToMeters(std::abs(location_.x() - ref_->location().x()))
          / resizer->dbuToMeters(length());
    double dy
        = resizer->dbuToMeters(std::abs(location_.y() - ref_->location().y()))
          / resizer->dbuToMeters(length());
    res = dx * resizer->wireSignalHResistance(corner)
          + dy * resizer->wireSignalVResistance(corner);
    cap = dx * resizer->wireSignalHCapacitance(corner)
          + dy * resizer->wireSignalVCapacitance(corner);
  } else {
    odb::dbTech* tech = resizer->db_->getTech();
    resizer->layerRC(tech->findRoutingLayer(layer_), corner, res, cap);
  }
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
    case BufferedNetType::buffer:
      return "buffer";
  }
  return "??";
}

////////////////////////////////////////////////////////////////

BufferedNetPtr Resizer::makeBufferedNet(const Pin* drvr_pin,
                                        const Corner* corner)
{
  switch (parasitics_src_) {
    case ParasiticsSrc::placement:
      return makeBufferedNetSteiner(drvr_pin, corner);
    case ParasiticsSrc::global_routing:
    case ParasiticsSrc::detailed_routing:
      return makeBufferedNetGroute(drvr_pin, corner);
    case ParasiticsSrc::none:
      return nullptr;
  }
  return nullptr;
}

using SteinerPtAdjacents = vector<vector<SteinerPt>>;
using SteinerPtPinVisited = std::unordered_set<Point, PointHash, PointEqual>;

static BufferedNetPtr makeBufferedNetFromTree(
    const SteinerTree* tree,
    const SteinerPt from,
    const SteinerPt to,
    const SteinerPtAdjacents& adjacents,
    const int level,
    SteinerPtPinVisited& pins_visited,
    const Corner* corner,
    const Resizer* resizer,
    Logger* logger,
    const Network* network)
{
  BufferedNetPtr bnet = nullptr;
  const PinSeq* pins = tree->pins(to);
  const Point to_loc = tree->location(to);
  // If there is more than one node at a location we don't want to
  // add the pins repeatedly.  The first node wins and the rest are skipped.
  if (pins && pins_visited.find(to_loc) == pins_visited.end()) {
    pins_visited.insert(to_loc);
    for (const Pin* pin : *pins) {
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
  for (int adj : adjacents[to]) {
    if (adj != from) {
      BufferedNetPtr bnet1 = makeBufferedNetFromTree(tree,
                                                     to,
                                                     adj,
                                                     adjacents,
                                                     level + 1,
                                                     pins_visited,
                                                     corner,
                                                     resizer,
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
  if (bnet && from != SteinerTree::null_pt
      && tree->location(to) != tree->location(from)) {
    bnet = make_shared<BufferedNet>(BufferedNetType::wire,
                                    tree->location(from),
                                    BufferedNet::null_layer,
                                    bnet,
                                    corner,
                                    resizer);
  }
  return bnet;
}

// Make BufferedNet from steiner tree.
BufferedNetPtr Resizer::makeBufferedNetSteiner(const Pin* drvr_pin,
                                               const Corner* corner)
{
  BufferedNetPtr bnet = nullptr;
  SteinerTree* tree = makeSteinerTree(drvr_pin);
  if (tree) {
    SteinerPt drvr_pt = tree->drvrPt();
    if (drvr_pt != SteinerTree::null_pt) {
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
      bnet = rsz::makeBufferedNetFromTree(tree,
                                          SteinerTree::null_pt,
                                          drvr_pt,
                                          adjacents,
                                          0,
                                          pins_visited,
                                          corner,
                                          this,
                                          logger_,
                                          network_);
    }
    delete tree;
  }
  return bnet;
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
    = std::unordered_map<RoutePt, vector<RoutePt>, RoutePtHash, RoutePtEqual>;

size_t RoutePtHash::operator()(const RoutePt& pt) const
{
  size_t hash = sta::hash_init_value;
  hashIncr(hash, pt.x());
  hashIncr(hash, pt.y());
  return hash;
}

bool RoutePtEqual::operator()(const RoutePt& pt1, const RoutePt& pt2) const
{
  // layers do NOT have to match
  return pt1.x() == pt2.x() && pt1.y() == pt2.y();
}

static const RoutePt route_pt_null(0, 0, 0);

static bool routePtLocEq(const RoutePt& p1, const RoutePt& p2)
{
  return p1.x() == p2.x() && p1.y() == p2.y();
}

using RoutePtSet = std::unordered_set<RoutePt, RoutePtHash, RoutePtEqual>;

static BufferedNetPtr makeBufferedNet(RoutePt& from,
                                      RoutePt& to,
                                      GRoutePtAdjacents& adjacents,
                                      LocPinMap& loc_pin_map,
                                      int level,
                                      const Corner* corner,
                                      const Resizer* resizer,
                                      Logger* logger,
                                      dbNetwork* db_network,
                                      RoutePtSet& visited)
{
  if (visited.find(to) != visited.end()) {
    debugPrint(logger, RSZ, "groute_bnet", 2, "Loop found in groute");
    return nullptr;
  }
  visited.insert(to);
  Point to_pt(to.x(), to.y());
  const PinSeq& pins = loc_pin_map[to_pt];
  Point from_pt(from.x(), from.y());
  BufferedNetPtr bnet = nullptr;
  for (const Pin* pin : pins) {
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
    if (!routePtLocEq(adj, from)) {
      BufferedNetPtr bnet1 = makeBufferedNet(to,
                                             adj,
                                             adjacents,
                                             loc_pin_map,
                                             level + 1,
                                             corner,
                                             resizer,
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
  if (bnet && !routePtLocEq(from, route_pt_null) && !routePtLocEq(to, from)) {
    bnet = make_shared<BufferedNet>(
        BufferedNetType::wire, from_pt, from.layer(), bnet, corner, resizer);
  }
  return bnet;
}

BufferedNetPtr Resizer::makeBufferedNetGroute(const Pin* drvr_pin,
                                              const Corner* corner)
{
  const Net* net = network_->isTopLevelPort(drvr_pin)
                       ? network_->net(db_network_->term(drvr_pin))
                       : network_->net(drvr_pin);
  dbNet* db_net = db_network_->staToDb(net);
  std::vector<grt::PinGridLocation> pin_grid_locs
      = global_router_->getPinGridPositions(db_net);
  LocPinMap loc_pin_map;
  bool found_drvr_grid_pt = false;
  Point drvr_grid_pt;
  bool is_local = true;
  Point first_pin_loc = pin_grid_locs[0].pt_;
  for (grt::PinGridLocation& pin_loc : pin_grid_locs) {
    Pin* pin = pin_loc.iterm_ ? db_network_->dbToSta(pin_loc.iterm_)
                              : db_network_->dbToSta(pin_loc.bterm_);
    Point& loc = pin_loc.pt_;
    is_local = is_local && loc == first_pin_loc;
    debugPrint(logger_,
               RSZ,
               "groute_bnet",
               3,
               "pin {}{} grid ({} {})",
               network_->pathName(pin),
               (pin == drvr_pin) ? " drvr" : "",
               loc.x(),
               loc.y());
    loc_pin_map[loc].push_back(pin);
    if (pin == drvr_pin) {
      drvr_grid_pt = loc;
      found_drvr_grid_pt = true;
    }
  }

  if (found_drvr_grid_pt) {
    RoutePt drvr_route_pt;
    bool found_drvr_route_pt = false;
    grt::NetRouteMap& route_map = global_router_->getRoutes();
    grt::GRoute& route = route_map[db_network_->staToDb(net)];
    GRoutePtAdjacents adjacents(route.size());
    for (grt::GSegment& seg : route) {
      if (!seg.isVia() || is_local) {
        RoutePt from(seg.init_x, seg.init_y, seg.init_layer);
        RoutePt to(seg.final_x, seg.final_y, seg.final_layer);
        debugPrint(logger_,
                   RSZ,
                   "groute_bnet",
                   2,
                   "route {} {} -> {} {}",
                   seg.init_x,
                   seg.init_y,
                   seg.final_x,
                   seg.final_y);
        adjacents[from].push_back(to);
        adjacents[to].push_back(from);
        if (from.x() == drvr_grid_pt.x() && from.y() == drvr_grid_pt.y()) {
          drvr_route_pt = from;
          found_drvr_route_pt = true;
        }
        if (to.x() == drvr_grid_pt.x() && to.y() == drvr_grid_pt.y()) {
          drvr_route_pt = to;
          found_drvr_route_pt = true;
        }
      }
    }
    if (found_drvr_route_pt) {
      RoutePt null_route_pt(0, 0, 0);
      RoutePtSet visited;
      return rsz::makeBufferedNet(null_route_pt,
                                  drvr_route_pt,
                                  adjacents,
                                  loc_pin_map,
                                  0,
                                  corner,
                                  this,
                                  logger_,
                                  db_network_,
                                  visited);
    }

    logger_->warn(RSZ,
                  73,
                  "driver pin {} not found in global routes",
                  db_network_->pathName(drvr_pin));
  } else {
    logger_->warn(RSZ,
                  74,
                  "driver pin {} not found in global route grid points",
                  db_network_->pathName(drvr_pin));
  }
  return nullptr;
}

}  // namespace rsz
