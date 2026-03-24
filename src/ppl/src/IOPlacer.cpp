// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ppl/IOPlacer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AbstractIOPlacerRenderer.h"
#include "Core.h"
#include "HungarianMatching.h"
#include "Netlist.h"
#include "SimulatedAnnealing.h"
#include "Slots.h"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "ppl/Parameters.h"
#include "utl/Logger.h"
#include "utl/validation.h"

namespace ppl {

using utl::PPL;

IOPlacer::IOPlacer(odb::dbDatabase* db, utl::Logger* logger)
    : logger_(logger), top_grid_(nullptr), ioplacer_renderer_(nullptr), db_(db)
{
  netlist_ = std::make_unique<Netlist>();
  core_ = std::make_unique<Core>();
  parms_ = std::make_unique<Parameters>();
  validator_ = std::make_unique<utl::Validator>(logger, PPL);
}

IOPlacer::~IOPlacer() = default;

odb::dbBlock* IOPlacer::getBlock() const
{
  auto chip = db_->getChip();
  if (!chip) {
    return nullptr;
  }
  return chip->getBlock();
}

odb::dbTech* IOPlacer::getTech() const
{
  return db_->getTech();
}

void IOPlacer::clear()
{
  hor_layers_.clear();
  ver_layers_.clear();
  zero_sink_ios_.clear();
  sections_.clear();
  slots_.clear();
  top_layer_slots_.clear();
  assignment_.clear();
  excluded_intervals_.clear();
  *parms_ = Parameters();
}

void IOPlacer::clearConstraints()
{
  constraints_.clear();
  if (getBlock() != nullptr) {
    for (odb::dbBTerm* bterm : getBlock()->getBTerms()) {
      bterm->resetConstraintRegion();
    }
  }
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

std::string IOPlacer::getDirectionString(Direction direction)
{
  std::string direction_str;
  if (direction == Direction::input) {
    direction_str = "INPUT";
  } else if (direction == Direction::output) {
    direction_str = "OUTPUT";
  } else if (direction == Direction::inout) {
    direction_str = "INOUT";
  } else if (direction == Direction::feedthru) {
    direction_str = "FEEDTHRU";
  } else if (direction == Direction::invalid) {
    direction_str = "INVALID";
  }

  return direction_str;
}

void IOPlacer::initNetlistAndCore(const std::set<int>& hor_layer_idx,
                                  const std::set<int>& ver_layer_idx)
{
  initCore(hor_layer_idx, ver_layer_idx);
  initNetlist();
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

std::vector<int> IOPlacer::findValidSlots(const Constraint& constraint,
                                          const bool top_layer)
{
  std::vector<int> valid_slots;
  if (top_layer) {
    for (const Section& section : constraint.sections) {
      std::vector<int> constraint_slots
          = getValidSlots(section.begin_slot, section.end_slot, top_layer);
      valid_slots.insert(
          valid_slots.end(), constraint_slots.begin(), constraint_slots.end());
    }
  } else {
    int first_slot = constraint.sections.front().begin_slot;
    int last_slot = constraint.sections.back().end_slot;
    valid_slots = getValidSlots(first_slot, last_slot, top_layer);
  }

  return valid_slots;
}

std::string IOPlacer::getSlotsLocation(Edge edge, bool top_layer)
{
  std::string slots_location;
  if (top_layer) {
    slots_location = "top layer grid";
  } else if (edge != Edge::invalid) {
    slots_location = getEdgeString(edge) + " edge";
  } else {
    slots_location = "die boundaries.";
  }

  return slots_location;
}

int IOPlacer::placeFallbackPins()
{
  int placed_pins_cnt = 0;
  // place groups in fallback mode
  for (const auto& group : fallback_pins_.groups) {
    bool constrained_group = false;
    bool have_mirrored = false;
    for (const int pin_idx : group.first) {
      const IOPin& pin = netlist_->getIoPin(pin_idx);
      if (pin.isMirrored()) {
        have_mirrored = true;
        break;
      }
    }

    int pin_idx = group.first[0];
    IOPin& io_pin = netlist_->getIoPin(pin_idx);
    // check if group is constrained
    odb::dbBTerm* bterm = io_pin.getBTerm();
    for (Constraint& constraint : constraints_) {
      if (constraint.pin_list.find(bterm) != constraint.pin_list.end()) {
        constrained_group = true;
        int first_slot = constraint.first_slot;
        int last_slot = constraint.last_slot;
        int available_slots = last_slot - first_slot;
        if (available_slots < group.first.size()) {
          logger_->error(PPL,
                         90,
                         "Group of size {} with pin {} does not fit in "
                         "constrained region. ({} available slots)",
                         group.first.size(),
                         bterm->getName(),
                         available_slots);
        }

        int mid_slot = (last_slot - first_slot) / 2 - group.first.size() / 2
                       + first_slot;

        // try to place fallback group in the middle of the edge
        int place_slot = getFirstSlotToPlaceGroup(
            mid_slot, last_slot, group.first.size(), have_mirrored, io_pin);

        // if the previous fails, try to place the fallback group from the
        // beginning of the edge
        if (place_slot == -1) {
          place_slot = getFirstSlotToPlaceGroup(
              first_slot, last_slot, group.first.size(), have_mirrored, io_pin);
        }

        if (place_slot == -1) {
          Interval& interval = constraint.interval;
          logger_->error(PPL,
                         93,
                         "Pin group of size {} does not fit in the "
                         "constrained region {:.2f}-{:.2f} at {} edge on a "
                         "single metal layer. "
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

void IOPlacer::assignMirroredPins(IOPin& io_pin, std::vector<IOPin>& assignment)
{
  odb::dbBTerm* mirrored_term = io_pin.getBTerm()->getMirroredBTerm();
  int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
  IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);

  odb::Point mirrored_pos = core_->getMirroredPosition(io_pin.getPosition());
  mirrored_pin.setPosition(mirrored_pos);
  mirrored_pin.setLayer(io_pin.getLayer());
  mirrored_pin.setPlaced();
  mirrored_pin.setEdge(getMirroredEdge(io_pin.getEdge()));
  assignment.push_back(mirrored_pin);
  int slot_index
      = getSlotIdxByPosition(mirrored_pos, mirrored_pin.getLayer(), slots_);
  if (slot_index < 0 || slots_[slot_index].used) {
    odb::dbTechLayer* layer
        = db_->getTech()->findRoutingLayer(mirrored_pin.getLayer());
    logger_->error(
        utl::PPL,
        91,
        "Mirrored position ({:.2f}um, {:.2f}um) at layer {} is not a "
        "valid position for pin {} placement.",
        getBlock()->dbuToMicrons(mirrored_pos.getX()),
        getBlock()->dbuToMicrons(mirrored_pos.getY()),
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
    logger_->error(
        utl::PPL,
        101,
        "Slot for position ({:.2f}um, {:.2f}um) in layer {} not found",
        getBlock()->dbuToMicrons(position.getX()),
        getBlock()->dbuToMicrons(position.getY()),
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
    IOPin& io_pin = netlist_->getIoPin(pin_idx);
    Slot& slot = slots_[place_slot];
    io_pin.setPosition(slot.pos);
    io_pin.setLayer(slot.layer);
    io_pin.setEdge(slot.edge);
    // Set line information only for polygon edges
    if (slot.edge == Edge::polygonEdge) {
      io_pin.setLine(slot.containing_line);
    }
    assignment_.push_back(io_pin);
    slot.used = true;
    slot.blocked = true;
    place_slot++;
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
      assignMirroredPins(io_pin, assignment_);
    }
  }

  logger_->info(PPL,
                100,
                "Group of size {} placed during fallback mode.",
                group.first.size());
}

bool IOPlacer::checkBlocked(Edge edge,
                            odb::Line line,
                            const odb::Point& pos,
                            int layer)
{
  for (odb::Rect fixed_pin_shape : layer_fixed_pins_shapes_[layer]) {
    if (fixed_pin_shape.intersects(pos)) {
      return true;
    }
  }
  bool vertical_pin = (edge == Edge::polygonEdge)
                          ? line.pt0().getY() == line.pt1().getY()
                          : (edge == Edge::top || edge == Edge::bottom);
  int coord = vertical_pin ? pos.getX() : pos.getY();
  for (Interval blocked_interval : excluded_intervals_) {
    // check if the blocked interval blocks all layers (== -1) or if it blocks
    // the layer of the position
    if (blocked_interval.getLayer() == -1
        || blocked_interval.getLayer() == layer) {
      if ((blocked_interval.getEdge() == edge || edge == Edge::polygonEdge)
          // Polygons need to be dealt with properly later
          && coord > blocked_interval.getBegin()
          && coord < blocked_interval.getEnd()) {
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

void IOPlacer::writePinPlacement(const char* file_name, const bool placed)
{
  validator_->check_non_null("Block", getBlock(), 113);
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
  if (!netlist_->getIOPins().empty()) {
    for (const Edge& edge : edges_list) {
      out << "#Edge: " << getEdgeString(edge) << "\n";
      for (const IOPin& io_pin : netlist_->getIOPins()) {
        if (io_pin.getEdge() == edge) {
          const int layer = io_pin.getLayer();
          odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
          const odb::Rect pin_rect{io_pin.getLowerBound(),
                                   io_pin.getUpperBound()};
          const odb::Point pos = pin_rect.center();
          out << "place_pin -pin_name " << io_pin.getName() << " -layer "
              << tech_layer->getName() << " -location {"
              << getBlock()->dbuToMicrons(pos.x()) << " "
              << getBlock()->dbuToMicrons(pos.y())
              << "} -force_to_die_boundary\n";
        }
      }
    }
  } else {
    for (odb::dbBTerm* bterm : getBlock()->getBTerms()) {
      int x_pos = 0;
      int y_pos = 0;
      bterm->getFirstPinLocation(x_pos, y_pos);
      odb::dbTechLayer* tech_layer = nullptr;
      for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
        if (!bterm_pin->getPlacementStatus().isPlaced()) {
          logger_->error(
              PPL, 49, "Pin {} is not placed.", bterm->getConstName());
        }
        for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
          tech_layer = bpin_box->getTechLayer();
          break;
        }
      }

      if (tech_layer != nullptr) {
        out << "place_pin -pin_name " << bterm->getName() << " -layer "
            << tech_layer->getName() << " -location {"
            << getBlock()->dbuToMicrons(x_pos) << " "
            << getBlock()->dbuToMicrons(y_pos) << "}";
        if (placed) {
          out << " -placed_status\n";
        } else {
          out << "\n";
        }
      } else {
        logger_->error(PPL,
                       14,
                       "Pin {} does not have layer assignment.",
                       bterm->getName());
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

void IOPlacer::computeRegionIncrease(const Interval& interval,
                                     const int num_pins,
                                     int& new_begin,
                                     int& new_end)
{
  const bool vertical_pin
      = interval.getEdge() == Edge::top || interval.getEdge() == Edge::bottom;

  int interval_length = std::abs(interval.getEnd() - interval.getBegin());
  int interval_begin = std::min(interval.getBegin(), interval.getEnd());
  int interval_end = std::max(interval.getBegin(), interval.getEnd());

  const int die_min = vertical_pin ? core_->getBoundary().xMin()
                                   : core_->getBoundary().yMin();
  const int die_max = vertical_pin ? core_->getBoundary().xMax()
                                   : core_->getBoundary().yMax();

  const int blocked_coord_min = corner_avoidance_ - die_min;
  const int blocked_coord_max = die_max - corner_avoidance_;

  if (interval_begin < blocked_coord_min) {
    interval_length -= blocked_coord_min - interval_begin;
  } else if (interval_end > blocked_coord_max) {
    interval_length -= interval_end - blocked_coord_max;
  }

  int min_dist = getMinDistanceForInterval(interval);

  const int increase = computeIncrease(min_dist, num_pins, interval_length);

  if (interval_end > blocked_coord_max) {
    new_begin -= increase;
  } else {
    new_end += increase;
  }
}

int IOPlacer::getMinDistanceForInterval(const Interval& interval)
{
  const bool vertical_pin
      = interval.getEdge() == Edge::top || interval.getEdge() == Edge::bottom;
  int min_dist = std::numeric_limits<int>::min();

  if (interval.getLayer() != -1) {
    const std::vector<int>& min_distances
        = vertical_pin ? core_->getMinDstPinsX().at(interval.getLayer())
                       : core_->getMinDstPinsY().at(interval.getLayer());
    min_dist = *(std::ranges::max_element(min_distances));
  } else {
    const std::set<int>& layers = vertical_pin ? ver_layers_ : hor_layers_;
    for (int layer_idx : layers) {
      std::vector<int> layer_min_distances
          = vertical_pin ? core_->getMinDstPinsX().at(layer_idx)
                         : core_->getMinDstPinsY().at(layer_idx);
      const int layer_min_dist
          = *(std::ranges::max_element(layer_min_distances));
      min_dist = std::max(layer_min_dist, min_dist);
    }
  }

  return min_dist;
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

void IOPlacer::findSlots(const std::set<int>& layers,
                         Edge edge,
                         odb::Line line,
                         bool is_die_polygon)
{
  for (int layer : layers) {
    odb::Line empty_line;
    std::vector<odb::Point> slots
        = is_die_polygon ? findLayerSlots(layer, Edge::polygonEdge, line, true)
                         : findLayerSlots(layer, edge, empty_line, false);

    // Remove slots that violates the min distance before reversing the vector.
    // This ensures that mirrored positions will exists for every slot.
    int slot_count = 0;
    odb::Point last = slots[0];
    int min_dst_pins = parms_->getMinDistance();
    const bool min_dist_in_tracks = parms_->getMinDistanceInTracks();
    for (auto it = slots.begin(); it != slots.end();) {
      odb::Point pos = *it;
      bool valid_slot;
      if (!min_dist_in_tracks) {
        // If user-defined min distance is not in tracks, use this value to
        // determine if slots are valid between each other.
        valid_slot = pos == last
                     || (std::abs(last.getX() - pos.getX()) >= min_dst_pins
                         || std::abs(last.getY() - pos.getY()) >= min_dst_pins);
      } else {
        valid_slot = pos == last || slot_count % min_dst_pins == 0;
      }
      if (valid_slot) {
        last = pos;
        ++it;
      } else {
        it = slots.erase(it);
      }
      slot_count++;
    }

    if (is_die_polygon) {
      if ((line.pt0().y() == line.pt1().y() && line.pt0().x() > line.pt1().x())
          || (line.pt0().x() == line.pt1().x()
              && line.pt0().y() > line.pt1().y())) {
        std::ranges::reverse(slots);
      }

      for (const odb::Point& pos : slots) {
        bool blocked = checkBlocked(Edge::invalid, line, pos, layer);
        slots_.push_back({blocked, false, pos, layer, Edge::polygonEdge, line});
      }
    }

    else {
      if (edge == Edge::top || edge == Edge::left) {
        std::ranges::reverse(slots);
      }

      for (const odb::Point& pos : slots) {
        // For regular die boundary edges, use default-constructed line
        odb::Line empty_line;  // Default constructor creates empty line
        bool blocked = checkBlocked(edge, empty_line, pos, layer);
        slots_.push_back({blocked, false, pos, layer, edge, empty_line});
      }
    }
  }
}

std::vector<odb::Point> IOPlacer::findLayerSlots(const int layer,
                                                 const Edge edge,
                                                 odb::Line line,
                                                 bool is_die_polygon)
{
  bool vertical_pin;
  int min, max;

  odb::Point lb = core_->getBoundary().ll();
  odb::Point ub = core_->getBoundary().ur();
  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  if (is_die_polygon) {
    const odb::Point& edge_start = line.pt0();
    const odb::Point& edge_end = line.pt1();

    vertical_pin = (edge_start.getY() == edge_end.getY());
    min = vertical_pin ? std::min(edge_start.getX(), edge_end.getX())
                       : std::min(edge_start.getY(), edge_end.getY());
    max = vertical_pin ? std::max(edge_start.getX(), edge_end.getX())
                       : std::max(edge_start.getY(), edge_end.getY());
  } else {
    vertical_pin = (edge == Edge::top || edge == Edge::bottom);
    min = vertical_pin ? lb_x : lb_y;
    max = vertical_pin ? ub_x : ub_y;
  }

  corner_avoidance_ = parms_->getCornerAvoidance();

  const std::vector<int>& layer_min_distances
      = vertical_pin ? core_->getMinDstPinsX().at(layer)
                     : core_->getMinDstPinsY().at(layer);

  const std::vector<int>& layer_init_tracks
      = vertical_pin ? core_->getInitTracksX().at(layer)
                     : core_->getInitTracksY().at(layer);

  const std::vector<int>& layer_num_tracks
      = vertical_pin ? core_->getNumTracksX().at(layer)
                     : core_->getNumTracksY().at(layer);

  std::vector<odb::Point> slots;
  for (int l = 0; l < layer_min_distances.size(); l++) {
    int tech_min_dst = layer_min_distances[l];

    // If Parameters::min_distance_ is zero, use the default min distance of 2
    // tracks. If it is not zero, use the tech min distance to create all
    // possible slots.
    int min_dst_pins = parms_->getMinDistance() == 0
                           ? default_min_dist_ * tech_min_dst
                           : tech_min_dst;

    if (corner_avoidance_ == -1) {
      corner_avoidance_ = num_tracks_offset_ * tech_min_dst;
      // limit default offset to 1um
      corner_avoidance_
          = std::min(corner_avoidance_, getBlock()->micronsToDbu(1.0));
    }

    int init_tracks = layer_init_tracks[l];
    int num_tracks = layer_num_tracks[l];

    float thickness_multiplier
        = vertical_pin ? parms_->getVerticalThicknessMultiplier()
                       : parms_->getHorizontalThicknessMultiplier();

    int half_width = vertical_pin
                         ? int(ceil(core_->getMinWidthX()[layer] / 2.0))
                         : int(ceil(core_->getMinWidthY()[layer] / 2.0));

    half_width *= thickness_multiplier;

    int num_tracks_offset
        = std::ceil(static_cast<double>(corner_avoidance_) / min_dst_pins);

    int start_idx = 0;
    int end_idx = 0;

    start_idx
        = std::max(0.0,
                   ceil(static_cast<double>((min + half_width - init_tracks))
                        / min_dst_pins))
          + num_tracks_offset;
    end_idx = std::min((num_tracks - 1),
                       ((max - half_width - init_tracks) / min_dst_pins))
              - num_tracks_offset;

    if (vertical_pin) {
      int curr_x;
      int curr_y;
      curr_x = init_tracks + start_idx * min_dst_pins;
      if (is_die_polygon) {
        curr_y = line.pt0().getY();
      } else {
        curr_y = (edge == Edge::bottom) ? lb_y : ub_y;
      }

      for (int i = start_idx; i <= end_idx; ++i) {
        odb::Point pos(curr_x, curr_y);
        slots.push_back(pos);
        curr_x += min_dst_pins;
      }
    } else {
      int curr_x;
      int curr_y;
      curr_y = init_tracks + start_idx * min_dst_pins;
      if (is_die_polygon) {
        curr_x = line.pt0().getX();
      } else {
        curr_x = (edge == Edge::left) ? lb_x : ub_x;
      }

      for (int i = start_idx; i <= end_idx; ++i) {
        odb::Point pos(curr_x, curr_y);
        slots.push_back(pos);
        curr_y += min_dst_pins;
      }
    }
  }

  std::ranges::sort(slots, [&](const odb::Point& p1, const odb::Point& p2) {
    if (vertical_pin) {
      return p1.getX() < p2.getX();
    }

    return p1.getY() < p2.getY();
  });

  return slots;
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

  bool isPolygon = getBlock()->getDieAreaPolygon().getPoints().size() > 5;
  if (!isPolygon) {
    odb::Line empty_line;
    findSlots(ver_layers_, Edge::bottom, empty_line, false);

    findSlots(hor_layers_, Edge::right, empty_line, false);

    findSlots(ver_layers_, Edge::top, empty_line, false);

    findSlots(hor_layers_, Edge::left, empty_line, false);
  } else {
    for (auto line : core_->getDieAreaEdges()) {
      bool is_vertical_pin = (line.pt0().getY() == line.pt1().getY());
      if (is_vertical_pin) {
        findSlots(ver_layers_, Edge::polygonEdge, line, true);
      } else {
        findSlots(hor_layers_, Edge::polygonEdge, line, true);
      }
    }
  }

  findSlotsForTopLayer();

  int regular_pin_count
      = static_cast<int>(netlist_->getIOPins().size()) - top_layer_pins_count_;
  int available_slots = 0;
  for (const Slot& slot : slots_) {
    if (slot.isAvailable()) {
      available_slots++;
    }
  }
  if (regular_pin_count > available_slots) {
    int min_dist = std::numeric_limits<int>::min();
    for (int layer_idx : ver_layers_) {
      std::vector<int> layer_min_distances
          = core_->getMinDstPinsX().at(layer_idx);
      const int layer_min_dist
          = *(std::ranges::max_element(layer_min_distances));
      min_dist = std::max(layer_min_dist, min_dist);
    }
    for (int layer_idx : hor_layers_) {
      std::vector<int> layer_min_distances
          = core_->getMinDstPinsY().at(layer_idx);
      const int layer_min_dist
          = *(std::ranges::max_element(layer_min_distances));
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
        available_slots,
        getBlock()->dbuToMicrons(die_margin),
        getBlock()->dbuToMicrons(new_margin));
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
    end_slot = std::min(end_slot, end);
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

    sections.push_back(std::move(n_sec));
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

    bool first_layer = true;
    for (int layer : layers) {
      std::vector<Slot>::iterator it
          = std::ranges::find_if(slots_, [&](const Slot& s) {
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
      if (first_layer) {
        constraint.first_slot = constraint_begin;
      }
      constraint.last_slot = constraint_end;

      findSections(constraint_begin, constraint_end, edge, sections);
      first_layer = false;
    }
  } else {
    sections = findSectionsForTopLayer(constraint.box);
  }

  return sections;
}

void IOPlacer::createSectionsPerEdge(Edge edge, const std::set<int>& layers)
{
  for (int layer : layers) {
    std::vector<Slot>::iterator it = std::ranges::find_if(
        slots_, [&](Slot s) { return s.edge == edge && s.layer == layer; });
    int edge_begin = it - slots_.begin();

    it = std::find_if(slots_.begin() + edge_begin, slots_.end(), [&](Slot s) {
      return s.edge != edge || s.layer != layer;
    });
    int edge_end = it - slots_.begin() - 1;

    findSections(edge_begin, edge_end, edge, sections_);
  }
}

bool IOPlacer::isPointOnLine(const odb::Point& point,
                             const odb::Line& line) const
{
  odb::Point p1 = line.pt0();
  odb::Point p2 = line.pt1();

  // Check if the line is horizontal
  if (p1.getY() == p2.getY()) {
    // odb::Point must have same Y coordinate and X must be within line bounds
    if (point.getY() != p1.getY()) {
      return false;
    }
    int min_x = std::min(p1.getX(), p2.getX());
    int max_x = std::max(p1.getX(), p2.getX());
    return (point.getX() >= min_x && point.getX() <= max_x);
  }

  // Check if the line is vertical
  if (p1.getX() == p2.getX()) {
    // odb::Point must have same X coordinate and Y must be within line bounds
    if (point.getX() != p1.getX()) {
      return false;
    }
    int min_y = std::min(p1.getY(), p2.getY());
    int max_y = std::max(p1.getY(), p2.getY());
    return (point.getY() >= min_y && point.getY() <= max_y);
  }

  // Not axis-aligned (shouldn't happen)
  return false;
}

void IOPlacer::createSectionsPerEdgePolygon(odb::Line poly_edge,
                                            const std::set<int>& layers)
{
  for (int layer : layers) {
    std::vector<Slot>::iterator it = std::ranges::find_if(slots_, [&](Slot s) {
      return s.edge == Edge::polygonEdge && s.layer == layer
             && isPointOnLine(s.pos, poly_edge);
    });
    int edge_begin = it - slots_.begin();

    it = std::find_if(slots_.begin() + edge_begin, slots_.end(), [&](Slot s) {
      return s.edge != Edge::polygonEdge || s.layer != layer
             || !isPointOnLine(s.pos, poly_edge);
    });
    int edge_end = it - slots_.begin() - 1;

    findSections(edge_begin, edge_end, Edge::polygonEdge, sections_);
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

void IOPlacer::createSectionsPolygon()
{
  sections_.clear();

  for (auto line : core_->getDieAreaEdges()) {
    bool is_vertical_pin = (line.pt0().getY() == line.pt1().getY());
    if (is_vertical_pin) {
      createSectionsPerEdgePolygon(line, ver_layers_);
    } else {
      createSectionsPerEdgePolygon(line, hor_layers_);
    }
  }
}

int IOPlacer::updateSection(Section& section, std::vector<Slot>& slots)
{
  int new_slots_count = 0;
  for (int slot_idx = section.begin_slot; slot_idx <= section.end_slot;
       slot_idx++) {
    new_slots_count += (slots[slot_idx].isAvailable()) ? 1 : 0;
  }
  // reset the num_slots and used_slots considering that all the other used
  // slots of the section don't exist, since they can't be used anymore
  section.num_slots = new_slots_count;
  section.used_slots = 0;
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

  std::vector<int> pin_indices
      = findPinsForConstraint(constraint, netlist_.get(), mirrored_only);

  for (int idx : pin_indices) {
    IOPin& io_pin = netlist_->getIoPin(idx);
    if (io_pin.getBTerm()->hasMirroredBTerm()
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
  for (auto& io_group : netlist_->getIOGroups()) {
    const PinSet& pin_list = constraint.pin_list;
    IOPin& io_pin = netlist_->getIoPin(io_group.pin_indices[0]);

    if (std::ranges::find(pin_list, io_pin.getBTerm()) != pin_list.end()) {
      if (mirrored_only && !groupHasMirroredPin(io_group.pin_indices)) {
        continue;
      }
      for (int pin_idx : io_group.pin_indices) {
        IOPin& io_pin = netlist_->getIoPin(pin_idx);
        if (io_pin.getBTerm()->hasMirroredBTerm() && mirrored_only) {
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
    IOPin& io_pin = netlist_->getIoPin(pin_idx);
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
      return true;
    }
  }

  return false;
}

int IOPlacer::assignGroupsToSections(int& mirrored_pins_cnt)
{
  int total_pins_assigned = 0;

  for (auto& io_group : netlist_->getIOGroups()) {
    int before_assignment = total_pins_assigned;
    total_pins_assigned += assignGroupToSection(
        io_group.pin_indices, sections_, io_group.order);

    // check if group was assigned here, and not during constrained groups
    if (total_pins_assigned > before_assignment) {
      for (int pin_idx : io_group.pin_indices) {
        IOPin& io_pin = netlist_->getIoPin(pin_idx);
        if (io_pin.getBTerm()->hasMirroredBTerm()) {
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
  Netlist* net = netlist_.get();
  int group_size = io_group.size();
  bool group_assigned = false;
  int total_pins_assigned = 0;

  IOPin& first_pin = net->getIoPin(io_group[0]);

  if (!first_pin.isAssignedToSection() && !first_pin.inFallback()) {
    std::vector<int64_t> dst(sections.size(), 0);
    std::vector<int64_t> larger_cost(sections.size(), 0);
    std::vector<int64_t> used_slots(sections.size(), 0);
    for (int i = 0; i < sections.size(); i++) {
      for (int pin_idx : io_group) {
        IOPin& pin = net->getIoPin(pin_idx);
        bool has_mirrored_pin = pin.getBTerm()->hasMirroredBTerm();
        int pin_hpwl = net->computeIONetHPWL(pin_idx, sections[i].pos);
        if (pin_hpwl == std::numeric_limits<int>::max()) {
          dst[i] = pin_hpwl;
          break;
        }
        const int mirrored_pin_cost = getMirroredPinCost(pin, sections[i].pos);
        dst[i] += pin_hpwl + mirrored_pin_cost;
        if (has_mirrored_pin) {
          larger_cost[i] += std::max(pin_hpwl, mirrored_pin_cost);
        }
      }
      used_slots[i] = sections[i].used_slots;
    }

    for (auto i : sortIndexes(dst, larger_cost, used_slots)) {
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
          if (io_pin.getBTerm()->hasMirroredBTerm()) {
            assignMirroredPinToSection(io_pin);
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
    IOPin& io_pin = netlist_->getIoPin(pin_idx);
    io_pin.setFallback();
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
      odb::dbBTerm* mirrored_term = io_pin.getBTerm()->getMirroredBTerm();
      int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
      IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);
      mirrored_pin.setFallback();
    }
  }
}

bool IOPlacer::assignPinsToSections(int assigned_pins_count)
{
  bool isPolygon = getBlock()->getDieAreaPolygon().getPoints().size() > 5;
  Netlist* net = netlist_.get();
  std::vector<Section>& sections = sections_;

  isPolygon ? createSectionsPolygon() : createSections();

  int mirrored_pins_cnt = 0;
  int total_pins_assigned = assignGroupsToSections(mirrored_pins_cnt);

  // Mirrored pins first
  int idx = 0;
  for (IOPin& io_pin : net->getIOPins()) {
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
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
  bool has_mirrored_pin = io_pin.getBTerm()->hasMirroredBTerm();

  if (!io_pin.isInGroup() && !io_pin.isAssignedToSection()
      && !io_pin.inFallback()) {
    std::vector<int> dst(sections.size());
    std::vector<int> larger_cost(sections.size());
    std::vector<int> used_slots(sections.size());

    for (int i = 0; i < sections.size(); i++) {
      const int io_net_hpwl = netlist_->computeIONetHPWL(idx, sections[i].pos);
      const int mirrored_pin_cost = getMirroredPinCost(io_pin, sections[i].pos);
      dst[i] = io_net_hpwl + mirrored_pin_cost;

      if (has_mirrored_pin) {
        larger_cost[i] = std::max(io_net_hpwl, mirrored_pin_cost);
        used_slots[i] = sections[i].used_slots;
      }
    }
    for (auto i : sortIndexes(dst, larger_cost, used_slots)) {
      if (sections[i].used_slots < sections[i].num_slots) {
        sections[i].pin_indices.push_back(idx);
        sections[i].used_slots++;
        pin_assigned = true;
        io_pin.assignToSection();

        if (io_pin.getBTerm()->hasMirroredBTerm()) {
          assignMirroredPinToSection(io_pin);
        }
        break;
      }
    }
  }

  return pin_assigned;
}

void IOPlacer::assignMirroredPinToSection(IOPin& io_pin)
{
  odb::dbBTerm* mirrored_term = io_pin.getBTerm()->getMirroredBTerm();
  int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
  IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);
  // Mark mirrored pin as assigned to section to prevent assigning it to
  // another section that is not aligned with its pair
  mirrored_pin.assignToSection();
}

int IOPlacer::getMirroredPinCost(IOPin& io_pin, const odb::Point& position)
{
  if (io_pin.getBTerm()->hasMirroredBTerm()) {
    odb::Point mirrored_pos = core_->getMirroredPosition(position);
    return netlist_->computeIONetHPWL(io_pin.getMirrorPinIdx(), mirrored_pos);
  }
  return 0;
}

void IOPlacer::printConfig(bool annealing)
{
  int available_slots = 0;
  for (const Slot& slot : slots_) {
    if (slot.isAvailable()) {
      available_slots++;
    }
  }
  logger_->info(PPL, 1, "Number of available slots {}", available_slots);
  if (!top_layer_slots_.empty()) {
    logger_->info(
        PPL, 62, "Number of top layer slots {}", top_layer_slots_.size());
  }
  logger_->info(PPL, 2, "Number of I/O             {}", netlist_->numIOPins());
  logger_->metric("floorplan__design__io", netlist_->numIOPins());
  logger_->info(PPL,
                3,
                "Number of I/O w/sink      {}",
                netlist_->numIOPins() - zero_sink_ios_.size());
  logger_->info(PPL, 4, "Number of I/O w/o sink    {}", zero_sink_ios_.size());
  if (!netlist_->getIOGroups().empty()) {
    logger_->info(
        PPL, 6, "Number of I/O Groups    {}", netlist_->getIOGroups().size());
  }
  if (!annealing) {
    logger_->info(PPL, 5, "Slots per section         {}", slots_per_section_);
  }
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

bool IOPlacer::isPointInsidePolygon(odb::Point point,
                                    const odb::Polygon& die_polygon)
{
  /*
   * This function determines if a given point is inside a polygon by casting a
   * horizontal ray from the point to the right (+X direction) and counting the
   * number of times it intersects with the polygon's edges.
   *
   * The logic is as follows:
   * 1. An even number of intersections means the point is outside the polygon.
   * 2. An odd number of intersections means the point is inside the polygon.
   */
  const std::vector<odb::Point>& vertices = die_polygon.getPoints();

  int x = point.getX();
  int y = point.getY();
  bool inside = false;

  size_t j = vertices.size() - 1;  // Start with last vertex

  for (size_t i = 0; i < vertices.size(); i++) {
    int xi = vertices[i].getX();
    int yi = vertices[i].getY();
    int xj = vertices[j].getX();
    int yj = vertices[j].getY();

    // Check if point is on different sides of the edge
    if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
      inside = !inside;
    }
    j = i;  // j follows i
  }

  return inside;
}

void IOPlacer::updateOrientationPolygon(IOPin& pin)
{
  const int x = pin.getX();
  const int y = pin.getY();

  odb::Line pin_line = pin.getLine();

  odb::Polygon die_polygon = getBlock()->getDieAreaPolygon();

  const int offset = 50;

  bool is_vertical = (pin_line.pt0().getX() == pin_line.pt1().getX());

  if (is_vertical) {
    odb::Point delta_right = pin.getPosition();
    odb::Point delta_left = pin.getPosition();
    delta_right.setX(delta_right.getX() + offset);
    delta_left.setX(delta_left.getX() - offset);
    if (isPointInsidePolygon(delta_right, die_polygon)) {
      pin.setOrientation(Orientation::east);
      return;
    }
    if (isPointInsidePolygon(delta_left, die_polygon)) {
      pin.setOrientation(Orientation::west);
      return;
    }
  } else {
    odb::Point delta_top = pin.getPosition();
    odb::Point delta_bottom = pin.getPosition();
    delta_top.setY(delta_top.getY() + offset);
    delta_bottom.setY(delta_bottom.getY() - offset);
    if (isPointInsidePolygon(delta_top, die_polygon)) {
      pin.setOrientation(Orientation::north);
      return;
    }
    if (isPointInsidePolygon(delta_bottom, die_polygon)) {
      pin.setOrientation(Orientation::south);
      return;
    }
  }

  logger_->warn(
      PPL,
      9999,
      "Could not determine orientation for pin {} at ({:.2f}um, {:.2f}um)\n",
      pin.getName(),
      getBlock()->dbuToMicrons(x),
      getBlock()->dbuToMicrons(y));
}

void IOPlacer::updatePinArea(IOPin& pin)
{
  const int mfg_grid = getTech()->getManufacturingGrid();

  if (mfg_grid == 0) {
    logger_->error(PPL, 20, "Manufacturing grid is not defined.");
  }

  if (top_grid_ == nullptr
      || pin.getLayer() != top_grid_->layer->getRoutingLevel()) {
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

  odb::Rect excluded_region;
  findConstraintRegion(excluded_interv, excluded_region, excluded_region);
  getBlock()->addBlockedRegionForPins(excluded_region);
}

void IOPlacer::excludeInterval(Interval interval)
{
  excluded_intervals_.push_back(interval);
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
    for (const IOPin& io_pin : netlist_->getIOPins()) {
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
    if ((mirrored_only && !io_pin.getBTerm()->hasMirroredBTerm())
        || (!mirrored_only && io_pin.getBTerm()->hasMirroredBTerm())) {
      continue;
    }

    if (io_pin.isMirrored() && !io_pin.getBTerm()->hasMirroredBTerm()) {
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
  for (IOPin& io_pin : netlist_->getIOPins()) {
    if (io_pin.getBTerm()->hasMirroredBTerm()) {
      int pin_idx = netlist_->getIoPinIdx(io_pin.getBTerm());
      io_pin.setMirrored();
      odb::dbBTerm* mirrored_term = io_pin.getBTerm()->getMirroredBTerm();
      int mirrored_pin_idx = netlist_->getIoPinIdx(mirrored_term);
      IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);
      mirrored_pin.setMirrored();
      io_pin.setMirrorPinIdx(mirrored_pin_idx);
      mirrored_pin.setMirrorPinIdx(pin_idx);
    }
  }
}

void IOPlacer::initExcludedIntervals()
{
  for (const odb::Rect& excluded_region :
       getBlock()->getBlockedRegionsForPins()) {
    Interval excluded_interv = findIntervalFromRect(excluded_region);
    excluded_intervals_.push_back(excluded_interv);
  }
}

Interval IOPlacer::findIntervalFromRect(const odb::Rect& rect)
{
  int begin;
  int end;
  Edge edge;

  const odb::Rect die_area = getBlock()->getDieArea();
  if (rect.xMin() == rect.xMax()) {
    begin = rect.yMin();
    end = rect.yMax();
    edge = die_area.xMin() == rect.xMin() ? Edge::left : Edge::right;
  } else {
    begin = rect.xMin();
    end = rect.xMax();
    edge = die_area.yMin() == rect.yMin() ? Edge::bottom : Edge::top;
  }

  return Interval(edge, begin, end);
}

void IOPlacer::getConstraintsFromDB()
{
  std::map<Interval, PinSet> pins_per_interval;
  std::map<odb::Rect, PinSet> pins_per_rect;
  for (odb::dbBTerm* bterm : getBlock()->getBTerms()) {
    auto constraint_region = bterm->getConstraintRegion();
    // Constraints derived from mirrored pins are not taken into account here.
    // Only pins with constraints directly assigned by the user should be
    // considered.
    if (constraint_region && !bterm->isMirrored()) {
      odb::Rect region = constraint_region.value();
      if (region.xMin() == region.xMax() || region.yMin() == region.yMax()) {
        Interval interval = findIntervalFromRect(constraint_region.value());
        pins_per_interval[interval].insert(bterm);
      } else {
        // TODO: support rectilinear shapes
        if (top_grid_ == nullptr) {
          logger_->error(utl::PPL, 121, "Top layer grid not found.");
        }
        if (top_grid_->region.isRect()) {
          const odb::Rect& top_grid_region
              = top_grid_->region.getEnclosingRect();
          if (!top_grid_region.contains(region)) {
            logger_->error(utl::PPL,
                           25,
                           "Constraint region ({:.2f}u, {:.2f}u)-({:.2f}u, "
                           "{:.2f}u) at top "
                           "layer is not contained in the top layer grid.",
                           getBlock()->dbuToMicrons(region.xMin()),
                           getBlock()->dbuToMicrons(region.yMin()),
                           getBlock()->dbuToMicrons(region.xMax()),
                           getBlock()->dbuToMicrons(region.yMax()));
          }
          pins_per_rect[region].insert(bterm);
        }
      }
    }
  }

  for (const auto& [interval, pins] : pins_per_interval) {
    std::string pin_names = getPinSetOrListString(pins);
    logger_->info(
        utl::PPL,
        67,
        "Restrict pins [ {} ] to region {:.2f}u-{:.2f}u at the {} edge.",
        pin_names,
        getBlock()->dbuToMicrons(interval.getBegin()),
        getBlock()->dbuToMicrons(interval.getEnd()),
        getEdgeString(interval.getEdge()));
    constraints_.emplace_back(pins, Direction::invalid, interval);
  }

  top_layer_pins_count_ = 0;
  if (top_grid_ != nullptr) {
    for (const auto& [region, pins] : pins_per_rect) {
      std::string pin_names = getPinSetOrListString(pins);
      logger_->info(
          utl::PPL,
          60,
          "Restrict pins [ {} ] to region ({:.2f}u, {:.2f}u)-({:.2f}u, "
          "{:.2f}u) at routing layer {}.",
          pin_names,
          getBlock()->dbuToMicrons(region.xMin()),
          getBlock()->dbuToMicrons(region.yMin()),
          getBlock()->dbuToMicrons(region.xMax()),
          getBlock()->dbuToMicrons(region.yMax()),
          top_grid_->layer->getConstName());
      top_layer_pins_count_ += pins.size();
      constraints_.emplace_back(pins, Direction::invalid, region);
    }
  }
}

void IOPlacer::initConstraints(bool annealing)
{
  constraints_.clear();
  getConstraintsFromDB();
  int constraints_no_slots = 0;
  if (top_layer_pins_count_ > top_layer_slots_.size()) {
    logger_->error(PPL,
                   11,
                   "Number of IO pins assigned to the top layer ({}) exceeds "
                   "maximum number of available "
                   "top layer positions ({}).",
                   top_layer_pins_count_,
                   top_layer_slots_.size());
  }
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
        int new_begin = std::min(region_begin, region_end);
        int new_end = std::max(region_begin, region_end);
        computeRegionIncrease(
            interval, constraint.pin_list.size(), new_begin, new_end);
        logger_->warn(PPL,
                      110,
                      "Constraint has {} pins, but only {} available slots.\n"
                      "Increase the region {:.2f}um-{:.2f}um on the {} edge "
                      "to {:.2f}um-{:.2f}um.",
                      constraint.pin_list.size(),
                      num_slots,
                      getBlock()->dbuToMicrons(region_begin),
                      getBlock()->dbuToMicrons(region_end),
                      region_edge,
                      getBlock()->dbuToMicrons(new_begin),
                      getBlock()->dbuToMicrons(new_end));
        constraints_no_slots++;
      }
    } else {
      logger_->error(PPL,
                     76,
                     "Constraint with {} pins on region {:.2f}um-{:.2f}um on "
                     "the {} edge does not have available slots.",
                     constraint.pin_list.size(),
                     getBlock()->dbuToMicrons(region_begin),
                     getBlock()->dbuToMicrons(region_end),
                     region_edge);
    }
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

  int constraint_idx = 0;
  for (Constraint& constraint : constraints_) {
    for (odb::dbBTerm* term : constraint.pin_list) {
      int pin_idx = netlist_->getIoPinIdx(term);
      IOPin& io_pin = netlist_->getIoPin(pin_idx);
      io_pin.setConstraintIdx(constraint_idx);
      io_pin.setInConstraint();
      constraint.pin_indices.push_back(pin_idx);
      constraint.mirrored_pins_count += io_pin.isMirrored() ? 1 : 0;
      if (io_pin.getGroupIdx() != -1) {
        constraint.pin_groups.insert(io_pin.getGroupIdx());
      }
    }
    constraint_idx++;
  }

  checkPinsInMultipleConstraints();
  checkPinsInMultipleGroups();
}

void IOPlacer::sortConstraints()
{
  std::ranges::stable_sort(constraints_,
                           [&](const Constraint& c1, const Constraint& c2) {
                             // treat every non-overlapping constraint as equal,
                             // so stable_sort keeps the user order
                             return (c1.pins_per_slots < c2.pins_per_slots)
                                    && overlappingConstraints(c1, c2);
                           });
}

void IOPlacer::checkPinsInMultipleConstraints()
{
  std::string pins_in_mult_constraints;
  if (!constraints_.empty()) {
    for (IOPin& io_pin : netlist_->getIOPins()) {
      int constraint_cnt = 0;
      for (Constraint& constraint : constraints_) {
        const PinSet& pin_list = constraint.pin_list;
        if (std::ranges::find(pin_list, io_pin.getBTerm()) != pin_list.end()) {
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
  const auto& bterm_groups = getBlock()->getBTermGroups();
  if (!bterm_groups.empty()) {
    for (IOPin& io_pin : netlist_->getIOPins()) {
      int group_cnt = 0;
      for (const auto& group : bterm_groups) {
        const std::vector<odb::dbBTerm*>& pin_list = group.bterms;
        if (std::ranges::find(pin_list, io_pin.getBTerm()) != pin_list.end()) {
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

/* static */
template <class PinSetOrList>
std::string IOPlacer::getPinSetOrListString(const PinSetOrList& group)
{
  std::string pin_names;
  int pin_cnt = 0;
  for (odb::dbBTerm* pin : group) {
    pin_names += (pin_cnt ? " " : "") + pin->getName();
    pin_cnt++;
    if (pin_cnt >= pins_per_report_
        && !logger_->debugCheck(utl::PPL, "pin_groups", 1)) {
      pin_names += " ...";
      break;
    }
  }
  return pin_names;
}

void IOPlacer::findPinAssignment(std::vector<Section>& sections,
                                 bool mirrored_groups_only)
{
  std::vector<HungarianMatching> hg_vec;
  for (const auto& section : sections) {
    if (!section.pin_indices.empty()) {
      if (section.edge == Edge::invalid) {
        HungarianMatching hg(section,
                             netlist_.get(),
                             core_.get(),
                             top_layer_slots_,
                             logger_,
                             db_);
        hg_vec.push_back(hg);
      } else {
        HungarianMatching hg(
            section, netlist_.get(), core_.get(), slots_, logger_, db_);
        hg_vec.push_back(hg);
      }
    }
  }

  for (auto& match : hg_vec) {
    match.findAssignmentForGroups();
  }

  for (auto& match : hg_vec) {
    match.getAssignmentForGroups(assignment_, mirrored_groups_only);
  }

  for (auto& sec : sections) {
    bool top_layer = sec.edge == Edge::invalid;
    std::vector<Slot>& slots = top_layer ? top_layer_slots_ : slots_;
    updateSection(sec, slots);
  }

  for (auto& match : hg_vec) {
    match.findAssignment();
  }

  for (bool mirrored_pins : {true, false}) {
    for (auto& match : hg_vec) {
      match.getFinalAssignment(assignment_, mirrored_pins);
    }
  }
}

void IOPlacer::updateSlots()
{
  for (Slot& slot : slots_) {
    slot.blocked = slot.blocked || slot.used;
  }
  for (Slot& slot : top_layer_slots_) {
    slot.blocked = slot.blocked || slot.used;
  }
}

void IOPlacer::runHungarianMatching()
{
  bool isPolygon = getBlock()->getDieAreaPolygon().getPoints().size() > 5;

  slots_per_section_ = parms_->getSlotsPerSection();
  initExcludedIntervals();
  initNetlistAndCore(hor_layers_, ver_layers_);
  getBlockedRegionsFromMacros();

  defineSlots();

  initMirroredPins();
  initConstraints();

  int constrained_pins_cnt = 0;
  int mirrored_pins_cnt = 0;
  printConfig();

  // add groups to fallback
  for (const auto& io_group : netlist_->getIOGroups()) {
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
  constrained_pins_cnt += placeFallbackPins();

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
  constrained_pins_cnt += placeFallbackPins();

  assignPinsToSections(constrained_pins_cnt);
  findPinAssignment(sections_, false);

  for (auto& pin : assignment_) {
    if (isPolygon) {
      updateOrientationPolygon(pin);
    } else {
      updateOrientation(pin);
    }
    updatePinArea(pin);
  }

  if (assignment_.size() != netlist_->numIOPins()) {
    logger_->error(PPL,
                   39,
                   "Assigned {} pins out of {} IO pins.",
                   assignment_.size(),
                   netlist_->numIOPins());
  }

  reportHPWL();

  checkPinPlacement();
  commitIOPlacementToDB(assignment_);
  writePinPlacement(parms_->getPinPlacementFile().c_str(), false);
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

void IOPlacer::runAnnealing()
{
  bool isPolygon = getBlock()->getDieAreaPolygon().getPoints().size() > 5;
  slots_per_section_ = parms_->getSlotsPerSection();
  initExcludedIntervals();
  initNetlistAndCore(hor_layers_, ver_layers_);
  getBlockedRegionsFromMacros();

  defineSlots();

  initMirroredPins(true);
  initConstraints(true);

  ppl::SimulatedAnnealing annealing(
      netlist_.get(), core_.get(), slots_, constraints_, logger_, db_);

  if (isAnnealingDebugOn()) {
    annealing.setDebugOn(std::move(ioplacer_renderer_));
  }

  printConfig(true);

  annealing.run(init_temperature_, max_iterations_, perturb_per_iter_, alpha_);
  annealing.getAssignment(assignment_);

  for (auto& pin : assignment_) {
    if (isPolygon) {
      updateOrientationPolygon(pin);
    } else {
      updateOrientation(pin);
    }
    updatePinArea(pin);
  }

  reportHPWL();

  checkPinPlacement();
  commitIOPlacementToDB(assignment_);
  writePinPlacement(parms_->getPinPlacementFile().c_str(), false);
  clear();
}

void IOPlacer::checkPinPlacement()
{
  bool invalid = false;
  std::map<int, std::vector<odb::Point>> layer_positions_map;

  for (const IOPin& pin : netlist_->getIOPins()) {
    int layer = pin.getLayer();

    if (layer_positions_map[layer].empty()) {
      layer_positions_map[layer].push_back(pin.getPosition());
    } else {
      for (odb::Point& pos : layer_positions_map[layer]) {
        if (pos == pin.getPosition()) {
          odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
          logger_->warn(PPL,
                        106,
                        "At least 2 pins in position ({:.2f}um, {:.2f}um), "
                        "layer {}, port {}.",
                        getBlock()->dbuToMicrons(pos.x()),
                        getBlock()->dbuToMicrons(pos.y()),
                        tech_layer->getName(),
                        pin.getName().c_str());
          invalid = true;
        }
      }
      layer_positions_map[layer].push_back(pin.getPosition());
    }
  }

  invalid = invalid || checkPinConstraints() || checkMirroredPins();
  if (invalid) {
    logger_->error(PPL, 107, "Invalid pin placement.");
  }
}

bool IOPlacer::checkPinConstraints()
{
  bool invalid = false;

  for (const IOPin& pin : netlist_->getIOPins()) {
    if (pin.isInConstraint()) {
      const int constraint_idx = pin.getConstraintIdx();
      const Constraint& constraint = constraints_[constraint_idx];
      const Interval& constraint_interval = constraint.interval;
      if (pin.getEdge() != constraint_interval.getEdge()) {
        logger_->warn(PPL,
                      92,
                      "Pin {} is not placed in its constraint edge.",
                      pin.getName());
        invalid = true;
      }

      if (constraint_interval.getEdge() != Edge::invalid) {
        const int constraint_begin = constraint_interval.getBegin();
        const int constraint_end = constraint_interval.getEnd();
        const int pin_coord
            = constraint_interval.getEdge() == Edge::bottom
                      || constraint_interval.getEdge() == Edge::top
                  ? pin.getPosition().getX()
                  : pin.getPosition().getY();
        if (pin_coord < constraint_begin || pin_coord > constraint_end) {
          logger_->warn(PPL,
                        102,
                        "Pin {} is not placed in its constraint interval.",
                        pin.getName());
          invalid = true;
        }
      } else {
        if (!constraint.box.intersects(pin.getPosition())) {
          logger_->warn(
              PPL,
              105,
              "Pin {} is not placed in the upper layer grid region ({:.2f}um, "
              "{:.2f}um) ({:.2f}um, {:.2f}um). ({:.2f}um, {:.2f}um)",
              pin.getName(),
              getBlock()->dbuToMicrons(constraint.box.xMin()),
              getBlock()->dbuToMicrons(constraint.box.yMin()),
              getBlock()->dbuToMicrons(constraint.box.xMax()),
              getBlock()->dbuToMicrons(constraint.box.yMax()),
              getBlock()->dbuToMicrons(pin.getPosition().x()),
              getBlock()->dbuToMicrons(pin.getPosition().y()));
          invalid = true;
        }
      }
    }
  }

  return invalid;
}

bool IOPlacer::checkMirroredPins()
{
  bool invalid = false;

  for (const IOPin& pin : netlist_->getIOPins()) {
    if (pin.isMirrored()) {
      int mirrored_pin_idx = pin.getMirrorPinIdx();
      const IOPin& mirrored_pin = netlist_->getIoPin(mirrored_pin_idx);
      if (pin.getPosition().getX() != mirrored_pin.getPosition().getX()
          && pin.getPosition().getY() != mirrored_pin.getPosition().getY()) {
        logger_->warn(PPL,
                      103,
                      "Pins {} and {} are not mirroring each other.",
                      pin.getName(),
                      mirrored_pin.getName());
        invalid = true;
      }
    }
  }

  return invalid;
}

void IOPlacer::reportHPWL()
{
  int64 total_hpwl = computeIONetsHPWL(netlist_.get());
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
                        bool force_to_die_bound,
                        bool placed_status)
{
  if (width == 0 && height == 0) {
    const int database_unit = getTech()->getDbUnitsPerMicron();
    const double min_area = layer->getArea() * database_unit * database_unit;
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

  odb::Rect die_boundary = getBlock()->getDieArea();
  odb::Point lb = die_boundary.ll();
  odb::Point ub = die_boundary.ur();

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
    // empty line created to comply with definition
    odb::Line empty_line;
    bool placed_at_blocked
        = horizontal
              ? checkBlocked(edge,
                             empty_line,
                             odb::Point(pos.x(), pos.y() - height / 2),
                             layer_level)
                    || checkBlocked(edge,
                                    empty_line,
                                    odb::Point(pos.x(), pos.y() + height / 2),
                                    layer_level)
              : checkBlocked(edge,
                             empty_line,
                             odb::Point(pos.x() - width / 2, pos.y()),
                             layer_level)
                    || checkBlocked(edge,
                                    empty_line,
                                    odb::Point(pos.x() + width / 2, pos.y()),
                                    layer_level);
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
                ? checkBlocked(
                      edge,
                      empty_line,
                      odb::Point(pos.x(), pos.y() - height / 2 + offset),
                      layer_level)
                      || checkBlocked(
                          edge,
                          empty_line,
                          odb::Point(pos.x(), pos.y() + height / 2 + offset),
                          layer_level)
                : checkBlocked(
                      edge,
                      empty_line,
                      odb::Point(pos.x() - width / 2 + offset, pos.y()),
                      layer_level)
                      || checkBlocked(
                          edge,
                          empty_line,
                          odb::Point(pos.x() + width / 2 + offset, pos.y()),
                          layer_level);
    }
    pos.addX(horizontal ? 0 : offset);
    pos.addY(horizontal ? offset : 0);
  }

  odb::Point ll = odb::Point(pos.x() - width / 2, pos.y() - height / 2);
  odb::Point ur = odb::Point(pos.x() + width / 2, pos.y() + height / 2);

  odb::dbPlacementStatus placement_status = placed_status
                                                ? odb::dbPlacementStatus::PLACED
                                                : odb::dbPlacementStatus::FIRM;
  IOPin io_pin
      = IOPin(bterm, pos, Direction::invalid, ll, ur, placement_status);
  io_pin.setLayer(layer_level);

  commitIOPinToDB(io_pin);

  Interval interval = getIntervalFromPin(io_pin, die_boundary);

  excludeInterval(interval);

  if (pos != odb::Point(x, y)) {
    logger_->info(PPL,
                  70,
                  "Pin {} placed at ({:.2f}um, {:.2f}um) instead of ({:.2f}um, "
                  "{:.2f}um). Pin was snapped to a routing track, to the "
                  "manufacturing grid or moved away from blocked region.",
                  bterm->getName(),
                  getBlock()->dbuToMicrons(pos.x()),
                  getBlock()->dbuToMicrons(pos.y()),
                  getBlock()->dbuToMicrons(x),
                  getBlock()->dbuToMicrons(y));
  }
}

void IOPlacer::movePinToTrack(odb::Point& pos,
                              int layer,
                              int width,
                              int height,
                              const odb::Rect& die_boundary)
{
  odb::Point lb = die_boundary.ll();
  odb::Point ub = die_boundary.ur();

  int lb_x = lb.x();
  int lb_y = lb.y();
  int ub_x = ub.x();
  int ub_y = ub.y();

  odb::dbTechLayer* tech_layer = getTech()->findRoutingLayer(layer);
  odb::dbTrackGrid* track_grid = getBlock()->findTrackGrid(tech_layer);
  int min_spacing, init_track, num_track;

  // pin is placed in the die boundaries
  if (top_grid_ == nullptr || layer != top_grid_->layer->getRoutingLevel()) {
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

Interval IOPlacer::getIntervalFromPin(IOPin& io_pin,
                                      const odb::Rect& die_boundary)
{
  Edge edge;
  int begin, end, layer;
  odb::Point lb = die_boundary.ll();
  odb::Point ub = die_boundary.ur();

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

void IOPlacer::initCore(const std::set<int>& hor_layer_idxs,
                        const std::set<int>& ver_layer_idxs)
{
  int database_unit = getTech()->getDbUnitsPerMicron();

  odb::Rect boundary = getBlock()->getDieArea();

  LayerToVector min_spacings_x;
  LayerToVector min_spacings_y;
  LayerToVector init_tracks_x;
  LayerToVector init_tracks_y;
  LayerToVector num_tracks_x;
  LayerToVector num_tracks_y;
  std::map<int, int> min_areas_x;
  std::map<int, int> min_areas_y;
  std::map<int, int> min_widths_x;
  std::map<int, int> min_widths_y;

  for (int hor_layer_idx : hor_layer_idxs) {
    odb::dbTechLayer* hor_layer = getTech()->findRoutingLayer(hor_layer_idx);
    odb::dbTrackGrid* hor_track_grid = getBlock()->findTrackGrid(hor_layer);
    const int track_patterns_count = hor_track_grid->getNumGridPatternsY();

    std::vector<int> init_track_y(track_patterns_count, 0);
    std::vector<int> num_track_y(track_patterns_count, 0);
    std::vector<int> min_spacing_y(track_patterns_count, 0);
    int min_area_y;
    int min_width_y;

    for (int i = 0; i < hor_track_grid->getNumGridPatternsY(); i++) {
      hor_track_grid->getGridPatternY(
          i, init_track_y[i], num_track_y[i], min_spacing_y[i]);
    }

    min_area_y = hor_layer->getArea() * database_unit * database_unit;
    min_width_y = hor_layer->getWidth();

    min_spacings_y[hor_layer_idx] = std::move(min_spacing_y);
    init_tracks_y[hor_layer_idx] = std::move(init_track_y);
    min_areas_y[hor_layer_idx] = min_area_y;
    min_widths_y[hor_layer_idx] = min_width_y;
    num_tracks_y[hor_layer_idx] = std::move(num_track_y);
  }

  for (int ver_layer_idx : ver_layer_idxs) {
    odb::dbTechLayer* ver_layer = getTech()->findRoutingLayer(ver_layer_idx);
    odb::dbTrackGrid* ver_track_grid = getBlock()->findTrackGrid(ver_layer);
    const int track_patterns_count = ver_track_grid->getNumGridPatternsX();

    std::vector<int> init_track_x(track_patterns_count, 0);
    std::vector<int> num_track_x(track_patterns_count, 0);
    std::vector<int> min_spacing_x(track_patterns_count, 0);
    int min_area_x;
    int min_width_x;

    for (int i = 0; i < ver_track_grid->getNumGridPatternsX(); i++) {
      ver_track_grid->getGridPatternX(
          i, init_track_x[i], num_track_x[i], min_spacing_x[i]);
    }

    min_area_x = ver_layer->getArea() * database_unit * database_unit;
    min_width_x = ver_layer->getWidth();

    min_spacings_x[ver_layer_idx] = std::move(min_spacing_x);
    init_tracks_x[ver_layer_idx] = std::move(init_track_x);
    min_areas_x[ver_layer_idx] = min_area_x;
    min_widths_x[ver_layer_idx] = min_width_x;
    num_tracks_x[ver_layer_idx] = std::move(num_track_x);
  }

  const std::vector<odb::Point>& points
      = getBlock()->getDieAreaPolygon().getPoints();
  std::vector<odb::Line> polygon_edges;
  polygon_edges.reserve(points.size());

  for (size_t i = points.size() - 1; i >= 1; i--) {
    polygon_edges.emplace_back(points[i], points[i - 1]);
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
                database_unit,
                polygon_edges);
}

void IOPlacer::initTopLayerGrid()
{
  auto top_layer_grid = getBlock()->getBTermTopLayerGrid();
  if (top_layer_grid) {
    top_grid_ = std::make_unique<odb::dbBlock::dbBTermTopLayerGrid>(
        top_layer_grid.value());
  }
}

void IOPlacer::findSlotsForTopLayer()
{
  initTopLayerGrid();
  const odb::Rect& die_area = getBlock()->getDieArea();

  if (top_grid_ != nullptr && top_layer_slots_.empty()
      && top_grid_->pin_width > 0) {
    if (top_grid_->region.isRect()) {
      const int half_width = top_grid_->pin_width / 2;
      const int half_height = top_grid_->pin_height / 2;
      const odb::Rect& top_grid_region = top_grid_->region.getEnclosingRect();
      for (int x = top_grid_region.xMin(); x < top_grid_region.xMax();
           x += top_grid_->x_step) {
        for (int y = top_grid_region.yMin(); y < top_grid_region.yMax();
             y += top_grid_->y_step) {
          odb::Point ll(x - half_width, y - half_height);
          odb::Point lr(x + half_width, y - half_height);
          odb::Point ul(x - half_width, y + half_height);
          odb::Point ur(x + half_width, y + half_height);
          bool blocked = !die_area.intersects(ll) || !die_area.intersects(lr)
                         || !die_area.intersects(ul)
                         || !die_area.intersects(ur);

          top_layer_slots_.push_back(
              {blocked,
               false,
               odb::Point(x, y),
               top_grid_->layer->getRoutingLevel(),
               Edge::invalid,
               odb::Line()});  // Default-constructed line for top layer
        }
      }
    }

    filterObstructedSlotsForTopLayer();
  }
}

void IOPlacer::filterObstructedSlotsForTopLayer()
{
  // Collect top_grid obstructions
  std::vector<odb::Rect> obstructions;

  // Get routing obstructions
  for (odb::dbObstruction* obstruction : getBlock()->getObstructions()) {
    odb::dbBox* box = obstruction->getBBox();
    if (top_grid_ != nullptr && top_grid_->layer != nullptr
        && box->getTechLayer()->getRoutingLevel()
               == top_grid_->layer->getRoutingLevel()) {
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
            if (top_grid_ != nullptr && top_grid_->layer != nullptr
                && wire->getTechLayer()->getRoutingLevel()
                       == top_grid_->layer->getRoutingLevel()) {
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
          if (top_grid_ != nullptr && top_grid_->layer != nullptr
              && box->getTechLayer()->getRoutingLevel()
                     == top_grid_->layer->getRoutingLevel()) {
            odb::Rect obstruction_rect = box->getBox();
            obstructions.push_back(obstruction_rect);
          }
        }
      }
    }
  }

  // check for slots that go beyond the die boundary
  odb::Rect die_area = getBlock()->getDieArea();
  if (top_grid_ != nullptr) {
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
  if (top_grid_->region.isRect()) {
    const odb::Rect& top_grid_region = top_grid_->region.getEnclosingRect();
    for (int x = top_grid_region.xMin(); x < top_grid_region.xMax();
         x += top_grid_->x_step) {
      if (x < lb_x || x > ub_x) {
        continue;
      }
      std::vector<Slot>::iterator it
          = std::ranges::find_if(top_layer_slots_, [&](Slot s) {
              return (s.pos.x() >= x && s.pos.x() >= lb_x && s.pos.y() >= lb_y);
            });
      int edge_begin = it - top_layer_slots_.begin();
      int edge_x = top_layer_slots_[edge_begin].pos.x();

      it = std::find_if(top_layer_slots_.begin() + edge_begin,
                        top_layer_slots_.end(),
                        [&](Slot s) {
                          return s.pos.x() != edge_x || s.pos.x() >= ub_x
                                 || s.pos.y() >= ub_y;
                        });
      int edge_end = it - top_layer_slots_.begin() - 1;
      int end_slot = 0;

      while (end_slot < edge_end) {
        int blocked_slots = 0;
        end_slot = edge_begin + slots_per_section_ - 1;
        end_slot = std::min(end_slot, edge_end);
        for (int i = edge_begin; i <= end_slot; ++i) {
          if (top_layer_slots_[i].blocked) {
            blocked_slots++;
          }
        }
        int half_length_pt = edge_begin + (end_slot - edge_begin) / 2;
        Section n_sec;
        n_sec.pos = top_layer_slots_.at(half_length_pt).pos;
        n_sec.num_slots = end_slot - edge_begin - blocked_slots + 1;
        n_sec.begin_slot = edge_begin;
        n_sec.end_slot = end_slot;
        n_sec.used_slots = 0;
        n_sec.edge = Edge::invalid;

        sections.push_back(std::move(n_sec));
        edge_begin = ++end_slot;
      }
    }
  }

  return sections;
}

void IOPlacer::initNetlist()
{
  netlist_->reset();
  const odb::Rect& coreBoundary = core_->getBoundary();
  int x_center = (coreBoundary.xMin() + coreBoundary.xMax()) / 2;
  int y_center = (coreBoundary.yMin() + coreBoundary.yMax()) / 2;

  odb::dbSet<odb::dbBTerm> bterms = getBlock()->getBTerms();

  for (odb::dbBTerm* bterm : bterms) {
    int x_pos = 0;
    int y_pos = 0;
    bterm->getFirstPinLocation(x_pos, y_pos);
    if (bterm->getFirstPinPlacementStatus().isFixed()) {
      for (odb::dbBPin* bterm_pin : bterm->getBPins()) {
        for (odb::dbBox* bpin_box : bterm_pin->getBoxes()) {
          int layer = bpin_box->getTechLayer()->getRoutingLevel();
          layer_fixed_pins_shapes_[layer].push_back(bpin_box->getBox());
        }
      }
      continue;
    }
    odb::dbNet* net = bterm->getNet();
    if (net == nullptr) {
      logger_->warn(PPL, 38, "Pin {} without net.", bterm->getConstName());
      continue;
    }

    Direction dir = Direction::inout;
    switch (bterm->getIoType().getValue()) {
      case odb::dbIoType::INPUT:
        dir = Direction::input;
        break;
      case odb::dbIoType::OUTPUT:
        dir = Direction::output;
        break;
      default:
        dir = Direction::inout;
    }

    odb::Point bounds(0, 0);
    IOPin io_pin(bterm,
                 odb::Point(x_pos, y_pos),
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
                             odb::Point(x, y));
    }

    netlist_->addIONet(io_pin, inst_pins);
    if (inst_pins.empty()) {
      zero_sink_ios_.push_back(io_pin);
    }
  }

  int group_idx = 0;
  for (const auto& [pins, order] : getBlock()->getBTermGroups()) {
    std::string pin_names = getPinSetOrListString(pins);
    logger_->info(utl::PPL, 44, "Pin group: [ {} ]", pin_names);
    int group_created = netlist_->createIOGroup(pins, order, group_idx);
    if (group_created != pins.size()) {
      logger_->error(PPL, 94, "Cannot create group of size {}.", pins.size());
    }
    group_idx++;
  }
}

void IOPlacer::findConstraintRegion(const Interval& interval,
                                    const odb::Rect& constraint_box,
                                    odb::Rect& region)
{
  const odb::Rect& die_bounds = getBlock()->getDieArea();
  if (interval.getEdge() == Edge::bottom) {
    region = odb::Rect(interval.getBegin(),
                       die_bounds.yMin(),
                       interval.getEnd(),
                       die_bounds.yMin());
  } else if (interval.getEdge() == Edge::top) {
    region = odb::Rect(interval.getBegin(),
                       die_bounds.yMax(),
                       interval.getEnd(),
                       die_bounds.yMax());
  } else if (interval.getEdge() == Edge::left) {
    region = odb::Rect(die_bounds.xMin(),
                       interval.getBegin(),
                       die_bounds.xMin(),
                       interval.getEnd());
  } else if (interval.getEdge() == Edge::right) {
    region = odb::Rect(die_bounds.xMax(),
                       interval.getBegin(),
                       die_bounds.xMax(),
                       interval.getEnd());
  } else {
    region = constraint_box;
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

  odb::Point lower_bound = pin.getLowerBound();
  odb::Point upper_bound = pin.getUpperBound();

  if (!netlist_->getIOPins().empty()) {
    const int netlist_pin_idx = netlist_->getIoPinIdx(pin.getBTerm());
    IOPin& netlist_pin = netlist_->getIoPin(netlist_pin_idx);
    netlist_pin.setLowerBound(lower_bound.getX(), lower_bound.getY());
    netlist_pin.setUpperBound(upper_bound.getX(), upper_bound.getY());
  }

  odb::dbBPin* bpin = odb::dbBPin::create(bterm);

  int x_min = lower_bound.x();
  int y_min = lower_bound.y();
  int x_max = upper_bound.x();
  int y_max = upper_bound.y();

  odb::dbBox::create(bpin, layer, x_min, y_min, x_max, y_max);
  bpin->setPlacementStatus(pin.getPlacementStatus());
}

}  // namespace ppl
