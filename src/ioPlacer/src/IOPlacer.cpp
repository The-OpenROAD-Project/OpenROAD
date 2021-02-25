/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "ioplacer/IOPlacer.h"

#include <algorithm>
#include <random>
#include <sstream>

#include "opendb/db.h"
#include "openroad/OpenRoad.hh"
#include "utility/Logger.h"

namespace ppl {

using utl::PPL;

void IOPlacer::init(odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  parms_ = std::make_unique<Parameters>();
}

void IOPlacer::clear()
{
  hor_layers_.clear();
  ver_layers_.clear();
  zero_sink_ios_.clear();
  sections_.clear();
  slots_.clear();
  assignment_.clear();
  netlist_io_pins_.clear();
  excluded_intervals_.clear();
  constraints_.clear();
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
  report_hpwl_ = false;
  slots_per_section_ = 200;
  slots_increase_factor_ = 0.01f;
  usage_per_section_ = 1;
  usage_increase_factor_ = 0.01f;
  force_pin_spread_ = true;
  netlist_ = Netlist();
  netlist_io_pins_ = Netlist();

  if (parms_->getReportHPWL()) {
    report_hpwl_ = true;
  }
  if (parms_->getForceSpread()) {
    force_pin_spread_ = true;
  } else {
    force_pin_spread_ = false;
  }
  if (parms_->getNumSlots() > -1) {
    slots_per_section_ = parms_->getNumSlots();
  }

  if (parms_->getSlotsFactor() > -1) {
    slots_increase_factor_ = parms_->getSlotsFactor();
  }
  if (parms_->getUsage() > -1) {
    usage_per_section_ = parms_->getUsage();
  }
  if (parms_->getUsageFactor() > -1) {
    usage_increase_factor_ = parms_->getUsageFactor();
  }
}

void IOPlacer::randomPlacement(const RandomMode mode)
{
  const double seed = parms_->getRandSeed();

  SlotVector valid_slots;
  for (Slot& slot : slots_) {
    if (!slot.blocked) {
      valid_slots.push_back(slot);
    }
  }

  int num_i_os = netlist_.numIOPins();
  int num_slots = valid_slots.size();
  double shift = num_slots / double(num_i_os);
  int mid1 = num_slots * 1 / 8 - num_i_os / 8;
  int mid2 = num_slots * 3 / 8 - num_i_os / 8;
  int mid3 = num_slots * 5 / 8 - num_i_os / 8;
  int mid4 = num_slots * 7 / 8 - num_i_os / 8;
  int idx = 0;
  int slots_per_edge = num_i_os / 4;
  int last_slots = (num_i_os - slots_per_edge * 3);
  std::vector<int> vSlots(num_slots);
  std::vector<int> vIOs(num_i_os);

  std::vector<InstancePin> instPins;
  netlist_.getSinksOfIO(idx, instPins);
  if (sections_.size() < 1) {
    Section s = {Point(0, 0)};
    sections_.push_back(s);
  }

  std::mt19937 g(seed);
  switch (mode) {
    case RandomMode::full:
      logger_->report("RandomMode Full");

      for (size_t i = 0; i < vSlots.size(); ++i) {
        vSlots[i] = i;
      }

      std::shuffle(vSlots.begin(), vSlots.end(), g);

      for (IOPin& io_pin : netlist_.getIOPins()) {
        int b = vSlots[0];
        io_pin.setPos(valid_slots.at(b).pos);
        io_pin.setLayer(valid_slots.at(b).layer);
        assignment_.push_back(io_pin);
        sections_[0].net.addIONet(io_pin, instPins);
        vSlots.erase(vSlots.begin());
      }
      break;
    case RandomMode::even:
      logger_->report("RandomMode Even");

      for (size_t i = 0; i < vIOs.size(); ++i) {
        vIOs[i] = i;
      }

      std::shuffle(vIOs.begin(), vIOs.end(), g);

      for (IOPin& io_pin : netlist_.getIOPins()) {
        int b = vIOs[0];
        io_pin.setPos(valid_slots.at(floor(b * shift)).pos);
        io_pin.setLayer(valid_slots.at(floor(b * shift)).layer);
        assignment_.push_back(io_pin);
        sections_[0].net.addIONet(io_pin, instPins);
        vIOs.erase(vIOs.begin());
      }
      break;
    case RandomMode::group:
      logger_->report("RandomMode Group");
      for (size_t i = mid1; i < mid1 + slots_per_edge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid2; i < mid2 + slots_per_edge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid3; i < mid3 + slots_per_edge; i++) {
        vIOs[idx++] = i;
      }
      for (size_t i = mid4; i < mid4 + last_slots; i++) {
        vIOs[idx++] = i;
      }

      std::shuffle(vIOs.begin(), vIOs.end(), g);

      for (IOPin& io_pin : netlist_.getIOPins()) {
        int b = vIOs[0];
        io_pin.setPos(valid_slots.at(b).pos);
        io_pin.setLayer(valid_slots.at(b).layer);
        assignment_.push_back(io_pin);
        sections_[0].net.addIONet(io_pin, instPins);
        vIOs.erase(vIOs.begin());
      }
      break;
    default:
      logger_->error(PPL, 39, "Random mode not found");
      break;
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

void IOPlacer::findSlots(const std::set<int>& layers, Edge edge)
{
  Point lb = core_.getBoundary().ll();
  Point ub = core_.getBoundary().ur();

  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  bool vertical = (edge == Edge::top || edge == Edge::bottom);
  int min = vertical ? lb_x : lb_y;
  int max = vertical ? ub_x : ub_y;

  int offset = parms_->getBoundariesOffset() * core_.getDatabaseUnit();

  int i = 0;
  for (int layer : layers) {
    int curr_x, curr_y, start_idx, end_idx;

    int min_dst_pins
        = vertical ? core_.getMinDstPinsX()[i] * parms_->getMinDistance()
                   : core_.getMinDstPinsY()[i] * parms_->getMinDistance();
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
        / (std::max(core_.getMinDstPinsX()[i] * parms_->getMinDistance(),
                    core_.getMinDstPinsY()[i] * parms_->getMinDistance())));

    start_idx
        = std::max(0.0, ceil((min + half_width - init_tracks) / min_dst_pins))
          + num_tracks_offset;
    end_idx
        = std::min((num_tracks - 1),
                   (int) floor((max - half_width - init_tracks) / min_dst_pins))
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
  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

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
}

void IOPlacer::createSectionsPerEdge(Edge edge)
{
  int edge_begin;
  for (int i = 0; i < slots_.size(); i++) {
    if (slots_[i].edge == edge) {
      edge_begin = i;
      break;
    }
  }

  int edge_end = -1;
  for (int i = edge_begin; i < slots_.size(); i++) {
    if (slots_[i].edge != edge) {
      edge_end = i-1;
      break;
    }
    edge_end = i;
  }

  int begin_slot = 0;
  int end_slot = 0;
  while (end_slot < edge_end) {
    int blocked_slots = 0;
    end_slot = edge_begin + slots_per_section_ - 1;
    if (end_slot > edge_end) {
      end_slot = edge_end;
    }
    for (int i = edge_begin; i < end_slot; ++i) {
      if (slots_[i].blocked) {
        blocked_slots++;
      }
    }
    int mid_point = (end_slot - edge_begin) / 2;
    Section n_sec = {slots_.at(edge_begin + mid_point).pos};
    if (usage_per_section_ > 1.f) {
      logger_->warn(PPL, 35, "section usage exeeded max");
      usage_per_section_ = 1.;
      logger_->report("Forcing slots per section to increase");
      if (slots_increase_factor_ != 0.0f) {
        slots_per_section_ *= (1 + slots_increase_factor_);
      } else if (usage_increase_factor_ != 0.0f) {
        slots_per_section_ *= (1 + usage_increase_factor_);
      } else {
        slots_per_section_ *= 1.1;
      }
    }
    n_sec.num_slots = end_slot - edge_begin - blocked_slots + 1;
    if (n_sec.num_slots < 0) {
      logger_->error(PPL, 40, "Negative number of slots");
    }
    n_sec.begin_slot = edge_begin;
    n_sec.end_slot = end_slot;
    n_sec.max_slots = n_sec.num_slots * usage_per_section_;
    n_sec.cur_slots = 0;
    n_sec.edge = edge;

    sections_.push_back(n_sec);
    edge_begin = ++end_slot;
  }
}

void IOPlacer::createSections()
{
  Point lb = core_.getBoundary().ll();
  Point ub = core_.getBoundary().ur();

  SlotVector& slots = slots_;
  sections_.clear();
  int num_slots = slots.size();
  int begin_slot = 0;
  int end_slot = 0;

  createSectionsPerEdge(Edge::bottom);
  createSectionsPerEdge(Edge::right);
  createSectionsPerEdge(Edge::top);
  createSectionsPerEdge(Edge::left);
}

int IOPlacer::assignGroupsToSections()
{
  int total_pins_assigned = 0;
  Netlist& net = netlist_io_pins_;
  SectionVector& sections = sections_;

  int total_groups_assigned = 0;

  for (std::vector<int>& io_group : net.getIOGroups()) {
    int group_size = io_group.size();
    bool group_assigned = false;
    std::vector<int64_t> dst(sections.size(), 0);
    for (int i = 0; i < sections.size(); i++) {
      for (int pin_idx : io_group) {
        int pin_hpwl = net.computeIONetHPWL(
            pin_idx, sections[i].pos, sections[i].edge, constraints_);
        if (pin_hpwl == std::numeric_limits<int>::max()) {
          dst[i] = pin_hpwl;
          break;
        } else {
          dst[i] += pin_hpwl;
        }
      }
    }

    for (auto i : sortIndexes(dst)) {
      if (sections[i].cur_slots+group_size < sections[i].max_slots) {
        std::vector<int> group;
        for (int pin_idx : io_group) {
          IOPin io_pin = net.getIoPin(pin_idx);

          std::vector<InstancePin> inst_pins_vector;
          net.getSinksOfIO(pin_idx, inst_pins_vector);

          sections[i].net.addIONet(io_pin, inst_pins_vector);
          group.push_back(sections[i].net.numIOPins()-1);
          sections[i].cur_slots++;
        }
        total_pins_assigned += group_size;
        sections[i].net.addIOGroup(group);
        group_assigned = true;
        break;
      }
      // Try to add pin just to first
      if (!force_pin_spread_)
        break;
    }
    if (!group_assigned) {
      break;
    } else {
      total_groups_assigned++;
    }
  }

  if (total_groups_assigned != net.numIOGroups()) {
    logger_->error(PPL, 42, "Unsuccessfully assigned I/O groups");
  }

  return total_pins_assigned;
}

bool IOPlacer::assignPinsSections()
{
  Netlist& net = netlist_io_pins_;
  SectionVector& sections = sections_;
  createSections();
  int total_pins_assigned = assignGroupsToSections();
  int idx = 0;
  for (IOPin& io_pin : net.getIOPins()) {
    if (!io_pin.isInGroup()) {
      bool pin_assigned = false;
      std::vector<int> dst(sections.size());
      std::vector<InstancePin> inst_pins_vector;
      for (int i = 0; i < sections.size(); i++) {
        Point& section_begin = slots_[sections[i].begin_slot].pos;
        Point& section_end = slots_[sections[i].end_slot].pos;
        dst[i] = net.computeIONetHPWL(
            idx, section_begin, section_end, sections[i].pos, sections[i].edge, constraints_);
      }
      net.getSinksOfIO(idx, inst_pins_vector);
      for (auto i : sortIndexes(dst)) {
        if (sections[i].cur_slots < sections[i].max_slots) {
          sections[i].net.addIONet(io_pin, inst_pins_vector);
          sections[i].cur_slots++;
          pin_assigned = true;
          total_pins_assigned++;
          break;
        }
        // Try to add pin just to first
        if (!force_pin_spread_)
          break;
      }
      if (!pin_assigned) {
        break;
      }
    }
    idx++;
  }
  
  if (total_pins_assigned == net.numIOPins()) {
    logger_->report("Successfully assigned I/O pins");
    return true;
  } else {
    logger_->report("Unsuccessfully assigned I/O pins");
    return false;
  }
}

void IOPlacer::printConfig()
{
  logger_->info(PPL, 1, " * Num of slots          {}", slots_.size());
  logger_->info(PPL, 2, " * Num of I/O            {}", netlist_.numIOPins());
  logger_->info(
      PPL, 3, " * Num of I/O w/sink     {}", netlist_io_pins_.numIOPins());
  logger_->info(PPL, 4, " * Num of I/O w/o sink   {}", zero_sink_ios_.size());
  logger_->info(PPL, 5, " * Slots Per Section     {}", slots_per_section_);
  logger_->info(PPL, 6, " * Slots Increase Factor {}", slots_increase_factor_);
  logger_->info(PPL, 7, " * Usage Per Section     {}", usage_per_section_);
  logger_->info(PPL, 8, " * Usage Increase Factor {}", usage_increase_factor_);
  logger_->info(PPL, 9, " * Force Pin Spread      {}", force_pin_spread_);
}

void IOPlacer::setupSections()
{
  bool all_assigned;
  int i = 0;

  do {
    logger_->info(PPL, 10, "Tentative {} to setup sections", i++);
    printConfig();

    all_assigned = assignPinsSections();

    usage_per_section_ *= (1 + usage_increase_factor_);
    slots_per_section_ *= (1 + slots_increase_factor_);
    if (sections_.size() > MAX_SECTIONS_RECOMMENDED) {
      logger_->warn(PPL,
                    36,
                    "Number of sections is {}"
                    " while the maximum recommended value is {}"
                    " this may negatively affect performance",
                    sections_.size(),
                    MAX_SECTIONS_RECOMMENDED);
    }
    if (slots_per_section_ > MAX_SLOTS_RECOMMENDED) {
      logger_->warn(PPL,
                    37,
                    "Number of slots per sections is {}"
                    " while the maximum recommended value is {}"
                    " this may negatively affect performance",
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
  const int x = pin.getX();
  const int y = pin.getY();
  const int l = pin.getLayer();
  int lower_x_bound = core_.getBoundary().ll().x();
  int lower_y_bound = core_.getBoundary().ll().y();
  int upper_x_bound = core_.getBoundary().ur().x();
  int upper_y_bound = core_.getBoundary().ur().y();

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

    if (pin.getOrientation() == Orientation::east) {
      pin.setLowerBound(pin.getX() - ext, pin.getY() - half_width);
      pin.setUpperBound(pin.getX() + height, pin.getY() + half_width);
    } else {
      pin.setLowerBound(pin.getX() - height, pin.getY() - half_width);
      pin.setUpperBound(pin.getX() + ext, pin.getY() + half_width);
    }
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

void IOPlacer::addDirectionConstraint(Direction direction,
                                      Edge edge,
                                      int begin,
                                      int end)
{
  Interval interval(edge, begin, end);
  Constraint constraint("INVALID", direction, interval);
  constraints_.push_back(constraint);
}

void IOPlacer::addNameConstraint(std::string name,
                                 Edge edge,
                                 int begin,
                                 int end)
{
  Interval interval(edge, begin, end);
  Constraint constraint(name, Direction::invalid, interval);
  constraints_.push_back(constraint);
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

PinGroup* IOPlacer::createPinGroup()
{
  pin_groups_.push_back(PinGroup());
  PinGroup* group = &pin_groups_.back();
  return group;
}

void IOPlacer::addPinToGroup(odb::dbBTerm* pin, PinGroup* pin_group)
{
  pin_group->push_back(pin);
}

void IOPlacer::run(bool random_mode)
{
  initParms();

  initNetlistAndCore(hor_layers_, ver_layers_);

  std::vector<HungarianMatching> hg_vec;
  int init_hpwl = 0;
  int total_hpwl = 0;
  int delta_hpwl = 0;

  initIOLists();
  defineSlots();

  if (report_hpwl_) {
    init_hpwl = returnIONetsHPWL(netlist_);
  }

  if (random_mode) {
    logger_->report("Random pin placement");
    randomPlacement(RandomMode::even);
  } else {
    setupSections();

    for (int idx = 0; idx < sections_.size(); idx++) {
      if (sections_[idx].net.numIOPins() > 0) {
        HungarianMatching hg(sections_[idx], slots_, logger_);
        hg_vec.push_back(hg);
      }
    }

    for (int idx = 0; idx < hg_vec.size(); idx++) {
      hg_vec[idx].findAssignmentForGroups(constraints_);
    }

    for (int idx = 0; idx < hg_vec.size(); idx++) {
      hg_vec[idx].getAssignmentForGroups(assignment_);
    }

    for (int idx = 0; idx < hg_vec.size(); idx++) {
      hg_vec[idx].findAssignment(constraints_);
    }

    for (int idx = 0; idx < hg_vec.size(); idx++) {
      hg_vec[idx].getFinalAssignment(assignment_);
    }
  }

  for (int i = 0; i < assignment_.size(); ++i) {
    updateOrientation(assignment_[i]);
    updatePinArea(assignment_[i]);
  }

  if (assignment_.size() != (int) netlist_.numIOPins()) {
    logger_->error(PPL,
                   41,
                   "Assigned {} pins out of {} IO pins",
                   assignment_.size(),
                   netlist_.numIOPins());
  }

  if (report_hpwl_) {
    for (int idx = 0; idx < sections_.size(); idx++) {
      total_hpwl += returnIONetsHPWL(sections_[idx].net);
    }
    delta_hpwl = init_hpwl - total_hpwl;
    logger_->info(PPL, 11, "***HPWL before ioPlacer: {}", init_hpwl);
    logger_->info(PPL, 12, "***HPWL after  ioPlacer: {}", total_hpwl);
    logger_->info(PPL, 13, "***HPWL delta  ioPlacer: {}", delta_hpwl);
  }

  commitIOPlacementToDB(assignment_);
  clear();
}

// db functions
void IOPlacer::populateIOPlacer(std::set<int> hor_layer_idx,
                                std::set<int> ver_layer_idx)
{
  tech_ = db_->getTech();
  block_ = db_->getChip()->getBlock();
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

void IOPlacer::initNetlist()
{
  const Rect& coreBoundary = core_.getBoundary();
  int x_center = (coreBoundary.xMin() + coreBoundary.xMax()) / 2;
  int y_center = (coreBoundary.yMin() + coreBoundary.yMax()) / 2;

  odb::dbSet<odb::dbBTerm> bterms = block_->getBTerms();

  for (odb::dbBTerm* b_term : bterms) {
    odb::dbNet* net = b_term->getNet();
    if (!net) {
      logger_->warn(PPL, 38, "Pin {} without net!", b_term->getConstName());
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
                 "FIXED");

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
  for (IOPin& pin : assignment) {
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

    int size = upper_bound.x() - lower_bound.x();

    int x_min = lower_bound.x();
    int y_min = lower_bound.y();
    int x_max = upper_bound.x();
    int y_max = upper_bound.y();

    int origin_x = x_max - int((x_max - x_min) / 2);
    int origin_y = y_max - int((y_max - y_min) / 2);
    odb::dbBox::create(bpin, layer, x_min, y_min, x_max, y_max);
    bpin->setPlacementStatus(odb::dbPlacementStatus::PLACED);
  }
}

}  // namespace ppl
