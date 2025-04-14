// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "odb/db.h"
#include "ppl/IOPlacer.h"

namespace ppl {

using odb::Point;
using odb::Rect;

struct Constraint;
struct Section;
struct Slot;
enum class Edge;

enum class Orientation
{
  north,
  south,
  east,
  west
};

class InstancePin
{
 public:
  InstancePin(const std::string& name, const odb::Point& pos)
      : name_(name), pos_(pos)
  {
  }
  std::string getName() const { return name_; }
  odb::Point getPos() const { return pos_; }
  int getX() const { return pos_.getX(); }
  int getY() const { return pos_.getY(); }

 private:
  std::string name_;
  odb::Point pos_;
};

class IOPin
{
 public:
  IOPin(odb::dbBTerm* bterm,
        const odb::Point& pos,
        Direction dir,
        const odb::Point& lower_bound,
        const odb::Point& upper_bound,
        const odb::dbPlacementStatus& placement_status)
      : bterm_(bterm),
        pos_(pos),
        direction_(dir),
        lower_bound_(lower_bound),
        upper_bound_(upper_bound),
        placement_status_(placement_status)
  {
  }

  void setOrientation(const Orientation o) { orientation_ = o; }
  Orientation getOrientation() const { return orientation_; }
  odb::Point getPosition() const { return pos_; }
  void setX(const int x) { pos_.setX(x); }
  void setY(const int y) { pos_.setY(y); }
  void setPosition(const odb::Point& pos) { pos_ = pos; }
  void setPosition(const int x, const int y) { pos_ = odb::Point(x, y); }
  void setLowerBound(const int x, const int y)
  {
    lower_bound_ = odb::Point(x, y);
  };
  void setUpperBound(const int x, const int y)
  {
    upper_bound_ = odb::Point(x, y);
  };
  std::string getName() const { return bterm_->getName(); }
  int getX() const { return pos_.getX(); }
  int getY() const { return pos_.getY(); }
  Direction getDirection() const { return direction_; }
  odb::Point getLowerBound() const { return lower_bound_; };
  odb::Point getUpperBound() const { return upper_bound_; };
  int getArea() const;
  std::string getNetName() const { return bterm_->getNet()->getName(); }
  odb::dbPlacementStatus getPlacementStatus() const
  {
    return placement_status_;
  };
  odb::dbBTerm* getBTerm() const { return bterm_; }
  int getLayer() const { return layer_; }
  void setLayer(const int layer) { layer_ = layer; }
  int getGroupIdx() const { return group_idx_; }
  void setGroupIdx(const int group_idx) { group_idx_ = group_idx; }
  int getConstraintIdx() const { return constraint_idx_; }
  void setConstraintIdx(const int constraint_idx)
  {
    constraint_idx_ = constraint_idx;
  }
  int getMirrorPinIdx() const { return mirror_pin_idx_; }
  void setMirrorPinIdx(const int mirror_pin_idx)
  {
    mirror_pin_idx_ = mirror_pin_idx;
  }
  bool isPlaced() const { return is_placed_; }
  void setPlaced() { is_placed_ = true; }
  bool isInGroup() const { return in_group_; }
  void setInGroup() { in_group_ = true; }
  bool isInConstraint() const { return in_constraint_; }
  void setInConstraint() { in_constraint_ = true; }
  void assignToSection() { assigned_to_section_ = true; }
  bool isAssignedToSection() { return assigned_to_section_; }
  void setMirrored() { is_mirrored_ = true; }
  bool isMirrored() const { return is_mirrored_; }
  bool inFallback() const { return in_fallback_; }
  void setFallback() { in_fallback_ = true; }
  Edge getEdge() const { return edge_; }
  void setEdge(Edge edge) { edge_ = edge; }

 private:
  odb::dbBTerm* bterm_;
  odb::Point pos_;
  Orientation orientation_{Orientation::north};
  Direction direction_;
  odb::Point lower_bound_;
  odb::Point upper_bound_;
  odb::dbPlacementStatus placement_status_;
  int layer_{-1};
  int group_idx_{-1};
  int constraint_idx_{-1};
  int mirror_pin_idx_{-1};
  bool is_placed_{false};
  bool in_group_{false};
  bool in_constraint_{false};
  bool assigned_to_section_{false};
  bool is_mirrored_{false};
  bool in_fallback_{false};
  Edge edge_{Edge::invalid};
};

class Netlist
{
 public:
  Netlist();

  void addIONet(const IOPin& io_pin, const std::vector<InstancePin>& inst_pins);
  int createIOGroup(const std::vector<odb::dbBTerm*>& pin_list,
                    bool order,
                    int group_idx);
  void addIOGroup(const std::vector<int>& pin_group, bool order);
  const std::vector<PinGroupByIndex>& getIOGroups() { return io_groups_; }
  void setIOGroups(const std::vector<PinGroupByIndex>& io_groups)
  {
    io_groups_ = io_groups;
  }
  int numSinksOfIO(int idx);
  int numIOPins();
  int numIOGroups() { return io_groups_.size(); }
  std::vector<IOPin>& getIOPins() { return io_pins_; }
  IOPin& getIoPin(int idx) { return io_pins_[idx]; }
  int getIoPinIdx(odb::dbBTerm* bterm) { return _db_pin_idx_map[bterm]; }
  void getSinksOfIO(int idx, std::vector<InstancePin>& sinks);

  int computeIONetHPWL(int idx, const odb::Point& slot_pos);
  int computeDstIOtoPins(int idx, const odb::Point& slot_pos);
  void sortPinsFromGroup(int group_idx, Edge edge);
  odb::Rect getBB(int idx, const odb::Point& slot_pos);
  void reset();

 private:
  std::vector<InstancePin> inst_pins_;
  std::vector<int> net_pointer_;
  std::vector<IOPin> io_pins_;
  std::vector<PinGroupByIndex> io_groups_;
  std::map<odb::dbBTerm*, int> _db_pin_idx_map;
};

}  // namespace ppl
