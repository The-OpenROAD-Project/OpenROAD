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
#include <random>
#include <sstream>

#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "Slots.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ppl {

using utl::PPL;

IOPlacer::IOPlacer()
    :

      slots_per_section_(0),
      slots_increase_factor_(0),
      logger_(nullptr),
      db_(nullptr)
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
  netlist_io_pins_->clear();
  excluded_intervals_.clear();
  netlist_->clear();
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

void IOPlacer::initNetlistAndCore(std::set<int> hor_layer_idx,
                                  std::set<int> ver_layer_idx)
{
  populateIOPlacer(hor_layer_idx, ver_layer_idx);
}

void IOPlacer::initParms()
{
  slots_per_section_ = 200;
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
    for (std::vector<int>& io_group : netlist_io_pins_->getIOGroups()) {
      const PinList& pin_list = constraint.pin_list;
      IOPin& io_pin = netlist_io_pins_->getIoPin(io_group[0]);
      if (io_pin.isPlaced()) {
        continue;
      }

      if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
          != pin_list.end()) {
        std::vector<int> valid_slots
            = getValidSlots(first_slot, last_slot, top_layer);
        randomPlacement(io_group, valid_slots, top_layer, true);
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
    randomPlacement(pin_indices, valid_slots, top_layer, false);
  }

  for (std::vector<int>& io_group : netlist_io_pins_->getIOGroups()) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(io_group[0]);
    if (io_pin.isPlaced()) {
      continue;
    }
    std::vector<int> valid_slots = getValidSlots(0, slots_.size() - 1, false);

    randomPlacement(io_group, valid_slots, false, true);
  }

  std::vector<int> valid_slots = getValidSlots(0, slots_.size() - 1, false);

  std::vector<int> pin_indices;
  for (int i = 0; i < netlist_io_pins_->numIOPins(); i++) {
    if (!netlist_io_pins_->getIoPin(i).isPlaced()) {
      pin_indices.push_back(i);
    }
  }

  randomPlacement(pin_indices, valid_slots, false, false);
}

void IOPlacer::randomPlacement(std::vector<int> pin_indices,
                               std::vector<int> slot_indices,
                               bool top_layer,
                               bool is_group)
{
  if (pin_indices.size() > slot_indices.size()) {
    logger_->error(PPL,
                   72,
                   "Number of pins ({}) exceed number of valid positions ({}).",
                   pin_indices.size(),
                   slot_indices.size());
  }

  const auto seed = parms_->getRandSeed();

  int num_i_os = pin_indices.size();
  int num_slots = slot_indices.size();
  double shift = is_group ? 1 : num_slots / double(num_i_os);
  std::vector<int> vSlots(num_slots);
  std::vector<int> io_pin_indices(num_i_os);

  std::vector<InstancePin> instPins;
  if (sections_.size() < 1) {
    Section s = {Point(0, 0)};
    sections_.push_back(s);
  }

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
      slots[slot_idx].used = true;
      slots[slot_idx].blocked = true;
      io_pin.setLayer(slots[slot_idx].layer);
      assignment_.push_back(io_pin);
      sections_[0].pin_indices.push_back(pin_idx);
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

  return slot_idx;
}

void IOPlacer::initIOLists()
{
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

  for (PinGroup pin_group : pin_groups_) {
    netlist_io_pins_->createIOGroup(pin_group);
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
          && pos >= blocked_interval.getBegin()
          && pos <= blocked_interval.getEnd()) {
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
    intervals.push_back(Interval(Edge::bottom, box.xMin(), box.xMax()));
  }
  // check intersect top edge
  if (die_area.yMax() == box.yMax()) {
    intervals.push_back(Interval(Edge::top, box.xMin(), box.xMax()));
  }
  // check intersect left edge
  if (die_area.xMin() == box.xMin()) {
    intervals.push_back(Interval(Edge::left, box.yMin(), box.yMax()));
  }
  // check intersect right edge
  if (die_area.xMax() == box.xMax()) {
    intervals.push_back(Interval(Edge::right, box.yMin(), box.yMax()));
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

double IOPlacer::dbuToMicrons(int64_t dbu)
{
  return (double) dbu / (getBlock()->getDbUnitsPerMicron());
}

void IOPlacer::findSlots(const std::set<int>& layers, Edge edge)
{
  const int default_min_dist = 2;
  Point lb = core_->getBoundary().ll();
  Point ub = core_->getBoundary().ur();

  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  bool vertical = (edge == Edge::top || edge == Edge::bottom);
  int min = vertical ? lb_x : lb_y;
  int max = vertical ? ub_x : ub_y;

  int offset = parms_->getCornerAvoidance();

  int i = 0;
  bool dist_in_tracks = parms_->getMinDistanceInTracks();
  for (int layer : layers) {
    int curr_x, curr_y, start_idx, end_idx;
    // get the on grid min distance
    int tech_min_dst
        = vertical ? core_->getMinDstPinsX()[i] : core_->getMinDstPinsY()[i];
    int min_dst_pins
        = dist_in_tracks
              ? tech_min_dst * parms_->getMinDistance()
              : tech_min_dst
                    * std::ceil(static_cast<float>(parms_->getMinDistance())
                                / tech_min_dst);

    min_dst_pins
        = (min_dst_pins == 0) ? default_min_dist * tech_min_dst : min_dst_pins;

    int init_tracks
        = vertical ? core_->getInitTracksX()[i] : core_->getInitTracksY()[i];
    int num_tracks
        = vertical ? core_->getNumTracksX()[i] : core_->getNumTracksY()[i];

    float thickness_multiplier
        = vertical ? parms_->getVerticalThicknessMultiplier()
                   : parms_->getHorizontalThicknessMultiplier();

    int half_width = vertical ? int(ceil(core_->getMinWidthX()[i] / 2.0))
                              : int(ceil(core_->getMinWidthY()[i] / 2.0));

    half_width *= thickness_multiplier;

    int num_tracks_offset = std::ceil(offset / min_dst_pins);

    start_idx
        = std::max(0.0, ceil((min + half_width - init_tracks) / min_dst_pins))
          + num_tracks_offset;
    end_idx = std::min((num_tracks - 1),
                       static_cast<int>(floor((max - half_width - init_tracks)
                                              / min_dst_pins)))
              - num_tracks_offset;
    if (vertical) {
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
      if (vertical) {
        curr_x += min_dst_pins;
      } else {
        curr_y += min_dst_pins;
      }
    }

    if (edge == Edge::top || edge == Edge::left) {
      std::reverse(slots.begin(), slots.end());
    }

    for (Point pos : slots) {
      curr_x = pos.getX();
      curr_y = pos.getY();
      bool blocked = vertical ? checkBlocked(edge, curr_x, layer)
                              : checkBlocked(edge, curr_y, layer);
      slots_.push_back({blocked, false, Point(curr_x, curr_y), layer, edge});
    }
    i++;
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
    Section n_sec = {slots_.at(half_length_pt).pos};
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
    const Constraint& constraint)
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
              if (edge == Edge::bottom || edge == Edge::right)
                return (s.edge == edge && s.layer == layer
                        && slot_xy >= interv.getBegin());
              return (s.edge == edge && s.layer == layer
                      && slot_xy <= interv.getEnd());
            });
      int constraint_begin = it - slots_.begin();

      it = std::find_if(
          slots_.begin() + constraint_begin, slots_.end(), [&](const Slot& s) {
            int slot_xy = (edge == Edge::left || edge == Edge::right)
                              ? s.pos.y()
                              : s.pos.x();
            if (edge == Edge::bottom || edge == Edge::right)
              return (slot_xy >= interv.getEnd() || s.edge != edge
                      || s.layer != layer);
            return (slot_xy <= interv.getBegin() || s.edge != edge
                    || s.layer != layer);
          });
      int constraint_end = it - slots_.begin() - 1;

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

std::vector<Section> IOPlacer::assignConstrainedPinsToSections(
    Constraint& constraint,
    int& mirrored_pins_cnt,
    bool mirrored_only)
{
  bool top_layer = constraint.interval.getEdge() == Edge::invalid;
  std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;
  if (!mirrored_only) {
    assignConstrainedGroupsToSections(constraint, constraint.sections);
  }

  int total_slots_count = 0;
  for (Section& sec : constraint.sections) {
    int new_slots_count = 0;
    for (int slot_idx = sec.begin_slot; slot_idx <= sec.end_slot; slot_idx++) {
      new_slots_count
          += (slots[slot_idx].blocked || slots[slot_idx].used) ? 0 : 1;
    }
    sec.num_slots = new_slots_count;
    total_slots_count += new_slots_count;
  }

  std::vector<int> pin_indices = findPinsForConstraint(
      constraint, netlist_io_pins_.get(), mirrored_only);

  if (pin_indices.size() > total_slots_count) {
    logger_->error(PPL,
                   74,
                   "Number of pins ({}) exceed number of valid positions ({}) "
                   "for constraint.",
                   pin_indices.size(),
                   total_slots_count);
  }

  for (int idx : pin_indices) {
    IOPin& io_pin = netlist_io_pins_->getIoPin(idx);
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      mirrored_pins_cnt++;
    }
    assignPinToSection(io_pin, idx, constraint.sections);
  }

  return constraint.sections;
}

void IOPlacer::assignConstrainedGroupsToSections(Constraint& constraint,
                                                 std::vector<Section>& sections)
{
  for (std::vector<int>& io_group : netlist_io_pins_->getIOGroups()) {
    const PinList& pin_list = constraint.pin_list;
    IOPin& io_pin = netlist_io_pins_->getIoPin(io_group[0]);

    if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm())
        != pin_list.end()) {
      assignGroupToSection(io_group, sections);
    }
  }
}

int IOPlacer::assignGroupsToSections()
{
  int total_pins_assigned = 0;

  for (std::vector<int>& io_group : netlist_io_pins_->getIOGroups()) {
    total_pins_assigned += assignGroupToSection(io_group, sections_);
  }

  return total_pins_assigned;
}

int IOPlacer::assignGroupToSection(const std::vector<int>& io_group,
                                   std::vector<Section>& sections)
{
  Netlist* net = netlist_io_pins_.get();
  int group_size = io_group.size();
  bool group_assigned = false;
  int total_pins_assigned = 0;

  IOPin& io_pin = net->getIoPin(io_group[0]);

  if (!io_pin.isAssignedToSection()) {
    std::vector<int64_t> dst(sections.size(), 0);
    for (int i = 0; i < sections.size(); i++) {
      for (int pin_idx : io_group) {
        int pin_hpwl = net->computeIONetHPWL(pin_idx, sections[i].pos);
        if (pin_hpwl == std::numeric_limits<int>::max()) {
          dst[i] = pin_hpwl;
          break;
        } else {
          dst[i] += pin_hpwl;
        }
      }
    }
    for (auto i : sortIndexes(dst)) {
      if (sections[i].used_slots + group_size <= sections[i].num_slots) {
        std::vector<int> group;
        for (int pin_idx : io_group) {
          IOPin& io_pin = net->getIoPin(pin_idx);
          sections[i].pin_indices.push_back(pin_idx);
          group.push_back(pin_idx);
          sections[i].used_slots++;
          io_pin.assignToSection();
        }
        total_pins_assigned += group_size;
        sections[i].pin_groups.push_back(group);
        group_assigned = true;
        break;
      } else {
        int available_slots = sections[i].num_slots - sections[i].used_slots;
        logger_->warn(PPL,
                      78,
                      "Not enough available positions ({}) to place the pin "
                      "group of size {}.",
                      available_slots,
                      group_size);
      }
    }
    if (!group_assigned) {
      logger_->error(PPL, 42, "Unsuccessfully assigned I/O groups.");
    }
  }

  return total_pins_assigned;
}

bool IOPlacer::assignPinsToSections(int assigned_pins_count)
{
  Netlist* net = netlist_io_pins_.get();
  std::vector<Section>& sections = sections_;

  createSections();

  int total_pins_assigned = assignGroupsToSections();

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

  total_pins_assigned += assigned_pins_count;

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
  } else {
    logger_->info(PPL,
                  9,
                  "Unsuccessfully assigned pins to sections ({} out of {}).",
                  total_pins_assigned,
                  net->numIOPins());
    return false;
  }
}

bool IOPlacer::assignPinToSection(IOPin& io_pin,
                                  int idx,
                                  std::vector<Section>& sections)
{
  bool pin_assigned = false;

  if (!io_pin.isInGroup() && !io_pin.isAssignedToSection()) {
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
          odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
          int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
          IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);
          // Mark mirrored pin as assigned to section to prevent assigning it to
          // another section that is not aligned with his pair
          mirrored_pin.assignToSection();
        }
        break;
      }
    }
  }

  return pin_assigned;
}

void IOPlacer::printConfig()
{
  logger_->info(PPL, 1, "Number of slots          {}", slots_.size());
  logger_->info(PPL, 2, "Number of I/O            {}", netlist_->numIOPins());
  logger_->metric("floorplan__design__io", netlist_->numIOPins());
  logger_->info(
      PPL, 3, "Number of I/O w/sink     {}", netlist_io_pins_->numIOPins());
  logger_->info(PPL, 4, "Number of I/O w/o sink   {}", zero_sink_ios_.size());
  logger_->info(PPL, 5, "Slots per section        {}", slots_per_section_);
  logger_->info(
      PPL, 6, "Slots increase factor    {:.1}", slots_increase_factor_);
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
    } else {
      pin.setOrientation(Orientation::east);
      return;
    }
  }
  if (x == upper_x_bound) {
    if (y == lower_y_bound) {
      pin.setOrientation(Orientation::north);
      return;
    } else {
      pin.setOrientation(Orientation::west);
      return;
    }
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
    int index = -1;
    int required_min_area = 0;

    int i = 0;
    for (int layer : hor_layers_) {
      if (layer == pin.getLayer())
        index = i;
      i++;
    }

    i = 0;
    for (int layer : ver_layers_) {
      if (layer == pin.getLayer())
        index = i;
      i++;
    }

    if (index == -1) {
      logger_->error(PPL,
                     77,
                     "Layer {} of Pin {} not found.",
                     pin.getLayer(),
                     pin.getName());
    }

    if (pin.getOrientation() == Orientation::north
        || pin.getOrientation() == Orientation::south) {
      float thickness_multiplier = parms_->getVerticalThicknessMultiplier();
      int half_width = int(ceil(core_->getMinWidthX()[index] / 2.0))
                       * thickness_multiplier;
      int height = int(
          std::max(2.0 * half_width,
                   ceil(core_->getMinAreaX()[index] / (2.0 * half_width))));
      required_min_area = core_->getMinAreaX()[index];

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
      int half_width = int(ceil(core_->getMinWidthY()[index] / 2.0))
                       * thickness_multiplier;
      int height = int(
          std::max(2.0 * half_width,
                   ceil(core_->getMinAreaY()[index] / (2.0 * half_width))));
      required_min_area = core_->getMinAreaY()[index];

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
                     dbuToMicrons(dbuToMicrons(pin.getArea())),
                     dbuToMicrons(dbuToMicrons(required_min_area)));
    }
  } else {
    int pin_width = top_grid_->pin_width;
    int pin_height = top_grid_->pin_height;

    if (pin_width % mfg_grid != 0) {
      pin_width
          = mfg_grid * std::ceil(static_cast<float>(pin_width) / mfg_grid);
    }

    if (pin_height % mfg_grid != 0) {
      pin_height
          = mfg_grid * std::ceil(static_cast<float>(pin_height) / mfg_grid);
    }

    pin.setLowerBound(pin.getX() - pin_width / 2, pin.getY() - pin_height / 2);
    pin.setUpperBound(pin.getX() + pin_width / 2, pin.getY() + pin_height / 2);
  }
}

int IOPlacer::computeIONetsHPWL(Netlist* netlist)
{
  int hpwl = 0;
  int idx = 0;
  for (IOPin& io_pin : netlist->getIOPins()) {
    hpwl += netlist->computeIONetHPWL(idx, io_pin.getPosition());
    idx++;
  }

  return hpwl;
}

int IOPlacer::computeIONetsHPWL()
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

void IOPlacer::addNamesConstraint(PinList* pins, Edge edge, int begin, int end)
{
  Interval interval(edge, begin, end);
  constraints_.push_back(Constraint(*pins, Direction::invalid, interval));
}

void IOPlacer::addDirectionConstraint(Direction direction,
                                      Edge edge,
                                      int begin,
                                      int end)
{
  Interval interval(edge, begin, end);
  Constraint constraint(PinList(), direction, interval);
  constraints_.push_back(constraint);
}

void IOPlacer::addTopLayerConstraint(PinList* pins, const odb::Rect& region)
{
  Constraint constraint(*pins, Direction::invalid, region);
  constraints_.push_back(constraint);
}

void IOPlacer::addMirroredPins(odb::dbBTerm* bterm1, odb::dbBTerm* bterm2)
{
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
  const PinList& pin_list = constraint.pin_list;
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

    if (!io_pin.isPlaced() && !io_pin.isAssignedToSection()) {
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

void IOPlacer::initMirroredPins()
{
  for (IOPin& io_pin : netlist_io_pins_.get()->getIOPins()) {
    if (mirrored_pins_.find(io_pin.getBTerm()) != mirrored_pins_.end()) {
      io_pin.setMirrored();
      odb::dbBTerm* mirrored_term = mirrored_pins_[io_pin.getBTerm()];
      int mirrored_pin_idx = netlist_io_pins_->getIoPinIdx(mirrored_term);
      IOPin& mirrored_pin = netlist_io_pins_->getIoPin(mirrored_pin_idx);
      mirrored_pin.setMirrored();
    }
  }
}

void IOPlacer::initConstraints()
{
  std::reverse(constraints_.begin(), constraints_.end());
  for (Constraint& constraint : constraints_) {
    getPinsFromDirectionConstraint(constraint);
    constraint.sections = createSectionsPerConstraint(constraint);
    int num_slots = 0;
    for (Section sec : constraint.sections) {
      num_slots += sec.num_slots;
    }
    if (num_slots > 0) {
      constraint.pins_per_slots
          = static_cast<float>(constraint.pin_list.size()) / num_slots;
    } else {
      logger_->error(
          PPL, 76, "Constraint does not have available slots for its pins.");
    }
  }
  sortConstraints();
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
  } else if (edge == "bottom") {
    return Edge::bottom;
  } else if (edge == "left") {
    return Edge::left;
  } else {
    return Edge::right;
  }

  return Edge::invalid;
}

/* static */
Direction IOPlacer::getDirection(const std::string& direction)
{
  if (direction == "input") {
    return Direction::input;
  } else if (direction == "output") {
    return Direction::output;
  } else if (direction == "inout") {
    return Direction::inout;
  } else {
    return Direction::feedthru;
  }

  return Direction::invalid;
}

void IOPlacer::addPinGroup(PinGroup* group)
{
  pin_groups_.push_back(*group);
}

void IOPlacer::findPinAssignment(std::vector<Section>& sections)
{
  std::vector<HungarianMatching> hg_vec;
  for (int idx = 0; idx < sections.size(); idx++) {
    if (sections[idx].pin_indices.size() > 0) {
      if (sections[idx].edge == Edge::invalid) {
        HungarianMatching hg(sections[idx],
                             netlist_io_pins_.get(),
                             core_.get(),
                             top_layer_slots_,
                             logger_,
                             db_);
        hg_vec.push_back(hg);
      } else {
        HungarianMatching hg(sections[idx],
                             netlist_io_pins_.get(),
                             core_.get(),
                             slots_,
                             logger_,
                             db_);
        hg_vec.push_back(hg);
      }
    }
  }

  for (int idx = 0; idx < hg_vec.size(); idx++) {
    hg_vec[idx].findAssignmentForGroups();
  }

  for (int idx = 0; idx < hg_vec.size(); idx++) {
    hg_vec[idx].getAssignmentForGroups(assignment_);
  }

  for (int idx = 0; idx < hg_vec.size(); idx++) {
    hg_vec[idx].findAssignment();
  }

  if (!mirrored_pins_.empty()) {
    for (int idx = 0; idx < hg_vec.size(); idx++) {
      hg_vec[idx].getFinalAssignment(assignment_, mirrored_pins_, true);
    }
  }

  for (int idx = 0; idx < hg_vec.size(); idx++) {
    hg_vec[idx].getFinalAssignment(assignment_, mirrored_pins_, false);
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
    for (bool mirrored_only : {true, false}) {
      std::vector<Section> sections_for_constraint;
      for (Constraint& constraint : constraints_) {
        sections_for_constraint = assignConstrainedPinsToSections(
            constraint, mirrored_pins_cnt, mirrored_only);

        findPinAssignment(sections_for_constraint);
        updateSlots();

        if (!mirrored_only) {
          for (Section& sec : sections_for_constraint) {
            constrained_pins_cnt += sec.pin_indices.size();
          }
          constrained_pins_cnt += mirrored_pins_cnt;
          mirrored_pins_cnt = 0;
        }
      }
    }

    setupSections(constrained_pins_cnt);

    findPinAssignment(sections_);
  }

  for (int i = 0; i < assignment_.size(); ++i) {
    updateOrientation(assignment_[i]);
    updatePinArea(assignment_[i]);
  }

  if (assignment_.size() != static_cast<int>(netlist_->numIOPins())) {
    logger_->error(PPL,
                   39,
                   "Assigned {} pins out of {} IO pins.",
                   assignment_.size(),
                   netlist_->numIOPins());
  }

  if (!random_mode) {
    int total_hpwl = computeIONetsHPWL(netlist_io_pins_.get());
    logger_->info(PPL,
                  12,
                  "I/O nets HPWL: {:.2f} um.",
                  static_cast<float>(dbuToMicrons(total_hpwl)));
  }

  commitIOPlacementToDB(assignment_);
  clear();
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
    const int min_area = layer->getArea() * database_unit * database_unit;
    if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      width = layer->getMinWidth();
      height = int(std::max((double) width, ceil(min_area / width)));
    } else {
      height = layer->getMinWidth();
      width = int(std::max((double) height, ceil(min_area / height)));
    }
  }
  const int mfg_grid = getTech()->getManufacturingGrid();
  if (width % mfg_grid != 0) {
    width = mfg_grid * std::ceil(static_cast<float>(width) / mfg_grid);
  }

  if (height % mfg_grid != 0) {
    height = mfg_grid * std::ceil(static_cast<float>(height) / mfg_grid);
  }

  odb::Point pos = odb::Point(x, y);

  Rect die_boundary = getBlock()->getDieArea();
  Point lb = die_boundary.ll();
  Point ub = die_boundary.ur();

  float pin_width = std::min(width, height);
  if (pin_width < layer->getWidth()) {
    logger_->error(PPL,
                   34,
                   "Pin {} has dimension {}u which is less than the min width "
                   "{}u of layer {}.",
                   bterm->getName(),
                   dbuToMicrons(pin_width),
                   dbuToMicrons(layer->getWidth()),
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

  IOPin io_pin = IOPin(
      bterm, pos, Direction::invalid, ll, ur, odb::dbPlacementStatus::FIRM);
  io_pin.setLayer(layer_level);

  commitIOPinToDB(io_pin);

  Interval interval = getIntervalFromPin(io_pin, die_boundary);

  excludeInterval(interval);

  logger_->info(PPL,
                70,
                "Pin {} placed at ({}um, {}um).",
                bterm->getName(),
                pos.x() / getTech()->getLefUnits(),
                pos.y() / getTech()->getLefUnits());
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
      pos.setY(round((pos.y() - init_track) / min_spacing) * min_spacing
               + init_track);
      int dist_lb = abs(pos.x() - lb_x);
      int dist_ub = abs(pos.x() - ub_x);
      int new_x = (dist_lb < dist_ub) ? lb_x + (width / 2) : ub_x - (width / 2);
      pos.setX(new_x);
    } else if (tech_layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
      track_grid->getGridPatternX(0, init_track, num_track, min_spacing);
      pos.setX(round((pos.x() - init_track) / min_spacing) * min_spacing
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
void IOPlacer::populateIOPlacer(std::set<int> hor_layer_idx,
                                std::set<int> ver_layer_idx)
{
  initCore(hor_layer_idx, ver_layer_idx);
  initNetlist();
}

void IOPlacer::initCore(std::set<int> hor_layer_idxs,
                        std::set<int> ver_layer_idxs)
{
  int database_unit = getTech()->getLefUnits();

  Rect boundary = getBlock()->getDieArea();

  std::vector<int> min_spacings_x;
  std::vector<int> min_spacings_y;
  std::vector<int> init_tracks_x;
  std::vector<int> init_tracks_y;
  std::vector<int> min_areas_x;
  std::vector<int> min_areas_y;
  std::vector<int> min_widths_x;
  std::vector<int> min_widths_y;
  std::vector<int> num_tracks_x;
  std::vector<int> num_tracks_y;

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

    min_spacings_y.push_back(min_spacing_y);
    init_tracks_y.push_back(init_track_y);
    min_areas_y.push_back(min_area_y);
    min_widths_y.push_back(min_width_y);
    num_tracks_y.push_back(num_track_y);
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

    min_spacings_x.push_back(min_spacing_x);
    init_tracks_x.push_back(init_track_x);
    min_areas_x.push_back(min_area_x);
    min_widths_x.push_back(min_width_x);
    num_tracks_x.push_back(num_track_x);
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
      Section n_sec = {slots.at(half_length_pt).pos};
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
    switch (b_term->getIoType()) {
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

      inst_pins.push_back(
          InstancePin(cur_i_term->getInst()->getConstName(), Point(x, y)));
    }

    netlist_->addIONet(io_pin, inst_pins);
  }

  int group_idx = 0;
  for (PinGroup pin_group : pin_groups_) {
    int group_created = netlist_->createIOGroup(pin_group);
    if (group_created == pin_group.size()) {
      group_idx++;
    }
  }
}

void IOPlacer::commitIOPlacementToDB(std::vector<IOPin>& assignment)
{
  for (const IOPin& pin : assignment) {
    commitIOPinToDB(pin);
  }
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
