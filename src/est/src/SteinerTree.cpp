// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "est/SteinerTree.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "odb/geom.h"
#include "sta/Hash.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/NetworkCmp.hh"
#include "sta/StringUtil.hh"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

namespace est {

using std::abs;
using std::string;

using utl::EST;

using sta::NetConnectedPinIterator;
using sta::stringPrintTmp;

void SteinerTree::setTree(const stt::Tree& tree)
{
  tree_ = tree;

  // Find driver steiner point.
  drvr_steiner_pt_ = null_pt;
  const odb::Point drvr_loc = drvr_location_;
  const int drvr_x = drvr_loc.getX();
  const int drvr_y = drvr_loc.getY();
  const int branch_count = tree_.branchCount();
  for (int i = 0; i < branch_count; i++) {
    const stt::Branch& pt1 = tree_.branch[i];
    if (pt1.x == drvr_x && pt1.y == drvr_y) {
      drvr_steiner_pt_ = i;
      break;
    }
  }
}

SteinerTree::SteinerTree(const sta::Pin* drvr_pin,
                         sta::dbNetwork* db_network,
                         utl::Logger* logger)
    : drvr_location_(db_network->location(drvr_pin)), logger_(logger)
{
}

SteinerTree::SteinerTree(odb::Point drvr_location, utl::Logger* logger)
    : drvr_location_(drvr_location), logger_(logger)
{
}

void SteinerTree::createSteinerPtToPinMap()
{
  const unsigned int pin_count = pinlocs_.size();

  point_pin_array_.resize(pin_count);
  for (unsigned int i = 0; i < pin_count; i++) {
    const stt::Branch& branch_pt = tree_.branch[i];
    const odb::Point pt(branch_pt.x, branch_pt.y);
    auto pin = loc_pin_map_[pt].back();
    point_pin_array_[i] = pin;
  }
  populateSides();
}

int SteinerTree::branchCount() const
{
  return tree_.branchCount();
}

void SteinerTree::locAddPin(const odb::Point& loc, const sta::Pin* pin)
{
  loc_pin_map_[loc].push_back(pin);
}

int SteinerTree::getMaxIndex() const
{
  int max_index = -1;
  for (int i = 0; i < branchCount(); i++) {
    const stt::Branch& branch_pt = tree_.branch[i];
    max_index = std::max(max_index, i);
    max_index = std::max({max_index, branch_pt.n});
  }

  return max_index;
}

void SteinerTree::branch(int index,
                         // Return values.
                         odb::Point& pt1,
                         int& steiner_pt1,
                         odb::Point& pt2,
                         int& steiner_pt2,
                         int& wire_length)
{
  stt::Branch& branch_pt1 = tree_.branch[index];
  steiner_pt1 = index;
  steiner_pt2 = branch_pt1.n;
  stt::Branch& branch_pt2 = tree_.branch[steiner_pt2];
  pt1 = odb::Point(branch_pt1.x, branch_pt1.y);
  pt2 = odb::Point(branch_pt2.x, branch_pt2.y);
  wire_length
      = abs(branch_pt1.x - branch_pt2.x) + abs(branch_pt1.y - branch_pt2.y);
}

void SteinerTree::report(utl::Logger* logger, const sta::Network* network)
{
  const int branch_count = branchCount();
  for (int i = 0; i < branch_count; i++) {
    const stt::Branch& pt1 = tree_.branch[i];
    const int j = pt1.n;
    const stt::Branch& pt2 = tree_.branch[j];
    const int wire_length = abs(pt1.x - pt2.x) + abs(pt1.y - pt2.y);
    logger->report(" {}{} ({} {}) - {} wire_length = {}",
                   name(i, network),
                   i == drvr_steiner_pt_ ? " drvr" : "",
                   pt1.x,
                   pt1.y,
                   name(j, network),
                   wire_length);
  }
}

const char* SteinerTree::name(const SteinerPt pt, const sta::Network* network)
{
  if (pt == null_pt) {
    return "NULL";
  }
  const sta::PinSeq* pt_pins = pins(pt);
  if (pt_pins) {
    string pin_names;
    bool first = true;
    for (const sta::Pin* pin : *pt_pins) {
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

const sta::PinSeq* SteinerTree::pins(const SteinerPt pt) const
{
  if (pt < tree_.deg) {
    auto loc_pins = loc_pin_map_.find(location(pt));
    if (loc_pins != loc_pin_map_.end()) {
      return &loc_pins->second;
    }
  }
  return nullptr;
}

SteinerPt SteinerTree::drvrPt() const
{
  return drvr_steiner_pt_;
}

odb::Point SteinerTree::location(const SteinerPt pt) const
{
  const stt::Branch branch_pt = tree_.branch[pt];
  return odb::Point(branch_pt.x, branch_pt.y);
}

SteinerPt SteinerTree::top() const
{
  const SteinerPt driver = drvrPt();
  SteinerPt top = left(driver);
  if (top == SteinerNull) {
    top = right(driver);
  }
  return top;
}

SteinerPt SteinerTree::left(const SteinerPt pt) const
{
  if (pt >= (int) left_.size()) {
    return SteinerNull;
  }
  return left_[pt];
}

SteinerPt SteinerTree::right(const SteinerPt pt) const
{
  if (pt >= (int) right_.size()) {
    return SteinerNull;
  }
  return right_[pt];
}

void SteinerTree::validatePoint(const SteinerPt pt) const
{
  if (pt < 0 || pt >= branchCount()) {
    logger_->error(
        EST,
        93,
        "Invalid Steiner point {} requested. 0 <= Valid values <  {}.",
        pt,
        branchCount());
  }
}

void SteinerTree::populateSides()
{
  const int branch_count = branchCount();
  left_.resize(branch_count, SteinerNull);
  right_.resize(branch_count, SteinerNull);
  std::vector<SteinerPt> adj1(branch_count, SteinerNull);
  std::vector<SteinerPt> adj2(branch_count, SteinerNull);
  std::vector<SteinerPt> adj3(branch_count, SteinerNull);
  for (int i = 0; i < branch_count; i++) {
    const stt::Branch& branch_pt = tree_.branch[i];
    const SteinerPt j = branch_pt.n;
    if (j != i) {
      if (adj1[i] == SteinerNull) {
        adj1[i] = j;
      } else if (adj2[i] == SteinerNull) {
        adj2[i] = j;
      } else {
        adj3[i] = j;
      }

      if (adj1[j] == SteinerNull) {
        adj1[j] = i;
      } else if (adj2[j] == SteinerNull) {
        adj2[j] = i;
      } else {
        adj3[j] = i;
      }
    }
  }

  const SteinerPt root = drvrPt();
  const SteinerPt root_adj = adj1[root];
  left_[root] = root_adj;
  populateSides(root, root_adj, adj1, adj2, adj3);
}

void SteinerTree::populateSides(const SteinerPt from,
                                const SteinerPt to,
                                const std::vector<SteinerPt>& adj1,
                                const std::vector<SteinerPt>& adj2,
                                const std::vector<SteinerPt>& adj3)
{
  if (to >= (int) pinlocs_.size()) {
    SteinerPt adj;
    adj = adj1[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
    adj = adj2[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
    adj = adj3[to];
    populateSides(from, to, adj, adj1, adj2, adj3);
  }
}

void SteinerTree::populateSides(const SteinerPt from,
                                const SteinerPt to,
                                const SteinerPt adj,
                                const std::vector<SteinerPt>& adj1,
                                const std::vector<SteinerPt>& adj2,
                                const std::vector<SteinerPt>& adj3)
{
  if (adj != from && adj != SteinerNull) {
    if (adj == to) {
      logger_->error(EST, 92, "Steiner tree creation error.");
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

const sta::Pin* SteinerTree::pin(const SteinerPt pt) const
{
  validatePoint(pt);
  if (pt < (int) pinlocs_.size()) {
    return point_pin_array_[pt];
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////

size_t PointHash::operator()(const odb::Point& pt) const
{
  size_t hash = sta::hash_init_value;
  sta::hashIncr(hash, pt.x());
  sta::hashIncr(hash, pt.y());
  return hash;
}

bool PointEqual::operator()(const odb::Point& pt1, const odb::Point& pt2) const
{
  return pt1.x() == pt2.x() && pt1.y() == pt2.y();
}

}  // namespace est
