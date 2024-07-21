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

#pragma once

#include <string>

#include "odb/geom.h"
#include "rsz/Resizer.hh"
#include "sta/Hash.hh"
#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"
#include "utl/Logger.h"

const int SteinerNull = -1;

namespace rsz {

using std::vector;

using utl::Logger;

using odb::Point;

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
  size_t operator()(const Point& pt) const;
};

class PointEqual
{
 public:
  bool operator()(const Point& pt1, const Point& pt2) const;
};

class PinLoc
{
 public:
  const Pin* pin;
  Point loc;
};

using LocPinMap = std::unordered_map<Point, PinSeq, PointHash, PointEqual>;

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
  SteinerTree(const Pin* drvr_pin, Resizer* resizer);
  Vector<PinLoc>& pinlocs() { return pinlocs_; }
  int pinCount() const { return pinlocs_.size(); }
  int branchCount() const;
  void branch(int index,
              // Return values.
              Point& pt1,
              int& steiner_pt1,
              Point& pt2,
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
  int distance(SteinerPt& from, SteinerPt& to) const;

  // "Accessors" for SteinerPts.
  const char* name(SteinerPt pt, const Network* network);
  const PinSeq* pins(SteinerPt pt) const;
  const Pin* pin(SteinerPt pt) const;
  Point location(SteinerPt pt) const;
  void setTree(const stt::Tree& tree, const dbNetwork* network);
  void setHasInputPort(bool input_port);
  stt::Tree& fluteTree() { return tree_; }
  void createSteinerPtToPinMap();

  static constexpr SteinerPt null_pt = -1;

 protected:
  void locAddPin(const Point& loc, const Pin* pin);

  stt::Tree tree_;
  const Pin* drvr_pin_;
  int drvr_steiner_pt_ = 0;  // index into tree_.branch
  Vector<PinLoc> pinlocs_;   // Initial input
  LocPinMap loc_pin_map_;    // location -> pins map
  std::vector<SteinerPt> left_;
  std::vector<SteinerPt> right_;
  std::vector<const Pin*> point_pin_array_;
  Resizer* resizer_;
  Logger* logger_;

  friend class Resizer;
  friend class GateCloner;
};

}  // namespace rsz
