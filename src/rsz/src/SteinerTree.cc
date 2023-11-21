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

#include "SteinerTree.hh"

#include <memory>
#include <string>

#include "AbstractSteinerRenderer.h"
#include "db_sta/dbNetwork.hh"
#include "rsz/Resizer.hh"
#include "sta/NetworkCmp.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace rsz {

using std::abs;
using std::string;

using utl::RSZ;

using sta::stringPrintTmp;
using sta::NetConnectedPinIterator;

static void
connectedPins(const Net *net,
              Network *network,
              // Return value.
              PinSeq &pins);

SteinerPt SteinerTree::null_pt = -1;

// Returns nullptr if net has less than 2 pins or any pin is not placed.
SteinerTree *
Resizer::makeSteinerTree(const Pin *drvr_pin)
{
  Network *sdc_network = network_->sdcNetwork();
  Net *net = network_->isTopLevelPort(drvr_pin)
    ? network_->net(network_->term(drvr_pin))
    : network_->net(drvr_pin);
  debugPrint(logger_, RSZ, "steiner", 1, "Net {}",
             sdc_network->pathName(net));
  SteinerTree *tree = new SteinerTree(drvr_pin, this);
  PinSeq &pins = tree->pins();
  // Find all the connected pins
  connectedPins(net, network_, pins);
  // Sort pins by location because connectedPins order is not deterministic.
  sort(pins, [=](const Pin *pin1, const Pin *pin2) {
      Point loc1 = db_network_->location(pin1);
      Point loc2 = db_network_->location(pin2);
      return loc1.getX() < loc2.getX()
        || (loc1.getX() == loc2.getX()
            && loc1.getY() < loc2.getY());
  });
  int pin_count = pins.size();
  bool is_placed = true;
  // Warn if there are too many pins (>10000)
  if (pin_count > max_steiner_pin_count_) {
    logger_->warn(RSZ, 69, "skipping net {} with {} pins.",
                  sdc_network->pathName(net),
                  pin_count);
  } else if (pin_count >= 2) {
    vector<int> x, y;  // Two separate vectors of coordinates needed by flute.
    int drvr_idx = 0; // The "driver_pin" or the root of the Steiner tree.
    for (int i = 0; i < pin_count; i++) {
      const Pin *pin = pins[i];
      if (pin == drvr_pin) {
        drvr_idx = i;  // drvr_index is needed by flute.
      }
      Point loc = db_network_->location(pin);
      x.push_back(loc.x());
      y.push_back(loc.y());
      debugPrint(logger_, RSZ, "steiner", 3, " {} ({} {})",
                 sdc_network->pathName(pin),
                 loc.x(), loc.y());
      // Track that all our pins are placed.
      is_placed &= db_network_->isPlaced(pin);

      // Flute may reorder the input points, so it takes some unravelling
      // to find the mapping back to the original pins. The complication is
      // that multiple pins can occupy the same location.
      tree->locAddPin(loc, pin);
    }
    if (is_placed) {
      stt::Tree ftree = stt_builder_->makeSteinerTree(db_network_->staToDb(net),
                                                      x, y, drvr_idx);
      
      tree->setTree(ftree, db_network_);
      tree->createSteinerPtToPinMap();
      return tree;
    }
  }
  delete tree;
  return nullptr;
}

static void
connectedPins(const Net *net,
              Network *network,
              // Return value.
              PinSeq &pins)
{
  NetConnectedPinIterator *pin_iter = network->connectedPinIterator(net);
  while (pin_iter->hasNext()) {
    const Pin *pin = pin_iter->next();
    pins.push_back(pin);
  }
  delete pin_iter;
}

void
SteinerTree::setTree(const stt::Tree& tree,
                     const dbNetwork *network)
{
  tree_ = tree;

  // Find driver steiner point.
  drvr_steiner_pt_ = null_pt;
  Point drvr_loc = network->location(drvr_pin_);
  int drvr_x = drvr_loc.getX();
  int drvr_y = drvr_loc.getY();
  int branch_count = tree_.branchCount();
  for (int i = 0; i < branch_count; i++) {
    stt::Branch &pt1 = tree_.branch[i];
    if (pt1.x == drvr_x
        && pt1.y == drvr_y) {
      drvr_steiner_pt_ = i;
      break;
    }
  }
}

SteinerTree::SteinerTree(const Pin *drvr_pin, Resizer *resizer) :
  drvr_pin_(drvr_pin),
  drvr_steiner_pt_(0),
  resizer_(resizer),
  logger_(resizer->logger())
{
}

void
SteinerTree::createSteinerPtToPinMap()
{
  unsigned int pin_count = pins_.size();

  point_pin_array_.resize(pin_count);
  for (unsigned int i = 0; i < pin_count; i++) {
    stt::Branch& branch_pt = tree_.branch[i];
    odb::Point pt(branch_pt.x, branch_pt.y);
    auto pin = loc_pin_map_[pt].back();
    point_pin_array_[i] = pin;
  }
  populateSides();
}

int
SteinerTree::branchCount() const
{
  return tree_.branchCount();
}

void
SteinerTree::locAddPin(Point &loc,
                       const Pin *pin)
{
  PinSeq &pins = loc_pin_map_[loc];
  pins.push_back(pin);
}

void
SteinerTree::branch(int index,
                    // Return values.
                    Point &pt1,
                    int &steiner_pt1,
                    Point &pt2,
                    int &steiner_pt2,
                    int &wire_length)
{
  stt::Branch &branch_pt1 = tree_.branch[index];
  steiner_pt1 = index;
  steiner_pt2 = branch_pt1.n;
  stt::Branch &branch_pt2 = tree_.branch[steiner_pt2];
  pt1 = Point(branch_pt1.x, branch_pt1.y);
  pt2 = Point(branch_pt2.x, branch_pt2.y);
  wire_length = abs(branch_pt1.x - branch_pt2.x)
    + abs(branch_pt1.y - branch_pt2.y);
}

void
SteinerTree::report(Logger *logger,
                    const Network *network)
{
  int branch_count = branchCount();
  for (int i = 0; i < branch_count; i++) {
    stt::Branch &pt1 = tree_.branch[i];
    int j = pt1.n;
    stt::Branch &pt2 = tree_.branch[j];
    int wire_length = abs(pt1.x - pt2.x) + abs(pt1.y - pt2.y);
    logger->report(" {}{} ({} {}) - {} wire_length = {}",
                   name(i, network),
                   i == drvr_steiner_pt_ ? " drvr" : "",
                   pt1.x,
                   pt1.y,
                   name(j, network),
                   wire_length);
  }
}

const char *
SteinerTree::name(SteinerPt pt,
                  const Network *network)
{
  if (pt == null_pt) {
    return "NULL";
  } 
  const PinSeq *pt_pins = pins(pt);
  if (pt_pins) {
    string pin_names;
    bool first = true;
    for (const Pin *pin : *pt_pins) {
      if (!first) {
        pin_names += " ";
      }
      pin_names += network->pathName(pin);
      first = false;
    }
    return stringPrintTmp("%s", pin_names.c_str());
  }
  return stringPrintTmp("S%d", pt);
}

const PinSeq *
SteinerTree::pins(SteinerPt pt) const
{
  if (pt < tree_.deg) {
    auto loc_pins = loc_pin_map_.find(location(pt));
    if (loc_pins != loc_pin_map_.end()) {
      return &loc_pins->second;
    }
  }
  return nullptr;
}

SteinerPt
SteinerTree::drvrPt() const
{
  return drvr_steiner_pt_;
}

Point
SteinerTree::location(SteinerPt pt) const
{
  stt::Branch branch_pt = tree_.branch[pt];
  return Point(branch_pt.x, branch_pt.y);
}

SteinerPt
SteinerTree::top() const
{
  SteinerPt driver = drvrPt();
  SteinerPt top    = left(driver);
  if (top == SteinerNull) {
    top = right(driver);
  }
  return top;
}

SteinerPt
SteinerTree::left(SteinerPt pt) const
{
  if (pt >= (int) left_.size()) {
    return SteinerNull;
  }
  return left_[pt];
}

SteinerPt
SteinerTree::right(SteinerPt pt) const
{
  if (pt >= (int) right_.size()) {
    return SteinerNull;
  }
  return right_[pt];
}

void
SteinerTree::validatePoint(SteinerPt pt) const
{
  if (pt < 0 || pt >= branchCount()) {
    logger_->error(RSZ, 93,
                   "Invalid Steiner point {} requested. 0 <= Valid values <  {}.",
                   pt, branchCount());
  }
}

void SteinerTree::populateSides()
{
  int branch_count = branchCount();
  left_.resize(branch_count, SteinerNull);
  right_.resize(branch_count, SteinerNull);
  std::vector<SteinerPt> adj1(branch_count, SteinerNull);
  std::vector<SteinerPt> adj2(branch_count, SteinerNull);
  std::vector<SteinerPt> adj3(branch_count, SteinerNull);
  for (int i = 0; i < branch_count; i++) {
    stt::Branch& branch_pt = tree_.branch[i];
    SteinerPt j = branch_pt.n;
    if (j != i) {
      if (adj1[i] == SteinerNull) {
        adj1[i] = j;
      }
      else if (adj2[i] == SteinerNull) {
        adj2[i] = j;
      }
      else {
        adj3[i] = j;
      }

      if (adj1[j] == SteinerNull) {
        adj1[j] = i;
      }
      else if (adj2[j] == SteinerNull) {
        adj2[j] = i;
      }
      else {
        adj3[j] = i;
      }
    }
  }

  SteinerPt root = drvrPt();
  SteinerPt root_adj = adj1[root];
  left_[root] = root_adj;
  populateSides(root, root_adj, adj1, adj2, adj3);
}

void
SteinerTree::populateSides(const SteinerPt from, const SteinerPt to,
                           const std::vector<SteinerPt>& adj1,
                           const std::vector<SteinerPt>& adj2,
                           const std::vector<SteinerPt>& adj3)
{
  if (to >= (int) pins_.size()) {
    SteinerPt adj;
    adj = adj1[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
    adj = adj2[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
    adj = adj3[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
  }
}

void
SteinerTree::populateSides(const SteinerPt from, const SteinerPt to, const SteinerPt adj,
                           const std::vector<SteinerPt>& adj1,
                           const std::vector<SteinerPt>& adj2,
                           const std::vector<SteinerPt>& adj3)
{
  if (adj != from && adj != SteinerNull) {
    if (adj == to) {
      logger_->error(RSZ, 92, "Steiner tree creation error.");
    }
    if (left_[to] == SteinerNull) {
      left_[to] = adj;
      populateSides(to, adj, adj1, adj2, adj3);
    } else if (right_[to] == SteinerNull) {
      right_[to] = adj;
      populateSides(to, adj, adj1, adj2, adj3);
    }
  }
}

int SteinerTree::distance(SteinerPt& from, SteinerPt& to) const
{
  int find_left, find_right;
  if (from == SteinerNull || to == SteinerNull) {
    return -1;
  }
  if (from == to) {
    return 0;
  }
  Point from_pt = location(from);
  Point to_pt = location(to);
  SteinerPt left_from = left(from);
  SteinerPt right_from = right(from);
  if (left_from == to || right_from == to) {
    return abs(from_pt.x() - to_pt.x()) + abs(from_pt.y() - to_pt.y());
  }
  if (left_from == SteinerNull && right_from == SteinerNull) {
    return -1;
  }

  find_left = distance(left_from, to);
  if (find_left >= 0) {
    return find_left + abs(from_pt.x() - to_pt.x())
           + abs(from_pt.y() - to_pt.y());
  }

  find_right = distance(right_from, to);
  if (find_right >= 0) {
    return find_right + abs(from_pt.x() - to_pt.x())
           + abs(from_pt.y() - to_pt.y());
  }
  return -1;
}

const Pin *SteinerTree::pin(SteinerPt pt) const
{
  validatePoint(pt);
  if (pt < (int)pins_.size()) {
    return point_pin_array_[pt];
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

void
Resizer::highlightSteiner(const Pin *drvr)
{
  if (steiner_renderer_) {
    SteinerTree *tree = nullptr;
    if (drvr) {
      tree = makeSteinerTree(drvr);
    }
    steiner_renderer_->highlight(tree);
  }
}

////////////////////////////////////////////////////////////////

size_t
PointHash::operator()(const Point &pt) const
{
  size_t hash = sta::hash_init_value;
  hashIncr(hash, pt.x());
  hashIncr(hash, pt.y());
  return hash;
}

bool
PointEqual::operator()(const Point &pt1,
                       const Point &pt2) const
{
  return pt1.x() == pt2.x()
    && pt1.y() == pt2.y();
}

} // namespace rsz
