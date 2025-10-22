// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/geom.h"
#include "sta/Hash.hh"
#include "sta/NetworkClass.hh"
#include "sta/Vector.hh"
#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"
#include "utl/Logger.h"

const int SteinerNull = -1;

namespace est {

using utl::Logger;

using sta::dbNetwork;
using sta::hashIncr;
using sta::Net;
using sta::Network;
using sta::Pin;
using sta::PinSeq;

using stt::SteinerTreeBuilder;

class PointHash
{
 public:
  size_t operator()(const odb::Point& pt) const;
};

class PointEqual
{
 public:
  bool operator()(const odb::Point& pt1, const odb::Point& pt2) const;
};

class PinLoc
{
 public:
  const Pin* pin;
  odb::Point loc;
};

using LocPinMap = std::unordered_map<odb::Point, PinSeq, PointHash, PointEqual>;
using SteinerPt = int;

class SteinerTree;

// Wrapper for stt::Tree
//
// Flute
//   rearranges pin order
//   compile time option to remove duplicate locations (disabled)
//
// pd/pdrev
//   preserves pin order
//   removes duplicate locations
class SteinerTree
{
 public:
  SteinerTree(const Pin* drvr_pin, sta::dbNetwork* db_network, Logger* logger);
  SteinerTree(odb::Point drvr_location, Logger* logger);
  sta::Vector<PinLoc>& pinlocs() { return pinlocs_; }
  int pinCount() const { return pinlocs_.size(); }
  int branchCount() const;
  void branch(int index,
              // Return values.
              odb::Point& pt1,
              int& steiner_pt1,
              odb::Point& pt2,
              int& steiner_pt2,
              int& wire_length);
  stt::Branch& branch(int index) { return tree_.branch[index]; }
  void report(Logger* logger, const Network* network);
  // Return the steiner pt connected to the driver pin.
  SteinerPt drvrPt() const;
  // new APIs for gate cloning
  SteinerPt top() const;
  SteinerPt left(SteinerPt pt) const;
  SteinerPt right(SteinerPt pt) const;
  void validatePoint(SteinerPt pt) const;

  void populateSides();
  void populateSides(SteinerPt from,
                     SteinerPt to,
                     const std::vector<SteinerPt>& adj1,
                     const std::vector<SteinerPt>& adj2,
                     const std::vector<SteinerPt>& adj3);
  void populateSides(SteinerPt from,
                     SteinerPt to,
                     SteinerPt adj,
                     const std::vector<SteinerPt>& adj1,
                     const std::vector<SteinerPt>& adj2,
                     const std::vector<SteinerPt>& adj3);

  // "Accessors" for SteinerPts.
  const char* name(SteinerPt pt, const Network* network);
  const PinSeq* pins(SteinerPt pt) const;
  const Pin* pin(SteinerPt pt) const;
  odb::Point location(SteinerPt pt) const;
  void setTree(const stt::Tree& tree);
  void setHasInputPort(bool input_port);
  stt::Tree& fluteTree() { return tree_; }
  void createSteinerPtToPinMap();
  void locAddPin(const odb::Point& loc, const Pin* pin);

  static constexpr SteinerPt null_pt = -1;

 private:
  stt::Tree tree_;
  const odb::Point drvr_location_;
  int drvr_steiner_pt_ = 0;      // index into tree_.branch
  sta::Vector<PinLoc> pinlocs_;  // Initial input
  LocPinMap loc_pin_map_;        // location -> pins map
  std::vector<SteinerPt> left_;
  std::vector<SteinerPt> right_;
  std::vector<const Pin*> point_pin_array_;
  Logger* logger_;
};

}  // namespace est
