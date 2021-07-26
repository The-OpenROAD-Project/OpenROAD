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

#include "opendb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace ppl {

using utl::PPL;

void IOPlacer::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  parms_ = std::make_unique<Parameters>();
  top_grid_ = TopLayerGrid(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
}

void IOPlacer::clear()
{
  hor_layers_.clear();
  ver_layers_.clear();
  top_grid_ = TopLayerGrid();
  zero_sink_ios_.clear();
  sections_.clear();
  slots_.clear();
  top_layer_slots_.clear();
  assignment_.clear();
  netlist_io_pins_.clear();
  excluded_intervals_.clear();
  netlist_.clear();
  pin_groups_.clear();
  *parms_ = Parameters();
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
  netlist_ = Netlist();
  netlist_io_pins_ = Netlist();

  if (parms_->getNumSlots() > -1) {
    slots_per_section_ = parms_->getNumSlots();
  }

  if (parms_->getSlotsFactor() > -1) {
    slots_increase_factor_ = parms_->getSlotsFactor();
  }
}

std::vector<int> IOPlacer::getValidSlots(int first, int last, bool top_layer) {
  std::vector<int> valid_slots;

  std::vector<Slot> &slots = top_layer ? top_layer_slots_ : slots_;

  for (int i = first; i <= last; i++) {
    if (!slots[i].blocked) {
      valid_slots.push_back(i);
    }
  }

  return valid_slots;
}

void IOPlacer::randomPlacement()
{
  for (Constraint &constraint : constraints_) {
    int first_slot = constraint.sections.front().begin_slot;
    int last_slot = constraint.sections.back().end_slot;

    bool top_layer = constraint.interval.edge == Edge::invalid;
    for (std::vector<int>& io_group : netlist_.getIOGroups()) {
      const PinList& pin_list = constraint.pin_list;
      IOPin& io_pin = netlist_.getIoPin(io_group[0]);
      if (io_pin.isPlaced()) {
        continue;
      }

      if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm()) != pin_list.end()) {
        std::vector<int> valid_slots = getValidSlots(first_slot, last_slot, top_layer);
        randomPlacement(io_group, valid_slots, top_layer, true);
      }
    }

    std::vector<int> valid_slots = getValidSlots(first_slot, last_slot, top_layer);
    std::vector<int> pin_indices = findPinsForConstraint(constraint, netlist_);
    randomPlacement(pin_indices, valid_slots, top_layer, false);
  }

  for (std::vector<int>& io_group : netlist_.getIOGroups()) {
    IOPin& io_pin = netlist_.getIoPin(io_group[0]);
    if (io_pin.isPlaced()) {
      continue;
    }
    std::vector<int> valid_slots = getValidSlots(0, slots_.size()-1, false);

    randomPlacement(io_group, valid_slots, false, true);
  }

  std::vector<int> valid_slots = getValidSlots(0, slots_.size()-1, false);

  std::vector<int> pin_indices;
  for (int i = 0; i < netlist_.numIOPins(); i++) {
    if (!netlist_.getIoPin(i).isPlaced()) {
      pin_indices.push_back(i);
    }
  }

  randomPlacement(pin_indices, valid_slots, false, false);
}

void IOPlacer::randomPlacement(std::vector<int> pin_indices, std::vector<int> slot_indices, bool top_layer, bool is_group)
{
  if (pin_indices.size() > slot_indices.size()) {
    logger_->error(PPL, 72, "Number of pins ({}) exceed number of valid positions ({}).", pin_indices.size(), slot_indices.size());
  }

  const double seed = parms_->getRandSeed();

  int num_i_os = pin_indices.size();
  int num_slots = slot_indices.size();
  double shift = is_group ? 1 : num_slots / double(num_i_os);
  int idx = 0;
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

  std::vector<Slot> &slots = top_layer ? top_layer_slots_ : slots_;

  std::vector<IOPin>& io_pins = netlist_.getIOPins();
  int io_idx = 0;
  for (int pin_idx : pin_indices) {
    int b = io_pin_indices[io_idx];
    int slot_idx = slot_indices[floor(b * shift)];
    IOPin& io_pin = io_pins[pin_idx];
    io_pin.setPos(slots[slot_idx].pos);
    io_pin.setPlaced();
    slots[slot_idx].used = true;
    slots[slot_idx].blocked = true;
    io_pin.setLayer(slots[slot_idx].layer);
    assignment_.push_back(io_pin);
    sections_[0].pin_indices.push_back(pin_idx);
    io_idx++;
  }
}

void IOPlacer::initIOLists()
{
  int idx = 0;
  for (IOPin& io_pin : netlist_.getIOPins()) {
    std::vector<InstancePin> inst_pins_vector;
    if (netlist_.numSinksOfIO(idx) != 0) {
      netlist_.getSinksOfIO(idx, inst_pins_vector);
      netlist_io_pins_.addIONet(io_pin, inst_pins_vector);
    } else {
      zero_sink_ios_.push_back(io_pin);
      netlist_io_pins_.addIONet(io_pin, inst_pins_vector);
    }
    idx++;
  }

  for (PinGroup pin_group : pin_groups_) {
    netlist_io_pins_.createIOGroup(pin_group);
  }
}

bool IOPlacer::checkBlocked(Edge edge, int pos)
{
  for (Interval blocked_interval : excluded_intervals_) {
    if (blocked_interval.getEdge() == edge && pos >= blocked_interval.getBegin()
        && pos <= blocked_interval.getEnd()) {
      return true;
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
    intervals.push_back(
      Interval(Edge::bottom, box.xMin(), box.xMax()));
  }
  // check intersect top edge
  if (die_area.yMax() == box.yMax()) {
    intervals.push_back(
      Interval(Edge::top, box.xMin(), box.xMax()));
  }
  // check intersect left edge
  if (die_area.xMin() == box.xMin()) {
    intervals.push_back(
      Interval(Edge::left, box.yMin(), box.yMax()));
  }
  // check intersect right edge
  if (die_area.xMax() == box.xMax()) {
    intervals.push_back(
      Interval(Edge::right, box.yMin(), box.yMax()));
  }

  return intervals;
}

void IOPlacer::getBlockedRegionsFromMacros()
{
  odb::Rect die_area;
  block_->getDieArea(die_area);

  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (master->isBlock() && inst->isPlaced()) {
      odb::Rect inst_area;
      inst->getBBox()->getBox(inst_area);
      odb::Rect intersect = die_area.intersect(inst_area);

      std::vector<Interval> intervals = findBlockedIntervals(die_area, intersect);
      for (Interval interval : intervals) {
        excludeInterval(interval);
      }
    }
  }
}

void IOPlacer::getBlockedRegionsFromDbObstructions()
{
  odb::Rect die_area;
  block_->getDieArea(die_area);

  for (odb::dbObstruction* obstruction : block_->getObstructions()) {
    odb::dbBox* obstructBox = obstruction->getBBox();
    odb::Rect obstructArea;
    obstructBox->getBox(obstructArea);
    odb::Rect intersect = die_area.intersect(obstructArea);

    std::vector<Interval> intervals = findBlockedIntervals(die_area, intersect);
    for (Interval interval : intervals) {
      excludeInterval(interval);
    }
  }
}

void IOPlacer::findSlots(const std::set<int>& layers, Edge edge)
{
  const int default_min_dist = 2;
  Point lb = core_.getBoundary().ll();
  Point ub = core_.getBoundary().ur();

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
    int min_dst_ver = dist_in_tracks ?
                      core_.getMinDstPinsX()[i]*parms_->getMinDistance() :
                      core_.getMinDstPinsX()[i]*
                      std::ceil(static_cast<float>(parms_->getMinDistance())/core_.getMinDstPinsX()[i]);
    int min_dst_hor = dist_in_tracks ?
                      core_.getMinDstPinsY()[i]*parms_->getMinDistance() :
                      core_.getMinDstPinsY()[i]*
                      std::ceil(static_cast<float>(parms_->getMinDistance())/core_.getMinDstPinsY()[i]);

    min_dst_ver = (min_dst_ver == 0) ? default_min_dist*core_.getMinDstPinsX()[i] : min_dst_ver;
    min_dst_hor = (min_dst_hor == 0) ? default_min_dist*core_.getMinDstPinsY()[i] : min_dst_hor;

    int min_dst_pins
        = vertical ? min_dst_ver
                   : min_dst_hor;
    int init_tracks
        = vertical ? core_.getInitTracksX()[i] : core_.getInitTracksY()[i];
    int num_tracks
        = vertical ? core_.getNumTracksX()[i] : core_.getNumTracksY()[i];

    float thickness_multiplier
        = vertical ? parms_->getVerticalThicknessMultiplier()
                   : parms_->getHorizontalThicknessMultiplier();

    int half_width = vertical ? int(ceil(core_.getMinWidthX()[i] / 2.0))
                              : int(ceil(core_.getMinWidthY()[i] / 2.0));

    half_width *= thickness_multiplier;

    int num_tracks_offset = std::ceil(
        offset
        / (std::max(min_dst_ver,
                    min_dst_hor)));

    start_idx
        = std::max(0.0, ceil((min + half_width - init_tracks) / min_dst_pins))
          + num_tracks_offset;
    end_idx
        = std::min((num_tracks - 1),
                   static_cast<int>(floor((max - half_width - init_tracks) / min_dst_pins)))
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
      bool blocked
          = vertical ? checkBlocked(edge, curr_x) : checkBlocked(edge, curr_y);
      slots_.push_back({blocked, false, Point(curr_x, curr_y), layer, edge});
    }
    i++;
  }
}

void IOPlacer::defineSlots()
{
  Point lb = core_.getBoundary().ll();
  Point ub = core_.getBoundary().ur();

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

void IOPlacer::findSections(int begin, int end, Edge edge, std::vector<Section>& sections)
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

std::vector<Section> IOPlacer::createSectionsPerConstraint(const Constraint &constraint)
{
  const Interval &interv = constraint.interval;
  const Edge &edge = interv.edge;

  std::vector<Section> sections;
  if (edge != Edge::invalid) {
    const std::set<int>& layers =
      (edge == Edge::left || edge == Edge::right) ?
      hor_layers_ : ver_layers_;

    for (int layer : layers) {
      std::vector<Slot>::iterator it =
        std::find_if(slots_.begin(), slots_.end(),
         [&](const Slot& s) {
          int slot_xy =
            (edge == Edge::left || edge == Edge::right) ?
            s.pos.y() : s.pos.x();
          if (edge == Edge::bottom || edge == Edge::right)
            return (s.edge == edge && s.layer == layer &&
                    slot_xy >= interv.begin);
          return (s.edge == edge && s.layer == layer &&
                    slot_xy <= interv.end);
         });
      int constraint_begin = it - slots_.begin();

      it =
        std::find_if(slots_.begin() + constraint_begin, slots_.end(),
         [&](const Slot& s) {
          int slot_xy =
            (edge == Edge::left || edge == Edge::right) ?
            s.pos.y() : s.pos.x();
          if (edge == Edge::bottom || edge == Edge::right)
            return (slot_xy >= interv.end || s.edge != edge || s.layer != layer);
          return (slot_xy <= interv.begin || s.edge != edge || s.layer != layer);
         });
      int constraint_end = it - slots_.begin()-1;

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
    std::vector<Slot>::iterator it = std::find_if(slots_.begin(), slots_.end(),
                                                 [&](Slot s) {
                                                    return s.edge == edge &&
                                                           s.layer == layer;
                                                 });
    int edge_begin = it - slots_.begin();

    it = std::find_if(slots_.begin()+edge_begin, slots_.end(),
                                                 [&](Slot s) {
                                                    return s.edge != edge ||
                                                           s.layer != layer;
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

std::vector<Section> IOPlacer::assignConstrainedPinsToSections(Constraint &constraint)
{
  bool top_layer = constraint.interval.edge == Edge::invalid;
  std::vector<Slot> &slots = top_layer ? top_layer_slots_ : slots_;
  Netlist& netlist = netlist_io_pins_;
  assignConstrainedGroupsToSections(constraint, constraint.sections);

  int total_slots_count = 0;
  for (Section& sec : constraint.sections) {
    int new_slots_count = 0;
    for (int slot_idx = sec.begin_slot; slot_idx <= sec.end_slot; slot_idx++) {
      new_slots_count += (slots[slot_idx].blocked || slots[slot_idx].used) ? 0 : 1;
    }
    sec.num_slots = new_slots_count;
    total_slots_count += new_slots_count;
  }

  std::vector<int> pin_indices = findPinsForConstraint(constraint, netlist);

  if (pin_indices.size() > total_slots_count) {
    logger_->error(PPL, 74, "Number of pins ({}) exceed number of valid positions ({}) for constraint.", pin_indices.size(), total_slots_count);
  }

  for (int idx : pin_indices) {
    IOPin& io_pin = netlist.getIoPin(idx);
    assignPinToSection(io_pin, idx, constraint.sections);
  }

  return constraint.sections;
}

void IOPlacer::assignConstrainedGroupsToSections(Constraint &constraint,
                                                 std::vector<Section> &sections)
{
  int total_pins_assigned = 0;
  Netlist& net = netlist_io_pins_;

  for (std::vector<int>& io_group : net.getIOGroups()) {
    const PinList& pin_list = constraint.pin_list;
    IOPin& io_pin = net.getIoPin(io_group[0]);

    if (std::find(pin_list.begin(), pin_list.end(), io_pin.getBTerm()) != pin_list.end()) {
      total_pins_assigned += assignGroupToSection(io_group, sections);
    }
  }
}

int IOPlacer::assignGroupsToSections()
{
  int total_pins_assigned = 0;
  Netlist& net = netlist_io_pins_;
  std::vector<Section>& sections = sections_;

  int total_groups_assigned = 0;

  for (std::vector<int>& io_group : net.getIOGroups()) {
    total_pins_assigned += assignGroupToSection(io_group, sections);
    total_groups_assigned++;
  }

  return total_pins_assigned;
}

int IOPlacer::assignGroupToSection(const std::vector<int> &io_group,
                                   std::vector<Section> &sections) {
  Netlist& net = netlist_io_pins_;
  int group_size = io_group.size();
  bool group_assigned = false;
  int total_pins_assigned = 0;

  IOPin &io_pin = net.getIoPin(io_group[0]);

  if (!io_pin.isAssignedToSection()) {
    std::vector<int64_t> dst(sections.size(), 0);
    for (int i = 0; i < sections.size(); i++) {
      for (int pin_idx : io_group) {
        int pin_hpwl = net.computeIONetHPWL(
            pin_idx, sections[i].pos);
        if (pin_hpwl == std::numeric_limits<int>::max()) {
          dst[i] = pin_hpwl;
          break;
        } else {
          dst[i] += pin_hpwl;
        }
      }
    }
    for (auto i : sortIndexes(dst)) {
      if (sections[i].used_slots+group_size < sections[i].num_slots) {
        std::vector<int> group;
        for (int pin_idx : io_group) {
          IOPin &io_pin = net.getIoPin(pin_idx);
          sections[i].pin_indices.push_back(pin_idx);
          group.push_back(pin_idx);
          sections[i].used_slots++;
          io_pin.assignToSection();
        }
        total_pins_assigned += group_size;
        sections[i].pin_groups.push_back(group);
        group_assigned = true;
        break;
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
  Netlist& net = netlist_io_pins_;
  std::vector<Section>& sections = sections_;

  createSections();

  int total_pins_assigned = assignGroupsToSections();

  int idx = 0;
  for (IOPin& io_pin : net.getIOPins()) {
    if (assignPinToSection(io_pin, idx, sections)) {
      total_pins_assigned++;
    }
    idx++;
  }

  total_pins_assigned += assigned_pins_count;

  if (total_pins_assigned == net.numIOPins()) {
    logger_->info(PPL, 8, "Successfully assigned pins to sections.");
    return true;
  } else {
    logger_->info(PPL, 9, "Unsuccessfully assigned pins to sections ({} out of {}).", total_pins_assigned, net.numIOPins());
    return false;
  }
}

bool IOPlacer::assignPinToSection(IOPin& io_pin, int idx, std::vector<Section>& sections)
{
  Netlist& net = netlist_io_pins_;
  bool pin_assigned = false;

  if (!io_pin.isInGroup() && !io_pin.isAssignedToSection()) {
    std::vector<int> dst(sections.size());
    for (int i = 0; i < sections.size(); i++) {
      dst[i] = net.computeIONetHPWL(
          idx, sections[i].pos);
    }
    for (auto i : sortIndexes(dst)) {
      if (sections[i].used_slots < sections[i].num_slots) {
        sections[i].pin_indices.push_back(idx);
        sections[i].used_slots++;
        pin_assigned = true;
        io_pin.assignToSection();
        break;
      }
    }
  }

  return pin_assigned;
}

void IOPlacer::printConfig()
{
  logger_->info(PPL, 1, "Number of slots          {}", slots_.size());
  logger_->info(PPL, 2, "Number of I/O            {}", netlist_.numIOPins());
  logger_->info(
      PPL, 3, "Number of I/O w/sink     {}", netlist_io_pins_.numIOPins());
  logger_->info(PPL, 4, "Number of I/O w/o sink   {}", zero_sink_ios_.size());
  logger_->info(PPL, 5, "Slots per section        {}", slots_per_section_);
  logger_->info(PPL, 6, "Slots increase factor    {:.1}", slots_increase_factor_);
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
  int lower_x_bound = core_.getBoundary().ll().x();
  int lower_y_bound = core_.getBoundary().ll().y();
  int upper_x_bound = core_.getBoundary().ur().x();
  int upper_y_bound = core_.getBoundary().ur().y();

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
  const int mfg_grid = tech_->getManufacturingGrid();

  if (mfg_grid == 0) {
    logger_->error(PPL, 20, "Manufacturing grid is not defined.");
  }

  if (pin.getLayer() != top_grid_.layer) {
    int index;

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

    if (pin.getOrientation() == Orientation::north
        || pin.getOrientation() == Orientation::south) {
      float thickness_multiplier = parms_->getVerticalThicknessMultiplier();
      int half_width
          = int(ceil(core_.getMinWidthX()[index] / 2.0)) * thickness_multiplier;
      int height
          = int(std::max(2.0 * half_width,
                         ceil(core_.getMinAreaX()[index] / (2.0 * half_width))));

      int ext = 0;
      if (parms_->getVerticalLength() != -1) {
        height = parms_->getVerticalLength() * core_.getDatabaseUnit();
      }

      if (parms_->getVerticalLengthExtend() != -1) {
        ext = parms_->getVerticalLengthExtend() * core_.getDatabaseUnit();
      }

      if (height % mfg_grid != 0) {
        height = mfg_grid*std::ceil(static_cast<float>(height)/mfg_grid);
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
      int half_width
          = int(ceil(core_.getMinWidthY()[index] / 2.0)) * thickness_multiplier;
      int height
          = int(std::max(2.0 * half_width,
                         ceil(core_.getMinAreaY()[index] / (2.0 * half_width))));

      int ext = 0;
      if (parms_->getHorizontalLengthExtend() != -1) {
        ext = parms_->getHorizontalLengthExtend() * core_.getDatabaseUnit();
      }
      if (parms_->getHorizontalLength() != -1) {
        height = parms_->getHorizontalLength() * core_.getDatabaseUnit();
      }

      if (height % mfg_grid != 0) {
        height = mfg_grid*std::ceil(static_cast<float>(height)/mfg_grid);
      }

      if (pin.getOrientation() == Orientation::east) {
        pin.setLowerBound(pin.getX() - ext, pin.getY() - half_width);
        pin.setUpperBound(pin.getX() + height, pin.getY() + half_width);
      } else {
        pin.setLowerBound(pin.getX() - height, pin.getY() - half_width);
        pin.setUpperBound(pin.getX() + ext, pin.getY() + half_width);
      }
    }
  } else {
    int width = top_grid_.width;
    int height = top_grid_.height;

    if (width % mfg_grid != 0) {
      width = mfg_grid*std::ceil(static_cast<float>(width)/mfg_grid);
    }

    if (height % mfg_grid != 0) {
      height = mfg_grid*std::ceil(static_cast<float>(height)/mfg_grid);
    }

    pin.setLowerBound(pin.getX() - width/2, pin.getY() - height/2);
    pin.setUpperBound(pin.getX() + width/2, pin.getY() + height/2);
  }
}

int IOPlacer::returnIONetsHPWL(Netlist& netlist)
{
  int pin_index = 0;
  int hpwl = 0;
  int idx = 0;
  for (IOPin& io_pin : netlist.getIOPins()) {
    hpwl += netlist.computeIONetHPWL(idx, io_pin.getPosition());
    pin_index++;
    idx++;
  }

  return hpwl;
}

int IOPlacer::returnIONetsHPWL()
{
  return returnIONetsHPWL(netlist_);
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
  constraints_.push_back(Constraint(*pins, Direction::invalid, interval));;
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

void IOPlacer::addTopLayerConstraint(PinList* pins,
                                     int x1, int y1,
                                     int x2, int y2)
{
  odb::Rect box = odb::Rect(x1,y1, x2, y2);
  Constraint constraint(*pins, Direction::invalid, box);
  constraints_.push_back(constraint);
}

void IOPlacer::getPinsFromDirectionConstraint(Constraint &constraint)
{
  Netlist& netlist = netlist_io_pins_;
  if (constraint.direction != Direction::invalid &&
      constraint.pin_list.empty()) {
    for (const IOPin& io_pin : netlist.getIOPins()) {
      if (io_pin.getDirection() == constraint.direction) {
        constraint.pin_list.insert(io_pin.getBTerm());
      }
    }
  }
}

std::vector<int> IOPlacer::findPinsForConstraint(const Constraint &constraint, Netlist& netlist)
{
  std::vector<int> pin_indices;
  const PinList &pin_list = constraint.pin_list;
  for (odb::dbBTerm* bterm : pin_list) {
    if (bterm->getFirstPinPlacementStatus().isFixed()){
      continue;
    }
    int idx = netlist.getIoPinIdx(bterm);
    IOPin& io_pin = netlist.getIoPin(idx);
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

void IOPlacer::initConstraints()
{
  std::reverse(constraints_.begin(), constraints_.end());
  for (Constraint &constraint : constraints_) {
    getPinsFromDirectionConstraint(constraint);
    constraint.sections = createSectionsPerConstraint(constraint);
    int num_slots = 0;
    for (Section sec : constraint.sections) {
      num_slots += sec.num_slots;
    }
    if (num_slots > 0) {
      constraint.pins_per_slots = static_cast<float>(constraint.pin_list.size())/num_slots;
    } else {
      logger_->error(PPL, 76, "Constraint does not have available slots for its pins.");
    }
  }
  sortConstraints();
}

void IOPlacer::sortConstraints() {
  std::stable_sort(constraints_.begin(),
                   constraints_.end(),
                   [&](const Constraint& c1, const Constraint& c2) {
                      // treat every non-overlapping constraint as equal, so stable_sort keeps the user order
                      return (c1.pins_per_slots < c2.pins_per_slots) && overlappingConstraints(c1, c2);
                   });
}

bool IOPlacer::overlappingConstraints(const Constraint& c1, const Constraint& c2) {
  const Interval& interv1 = c1.interval;
  const Interval& interv2 = c2.interval;

  if (interv1.edge == interv2.edge) {
    return std::max(interv1.begin, interv2.begin) <= std::min(interv1.end, interv2.end);
  }

  return false;
}

Edge IOPlacer::getEdge(std::string edge)
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

Direction IOPlacer::getDirection(std::string direction)
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
        HungarianMatching hg(sections[idx], netlist_io_pins_, top_layer_slots_, logger_);
        hg_vec.push_back(hg);
      } else {
        HungarianMatching hg(sections[idx], netlist_io_pins_, slots_, logger_);
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

  for (int idx = 0; idx < hg_vec.size(); idx++) {
    hg_vec[idx].getFinalAssignment(assignment_);
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

  int init_hpwl = 0;
  int total_hpwl = 0;
  int delta_hpwl = 0;

  initIOLists();
  defineSlots();

  initConstraints();

  init_hpwl = returnIONetsHPWL(netlist_);

  if (random_mode) {
    logger_->info(PPL, 7, "Random pin placement.");
    randomPlacement();
  } else {
    int constrained_pins_cnt = 0;
    for (Constraint &constraint : constraints_) {
      std::vector<Section> sections_for_constraint = assignConstrainedPinsToSections(constraint);
      for (Section& sec : sections_for_constraint) {
        constrained_pins_cnt += sec.pin_indices.size();
      }

      findPinAssignment(sections_for_constraint);
      updateSlots();
    }

    setupSections(constrained_pins_cnt);

    findPinAssignment(sections_);
  }

  for (int i = 0; i < assignment_.size(); ++i) {
    updateOrientation(assignment_[i]);
    updatePinArea(assignment_[i]);
  }

  if (assignment_.size() != static_cast<int>(netlist_.numIOPins())) {
    logger_->error(PPL,
                   39,
                   "Assigned {} pins out of {} IO pins.",
                   assignment_.size(),
                   netlist_.numIOPins());
  }

  if (!random_mode) {
    total_hpwl += returnIONetsHPWL(netlist_io_pins_);
    delta_hpwl = init_hpwl - total_hpwl;
    logger_->info(PPL, 11, "HPWL before pin placement: {}", init_hpwl);
    logger_->info(PPL, 12, "HPWL after  pin placement: {}", total_hpwl);
    logger_->info(PPL, 13, "HPWL delta  pin placement: {}", delta_hpwl);
  }

  commitIOPlacementToDB(assignment_);
  clear();
}

void IOPlacer::placePin(odb::dbBTerm* bterm, int layer, int x, int y, int width, int height)
{
  tech_ = db_->getTech();
  block_ = db_->getChip()->getBlock();
  const int mfg_grid = tech_->getManufacturingGrid();
  if (width % mfg_grid != 0) {
    width = mfg_grid*std::ceil(static_cast<float>(width)/mfg_grid);
  }

  if (height % mfg_grid != 0) {
    height = mfg_grid*std::ceil(static_cast<float>(height)/mfg_grid);
  }

  odb::Point pos = odb::Point(x, y);
  odb::Point ll = odb::Point(pos.x() - width/2, pos.y() - height/2);
  odb::Point ur = odb::Point(pos.x() + width/2, pos.y() + height/2);

  IOPin io_pin = IOPin(bterm, pos, Direction::invalid, ll, ur, odb::dbPlacementStatus::FIRM);
  io_pin.setLayer(layer);

  commitIOPinToDB(io_pin);

  logger_->info(PPL,
                70,
                "Pin {} placed at ({}um, {}um).",
                bterm->getName(),
                x/tech_->getLefUnits(),
                y/tech_->getLefUnits());
}

// db functions
void IOPlacer::populateIOPlacer(std::set<int> hor_layer_idx,
                                std::set<int> ver_layer_idx)
{
  if (tech_ == nullptr) {
    tech_ = db_->getTech();
  }

  if (block_ == nullptr) {
    block_ = db_->getChip()->getBlock();
  }

  initCore(hor_layer_idx, ver_layer_idx);
  initNetlist();
}

void IOPlacer::initCore(std::set<int> hor_layer_idxs,
                        std::set<int> ver_layer_idxs)
{
  int database_unit = tech_->getLefUnits();

  Rect boundary;
  block_->getDieArea(boundary);

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

    odb::dbTechLayer* hor_layer = tech_->findRoutingLayer(hor_layer_idx);
    odb::dbTrackGrid* hor_track_grid = block_->findTrackGrid(hor_layer);
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

    odb::dbTechLayer* ver_layer = tech_->findRoutingLayer(ver_layer_idx);
    odb::dbTrackGrid* ver_track_grid = block_->findTrackGrid(ver_layer);
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

  core_ = Core(boundary,
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

void IOPlacer::addTopLayerPinPattern(int layer, int x_step, int y_step,
                                     int llx, int lly, int urx, int ury,
                                     int width, int height, int keepout)
{
  top_grid_ = TopLayerGrid(layer, x_step, y_step, llx, lly, urx, ury, width, height, keepout);
}

void IOPlacer::findSlotsForTopLayer()
{
  if (top_layer_slots_.empty() && top_grid_.width > 0) {
    for (int x = top_grid_.llx; x < top_grid_.urx; x += top_grid_.x_step) {
      for (int y = top_grid_.lly; y < top_grid_.ury; y += top_grid_.y_step) {
        top_layer_slots_.push_back({false, false, Point(x, y), top_grid_.layer, Edge::invalid});
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
  for (odb::dbObstruction* obstruction : block_->getObstructions()) {
    odb::dbBox* box = obstruction->getBBox();
    if (box->getTechLayer()->getRoutingLevel() == top_grid_.layer) {
      odb::Rect obstruction_rect;
      box->getBox(obstruction_rect);
      obstructions.push_back(obstruction_rect);
    }
  }

  // Get already routed special nets
  for (odb::dbNet* net : block_->getNets()) {
    if (net->isSpecial()) {
      for (odb::dbSWire* swire : net->getSWires()) {
        for (odb::dbSBox* wire : swire->getWires()) {
          if (!wire->isVia()) {
            if (wire->getTechLayer()->getRoutingLevel() == top_grid_.layer) {
              odb::Rect obstruction_rect;
              wire->getBox(obstruction_rect);
              obstructions.push_back(obstruction_rect);
            }
          }
        }
      }
    }
  }

  // Get already placed pins
  for (odb::dbBTerm* term : block_->getBTerms()) {
    for (odb::dbBPin* pin : term->getBPins()) {
      if (pin->getPlacementStatus().isFixed()) {
        for (odb::dbBox* box : pin->getBoxes()) {
          if (box->getTechLayer()->getRoutingLevel() == top_grid_.layer) {
            odb::Rect obstruction_rect;
            box->getBox(obstruction_rect);
            obstructions.push_back(obstruction_rect);
          }
        }
      }
    }
  }

  // check for slots that go beyond the die boundary
  odb::Rect die_area;
  block_->getDieArea(die_area);
  for (auto& slot : top_layer_slots_) {
    odb::Point& point = slot.pos;
    if (point.x() - top_grid_.width/2 < die_area.xMin()
        || point.y() - top_grid_.height/2 < die_area.yMin()
        || point.x() + top_grid_.width/2 > die_area.xMax()
        || point.y() + top_grid_.height/2 > die_area.yMax()) {
      // mark slot as blocked since it extends beyond the die area
      slot.blocked = true;
    }
  }

  // check for slots that overlap with obstructions
  for (odb::Rect& rect : obstructions) {
    for (auto& slot : top_layer_slots_) {
      odb::Point& point = slot.pos;
      // mock slot with keepout
      odb::Rect pin_rect(point.x() - top_grid_.width/2  - top_grid_.keepout,
                         point.y() - top_grid_.height/2 - top_grid_.keepout,
                         point.x() + top_grid_.width/2  + top_grid_.keepout,
                         point.y() + top_grid_.height/2 + top_grid_.keepout);
      if (rect.intersects(pin_rect)) { // mark slot as blocked
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
  for (int x = top_grid_.llx; x < top_grid_.urx; x += top_grid_.x_step) {
    std::vector<Slot>& slots = top_layer_slots_;
    std::vector<Slot>::iterator it = std::find_if(slots.begin(), slots.end(),
                                                   [&](Slot s) {
                                                      return (s.pos.x() >= x &&
                                                              s.pos.x() >= lb_x &&
                                                              s.pos.y() >= lb_y);
                                                   });
    int edge_begin = it - slots.begin();
    int edge_x = slots[edge_begin].pos.x();

    it = std::find_if(slots.begin()+edge_begin, slots.end(),
                                                 [&](Slot s) {
                                                    return s.pos.x() != edge_x ||
                                                           s.pos.x() >= ub_x ||
                                                           s.pos.y() >= ub_y;
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
  const Rect& coreBoundary = core_.getBoundary();
  int x_center = (coreBoundary.xMin() + coreBoundary.xMax()) / 2;
  int y_center = (coreBoundary.yMin() + coreBoundary.yMax()) / 2;

  odb::dbSet<odb::dbBTerm> bterms = block_->getBTerms();

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

      if (cur_i_term->getInst()->getPlacementStatus() == odb::dbPlacementStatus::NONE ||
          cur_i_term->getInst()->getPlacementStatus() == odb::dbPlacementStatus::UNPLACED) {
        x = x_center;
        y = y_center;
      } else {
        cur_i_term->getAvgXY(&x, &y);
      }

      inst_pins.push_back(
          InstancePin(cur_i_term->getInst()->getConstName(), Point(x, y)));
    }

    netlist_.addIONet(io_pin, inst_pins);
  }

  int group_idx = 0;
  for (PinGroup pin_group : pin_groups_) {
    int group_created = netlist_.createIOGroup(pin_group);
    if(group_created == pin_group.size()) {
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
  odb::dbTechLayer* layer = tech_->findRoutingLayer(pin.getLayer());

  odb::dbBTerm* bterm = block_->findBTerm(pin.getName().c_str());
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
