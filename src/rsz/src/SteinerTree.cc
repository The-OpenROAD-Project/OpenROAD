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

#include "rsz/SteinerTree.hh"

#include <string>

#include "utl/Logger.h"
// Move logger macro out of the way.
#undef debugPrint

#include "sta/Debug.hh"
#include "sta/NetworkCmp.hh"

#include "stt/SteinerTreeBuilder.h"

namespace rsz {

using std::abs;
using std::string;

using utl::RSZ;

using odb::dbShape;
using odb::dbPlacementStatus;

using sta::Debug;
using sta::Report;
using sta::stringPrint;
using sta::stringPrintTmp;
using sta::PinPathNameLess;
using sta::NetConnectedPinIterator;

static void
connectedPins(const Net *net,
              Network *network,
              // Return value.
              PinSeq &pins);

SteinerPt SteinerTree::null_pt = -1;

SteinerTree *
makeSteinerTree(const Pin *drvr_pin,
                bool find_left_rights,
                dbNetwork *network,
                Logger *logger,
                SteinerTreeBuilder *stt_builder)
{
  Network *sdc_network = network->sdcNetwork();
  Debug *debug = network->debug();
  Net *net = network->isTopLevelPort(drvr_pin)
    ? network->net(network->term(drvr_pin))
    : network->net(drvr_pin);
  debugPrint(debug, "steiner", 1, "Net %s",
             sdc_network->pathName(net));
  SteinerTree *tree = new SteinerTree(drvr_pin);
  PinSeq &pins = tree->pins();
  connectedPins(net, network, pins);
  // Steiner tree is apparently sensitive to pin order.
  // Pay the price to stabilize the results.
  sort(pins, PinPathNameLess(network));
  int pin_count = pins.size();
  bool is_placed = true;
  if (pin_count >= 2) {
    int x[pin_count];
    int y[pin_count];
    int drvr_idx = 0;
    for (int i = 0; i < pin_count; i++) {
      Pin *pin = pins[i];
      if (pin == drvr_pin)
        drvr_idx = i;
      Point loc = network->location(pin);
      x[i] = loc.x();
      y[i] = loc.y();
      debugPrint(debug, "steiner", 3, " %s (%d %d)",
                 sdc_network->pathName(pin),
                 loc.x(), loc.y());
      is_placed &= network->isPlaced(pin);

      // Flute may reorder the input points, so it takes some unravelling
      // to find the mapping back to the original pins. The complication is
      // that multiple pins can occupy the same location.
      tree->locAddPin(loc, pin);
    }
    if (is_placed) {
      std::vector<int> x1(x, x + pin_count);
      std::vector<int> y1(y, y + pin_count);
      stt::Tree ftree = stt_builder->buildSteinerTree(network->staToDb(net), x1, y1, drvr_idx);
      
      if (debug->check("steiner", 3))
        ftree.printTree(logger);
      tree->setTree(ftree, network);
      if (find_left_rights)
        tree->findLeftRights(network, logger);
      if (debug->check("steiner", 3))
        tree->report(logger, network);
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
    Pin *pin = pin_iter->next();
    pins.push_back(pin);
  }
  delete pin_iter;
}

void
SteinerTree::setTree(stt::Tree tree,
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

SteinerTree::SteinerTree(const Pin *drvr_pin) :
  drvr_pin_(drvr_pin)
{
}

int
SteinerTree::branchCount() const
{
  return tree_.branchCount();
}

void
SteinerTree::locAddPin(Point &loc,
                       Pin *pin)
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
    logger->report(" {}{} ({} {}) - {} wire_length = {} left = {} right = {}",
                   name(i, network),
                   i == drvr_steiner_pt_ ? " drvr" : "",
                   pt1.x,
                   pt1.y,
                   name(j, network),
                   wire_length,
                   left_.size() ? name(this->left(i), network) : "",
                   right_.size() ? name(this->right(i), network) : "");
  }
}

const char *
SteinerTree::name(SteinerPt pt,
                  const Network *network)
{
  if (pt == null_pt)
    return "NULL";
  else {
    const PinSeq *pt_pins = pins(pt);
    if (pt_pins) {
      string pin_names;
      bool first = true;
      for (const Pin *pin : *pt_pins) {
        if (!first)
          pin_names += " ";
        pin_names += network->pathName(pin);
        first = false;
      }
      return stringPrintTmp("%s", pin_names.c_str());
    }
    else
      return stringPrintTmp("S%d", pt);
  }
}

const PinSeq *
SteinerTree::pins(SteinerPt pt) const
{
  if (pt < tree_.deg) {
    auto loc_pins = loc_pin_map_.find(location(pt));
    if (loc_pins != loc_pin_map_.end())
      return &loc_pins->second;
  }
  return nullptr;
}

SteinerPt
SteinerTree::drvrPt(const Network *network) const
{
  return drvr_steiner_pt_;
}

Point
SteinerTree::location(SteinerPt pt) const
{
  stt::Branch branch_pt = tree_.branch[pt];
  return Point(branch_pt.x, branch_pt.y);
}

void
SteinerTree::findLeftRights(const Network *network,
                            Logger *logger)
{
  Debug *debug = network->debug();
  int branch_count = branchCount();
  left_.resize(branch_count, null_pt);
  right_.resize(branch_count, null_pt);
  SteinerPtSeq adj1(branch_count, null_pt);
  SteinerPtSeq adj2(branch_count, null_pt);
  SteinerPtSeq adj3(branch_count, null_pt);
  for (int i = 0; i < branch_count; i++) {
    stt::Branch &branch_pt = tree_.branch[i];
    SteinerPt j = branch_pt.n;
    if (j != i) {
      if (adj1[i] == null_pt)
        adj1[i] = j;
      else if (adj2[i] == null_pt)
        adj2[i] = j;
      else
        adj3[i] = j;

      if (adj1[j] == null_pt)
        adj1[j] = i;
      else if (adj2[j] == null_pt)
        adj2[j] = i;
      else
        adj3[j] = i;
    }
  }
  if (debug->check("steiner", 3)) {
    printf("adjacent\n");
    for (int i = 0; i < branch_count; i++) {
      printf("%d:", i);
      if (adj1[i] != null_pt)
        printf(" %d", adj1[i]);
      if (adj2[i] != null_pt)
        printf(" %d", adj2[i]);
      if (adj3[i] != null_pt)
        printf(" %d", adj3[i]);
      printf("\n");
    }
  }
  SteinerPt root = drvrPt(network);
  SteinerPt root_adj = adj1[root];
  left_[root] = root_adj;
  findLeftRights(root, root_adj, adj1, adj2, adj3, logger);
}

void
SteinerTree::findLeftRights(SteinerPt from,
                            SteinerPt to,
                            SteinerPtSeq &adj1,
                            SteinerPtSeq &adj2,
                            SteinerPtSeq &adj3,
                            Logger *logger)
{
  if (to >= tree_.deg) {
    SteinerPt adj;
    adj = adj1[to];
    findLeftRights(from, to, adj, adj1, adj2, adj3, logger);
    adj = adj2[to];
    findLeftRights(from, to, adj, adj1, adj2, adj3, logger);
    adj = adj3[to];
    findLeftRights(from, to, adj, adj1, adj2, adj3, logger);
  }
}

void
SteinerTree::findLeftRights(SteinerPt from,
                            SteinerPt to,
                            SteinerPt adj,
                            SteinerPtSeq &adj1,
                            SteinerPtSeq &adj2,
                            SteinerPtSeq &adj3,
                            Logger *logger)
{
  if (adj != from && adj != null_pt) {
    if (adj == to)
      logger->critical(RSZ, 45, "steiner left/right failed.");
    if (left_[to] == null_pt) {
      left_[to] = adj;
      findLeftRights(to, adj, adj1, adj2, adj3, logger);
    }
    else if (right_[to] == null_pt) {
      right_[to] = adj;
      findLeftRights(to, adj, adj1, adj2, adj3, logger);
    }
  }
}

SteinerPt
SteinerTree::left(SteinerPt pt)
{
  return left_[pt];
}

SteinerPt
SteinerTree::right(SteinerPt pt)
{
  return right_[pt];
}

}
