/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
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

#include "ppl/IOPlacer.h"

#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>

#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "SimulatedAnnealing.h"
#include "Slots.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "ppl/AbstractIOPlacerRenderer.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ppl {

using utl::PPL;

IOPlacer::IOPlacer() : ioplacer_renderer_(nullptr)
{
  netlist_ = std::make_unique<Netlist>();
  core_ = std::make_unique<Core>();
  parms_ = std::make_unique<Parameters>();
  netlist_io_pins_ = std::make_unique<Netlist>();
  top_grid_ = std::make_unique<TopLayerGrid>();
}

IOPlacer::~IOPlacer() = default;

void IOPlacer::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  parms_ = std::make_unique<Parameters>();
}

odb::dbBlock* IOPlacer::getBlock() const
{
  return db_->getChip()->getBlock();
}

odb::dbTech* IOPlacer::getTech() const
{
  return db_->getTech();
}

void IOPlacer::clear()
{
  hor_layers_.clear();
  ver_layers_.clear();
  *top_grid_ = TopLayerGrid();
  zero_sink_ios_.clear();
  sections_.clear();
  slots_.clear();
  top_layer_slots_.clear();
  assignment_.clear();
  excluded_intervals_.clear();
  pin_groups_.clear();
  *parms_ = Parameters();
}

odb::dbTechLayer* IOPlacer::getTopLayer() const
{
  return getTech()->findRoutingLayer(top_grid_->layer);
}

void IOPlacer::clearConstraints()
{
  constraints_.clear();
}

std::string IOPlacer::getEdgeString(Edge edge)
{
  std::string edge_str;
  if (edge == Edge::bottom) {
    edge_str = "BOTTOM";
  } else if (edge == Edge::top) {
    edge_str = "TOP";
  } else if (edge == Edge::left) {
    edge_str = "LEFT";
  } else if (edge == Edge::right) {
    edge_str = "RIGHT";
  }

  return edge_str;
}

void IOPlacer::initNetlistAndCore(const std::set<int>& hor_layer_idx,
                                  const std::set<int>& ver_layer_idx)
{
  populateIOPlacer(hor_layer_idx, ver_layer_idx);
}

void IOPlacer::initParms()
{
  slots_per_section_ = parms_->getSlotsPerSection();
  slots_increase_factor_ = 0.01f;
  netlist_ = std::make_unique<Netlist>();
  netlist_io_pins_ = std::make_unique<Netlist>();

  if (parms_->getNumSlots() > -1) {
    slots_per_section_ = parms_->getNumSlots();
  }

  if (parms_->getSlotsFactor() > -1) {
    slots_increase_factor_ = parms_->getSlotsFactor();
  }
}

std::vector<int> IOPlacer::getValidSlots(int first, int last, bool top_layer)
{
  std::vector<int> valid_slots;

  std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;

  for (int i = first; i <= last; i++) {
    if (!slots[i].blocked) {
      valid_slots.push_back(i);
    }
  }

  return valid_slots;
}

void IOPlacer::randomPlacement()
{
  for (Constraint& constraint : constraints_) {
    int first_slot = constraint.sections.front().begin_slot;
    int last_slot = constraint.sections.back().end_slot;

    bool top_layer = constraint.interval.getEdge() == Edge::invalid;
    for (auto& io_group : netlist_io_pins_->getIOGroups()) {
      const PinSet& pin_list = constraint.pin_list;
      IOPin& io_pin = netlist_io_pins_->getIoPin(io_group.pin_indices[0]);
      if (io_pin.isPlaced() || io_pin.inFallback()) {
        continue;
      }

      if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
          != pin_list.end()) {
        std::vector<int> valid_slots
            = getValidSlots(first_slot, last_slot, top_layer);
        randomPlacement(
            io_group.pin_indices, std::move(valid_slots), top_layer, true);
      }
    }

    std::vector<int> valid_slots
        = getValidSlots(first_slot, last_slot, top_layer);
    std::vector<int> pin_indices;
    for (bool mirrored_pins : {true, false}) {
      std::vector<int> indices = findPinsForConstraint(
          constraint, netlist_io_pins_.get(), mirrored_pins);
      pin_indices.insert(pin_indices.end(), indices.begin(), indices.end());
    }
    randomPlacement(
        std::move(pin_indices), std::move(valid_slots), top_layer, false);
  }

  for (auto& io_group : netlist_io_pins_->getIOGroups()) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(io_group.pin_indices[0]);
    if (io_pin.isPlaced() || io_pin.inFallback()) {
      continue;
    }
    std::vector<int> valid_slots = getValidSlots(0, slots_.size() - 1, false);

    randomPlacement(io_group.pin_indices, std::move(valid_slots), false, true);
  }

  std::vector<int> valid_slots = getValidSlots(0, slots_.size() - 1, false);

  std::vector<int> pin_indices;
  for (int i = 0; i < netlist_io_pins_->numIOPins(); i++) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(i);
    if (!io_pin.isPlaced() && !io_pin.inFallback()) {
      pin_indices.push_back(i);
    }
  }

  randomPlacement(std::move(pin_indices), std::move(valid_slots), false, false);
  placeFallbackPins(true);
}

void IOPlacer::randomPlacement(std::vector<int> pin_indices,
                               std::vector<int> slot_indices,
                               bool top_layer,
                               bool is_group)
{
  if (pin_indices.size() > slot_indices.size()) {
    if (is_group) {
      logger_->warn(PPL,
                    96,
                    "Pin group of size {} does not fit constraint region. "
                    "Adding to fallback mode.",
                    pin_indices.size());
      addGroupToFallback(pin_indices, false);
      return;
    } else {
      logger_->error(
          PPL,
          72,
          "Number of pins ({}) exceed number of valid positions ({}).",
          pin_indices.size(),
          slot_indices.size());
    }
  }

  const auto seed = parms_->getRandSeed();

  const int num_i_os = pin_indices.size();
  if (num_i_os == 0) {
    return;
  }
  const int num_slots = slot_indices.size();
  const double shift = is_group ? 1 : num_slots / double(num_i_os);
  std::vector<int> vSlots(num_slots);
  std::vector<int> io_pin_indices(num_i_os);

  std::vector<InstancePin> instPins;

  std::mt19937 g;
  g.seed(seed);

  for (size_t i = 0; i < io_pin_indices.size(); ++i) {
    io_pin_indices[i] = i;
  }

  if (io_pin_indices.size() > 1 && !is_group) {
    utl::shuffle(io_pin_indices.begin(), io_pin_indices.end(), g);
  }

  std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;

  std::vector<IOPin>& io_pins = netlist_io_pins_->getIOPins();
  int io_idx = 0;
  for (bool assign_mirrored : {true, false}) {
    for (int pin_idx : pin_indices) {
      IOPin& io_pin = io_pins[pin_idx];
      if ((assign_mirrored
           && mirrored_pins_.find(io_pin.getBTerm()) == mirrored_pins_.end())
          || (!assign_mirrored
              && mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end())
          || io_pin.isPlaced()) {
        continue;
      }

      int b = io_pin_indices[io_idx];
      int slot_idx = slot_indices[floor(b * shift)];
      while (slots[slot_idx].used == true || slots[slot_idx].blocked == true) {
        slot_idx++;
      }
      io_pin.setPos(slots[slot_idx].pos);
      io_pin.setPlaced();
      io_pin.setEdge(slots[slot_idx].edge);
      slots[slot_idx].used = true;
      slots[slot_idx].blocked = true;
      io_pin.setLayer(slots[slot_idx].layer);
      assignment_.push_back(io_pin);
      io_idx++;

      if (assign_mirrored
          && mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
        odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
        int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
        IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);

        odb::Point mirrored_pos = core_->getMirroredPosition(io_pin.getPos());
        mirrored_pin.setPos(mirrored_pos);
        mirrored_pin.setLayer(slots_[slot_idx].layer);
        mirrored_pin.setPlaced();
        mirrored_pin.setEdge(slots_[slot_idx].edge);
        assignment_.push_back(mirrored_pin);
        slot_idx = getSlotIdxByPosition(
            mirrored_pos, mirrored_pin.getLayer(), slots);
        if (slot_idx < 0) {
          odb::dbTechLayer* layer
              = db_->getTech()->findRoutingLayer(mirrored_pin.getLayer());
          logger_->error(utl::PPL,
                         85,
                         "Mirrored position ({}, {}) at layer {} is not a "
                         "valid position for pin placement.",
                         mirrored_pos.getX(),
                         mirrored_pos.getY(),
                         layer->getName());
        }
        slots[slot_idx].used = true;
        slots[slot_idx].blocked = true;
      }
    }
  }
}

int IOPlacer::placeFallbackPins(bool random)
{
  int placed_pins_cnt = 0;
  // place groups in fallback mode
  for (const auto& group : fallback_pins_.groups) {
    bool constrained_group = false;
    bool have_mirrored = false;
    for (const int pin_idx : group.first) {
      const IOPin& pin = netlist_io_pins_->getIoPin(pin_idx);
      if (pin.isMirrored()) {
        have_mirrored = true;
        break;
      }
    }

    int pin_idx = group.first[0];
    IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
    if (!random) {
      // check if group is constrained
      odb::dbBTerm* bterm = io_pin.getBTerm();
      for (Constraint& constraint : constraints_) {
        if (constraint.pin_list.find(bterm) != constraint.pin_list.end()) {
          constrained_group = true;
          int first_slot = constraint.first_slot;
          int last_slot = constraint.last_slot;
          int available_slots = last_slot - first_slot;
          if (available_slots < group.first.size()) {
            logger_->error(
                PPL,
                90,
                "Group of size {} does not fit in constrained region.",
                group.first.size());
          }

          int mid_slot = (last_slot - first_slot) / 2 - group.first.size() / 2
                         + first_slot;

          // try to place fallback group in the middle of the edge
          int place_slot = getFirstSlotToPlaceGroup(
              mid_slot, last_slot, group.first.size(), have_mirrored, io_pin);

          // if the previous fails, try to place the fallback group from the
          // beginning of the edge
          if (place_slot == -1) {
            place_slot = getFirstSlotToPlaceGroup(first_slot,
                                                  last_slot,
                                                  group.first.size(),
                                                  have_mirrored,
                                                  io_pin);
          }

          if (place_slot == -1) {
            Interval& interval = constraint.interval;
            logger_->error(PPL,
                           93,
                           "Pin group of size {} does not fit in the "
                           "constrained region {:.2f}-{:.2f} at {} edge. "
                           "First pin of the group is {}.",
                           group.first.size(),
                           getBlock()->dbuToMicrons(interval.getBegin()),
                           getBlock()->dbuToMicrons(interval.getEnd()),
                           getEdgeString(interval.getEdge()),
                           io_pin.getName());
          }

          placeFallbackGroup(group, place_slot);
          break;
        }
      }
    }

    if (!constrained_group) {
      int first_slot = 0;
      int last_slot = slots_.size() - 1;
      int place_slot = getFirstSlotToPlaceGroup(
          first_slot, last_slot, group.first.size(), have_mirrored, io_pin);

      if (place_slot == -1) {
        logger_->error(
            PPL,
            109,
            "Pin group of size {} does not fit any region in the die "
            "boundaries. Not enough contiguous slots available. The first pin "
            "of the group is {}.",
            group.first.size(),
            io_pin.getName());
      }
      placeFallbackGroup(group, place_slot);
    }
  }

  for (const auto& group : fallback_pins_.groups) {
    placed_pins_cnt += group.first.size();
  }
  placed_pins_cnt += fallback_pins_.pins.size();

  fallback_pins_.groups.clear();
  fallback_pins_.pins.clear();

  return placed_pins_cnt;
}

void IOPlacer::assignMirroredPins(IOPin& io_pin,
                                  MirroredPins& mirrored_pins,
                                  std::vector<IOPin>& assignment)
{
  odb::dbBTerm* mirrored_term = mirrored_pins[io_pin.getBTerm()];
  int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
  IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);

  odb::Point mirrored_pos = core_->getMirroredPosition(io_pin.getPos());
  mirrored_pin.setPos(mirrored_pos);
  mirrored_pin.setLayer(io_pin.getLayer());
  mirrored_pin.setPlaced();
  mirrored_pin.setEdge(getMirroredEdge(io_pin.getEdge()));
  assignment.push_back(mirrored_pin);
  int slot_index
      = getSlotIdxByPosition(mirrored_pos, mirrored_pin.getLayer(), slots_);
  if (slot_index < 0 || slots_[slot_index].used) {
    odb::dbTechLayer* layer
        = db_->getTech()->findRoutingLayer(mirrored_pin.getLayer());
    logger_->error(utl::PPL,
                   91,
                   "Mirrored position ({}, {}) at layer {} is not a "
                   "valid position for pin {} placement.",
                   mirrored_pos.getX(),
                   mirrored_pos.getY(),
                   layer ? layer->getName() : "NA",
                   mirrored_pin.getName());
  }
  slots_[slot_index].used = true;
}

int IOPlacer::getSlotIdxByPosition(const odb::Point& position,
                                   int layer,
                                   std::vector<Slot>& slots)
{
  int slot_idx = -1;
  for (int i = 0; i < slots.size(); i++) {
    if (slots[i].pos == position && slots[i].layer == layer) {
      slot_idx = i;
      break;
    }
  }

  if (slot_idx == -1) {
    logger_->error(utl::PPL,
                   101,
                   "Slot for position ({}, {}) in layer {} not found",
                   position.getX(),
                   position.getY(),
                   layer);
  }

  return slot_idx;
}

int IOPlacer::getFirstSlotToPlaceGroup(int first_slot,
                                       int last_slot,
                                       int group_size,
                                       bool check_mirrored,
                                       IOPin& first_pin)
{
  int max_contiguous_slots = std::numeric_limits<int>::min();
  int place_slot = 0;
  for (int s = first_slot; s <= last_slot; s++) {
    odb::Point mirrored_pos = core_->getMirroredPosition(slots_[s].pos);
    int mirrored_slot
        = getSlotIdxByPosition(mirrored_pos, slots_[s].layer, slots_);
    while (s < last_slot
           && (!slots_[s].isAvailable()
               || (check_mirrored && !slots_[mirrored_slot].isAvailable()))) {
      s++;
      mirrored_pos = core_->getMirroredPosition(slots_[s].pos);
      mirrored_slot
          = getSlotIdxByPosition(mirrored_pos, slots_[s].layer, slots_);
    }

    place_slot = s;
    int contiguous_slots = 0;
    mirrored_pos = core_->getMirroredPosition(slots_[s].pos);
    mirrored_slot = getSlotIdxByPosition(mirrored_pos, slots_[s].layer, slots_);
    while (s < last_slot && slots_[s].isAvailable()
           && ((check_mirrored && slots_[mirrored_slot].isAvailable())
               || !check_mirrored)) {
      contiguous_slots++;
      s++;
      mirrored_pos = core_->getMirroredPosition(slots_[s].pos);
      mirrored_slot
          = getSlotIdxByPosition(mirrored_pos, slots_[s].layer, slots_);
    }

    max_contiguous_slots = std::max(max_contiguous_slots, contiguous_slots);
    if (max_contiguous_slots >= group_size) {
      break;
    }
  }

  if (max_contiguous_slots < group_size) {
    logger_->warn(
        PPL,
        97,
        "The max contiguous slots ({}) is smaller than the group size ({}).",
        max_contiguous_slots,
        group_size);
    return -1;
  }

  return place_slot;
}

void IOPlacer::placeFallbackGroup(
    const std::pair<std::vector<int>, bool>& group,
    int place_slot)
{
  auto edge = slots_[place_slot].edge;
  const bool reverse = edge == Edge::top || edge == Edge::left;
  const int group_last = group.first.size() - 1;

  for (int i = 0; i <= group_last; ++i) {
    const int pin_idx = group.first[reverse ? group_last - i : i];
    IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
    Slot& slot = slots_[place_slot];
    io_pin.setPos(slot.pos);
    io_pin.setLayer(slot.layer);
    io_pin.setEdge(slot.edge);
    assignment_.push_back(io_pin);
    slot.used = true;
    slot.blocked = true;
    place_slot++;
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      assignMirroredPins(io_pin, mirrored_pins_, assignment_);
    }
  }

  logger_->info(PPL,
                100,
                "Group of size {} placed during fallback mode.",
                group.first.size());
}

void IOPlacer::initIOLists()
{
  netlist_io_pins_->reset();
  int idx = 0;
  for (IOPin& io_pin : netlist_->getIOPins()) {
    std::vector<InstancePin> inst_pins_vector;
    if (netlist_->numSinksOfIO(idx) != 0) {
      netlist_->getSinksOfIO(idx, inst_pins_vector);
      netlist_io_pins_->addIONet(io_pin, inst_pins_vector);
    } else {
      zero_sink_ios_.push_back(io_pin);
      netlist_io_pins_->addIONet(io_pin, inst_pins_vector);
    }
    idx++;
  }

  int group_idx = 0;
  for (const auto& [pins, order] : pin_groups_) {
    netlist_io_pins_->createIOGroup(pins, order, group_idx);
    group_idx++;
  }
}

bool IOPlacer::checkBlocked(Edge edge, int pos, int layer)
{
  for (Interval blocked_interval : excluded_intervals_) {
    // check if the blocked interval blocks all layers (== -1) or if it blocks
    // the layer of the position
    if (blocked_interval.getLayer() == -1
        || blocked_interval.getLayer() == layer) {
      if (blocked_interval.getEdge() == edge
          && pos > blocked_interval.getBegin()
          && pos < blocked_interval.getEnd()) {
        return true;
      }
    }
  }

  return false;
}

std::vector<Interval> IOPlacer::findBlockedIntervals(const odb::Rect& die_area,
                                                     const odb::Rect& box)
{
  std::vector<Interval> intervals;

  // check intersect bottom edge
  if (die_area.yMin() == box.yMin()) {
    intervals.emplace_back(Edge::bottom, box.xMin(), box.xMax());
  }
  // check intersect top edge
  if (die_area.yMax() == box.yMax()) {
    intervals.emplace_back(Edge::top, box.xMin(), box.xMax());
  }
  // check intersect left edge
  if (die_area.xMin() == box.xMin()) {
    intervals.emplace_back(Edge::left, box.yMin(), box.yMax());
  }
  // check intersect right edge
  if (die_area.xMax() == box.xMax()) {
    intervals.emplace_back(Edge::right, box.yMin(), box.yMax());
  }

  return intervals;
}

void IOPlacer::getBlockedRegionsFromMacros()
{
  odb::Rect die_area = getBlock()->getDieArea();

  for (odb::dbInst* inst : getBlock()->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (master->isBlock() && inst->isPlaced()) {
      odb::Rect inst_area = inst->getBBox()->getBox();
      odb::Rect intersect = die_area.intersect(inst_area);

      std::vector<Interval> intervals
          = findBlockedIntervals(die_area, intersect);
      for (Interval interval : intervals) {
        excludeInterval(interval);
      }
    }
  }
}

void IOPlacer::getBlockedRegionsFromDbObstructions()
{
  odb::Rect die_area = getBlock()->getDieArea();

  for (odb::dbObstruction* obstruction : getBlock()->getObstructions()) {
    odb::dbBox* obstructBox = obstruction->getBBox();
    odb::Rect obstructArea = obstructBox->getBox();
    odb::Rect intersect = die_area.intersect(obstructArea);

    std::vector<Interval> intervals = findBlockedIntervals(die_area, intersect);
    for (Interval interval : intervals) {
      excludeInterval(interval);
    }
  }
}

void IOPlacer::writePinPlacement(const char* file_name)
{
  std::string filename = file_name;
  if (filename.empty()) {
    return;
  }

  std::ofstream out(filename);

  if (!out) {
    logger_->error(PPL, 35, "Cannot open file {}.", filename);
  }

  std::vector<Edge> edges_list
      = {Edge::bottom, Edge::right, Edge::top, Edge::left};
  for (const Edge& edge : edges_list) {
    out << "#Edge: " << getEdgeString(edge) << "\n";
    for (const IOPin& io_pin : netlist_io_pins_->getIOPins()) {
      if (io_pin.getEdge() == edge) {
        const int layer = io_pin.getLayer();
        odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
        const odb::Point& pos = io_pin.getPosition();
        out << "place_pin -pin_name " << io_pin.getName() << " -layer "
            << tech_layer->getName() << " -location {"
            << getBlock()->dbuToMicrons(pos.x()) << " "
            << getBlock()->dbuToMicrons(pos.y())
            << "} -force_to_die_boundary\n";
      }
    }
  }
}

Edge IOPlacer::getMirroredEdge(const Edge& edge)
{
  Edge mirrored_edge = Edge::invalid;
  if (edge == Edge::bottom) {
    mirrored_edge = Edge::top;
  } else if (edge == Edge::top) {
    mirrored_edge = Edge::bottom;
  } else if (edge == Edge::left) {
    mirrored_edge = Edge::right;
  } else if (edge == Edge::right) {
    mirrored_edge = Edge::left;
  } else {
    mirrored_edge = Edge::invalid;
  }

  return mirrored_edge;
}

int IOPlacer::computeNewRegionLength(const Interval& interval,
                                     const int num_pins)
{
  const bool vertical_pin
      = interval.getEdge() == Edge::top || interval.getEdge() == Edge::bottom;
  const int interval_length = std::abs(interval.getEnd() - interval.getBegin());
  int min_dist = std::numeric_limits<int>::min();

  if (interval.getLayer() != -1) {
    min_dist = vertical_pin ? core_->getMinDstPinsX()[interval.getLayer()]
                            : core_->getMinDstPinsY()[interval.getLayer()];
  } else if (vertical_pin) {
    for (int layer_idx : ver_layers_) {
      const int layer_min_dist = core_->getMinDstPinsX()[layer_idx];
      min_dist = std::max(layer_min_dist, min_dist);
    }
  } else {
    for (int layer_idx : hor_layers_) {
      const int layer_min_dist = core_->getMinDstPinsY()[layer_idx];
      min_dist = std::max(layer_min_dist, min_dist);
    }
  }

  const int increase = computeIncrease(min_dist, num_pins, interval_length);
  return increase + interval_length;
}

int64_t IOPlacer::computeIncrease(int min_dist,
                                  const int64_t num_pins,
                                  const int64_t curr_length)
{
  const bool dist_in_tracks = parms_->getMinDistanceInTracks();
  const int user_min_dist = parms_->getMinDistance();
  if (dist_in_tracks) {
    min_dist *= user_min_dist;
  } else if (user_min_dist != 0) {
    min_dist
        = std::ceil(static_cast<float>(user_min_dist) / min_dist) * min_dist;
  } else {
    min_dist *= default_min_dist_;
  }

  const int64_t increase = (num_pins * min_dist) - curr_length;
  return increase;
}

void IOPlacer::findSlots(const std::set<int>& layers, Edge edge)
{
  Point lb = core_->getBoundary().ll();
  Point ub = core_->getBoundary().ur();

  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  bool vertical_pin = (edge == Edge::top || edge == Edge::bottom);
  int min = vertical_pin ? lb_x : lb_y;
  int max = vertical_pin ? ub_x : ub_y;

  int offset = parms_->getCornerAvoidance();
  bool dist_in_tracks = parms_->getMinDistanceInTracks();
  for (int layer : layers) {
    int curr_x, curr_y, start_idx, end_idx;
    // get the on grid min distance
    int tech_min_dst = vertical_pin ? core_->getMinDstPinsX()[layer]
                                    : core_->getMinDstPinsY()[layer];
    int min_dst_pins
        = dist_in_tracks
              ? tech_min_dst * parms_->getMinDistance()
              : tech_min_dst
                    * std::ceil(static_cast<float>(parms_->getMinDistance())
                                / tech_min_dst);

    min_dst_pins
        = (min_dst_pins == 0) ? default_min_dist_ * tech_min_dst : min_dst_pins;

    if (offset == -1) {
      offset = num_tracks_offset_ * tech_min_dst;
      // limit default offset to 1um
      if (offset > getBlock()->micronsToDbu(1.0)) {
        offset = getBlock()->micronsToDbu(1.0);
      }
    }

    int init_tracks = vertical_pin ? core_->getInitTracksX()[layer]
                                   : core_->getInitTracksY()[layer];
    int num_tracks = vertical_pin ? core_->getNumTracksX()[layer]
                                  : core_->getNumTracksY()[layer];

    float thickness_multiplier
        = vertical_pin ? parms_->getVerticalThicknessMultiplier()
                       : parms_->getHorizontalThicknessMultiplier();

    int half_width = vertical_pin
                         ? int(ceil(core_->getMinWidthX()[layer] / 2.0))
                         : int(ceil(core_->getMinWidthY()[layer] / 2.0));

    half_width *= thickness_multiplier;

    int num_tracks_offset = std::ceil(offset / min_dst_pins);

    start_idx
        = std::max(0.0,
                   ceil(static_cast<double>((min + half_width - init_tracks))
                        / min_dst_pins))
          + num_tracks_offset;
    end_idx = std::min((num_tracks - 1),
                       static_cast<int>(floor((max - half_width - init_tracks)
                                              / min_dst_pins)))
              - num_tracks_offset;
    if (vertical_pin) {
      curr_x = init_tracks + start_idx * min_dst_pins;
      curr_y = (edge == Edge::bottom) ? lb_y : ub_y;
    } else {
      curr_y = init_tracks + start_idx * min_dst_pins;
      curr_x = (edge == Edge::left) ? lb_x : ub_x;
    }

    std::vector<Point> slots;
    for (int i = start_idx; i <= end_idx; ++i) {
      Point pos(curr_x, curr_y);
      slots.push_back(pos);
      if (vertical_pin) {
        curr_x += min_dst_pins;
      } else {
        curr_y += min_dst_pins;
      }
    }

    if (edge == Edge::top || edge == Edge::left) {
      std::reverse(slots.begin(), slots.end());
    }

    for (const Point& pos : slots) {
      curr_x = pos.getX();
      curr_y = pos.getY();
      bool blocked = vertical_pin ? checkBlocked(edge, curr_x, layer)
                                  : checkBlocked(edge, curr_y, layer);
      slots_.push_back({blocked, false, Point(curr_x, curr_y), layer, edge});
    }
  }
}

void IOPlacer::defineSlots()
{
  /*******************************************
   *  Order of the edges when creating slots  *
   ********************************************
   *                 <----                    *
   *                                          *
   *                 3st edge     upperBound  *
   *           *------------------x           *
   *           |                  |           *
   *   |       |                  |      ^    *
   *   |  4th  |                  | 2nd  |    *
   *   |  edge |                  | edge |    *
   *   V       |                  |      |    *
   *           |                  |           *
   *           x------------------*           *
   *   lowerBound    1st edge                 *
   *                 ---->                    *
   *******************************************/

  // For wider pins (when set_hor|ver_thick multiplier is used), a valid
  // slot is one that does not cause a part of the pin to lie outside
  // the die area (OffGrid violation):
  // (offset + k_start * pitch) - halfWidth >= lower_bound , where k_start is a
  // non-negative integer => start_idx is k_start (offset + k_end * pitch) +
  // halfWidth <= upper_bound, where k_end is a non-negative integer => end_idx
  // is k_end
  //     ^^^^^^^^ position of tracks(slots)

  findSlots(ver_layers_, Edge::bottom);

  findSlots(hor_layers_, Edge::right);

  findSlots(ver_layers_, Edge::top);

  findSlots(hor_layers_, Edge::left);

  findSlotsForTopLayer();

  int regular_pin_count = static_cast<int>(netlist_io_pins_->getIOPins().size())
                          - top_layer_pins_count_;
  if (regular_pin_count > slots_.size()) {
    int min_dist = std::numeric_limits<int>::min();
    for (int layer_idx : ver_layers_) {
      const int layer_min_dist = core_->getMinDstPinsX()[layer_idx];
      min_dist = std::max(layer_min_dist, min_dist);
    }
    for (int layer_idx : hor_layers_) {
      const int layer_min_dist = core_->getMinDstPinsY()[layer_idx];
      min_dist = std::max(layer_min_dist, min_dist);
    }

    const int64_t die_margin = getBlock()->getDieArea().margin();
    const int64_t new_margin
        = computeIncrease(min_dist, regular_pin_count, die_margin) + die_margin;

    logger_->error(
        PPL,
        24,
        "Number of IO pins ({}) exceeds maximum number of available "
        "positions ({}). Increase the die perimeter from {:.2f}um to {:.2f}um.",
        regular_pin_count,
        slots_.size(),
        getBlock()->dbuToMicrons(die_margin),
        getBlock()->dbuToMicrons(new_margin));
  }

  if (top_layer_pins_count_ > top_layer_slots_.size()) {
    logger_->error(PPL,
                   11,
                   "Number of IO pins assigned to the top layer ({}) exceeds "
                   "maximum number of available "
                   "top layer positions ({}).",
                   top_layer_pins_count_,
                   top_layer_slots_.size());
  }
}

void IOPlacer::findSections(int begin,
                            int end,
                            Edge edge,
                            std::vector<Section>& sections)
{
  int end_slot = 0;
  while (end_slot < end) {
    int blocked_slots = 0;
    end_slot = begin + slots_per_section_ - 1;
    if (end_slot > end) {
      end_slot = end;
    }
    for (int i = begin; i <= end_slot; ++i) {
      if (slots_[i].blocked) {
        blocked_slots++;
      }
    }
    int half_length_pt = begin + (end_slot - begin) / 2;
    Section n_sec;
    n_sec.pos = slots_.at(half_length_pt).pos;
    n_sec.num_slots = end_slot - begin - blocked_slots + 1;
    if (n_sec.num_slots < 0) {
      logger_->error(PPL, 40, "Negative number of slots.");
    }
    n_sec.begin_slot = begin;
    n_sec.end_slot = end_slot;
    n_sec.used_slots = 0;
    n_sec.edge = edge;

    sections.push_back(n_sec);
    begin = ++end_slot;
  }
}

std::vector<Section> IOPlacer::createSectionsPerConstraint(
    Constraint& constraint)
{
  const Interval& interv = constraint.interval;
  const Edge edge = interv.getEdge();

  std::vector<Section> sections;
  if (edge != Edge::invalid) {
    const std::set<int>& layers = (edge == Edge::left || edge == Edge::right)
                                      ? hor_layers_
                                      : ver_layers_;

    for (int layer : layers) {
      std::vector<Slot>::iterator it
          = std::find_if(slots_.begin(), slots_.end(), [&](const Slot& s) {
              int slot_xy = (edge == Edge::left || edge == Edge::right)
                                ? s.pos.y()
                                : s.pos.x();
              if (edge == Edge::bottom || edge == Edge::right) {
                return (s.edge == edge && s.layer == layer
                        && slot_xy >= interv.getBegin());
              }
              return (s.edge == edge && s.layer == layer
                      && slot_xy <= interv.getEnd());
            });
      int constraint_begin = it - slots_.begin();

      it = std::find_if(
          slots_.begin() + constraint_begin, slots_.end(), [&](const Slot& s) {
            int slot_xy = (edge == Edge::left || edge == Edge::right)
                              ? s.pos.y()
                              : s.pos.x();
            if (edge == Edge::bottom || edge == Edge::right) {
              return (slot_xy >= interv.getEnd() || s.edge != edge
                      || s.layer != layer);
            }
            return (slot_xy <= interv.getBegin() || s.edge != edge
                    || s.layer != layer);
          });
      int constraint_end = it - slots_.begin() - 1;
      constraint.first_slot = constraint_begin;
      constraint.last_slot = constraint_end;

      findSections(constraint_begin, constraint_end, edge, sections);
    }
  } else {
    sections = findSectionsForTopLayer(constraint.box);
  }

  return sections;
}

void IOPlacer::createSectionsPerEdge(Edge edge, const std::set<int>& layers)
{
  for (int layer : layers) {
    std::vector<Slot>::iterator it
        = std::find_if(slots_.begin(), slots_.end(), [&](Slot s) {
            return s.edge == edge && s.layer == layer;
          });
    int edge_begin = it - slots_.begin();

    it = std::find_if(slots_.begin() + edge_begin, slots_.end(), [&](Slot s) {
      return s.edge != edge || s.layer != layer;
    });
    int edge_end = it - slots_.begin() - 1;

    findSections(edge_begin, edge_end, edge, sections_);
  }
}

void IOPlacer::createSections()
{
  sections_.clear();

  // sections only have slots at the same edge of the die boundary
  createSectionsPerEdge(Edge::bottom, ver_layers_);
  createSectionsPerEdge(Edge::right, hor_layers_);
  createSectionsPerEdge(Edge::top, ver_layers_);
  createSectionsPerEdge(Edge::left, hor_layers_);
}

int IOPlacer::updateSection(Section& section, std::vector<Slot>& slots)
{
  int new_slots_count = 0;
  for (int slot_idx = section.begin_slot; slot_idx <= section.end_slot;
       slot_idx++) {
    new_slots_count += (slots[slot_idx].isAvailable()) ? 1 : 0;
  }
  section.num_slots = new_slots_count;
  return new_slots_count;
}

int IOPlacer::updateConstraintSections(Constraint& constraint)
{
  bool top_layer = constraint.interval.getEdge() == Edge::invalid;
  std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;
  int total_slots_count = 0;
  for (Section& sec : constraint.sections) {
    total_slots_count += updateSection(sec, slots);
    sec.pin_groups.clear();
    sec.pin_indices.clear();
  }

  return total_slots_count;
}

std::vector<Section> IOPlacer::assignConstrainedPinsToSections(
    Constraint& constraint,
    int& mirrored_pins_cnt,
    bool mirrored_only)
{
  assignConstrainedGroupsToSections(
      constraint, constraint.sections, mirrored_pins_cnt, mirrored_only);

  std::vector<int> pin_indices = findPinsForConstraint(
      constraint, netlist_io_pins_.get(), mirrored_only);

  for (int idx : pin_indices) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(idx);
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()
        && !io_pin.isAssignedToSection()) {
      mirrored_pins_cnt++;
    }
    assignPinToSection(io_pin, idx, constraint.sections);
  }

  return constraint.sections;
}

void IOPlacer::assignConstrainedGroupsToSections(Constraint& constraint,
                                                 std::vector<Section>& sections,
                                                 int& mirrored_pins_cnt,
                                                 bool mirrored_only)
{
  for (auto& io_group : netlist_io_pins_->getIOGroups()) {
    const PinSet& pin_list = constraint.pin_list;
    IOPin& io_pin = netlist_io_pins_->getIoPin(io_group.pin_indices[0]);

    if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
        != pin_list.end()) {
      if (mirrored_only && !groupHasMirroredPin(io_group.pin_indices)) {
        continue;
      }
      for (int pin_idx : io_group.pin_indices) {
        IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
        if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()
            && mirrored_only) {
          mirrored_pins_cnt++;
        }
      }
      assignGroupToSection(io_group.pin_indices, sections, io_group.order);
    }
  }
}

bool IOPlacer::groupHasMirroredPin(const std::vector<int>& group)
{
  for (int pin_idx : group) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      return true;
    }
  }

  return false;
}

int IOPlacer::assignGroupsToSections(int& mirrored_pins_cnt)
{
  int total_pins_assigned = 0;

  for (auto& io_group : netlist_io_pins_->getIOGroups()) {
    int before_assignment = total_pins_assigned;
    total_pins_assigned += assignGroupToSection(
        io_group.pin_indices, sections_, io_group.order);

    // check if group was assigned here, and not during constrained groups
    if (total_pins_assigned > before_assignment) {
      for (int pin_idx : io_group.pin_indices) {
        IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
        if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
          mirrored_pins_cnt++;
        }
      }
    }
  }

  return total_pins_assigned;
}

int IOPlacer::assignGroupToSection(const std::vector<int>& io_group,
                                   std::vector<Section>& sections,
                                   bool order)
{
  Netlist* net = netlist_io_pins_.get();
  int group_size = io_group.size();
  bool group_assigned = false;
  int total_pins_assigned = 0;

  IOPin& io_pin = net->getIoPin(io_group[0]);

  if (!io_pin.isAssignedToSection() && !io_pin.inFallback()) {
    std::vector<int64_t> dst(sections.size(), 0);
    for (int i = 0; i < sections.size(); i++) {
      for (int pin_idx : io_group) {
        int pin_hpwl = net->computeIONetHPWL(pin_idx, sections[i].pos);
        if (pin_hpwl == std::numeric_limits<int>::max()) {
          dst[i] = pin_hpwl;
          break;
        }
        dst[i] += pin_hpwl;
      }
    }

    for (auto i : sortIndexes(dst)) {
      int section_available_slots
          = sections[i].num_slots - sections[i].used_slots;

      int section_max_group = 0;
      for (const auto& group : sections[i].pin_groups) {
        section_max_group = std::max(static_cast<int>(group.pin_indices.size()),
                                     section_max_group);
      }

      // avoid two or more groups in a same section when one of the number of
      // pins of one group is greater than half of the number of slots per
      // section. it avoids errors during Hungarian matching.
      if ((group_size > slots_per_section_ / 2
           && !sections[i].pin_groups.empty())
          || (section_max_group > slots_per_section_ / 2)) {
        continue;
      }

      if (group_size <= sections[i].getMaxContiguousSlots(slots_)
          && group_size <= section_available_slots) {
        std::vector<int> group;
        for (int pin_idx : io_group) {
          IOPin& io_pin = net->getIoPin(pin_idx);
          sections[i].pin_indices.push_back(pin_idx);
          group.push_back(pin_idx);
          sections[i].used_slots++;
          io_pin.assignToSection();
          if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
            assignMirroredPin(io_pin);
          }
        }
        total_pins_assigned += group_size;
        sections[i].pin_groups.push_back({std::move(group), order});
        group_assigned = true;
        break;
      }
      int available_slots = sections[i].num_slots - sections[i].used_slots;
      std::string edge_str = getEdgeString(sections[i].edge);
      const odb::Point& section_begin = slots_[sections[i].begin_slot].pos;
      const odb::Point& section_end = slots_[sections[i].end_slot].pos;
      logger_->warn(PPL,
                    78,
                    "Not enough available positions ({}) in section ({}, "
                    "{})-({}, {}) at edge {} to place the pin "
                    "group of size {}.",
                    available_slots,
                    getBlock()->dbuToMicrons(section_begin.getX()),
                    getBlock()->dbuToMicrons(section_begin.getY()),
                    getBlock()->dbuToMicrons(section_end.getX()),
                    getBlock()->dbuToMicrons(section_end.getY()),
                    edge_str,
                    group_size);
    }
    if (!group_assigned) {
      addGroupToFallback(io_group, order);
      logger_->warn(PPL, 42, "Unsuccessfully assigned I/O groups.");
    }
  }

  return total_pins_assigned;
}

void IOPlacer::addGroupToFallback(const std::vector<int>& pin_group, bool order)
{
  fallback_pins_.groups.emplace_back(pin_group, order);
  for (int pin_idx : pin_group) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
    io_pin.setFallback();
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
      int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
      IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);
      mirrored_pin.setFallback();
    }
  }
}

bool IOPlacer::assignPinsToSections(int assigned_pins_count)
{
  Netlist* net = netlist_io_pins_.get();
  std::vector<Section>& sections = sections_;

  createSections();

  int mirrored_pins_cnt = 0;
  int total_pins_assigned = assignGroupsToSections(mirrored_pins_cnt);

  // Mirrored pins first
  int idx = 0;
  for (IOPin& io_pin : net->getIOPins()) {
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      if (assignPinToSection(io_pin, idx, sections)) {
        total_pins_assigned += 2;
      }
    }
    idx++;
  }

  // Remaining pins
  idx = 0;
  for (IOPin& io_pin : net->getIOPins()) {
    if (assignPinToSection(io_pin, idx, sections)) {
      total_pins_assigned++;
    }
    idx++;
  }

  total_pins_assigned += assigned_pins_count + mirrored_pins_cnt;

  if (total_pins_assigned > net->numIOPins()) {
    logger_->error(
        PPL,
        13,
        "Internal error, placed more pins than exist ({} out of {}).",
        total_pins_assigned,
        net->numIOPins());
  }

  if (total_pins_assigned == net->numIOPins()) {
    logger_->info(PPL, 8, "Successfully assigned pins to sections.");
    return true;
  }
  logger_->info(PPL,
                9,
                "Unsuccessfully assigned pins to sections ({} out of {}).",
                total_pins_assigned,
                net->numIOPins());
  return false;
}

bool IOPlacer::assignPinToSection(IOPin& io_pin,
                                  int idx,
                                  std::vector<Section>& sections)
{
  bool pin_assigned = false;

  if (!io_pin.isInGroup() && !io_pin.isAssignedToSection()
      && !io_pin.inFallback()) {
    std::vector<int> dst(sections.size());
    for (int i = 0; i < sections.size(); i++) {
      dst[i] = netlist_io_pins_->computeIONetHPWL(idx, sections[i].pos);
    }
    for (auto i : sortIndexes(dst)) {
      if (sections[i].used_slots < sections[i].num_slots) {
        sections[i].pin_indices.push_back(idx);
        sections[i].used_slots++;
        pin_assigned = true;
        io_pin.assignToSection();

        if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
          assignMirroredPin(io_pin);
        }
        break;
      }
    }
  }

  return pin_assigned;
}

void IOPlacer::assignMirroredPin(IOPin& io_pin)
{
  odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
  int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
  IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);
  // Mark mirrored pin as assigned to section to prevent assigning it to
  // another section that is not aligned with his pair
  mirrored_pin.assignToSection();
}

void IOPlacer::printConfig(bool annealing)
{
  logger_->info(PPL, 1, "Number of slots           {}", slots_.size());
  if (!top_layer_slots_.empty()) {
    logger_->info(
        PPL, 62, "Number of top layer slots {}", top_layer_slots_.size());
  }
  logger_->info(PPL, 2, "Number of I/O             {}", netlist_->numIOPins());
  logger_->metric("floorplan__design__io", netlist_->numIOPins());
  logger_->info(PPL,
                3,
                "Number of I/O w/sink      {}",
                netlist_io_pins_->numIOPins() - zero_sink_ios_.size());
  logger_->info(PPL, 4, "Number of I/O w/o sink    {}", zero_sink_ios_.size());
  if (!annealing) {
    logger_->info(PPL, 5, "Slots per section         {}", slots_per_section_);
    logger_->info(
        PPL, 6, "Slots increase factor     {:.1}", slots_increase_factor_);
  }
}

void IOPlacer::setupSections(int assigned_pins_count)
{
  bool all_assigned;
  int i = 0;

  do {
    logger_->info(PPL, 10, "Tentative {} to set up sections.", i++);
    printConfig();

    all_assigned = assignPinsToSections(assigned_pins_count);

    slots_per_section_ *= (1 + slots_increase_factor_);
    if (sections_.size() > MAX_SECTIONS_RECOMMENDED) {
      logger_->warn(PPL,
                    36,
                    "Number of sections is {}"
                    " while the maximum recommended value is {}"
                    " this may negatively affect performance.",
                    sections_.size(),
                    MAX_SECTIONS_RECOMMENDED);
    }
    if (slots_per_section_ > MAX_SLOTS_RECOMMENDED) {
      logger_->warn(PPL,
                    37,
                    "Number of slots per sections is {}"
                    " while the maximum recommended value is {}"
                    " this may negatively affect performance.",
                    slots_per_section_,
                    MAX_SLOTS_RECOMMENDED);
    }
  } while (!all_assigned);
}

void IOPlacer::updateOrientation(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();
  int lower_x_bound = core_->getBoundary().ll().x();
  int lower_y_bound = core_->getBoundary().ll().y();
  int upper_x_bound = core_->getBoundary().ur().x();
  int upper_y_bound = core_->getBoundary().ur().y();

  if (x == lower_x_bound) {
    if (y == upper_y_bound) {
      pin.setOrientation(Orientation::south);
      return;
    }
    pin.setOrientation(Orientation::east);
    return;
  }
  if (x == upper_x_bound) {
    if (y == lower_y_bound) {
      pin.setOrientation(Orientation::north);
      return;
    }
    pin.setOrientation(Orientation::west);
    return;
  }
  if (y == lower_y_bound) {
    pin.setOrientation(Orientation::north);
    return;
  }
  if (y == upper_y_bound) {
    pin.setOrientation(Orientation::south);
    return;
  }
}

void IOPlacer::updatePinArea(IOPin& pin)
{
  const int mfg_grid = getTech()->getManufacturingGrid();

  if (mfg_grid == 0) {
    logger_->error(PPL, 20, "Manufacturing grid is not defined.");
  }

  if (pin.getLayer() != top_grid_->layer) {
    int required_min_area = 0;

    if (hor_layers_.find(pin.getLayer()) == hor_layers_.end()
        && ver_layers_.find(pin.getLayer()) == ver_layers_.end()) {
      logger_->error(PPL,
                     77,
                     "Layer {} of Pin {} not found.",
                     pin.getLayer(),
                     pin.getName());
    }

    if (pin.getOrientation() == Orientation::north
        || pin.getOrientation() == Orientation::south) {
      float thickness_multiplier = parms_->getVerticalThicknessMultiplier();
      int half_width = int(ceil(core_->getMinWidthX()[pin.getLayer()] / 2.0))
                       * thickness_multiplier;
      int height = int(std::max(
          2.0 * half_width,
          ceil(core_->getMinAreaX()[pin.getLayer()] / (2.0 * half_width))));
      required_min_area = core_->getMinAreaX()[pin.getLayer()];

      int ext = 0;
      if (parms_->getVerticalLength() != -1) {
        height = parms_->getVerticalLength();
      }

      if (parms_->getVerticalLengthExtend() != -1) {
        ext = parms_->getVerticalLengthExtend();
      }

      if (height % mfg_grid != 0) {
        height = mfg_grid * std::ceil(static_cast<float>(height) / mfg_grid);
      }

      if (pin.getOrientation() == Orientation::north) {
        pin.setLowerBound(pin.getX() - half_width, pin.getY() - ext);
        pin.setUpperBound(pin.getX() + half_width, pin.getY() + height);
      } else {
        pin.setLowerBound(pin.getX() - half_width, pin.getY() + ext);
        pin.setUpperBound(pin.getX() + half_width, pin.getY() - height);
      }
    }

    if (pin.getOrientation() == Orientation::west
        || pin.getOrientation() == Orientation::east) {
      float thickness_multiplier = parms_->getHorizontalThicknessMultiplier();
      int half_width = int(ceil(core_->getMinWidthY()[pin.getLayer()] / 2.0))
                       * thickness_multiplier;
      int height = int(std::max(
          2.0 * half_width,
          ceil(core_->getMinAreaY()[pin.getLayer()] / (2.0 * half_width))));
      required_min_area = core_->getMinAreaY()[pin.getLayer()];

      int ext = 0;
      if (parms_->getHorizontalLengthExtend() != -1) {
        ext = parms_->getHorizontalLengthExtend();
      }
      if (parms_->getHorizontalLength() != -1) {
        height = parms_->getHorizontalLength();
      }

      if (height % mfg_grid != 0) {
        height = mfg_grid * std::ceil(static_cast<float>(height) / mfg_grid);
      }

      if (pin.getOrientation() == Orientation::east) {
        pin.setLowerBound(pin.getX() - ext, pin.getY() - half_width);
        pin.setUpperBound(pin.getX() + height, pin.getY() + half_width);
      } else {
        pin.setLowerBound(pin.getX() - height, pin.getY() - half_width);
        pin.setUpperBound(pin.getX() + ext, pin.getY() + half_width);
      }
    }

    if (pin.getArea() < required_min_area) {
      logger_->error(PPL,
                     79,
                     "Pin {} area {:2.4f}um^2 is lesser than the minimum "
                     "required area {:2.4f}um^2.",
                     pin.getName(),
                     getBlock()->dbuAreaToMicrons(pin.getArea()),
                     getBlock()->dbuAreaToMicrons(required_min_area));
    }
  } else {
    int pin_width = top_grid_->pin_width;
    int pin_height = top_grid_->pin_height;

    if (pin_width % mfg_grid != 0) {
      pin_width
          = mfg_grid * std::ceil(static_cast<float>(pin_width) / mfg_grid);
    }
    if (pin_width % 2 != 0) {
      // ensure pin_width is a even multiple of mfg_grid since its divided by 2
      // later
      pin_width += mfg_grid;
    }

    if (pin_height % mfg_grid != 0) {
      pin_height
          = mfg_grid * std::ceil(static_cast<float>(pin_height) / mfg_grid);
    }
    if (pin_height % 2 != 0) {
      // ensure pin_height is a even multiple of mfg_grid since its divided by 2
      // later
      pin_height += mfg_grid;
    }

    pin.setLowerBound(pin.getX() - pin_width / 2, pin.getY() - pin_height / 2);
    pin.setUpperBound(pin.getX() + pin_width / 2, pin.getY() + pin_height / 2);
  }
}

int64 IOPlacer::computeIONetsHPWL(Netlist* netlist)
{
  int64 hpwl = 0;
  int idx = 0;
  for (IOPin& io_pin : netlist->getIOPins()) {
    hpwl += netlist->computeIONetHPWL(idx, io_pin.getPosition());
    idx++;
  }

  return hpwl;
}

int64 IOPlacer::computeIONetsHPWL()
{
  return computeIONetsHPWL(netlist_.get());
}

void IOPlacer::excludeInterval(Edge edge, int begin, int end)
{
  Interval excluded_interv = Interval(edge, begin, end);

  excluded_intervals_.push_back(excluded_interv);
}

void IOPlacer::excludeInterval(Interval interval)
{
  excluded_intervals_.push_back(interval);
}

void IOPlacer::addNamesConstraint(PinSet* pins, Edge edge, int begin, int end)
{
  Interval interval(edge, begin, end);
  bool inserted = false;
  std::string pin_names;
  int pin_cnt = 0;
  for (odb::dbBTerm* pin : *pins) {
    pin_names += pin->getName() + " ";
    pin_cnt++;
    if (pin_cnt >= pins_per_report_
        && !logger_->debugCheck(utl::PPL, "pin_groups", 1)) {
      pin_names += "... ";
      break;
    }
  }

  if (logger_->debugCheck(utl::PPL, "pin_groups", 1)) {
    debugPrint(logger_,
               utl::PPL,
               "pin_groups",
               1,
               "Restrict pins [ {}] to region {:.2f}u-{:.2f}u at the {} edge.",
               pin_names,
               getBlock()->dbuToMicrons(begin),
               getBlock()->dbuToMicrons(end),
               getEdgeString(edge));
  } else {
    logger_->info(
        utl::PPL,
        48,
        "Restrict pins [ {}] to region {:.2f}u-{:.2f}u at the {} edge.",
        pin_names,
        getBlock()->dbuToMicrons(begin),
        getBlock()->dbuToMicrons(end),
        getEdgeString(edge));
  }

  for (Constraint& constraint : constraints_) {
    if (constraint.interval == interval) {
      constraint.pin_list.insert(pins->begin(), pins->end());
      inserted = true;
      break;
    }
  }

  if (!inserted) {
    constraints_.emplace_back(*pins, Direction::invalid, interval);
  }
}

void IOPlacer::addDirectionConstraint(Direction direction,
                                      Edge edge,
                                      int begin,
                                      int end)
{
  Interval interval(edge, begin, end);
  Constraint constraint(PinSet(), direction, interval);
  constraints_.push_back(constraint);
}

void IOPlacer::addTopLayerConstraint(PinSet* pins, const odb::Rect& region)
{
  Constraint constraint(*pins, Direction::invalid, region);
  constraints_.push_back(constraint);
  for (odb::dbBTerm* bterm : *pins) {
    if (!bterm->getFirstPinPlacementStatus().isFixed()) {
      top_layer_pins_count_++;
    }
  }
}

void IOPlacer::addMirroredPins(odb::dbBTerm* bterm1, odb::dbBTerm* bterm2)
{
  debugPrint(logger_,
             utl::PPL,
             "mirrored_pins",
             1,
             "Mirroring pins {} and {}",
             bterm1->getName(),
             bterm2->getName());
  mirrored_pins_[bterm1] = bterm2;
}

void IOPlacer::addHorLayer(odb::dbTechLayer* layer)
{
  hor_layers_.insert(layer->getRoutingLevel());
}

void IOPlacer::addVerLayer(odb::dbTechLayer* layer)
{
  ver_layers_.insert(layer->getRoutingLevel());
}

void IOPlacer::getPinsFromDirectionConstraint(Constraint& constraint)
{
  if (constraint.direction != Direction::invalid
      && constraint.pin_list.empty()) {
    for (const IOPin& io_pin : netlist_io_pins_->getIOPins()) {
      if (io_pin.getDirection() == constraint.direction) {
        constraint.pin_list.insert(io_pin.getBTerm());
      }
    }
  }
}

std::vector<int> IOPlacer::findPinsForConstraint(const Constraint& constraint,
                                                 Netlist* netlist,
                                                 bool mirrored_only)
{
  std::vector<int> pin_indices;
  const PinSet& pin_list = constraint.pin_list;
  for (odb::dbBTerm* bterm : pin_list) {
    if (bterm->getFirstPinPlacementStatus().isFixed()) {
      continue;
    }
    int idx = netlist->getIoPinIdx(bterm);
    IOPin& io_pin = netlist->getIoPin(idx);
    if ((mirrored_only
         && mirrored_pins_.find(io_pin.getBTerm()) == mirrored_pins_.end())
        || (!mirrored_only
            && mirrored_pins_.find(io_pin.getBTerm())
                   != mirrored_pins_.end())) {
      continue;
    }

    if (io_pin.isMirrored()
        && mirrored_pins_.find(io_pin.getBTerm()) == mirrored_pins_.end()) {
      logger_->warn(PPL,
                    84,
                    "Pin {} is mirrored with another pin. The constraint for "
                    "this pin will be dropped.",
                    io_pin.getName());
      continue;
    }

    if (io_pin.isAssignedToSection() && io_pin.isMirrored()) {
      continue;
    }

    if (!io_pin.isPlaced() && !io_pin.isAssignedToSection()
        && !io_pin.inFallback()) {
      pin_indices.push_back(idx);
    } else if (!io_pin.isInGroup()) {
      logger_->warn(PPL,
                    75,
                    "Pin {} is assigned to more than one constraint, "
                    "using last defined constraint.",
                    io_pin.getName());
    }
  }

  return pin_indices;
}

void IOPlacer::initMirroredPins(bool annealing)
{
  for (IOPin& io_pin : netlist_io_pins_->getIOPins()) {
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      int pin_idx = netlist_io_pins_->getIoPinIdx(io_pin.getBTerm());
      io_pin.setMirrored();
      odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
      int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
      IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);
      mirrored_pin.setMirrored();
      io_pin.setMirrorPinIdx(mirrored_pin_idx);
      mirrored_pin.setMirrorPinIdx(pin_idx);
    }
  }
}

void IOPlacer::initConstraints(bool annealing)
{
  std::reverse(constraints_.begin(), constraints_.end());
  int constraint_idx = 0;
  int constraints_no_slots = 0;
  for (Constraint& constraint : constraints_) {
    getPinsFromDirectionConstraint(constraint);
    constraint.sections = createSectionsPerConstraint(constraint);
    int num_slots = 0;
    const int region_begin = constraint.interval.getBegin();
    const int region_end = constraint.interval.getEnd();
    const std::string region_edge
        = getEdgeString(constraint.interval.getEdge());

    for (const Section& sec : constraint.sections) {
      num_slots += sec.num_slots;
    }
    if (num_slots > 0) {
      constraint.pins_per_slots
          = static_cast<float>(constraint.pin_list.size()) / num_slots;
      if (constraint.pins_per_slots > 1) {
        const Interval& interval = constraint.interval;
        const int interval_length
            = std::abs(interval.getEnd() - interval.getBegin());
        int new_length
            = computeNewRegionLength(interval, constraint.pin_list.size());
        logger_->warn(PPL,
                      110,
                      "Constraint has {} pins, but only {} available slots.\n"
                      "Increase the region {:.2f}um-{:.2f}um on the {} edge "
                      "from {:.2f}um "
                      "to at least {:.2f}um.",
                      constraint.pin_list.size(),
                      num_slots,
                      getBlock()->dbuToMicrons(region_begin),
                      getBlock()->dbuToMicrons(region_end),
                      region_edge,
                      getBlock()->dbuToMicrons(interval_length),
                      getBlock()->dbuToMicrons(new_length));
        constraints_no_slots++;
      }
    } else {
      logger_->error(PPL, 76, "Constraint does not have available slots.");
    }

    for (odb::dbBTerm* term : constraint.pin_list) {
      int pin_idx = netlist_io_pins_->getIoPinIdx(term);
      IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
      io_pin.setConstraintIdx(constraint_idx);
      constraint.pin_indices.push_back(pin_idx);
      if (io_pin.getGroupIdx() != -1) {
        constraint.pin_groups.insert(io_pin.getGroupIdx());
      }
    }
    constraint_idx++;
  }

  if (constraints_no_slots > 0) {
    logger_->error(PPL,
                   111,
                   "{} constraint(s) does not have available slots "
                   "for the pins.",
                   constraints_no_slots);
  }

  if (!annealing) {
    sortConstraints();
  }

  checkPinsInMultipleConstraints();
  checkPinsInMultipleGroups();
}

void IOPlacer::sortConstraints()
{
  std::stable_sort(constraints_.begin(),
                   constraints_.end(),
                   [&](const Constraint& c1, const Constraint& c2) {
                     // treat every non-overlapping constraint as equal, so
                     // stable_sort keeps the user order
                     return (c1.pins_per_slots < c2.pins_per_slots)
                            && overlappingConstraints(c1, c2);
                   });
}

void IOPlacer::checkPinsInMultipleConstraints()
{
  std::string pins_in_mult_constraints;
  if (!constraints_.empty()) {
    for (IOPin& io_pin : netlist_io_pins_->getIOPins()) {
      int constraint_cnt = 0;
      for (Constraint& constraint : constraints_) {
        const PinSet& pin_list = constraint.pin_list;
        if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
            != pin_list.end()) {
          constraint_cnt++;
        }

        if (constraint_cnt > 1) {
          pins_in_mult_constraints.append(" " + io_pin.getName());
          break;
        }
      }
    }

    if (!pins_in_mult_constraints.empty()) {
      logger_->error(PPL,
                     98,
                     "Pins {} are assigned to multiple constraints.",
                     pins_in_mult_constraints);
    }
  }
}

void IOPlacer::checkPinsInMultipleGroups()
{
  std::string pins_in_mult_groups;
  if (!pin_groups_.empty()) {
    for (IOPin& io_pin : netlist_io_pins_->getIOPins()) {
      int group_cnt = 0;
      for (PinGroup& group : pin_groups_) {
        const PinList& pin_list = group.pins;
        if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
            != pin_list.end()) {
          group_cnt++;
        }

        if (group_cnt > 1) {
          pins_in_mult_groups.append(" " + io_pin.getName());
          break;
        }
      }
    }

    if (!pins_in_mult_groups.empty()) {
      logger_->error(PPL,
                     104,
                     "Pins {} are assigned to multiple groups.",
                     pins_in_mult_groups);
    }
  }
}

bool IOPlacer::overlappingConstraints(const Constraint& c1,
                                      const Constraint& c2)
{
  const Interval& interv1 = c1.interval;
  const Interval& interv2 = c2.interval;

  if (interv1.getEdge() == interv2.getEdge()) {
    return std::max(interv1.getBegin(), interv2.getBegin())
           <= std::min(interv1.getEnd(), interv2.getEnd());
  }

  return false;
}

/* static */
Edge IOPlacer::getEdge(const std::string& edge)
{
  if (edge == "top") {
    return Edge::top;
  }
  if (edge == "bottom") {
    return Edge::bottom;
  }
  if (edge == "left") {
    return Edge::left;
  }
  return Edge::right;
}

/* static */
Direction IOPlacer::getDirection(const std::string& direction)
{
  if (direction == "input") {
    return Direction::input;
  }
  if (direction == "output") {
    return Direction::output;
  }
  if (direction == "inout") {
    return Direction::inout;
  }
  return Direction::feedthru;
}

void IOPlacer::addPinGroup(PinList* group, bool order)
{
  std::string pin_names;
  int pin_cnt = 0;
  for (odb::dbBTerm* pin : *group) {
    pin_names += pin->getName() + " ";
    pin_cnt++;
    if (pin_cnt >= pins_per_report_
        && !logger_->debugCheck(utl::PPL, "pin_groups", 1)) {
      pin_names += "... ";
      break;
    }
  }

  if (logger_->debugCheck(utl::PPL, "pin_groups", 1)) {
    debugPrint(
        logger_, utl::PPL, "pin_groups", 1, "Pin group: [ {}]", pin_names);
  } else {
    logger_->info(utl::PPL, 44, "Pin group: [ {}]", pin_names);
  }
  pin_groups_.push_back({*group, order});
}

void IOPlacer::findPinAssignment(std::vector<Section>& sections,
                                 bool mirrored_groups_only)
{
  std::vector<HungarianMatching> hg_vec;
  for (const auto& section : sections) {
    if (!section.pin_indices.empty()) {
      if (section.edge == Edge::invalid) {
        HungarianMatching hg(section,
                             netlist_io_pins_.get(),
                             core_.get(),
                             top_layer_slots_,
                             logger_,
                             db_);
        hg_vec.push_back(hg);
      } else {
        HungarianMatching hg(
            section, netlist_io_pins_.get(), core_.get(), slots_, logger_, db_);
        hg_vec.push_back(hg);
      }
    }
  }

  for (auto& match : hg_vec) {
    match.findAssignmentForGroups();
  }

  for (auto& match : hg_vec) {
    match.getAssignmentForGroups(
        assignment_, mirrored_pins_, mirrored_groups_only);
  }

  for (auto& sec : sections) {
    bool top_layer = sec.edge == Edge::invalid;
    std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;
    updateSection(sec, slots);
  }

  for (auto& match : hg_vec) {
    match.findAssignment();
  }

  if (!mirrored_pins_.empty()) {
    for (auto& match : hg_vec) {
      match.getFinalAssignment(assignment_, mirrored_pins_, true);
    }
  }

  for (auto& match : hg_vec) {
    match.getFinalAssignment(assignment_, mirrored_pins_, false);
  }
}

void IOPlacer::updateSlots()
{
  for (Slot& slot : slots_) {
    slot.blocked = slot.used;
  }
  for (Slot& slot : top_layer_slots_) {
    slot.blocked = slot.used;
  }
}

void IOPlacer::run(bool random_mode)
{
  initParms();

  initNetlistAndCore(hor_layers_, ver_layers_);
  getBlockedRegionsFromMacros();

  initIOLists();
  defineSlots();

  initMirroredPins();
  initConstraints();

  if (random_mode) {
    logger_->info(PPL, 7, "Random pin placement.");
    randomPlacement();
  } else {
    int constrained_pins_cnt = 0;
    int mirrored_pins_cnt = 0;

    // add groups to fallback
    for (const auto& io_group : netlist_io_pins_->getIOGroups()) {
      if (io_group.pin_indices.size() > slots_per_section_) {
        debugPrint(logger_,
                   utl::PPL,
                   "pin_groups",
                   1,
                   "Pin group of size {} does not fit any section. Adding "
                   "to fallback mode.",
                   io_group.pin_indices.size());
        addGroupToFallback(io_group.pin_indices, io_group.order);
      }
    }
    constrained_pins_cnt += placeFallbackPins(false);

    for (bool mirrored_only : {true, false}) {
      for (Constraint& constraint : constraints_) {
        updateConstraintSections(constraint);
        std::vector<Section> sections_for_constraint
            = assignConstrainedPinsToSections(
                constraint, mirrored_pins_cnt, mirrored_only);

        int slots_available = 0;
        for (auto& sec : sections_for_constraint) {
          slots_available += sec.num_slots - sec.used_slots;
        }

        if (slots_available < 0) {
          std::string edge_str = getEdgeString(constraint.interval.getEdge());
          logger_->error(
              PPL,
              88,
              "Cannot assign {} constrained pins to region {:.2f}u-{:.2f}u "
              "at edge {}. Not "
              "enough space in the defined region.",
              constraint.pin_list.size(),
              static_cast<float>(
                  getBlock()->dbuToMicrons(constraint.interval.getBegin())),
              static_cast<float>(
                  getBlock()->dbuToMicrons(constraint.interval.getEnd())),
              edge_str);
        }

        findPinAssignment(sections_for_constraint, mirrored_only);
        updateSlots();

        for (Section& sec : sections_for_constraint) {
          constrained_pins_cnt += sec.pin_indices.size();
        }
        constrained_pins_cnt += mirrored_pins_cnt;
        mirrored_pins_cnt = 0;
      }
    }
    constrained_pins_cnt += placeFallbackPins(false);

    setupSections(constrained_pins_cnt);
    findPinAssignment(sections_, false);
  }

  for (auto& pin : assignment_) {
    updateOrientation(pin);
    updatePinArea(pin);
  }

  if (assignment_.size() != static_cast<int>(netlist_->numIOPins())) {
    logger_->error(PPL,
                   39,
                   "Assigned {} pins out of {} IO pins.",
                   assignment_.size(),
                   netlist_->numIOPins());
  }

  if (!random_mode) {
    reportHPWL();
  }

  checkPinPlacement();
  commitIOPlacementToDB(assignment_);
  writePinPlacement(parms_->getPinPlacementFile().c_str());
  clear();
}

void IOPlacer::setAnnealingConfig(float temperature,
                                  int max_iterations,
                                  int perturb_per_iter,
                                  float alpha)
{
  init_temperature_ = temperature;
  max_iterations_ = max_iterations;
  perturb_per_iter_ = perturb_per_iter;
  alpha_ = alpha;
}

void IOPlacer::setRenderer(
    std::unique_ptr<AbstractIOPlacerRenderer> ioplacer_renderer)
{
  ioplacer_renderer_ = std::move(ioplacer_renderer);
}

AbstractIOPlacerRenderer* IOPlacer::getRenderer()
{
  return ioplacer_renderer_.get();
}

void IOPlacer::setAnnealingDebugOn()
{
  annealing_debug_mode_ = true;
}

bool IOPlacer::isAnnealingDebugOn() const
{
  return annealing_debug_mode_;
}

void IOPlacer::setAnnealingDebugPaintInterval(const int iters_between_paintings)
{
  ioplacer_renderer_->setPaintingInterval(iters_between_paintings);
}

void IOPlacer::setAnnealingDebugNoPauseMode(const bool no_pause_mode)
{
  ioplacer_renderer_->setIsNoPauseMode(no_pause_mode);
}

void IOPlacer::runAnnealing(bool random)
{
  initParms();

  initNetlistAndCore(hor_layers_, ver_layers_);
  getBlockedRegionsFromMacros();

  initIOLists();
  defineSlots();

  initMirroredPins(true);
  initConstraints(true);

  ppl::SimulatedAnnealing annealing(
      netlist_io_pins_.get(), core_.get(), slots_, constraints_, logger_, db_);

  if (isAnnealingDebugOn()) {
    annealing.setDebugOn(std::move(ioplacer_renderer_));
  }

  printConfig(true);

  annealing.run(
      init_temperature_, max_iterations_, perturb_per_iter_, alpha_, random);
  annealing.getAssignment(assignment_);

  for (auto& pin : assignment_) {
    updateOrientation(pin);
    updatePinArea(pin);
  }

  reportHPWL();

  checkPinPlacement();
  commitIOPlacementToDB(assignment_);
  writePinPlacement(parms_->getPinPlacementFile().c_str());
  clear();
}

void IOPlacer::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<odb::Point>> layer_positions_map;

  for (const IOPin& pin : netlist_io_pins_->getIOPins()) {
    int layer = pin.getLayer();

    if (layer_positions_map[layer].empty()) {
      layer_positions_map[layer].push_back(pin.getPosition());
    } else {
      odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
      for (odb::Point& pos : layer_positions_map[layer]) {
        if (pos == pin.getPosition()) {
          logger_->warn(
              PPL,
              106,
              "At least 2 pins in position ({}, {}), layer {}, port {}.",
              pos.x(),
              pos.y(),
              tech_layer->getName(),
              pin.getName().c_str());
          invalid = true;
        }
      }
      layer_positions_map[layer].push_back(pin.getPosition());
    }
  }

  if (invalid) {
    logger_->error(PPL, 107, "Invalid pin placement.");
  }
}

void IOPlacer::reportHPWL()
{
  int64 total_hpwl = computeIONetsHPWL(netlist_io_pins_.get());
  logger_->metric("design__io__hpwl", total_hpwl);
  logger_->info(PPL,
                12,
                "I/O nets HPWL: {:.2f} um.",
                static_cast<float>(getBlock()->dbuToMicrons(total_hpwl)));
}

void IOPlacer::placePin(odb::dbBTerm* bterm,
                        odb::dbTechLayer* layer,
                        int x,
                        int y,
                        int width,
                        int height,
                        bool force_to_die_bound)
{
  if (width == 0 && height == 0) {
    const int database_unit = getTech()->getLefUnits();
    const double min_area
        = static_cast<double>(layer->getArea()) * database_unit * database_unit;
    if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      width = layer->getMinWidth();
      height
          = int(std::max(static_cast<double>(width), ceil(min_area / width)));
    } else {
      height = layer->getMinWidth();
      width
          = int(std::max(static_cast<double>(height), ceil(min_area / height)));
    }
  }
  const int mfg_grid = getTech()->getManufacturingGrid();
  if (width % mfg_grid != 0) {
    width = mfg_grid * std::ceil(static_cast<float>(width) / mfg_grid);
  }
  if (width % 2 != 0) {
    // ensure width is a even multiple of mfg_grid since its divided by 2 later
    width += mfg_grid;
  }

  if (height % mfg_grid != 0) {
    height = mfg_grid * std::ceil(static_cast<float>(height) / mfg_grid);
  }
  if (height % 2 != 0) {
    // ensure height is a even multiple of mfg_grid since its divided by 2 later
    height += mfg_grid;
  }

  odb::Point pos = odb::Point(x, y);

  Rect die_boundary = getBlock()->getDieArea();
  Point lb = die_boundary.ll();
  Point ub = die_boundary.ur();

  float pin_width = std::min(width, height);
  if (pin_width < layer->getWidth()) {
    logger_->error(
        PPL,
        34,
        "Pin {} has dimension {:.2f}u which is less than the min width "
        "{:.2f}u of layer {}.",
        bterm->getName(),
        getBlock()->dbuToMicrons(pin_width),
        getBlock()->dbuToMicrons(layer->getWidth()),
        layer->getName());
  }

  const int layer_level = layer->getRoutingLevel();
  if (force_to_die_bound) {
    movePinToTrack(pos, layer_level, width, height, die_boundary);
    Edge edge;
    odb::dbTrackGrid* track_grid = getBlock()->findTrackGrid(layer);
    int min_spacing, init_track, num_track;
    bool horizontal = layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

    if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      track_grid->getGridPatternY(0, init_track, num_track, min_spacing);
      int dist_lb = abs(pos.x() - lb.x());
      int dist_ub = abs(pos.x() - ub.x());
      edge = (dist_lb < dist_ub) ? Edge::left : Edge::right;
    } else {
      track_grid->getGridPatternX(0, init_track, num_track, min_spacing);
      int dist_lb = abs(pos.y() - lb.y());
      int dist_ub = abs(pos.y() - ub.y());
      edge = (dist_lb < dist_ub) ? Edge::bottom : Edge::top;
    }

    // check the whole pin shape to make sure no overlaps will happen
    // between pins
    bool placed_at_blocked
        = horizontal
              ? checkBlocked(edge, pos.y() - height / 2, layer_level)
                    || checkBlocked(edge, pos.y() + height / 2, layer_level)
              : checkBlocked(edge, pos.x() - width / 2, layer_level)
                    || checkBlocked(edge, pos.x() + width / 2, layer_level);
    bool sum = true;
    int offset_sum = 1;
    int offset_sub = 1;
    int offset = 0;
    while (placed_at_blocked) {
      if (sum) {
        offset = offset_sum * min_spacing;
        offset_sum++;
        sum = false;
      } else {
        offset = -(offset_sub * min_spacing);
        offset_sub++;
        sum = true;
      }

      // check the whole pin shape to make sure no overlaps will happen
      // between pins
      placed_at_blocked
          = horizontal
                ? checkBlocked(edge, pos.y() - height / 2 + offset, layer_level)
                      || checkBlocked(
                          edge, pos.y() + height / 2 + offset, layer_level)
                : checkBlocked(edge, pos.x() - width / 2 + offset, layer_level)
                      || checkBlocked(
                          edge, pos.x() + width / 2 + offset, layer_level);
    }
    pos.addX(horizontal ? 0 : offset);
    pos.addY(horizontal ? offset : 0);
  }

  odb::Point ll = odb::Point(pos.x() - width / 2, pos.y() - height / 2);
  odb::Point ur = odb::Point(pos.x() + width / 2, pos.y() + height / 2);

  odb::dbPlacementStatus placement_status = odb::dbPlacementStatus::FIRM;
  IOPin io_pin
      = IOPin(bterm, pos, Direction::invalid, ll, ur, placement_status);
  io_pin.setLayer(layer_level);

  commitIOPinToDB(io_pin);

  Interval interval = getIntervalFromPin(io_pin, die_boundary);

  excludeInterval(interval);

  logger_->info(PPL,
                70,
                "Pin {} placed at ({:.2f}um, {:.2f}um).",
                bterm->getName(),
                getBlock()->dbuToMicrons(pos.x()),
                getBlock()->dbuToMicrons(pos.y()));
}

void IOPlacer::movePinToTrack(odb::Point& pos,
                              int layer,
                              int width,
                              int height,
                              const Rect& die_boundary)
{
  Point lb = die_boundary.ll();
  Point ub = die_boundary.ur();

  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
  odb::dbTrackGrid* track_grid = getBlock()->findTrackGrid(tech_layer);
  int min_spacing, init_track, num_track;

  if (layer != top_grid_->layer) {  // pin is placed in the die boundaries
    if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      track_grid->getGridPatternY(0, init_track, num_track, min_spacing);
      pos.setY(round(static_cast<double>((pos.y() - init_track)) / min_spacing)
                   * min_spacing
               + init_track);
      int dist_lb = abs(pos.x() - lb_x);
      int dist_ub = abs(pos.x() - ub_x);
      int new_x = (dist_lb < dist_ub) ? lb_x + (width / 2) : ub_x - (width / 2);
      pos.setX(new_x);
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      track_grid->getGridPatternX(0, init_track, num_track, min_spacing);
      pos.setX(round(static_cast<double>((pos.x() - init_track)) / min_spacing)
                   * min_spacing
               + init_track);
      int dist_lb = abs(pos.y() - lb_y);
      int dist_ub = abs(pos.y() - ub_y);
      int new_y
          = (dist_lb < dist_ub) ? lb_y + (height / 2) : ub_y - (height / 2);
      pos.setY(new_y);
    }
  }
}

Interval IOPlacer::getIntervalFromPin(IOPin& io_pin, const Rect& die_boundary)
{
  Edge edge;
  int begin, end, layer;
  Point lb = die_boundary.ll();
  Point ub = die_boundary.ur();

  odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(io_pin.getLayer());
  // sum the half width of the layer to avoid overlaps in adjacent tracks
  int half_width = int(ceil(tech_layer->getWidth()));

  if (tech_layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    // pin is on the left or right edge
    int dist_lb = abs(io_pin.getPosition().x() - lb.x());
    int dist_ub = abs(io_pin.getPosition().x() - ub.x());
    edge = (dist_lb < dist_ub) ? Edge::left : Edge::right;
    begin = io_pin.getLowerBound().y() - half_width;
    end = io_pin.getUpperBound().y() + half_width;
  } else {
    // pin is on the top or bottom edge
    int dist_lb = abs(io_pin.getPosition().y() - lb.y());
    int dist_ub = abs(io_pin.getPosition().y() - ub.y());
    edge = (dist_lb < dist_ub) ? Edge::bottom : Edge::top;
    begin = io_pin.getLowerBound().x() - half_width;
    end = io_pin.getUpperBound().x() + half_width;
  }

  layer = io_pin.getLayer();

  return Interval(edge, begin, end, layer);
}

// db functions
void IOPlacer::populateIOPlacer(const std::set<int>& hor_layer_idx,
                                const std::set<int>& ver_layer_idx)
{
  initCore(hor_layer_idx, ver_layer_idx);
  initNetlist();
}

void IOPlacer::initCore(const std::set<int>& hor_layer_idxs,
                        const std::set<int>& ver_layer_idxs)
{
  int database_unit = getTech()->getLefUnits();

  Rect boundary = getBlock()->getDieArea();

  std::map<int, int> min_spacings_x;
  std::map<int, int> min_spacings_y;
  std::map<int, int> init_tracks_x;
  std::map<int, int> init_tracks_y;
  std::map<int, int> min_areas_x;
  std::map<int, int> min_areas_y;
  std::map<int, int> min_widths_x;
  std::map<int, int> min_widths_y;
  std::map<int, int> num_tracks_x;
  std::map<int, int> num_tracks_y;

  for (int hor_layer_idx : hor_layer_idxs) {
    int min_spacing_y = 0;
    int init_track_y = 0;
    int min_area_y = 0;
    int min_width_y = 0;
    int num_track_y = 0;

    odb::dbTechLayer* hor_layer = getTech()->findRoutingLayer(hor_layer_idx);
    odb::dbTrackGrid* hor_track_grid = getBlock()->findTrackGrid(hor_layer);
    hor_track_grid->getGridPatternY(
        0, init_track_y, num_track_y, min_spacing_y);

    min_area_y = hor_layer->getArea() * database_unit * database_unit;
    min_width_y = hor_layer->getWidth();

    min_spacings_y[hor_layer_idx] = min_spacing_y;
    init_tracks_y[hor_layer_idx] = init_track_y;
    min_areas_y[hor_layer_idx] = min_area_y;
    min_widths_y[hor_layer_idx] = min_width_y;
    num_tracks_y[hor_layer_idx] = num_track_y;
  }

  for (int ver_layer_idx : ver_layer_idxs) {
    int min_spacing_x = 0;
    int init_track_x = 0;
    int min_area_x = 0;
    int min_width_x = 0;
    int num_track_x = 0;

    odb::dbTechLayer* ver_layer = getTech()->findRoutingLayer(ver_layer_idx);
    odb::dbTrackGrid* ver_track_grid = getBlock()->findTrackGrid(ver_layer);
    ver_track_grid->getGridPatternX(
        0, init_track_x, num_track_x, min_spacing_x);

    min_area_x = ver_layer->getArea() * database_unit * database_unit;
    min_width_x = ver_layer->getWidth();

    min_spacings_x[ver_layer_idx] = min_spacing_x;
    init_tracks_x[ver_layer_idx] = init_track_x;
    min_areas_x[ver_layer_idx] = min_area_x;
    min_widths_x[ver_layer_idx] = min_width_x;
    num_tracks_x[ver_layer_idx] = num_track_x;
  }

  *core_ = Core(boundary,
                min_spacings_x,
                min_spacings_y,
                init_tracks_x,
                init_tracks_y,
                num_tracks_x,
                num_tracks_y,
                min_areas_x,
                min_areas_y,
                min_widths_x,
                min_widths_y,
                database_unit);
}

void IOPlacer::addTopLayerPinPattern(odb::dbTechLayer* layer,
                                     int x_step,
                                     int y_step,
                                     const Rect& region,
                                     int pin_width,
                                     int pin_height,
                                     int keepout)
{
  *top_grid_ = {layer->getRoutingLevel(),
                x_step,
                y_step,
                region,
                pin_width,
                pin_height,
                keepout};
}

void IOPlacer::findSlotsForTopLayer()
{
  if (top_layer_slots_.empty() && top_grid_->pin_width > 0) {
    for (int x = top_grid_->llx(); x < top_grid_->urx();
         x += top_grid_->x_step) {
      for (int y = top_grid_->lly(); y < top_grid_->ury();
           y += top_grid_->y_step) {
        top_layer_slots_.push_back(
            {false, false, Point(x, y), top_grid_->layer, Edge::invalid});
      }
    }

    filterObstructedSlotsForTopLayer();
  }
}

void IOPlacer::filterObstructedSlotsForTopLayer()
{
  // Collect top_grid_ obstructions
  std::vector<odb::Rect> obstructions;

  // Get routing obstructions
  for (odb::dbObstruction* obstruction : getBlock()->getObstructions()) {
    odb::dbBox* box = obstruction->getBBox();
    if (box->getTechLayer()->getRoutingLevel() == top_grid_->layer) {
      odb::Rect obstruction_rect = box->getBox();
      obstructions.push_back(obstruction_rect);
    }
  }

  // Get already routed special nets
  for (odb::dbNet* net : getBlock()->getNets()) {
    if (net->isSpecial()) {
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* wire : swire->getWires()) {
          if (!wire->isVia()) {
            if (wire->getTechLayer()->getRoutingLevel() == top_grid_->layer) {
              odb::Rect obstruction_rect = wire->getBox();
              obstructions.push_back(obstruction_rect);
            }
          }
        }
      }
    }
  }

  // Get already placed pins
  for (odb::dbBTerm* term : getBlock()->getBTerms()) {
    for (odb::dbBPin* pin : term->getBPins()) {
      if (pin->getPlacementStatus().isFixed()) {
        for (odb::dbBox* box : pin->getBoxes()) {
          if (box->getTechLayer()->getRoutingLevel() == top_grid_->layer) {
            odb::Rect obstruction_rect = box->getBox();
            obstructions.push_back(obstruction_rect);
          }
        }
      }
    }
  }

  // check for slots that go beyond the die boundary
  odb::Rect die_area = getBlock()->getDieArea();
  for (auto& slot : top_layer_slots_) {
    odb::Point& point = slot.pos;
    if (point.x() - top_grid_->pin_width / 2 < die_area.xMin()
        || point.y() - top_grid_->pin_height / 2 < die_area.yMin()
        || point.x() + top_grid_->pin_width / 2 > die_area.xMax()
        || point.y() + top_grid_->pin_height / 2 > die_area.yMax()) {
      // mark slot as blocked since it extends beyond the die area
      slot.blocked = true;
    }
  }

  // check for slots that overlap with obstructions
  for (odb::Rect& rect : obstructions) {
    for (auto& slot : top_layer_slots_) {
      odb::Point& point = slot.pos;
      // mock slot with keepout
      odb::Rect pin_rect(
          point.x() - top_grid_->pin_width / 2 - top_grid_->keepout,
          point.y() - top_grid_->pin_height / 2 - top_grid_->keepout,
          point.x() + top_grid_->pin_width / 2 + top_grid_->keepout,
          point.y() + top_grid_->pin_height / 2 + top_grid_->keepout);
      if (rect.intersects(pin_rect)) {  // mark slot as blocked
        slot.blocked = true;
      }
    }
  }
}

std::vector<Section> IOPlacer::findSectionsForTopLayer(const odb::Rect& region)
{
  int lb_x = region.xMin();
  int lb_y = region.yMin();
  int ub_x = region.xMax();
  int ub_y = region.yMax();

  std::vector<Section> sections;
  for (int x = top_grid_->llx(); x < top_grid_->urx(); x += top_grid_->x_step) {
    std::vector<Slot>& slots = top_layer_slots_;
    std::vector<Slot>::iterator it
        = std::find_if(slots.begin(), slots.end(), [&](Slot s) {
            return (s.pos.x() >= x && s.pos.x() >= lb_x && s.pos.y() >= lb_y);
          });
    int edge_begin = it - slots.begin();
    int edge_x = slots[edge_begin].pos.x();

    it = std::find_if(slots.begin() + edge_begin, slots.end(), [&](Slot s) {
      return s.pos.x() != edge_x || s.pos.x() >= ub_x || s.pos.y() >= ub_y;
    });
    int edge_end = it - slots.begin() - 1;

    int end_slot = 0;

    while (end_slot < edge_end) {
      int blocked_slots = 0;
      end_slot = edge_begin + slots_per_section_ - 1;
      if (end_slot > edge_end) {
        end_slot = edge_end;
      }
      for (int i = edge_begin; i <= end_slot; ++i) {
        if (slots[i].blocked) {
          blocked_slots++;
        }
      }
      int half_length_pt = edge_begin + (end_slot - edge_begin) / 2;
      Section n_sec;
      n_sec.pos = slots.at(half_length_pt).pos;
      n_sec.num_slots = end_slot - edge_begin - blocked_slots + 1;
      n_sec.begin_slot = edge_begin;
      n_sec.end_slot = end_slot;
      n_sec.used_slots = 0;
      n_sec.edge = Edge::invalid;

      sections.push_back(n_sec);
      edge_begin = ++end_slot;
    }
  }

  return sections;
}

void IOPlacer::initNetlist()
{
  netlist_->reset();
  const Rect& coreBoundary = core_->getBoundary();
  int x_center = (coreBoundary.xMin() + coreBoundary.xMax()) / 2;
  int y_center = (coreBoundary.yMin() + coreBoundary.yMax()) / 2;

  odb::dbSet<odb::dbBTerm> bterms = getBlock()->getBTerms();

  for (odb::dbBTerm* b_term : bterms) {
    if (b_term->getFirstPinPlacementStatus().isFixed()) {
      continue;
    }
    odb::dbNet* net = b_term->getNet();
    if (net == nullptr) {
      logger_->warn(PPL, 38, "Pin {} without net.", b_term->getConstName());
      continue;
    }

    Direction dir = Direction::inout;
    switch (b_term->getIoType().getValue()) {
      case odb::dbIoType::INPUT:
        dir = Direction::input;
        break;
      case odb::dbIoType::OUTPUT:
        dir = Direction::output;
        break;
      default:
        dir = Direction::inout;
    }

    int x_pos = 0;
    int y_pos = 0;
    b_term->getFirstPinLocation(x_pos, y_pos);

    Point bounds(0, 0);
    IOPin io_pin(b_term,
                 Point(x_pos, y_pos),
                 dir,
                 bounds,
                 bounds,
                 odb::dbPlacementStatus::PLACED);

    std::vector<InstancePin> inst_pins;
    odb::dbSet<odb::dbITerm> iterms = net->getITerms();
    odb::dbSet<odb::dbITerm>::iterator i_iter;
    for (i_iter = iterms.begin(); i_iter != iterms.end(); ++i_iter) {
      odb::dbITerm* cur_i_term = *i_iter;
      int x, y;

      if (cur_i_term->getInst()->getPlacementStatus()
              == odb::dbPlacementStatus::NONE
          || cur_i_term->getInst()->getPlacementStatus()
                 == odb::dbPlacementStatus::UNPLACED) {
        x = x_center;
        y = y_center;
      } else {
        cur_i_term->getAvgXY(&x, &y);
      }

      inst_pins.emplace_back(cur_i_term->getInst()->getConstName(),
                             Point(x, y));
    }

    netlist_->addIONet(io_pin, inst_pins);
  }

  int group_idx = 0;
  for (const auto& [pins, order] : pin_groups_) {
    int group_created = netlist_->createIOGroup(pins, order, group_idx);
    if (group_created != pins.size()) {
      logger_->error(PPL, 94, "Cannot create group of size {}.", pins.size());
    }
    group_idx++;
  }
}

void IOPlacer::findConstraintRegion(const Interval& interval,
                                    const Rect& constraint_box,
                                    Rect& region)
{
  const Rect& die_bounds = core_->getBoundary();
  if (interval.getEdge() == Edge::bottom) {
    region = Rect(interval.getBegin(),
                  die_bounds.yMin(),
                  interval.getEnd(),
                  die_bounds.yMin());
  } else if (interval.getEdge() == Edge::top) {
    region = Rect(interval.getBegin(),
                  die_bounds.yMax(),
                  interval.getEnd(),
                  die_bounds.yMax());
  } else if (interval.getEdge() == Edge::left) {
    region = Rect(die_bounds.xMin(),
                  interval.getBegin(),
                  die_bounds.xMin(),
                  interval.getEnd());
  } else if (interval.getEdge() == Edge::right) {
    region = Rect(die_bounds.xMax(),
                  interval.getBegin(),
                  die_bounds.xMax(),
                  interval.getEnd());
  } else {
    region = constraint_box;
  }
}

void IOPlacer::commitConstraintsToDB()
{
  for (Constraint& constraint : constraints_) {
    for (odb::dbBTerm* bterm : constraint.pin_list) {
      int pin_idx = netlist_io_pins_->getIoPinIdx(bterm);
      IOPin& io_pin = netlist_io_pins_->getIoPin(pin_idx);
      Rect constraint_region;
      const Interval& interval = constraint.interval;
      findConstraintRegion(interval, constraint.box, constraint_region);
      bterm->setConstraintRegion(constraint_region);

      if (io_pin.isMirrored()) {
        IOPin& mirrored_pin
            = netlist_io_pins_->getIoPin(io_pin.getMirrorPinIdx());
        odb::dbBTerm* mirrored_bterm
            = getBlock()->findBTerm(mirrored_pin.getName().c_str());
        Edge mirrored_edge = getMirroredEdge(interval.getEdge());
        Rect mirrored_constraint_region;
        Interval mirrored_interval(mirrored_edge,
                                   interval.getBegin(),
                                   interval.getEnd(),
                                   interval.getLayer());
        findConstraintRegion(
            mirrored_interval, constraint.box, mirrored_constraint_region);
        mirrored_bterm->setConstraintRegion(mirrored_constraint_region);
      }
    }
  }
}

void IOPlacer::commitIOPlacementToDB(std::vector<IOPin>& assignment)
{
  for (const IOPin& pin : assignment) {
    commitIOPinToDB(pin);
  }

  commitConstraintsToDB();
}

void IOPlacer::commitIOPinToDB(const IOPin& pin)
{
  odb::dbTechLayer* layer = getTech()->findRoutingLayer(pin.getLayer());

  odb::dbBTerm* bterm = getBlock()->findBTerm(pin.getName().c_str());
  odb::dbSet<odb::dbBPin> bpins = bterm->getBPins();
  odb::dbSet<odb::dbBPin>::iterator bpin_iter;
  std::vector<odb::dbBPin*> all_b_pins;
  for (bpin_iter = bpins.begin(); bpin_iter != bpins.end(); ++bpin_iter) {
    odb::dbBPin* cur_b_pin = *bpin_iter;
    all_b_pins.push_back(cur_b_pin);
  }

  for (odb::dbBPin* bpin : all_b_pins) {
    odb::dbBPin::destroy(bpin);
  }

  Point lower_bound = pin.getLowerBound();
  Point upper_bound = pin.getUpperBound();

  odb::dbBPin* bpin = odb::dbBPin::create(bterm);

  int x_min = lower_bound.x();
  int y_min = lower_bound.y();
  int x_max = upper_bound.x();
  int y_max = upper_bound.y();

  odb::dbBox::create(bpin, layer, x_min, y_min, x_max, y_max);
  bpin->setPlacementStatus(pin.getPlacementStatus());
}

}  // namespace ppl
