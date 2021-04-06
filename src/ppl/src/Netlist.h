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

#pragma once

#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "opendb/db.h"

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

enum class Direction
{
  input,
  output,
  inout,
  feedthru,
  invalid
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
        odb::Point lower_bound,
        odb::Point upper_bound,
        std::string location_type)
      : bterm_(bterm),
        pos_(pos),
        orientation_(Orientation::north),
        direction_(dir),
        lower_bound_(lower_bound),
        upper_bound_(upper_bound),
        location_type_(location_type),
        layer_(-1),
        in_group(false)
  {
  }

  void setOrientation(const Orientation o) { orientation_ = o; }
  Orientation getOrientation() const { return orientation_; }
  odb::Point getPosition() const { return pos_; }
  void setX(const int x) { pos_.setX(x); }
  void setY(const int y) { pos_.setY(y); }
  void setPos(const odb::Point pos) { pos_ = pos; }
  void setPos(const int x, const int y) { pos_ = odb::Point(x, y); }
  void setLowerBound(const int x, const int y)
  {
    lower_bound_ = odb::Point(x, y);
  };
  void setUpperBound(const int x, const int y)
  {
    upper_bound_ = odb::Point(x, y);
  };
  void setLayer(const int layer) { layer_ = layer; }
  std::string getName() const { return bterm_->getName(); }
  odb::Point getPos() const { return pos_; }
  int getX() const { return pos_.getX(); }
  int getY() const { return pos_.getY(); }
  Direction getDirection() const { return direction_; }
  odb::Point getLowerBound() const { return lower_bound_; };
  odb::Point getUpperBound() const { return upper_bound_; };
  std::string getNetName() const { return bterm_->getNet()->getName(); }
  std::string getLocationType() const { return location_type_; };
  odb::dbBTerm* getBTerm() const { return bterm_; }
  int getLayer() const { return layer_; }
  bool isInGroup() const { return in_group; }
  void inGroup() { in_group = true; }

 private:
  odb::dbBTerm* bterm_;
  odb::Point pos_;
  Orientation orientation_;
  Direction direction_;
  odb::Point lower_bound_;
  odb::Point upper_bound_;
  std::string location_type_;
  int layer_;
  bool in_group;
};

class Netlist
{
 public:
  Netlist();

  void addIONet(const IOPin&, const std::vector<InstancePin>&);
  int createIOGroup(const std::vector<odb::dbBTerm*>& pin_list);
  void addIOGroup(const std::vector<int>& pin_group);
  std::vector<std::vector<int>>& getIOGroups() { return io_groups_; }
  void setIOGroups(const std::vector<std::vector<int>>& io_groups) { io_groups_ = io_groups; }
  int numSinksOfIO(int);
  int numIOPins();
  int numIOGroups() { return io_groups_.size(); }
  std::vector<IOPin>& getIOPins() { return io_pins_; }
  IOPin& getIoPin(int idx) { return io_pins_[idx]; }
  void getSinksOfIO(int idx, std::vector<InstancePin>& sinks);

  int computeIONetHPWL(int, odb::Point);
  int computeIONetHPWL(int, odb::Point, Edge, const std::vector<Constraint>&);
  int computeIONetHPWL(int, const Section&, const std::vector<Constraint>&, const std::vector<Slot>&);
  int computeDstIOtoPins(int, odb::Point);
  odb::Rect getBB(int, odb::Point);
  void clear();

 private:
  std::vector<InstancePin> inst_pins_;
  std::vector<int> net_pointer_;
  std::vector<IOPin> io_pins_;
  std::vector<std::vector<int>> io_groups_;
  std::map<odb::dbBTerm*, int> _db_pin_idx_map;

  bool checkSlotForPin(const IOPin& pin,
                       Edge edge,
                       const odb::Point& point,
                       const std::vector<Constraint>& constraints) const;
  bool checkSectionForPin(const IOPin& pin,
                          const Section& section,
                          const std::vector<Constraint>& constraints,
                          const std::vector<Slot>& slots,
                          int& available_slots) const;
  bool checkInterval(Constraint constraint, Edge edge, int pos) const;
};

}  // namespace ppl
