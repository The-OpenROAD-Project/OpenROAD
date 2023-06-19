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

#include "rsz/Resizer.hh"
#include "utl/Logger.h"

#include "sta/Hash.hh"

#include "odb/geom.h"

#include "stt/SteinerTreeBuilder.h"
#include "stt/flute.h"

const int   SteinerNull = -1;

namespace rsz {

using std::vector;

using utl::Logger;

using odb::Point;

using sta::UnorderedMap;
using sta::Network;
using sta::dbNetwork;
using sta::Net;
using sta::Pin;
using sta::PinSeq;
using sta::hashIncr;

using stt::SteinerTreeBuilder;

class PointHash
{
public:
  size_t operator()(const Point &pt) const;
};

class PointEqual
{
public:
  bool operator()(const Point &pt1,
                  const Point &pt2) const;
};

typedef std::unordered_map<Point, PinSeq, PointHash, PointEqual> LocPinMap;

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
  SteinerTree(const Pin *drvr_pin, Resizer *resizer);
  PinSeq &pins() { return pins_; }
  int pinCount() const { return pins_.size(); }
  int branchCount() const;
  void branch(int index,
              // Return values.
              Point &pt1,
              int &steiner_pt1,
              Point &pt2,
              int &steiner_pt2,
              int &wire_length);
  stt::Branch &branch(int index) { return tree_.branch[index]; }
  void report(Logger *logger,
              const Network *network);
  // Return the steiner pt connected to the driver pin.
  SteinerPt drvrPt() const;
  // new APIs for gate cloning
  SteinerPt top() const;
  SteinerPt left(SteinerPt pt) const;
  SteinerPt right(SteinerPt pt) const;
  void validatePoint(SteinerPt pt) const;

  void populateSides();
  void populateSides(const SteinerPt from,
		     const SteinerPt to,
                     const std::vector<SteinerPt>& adj1,
                     const std::vector<SteinerPt>& adj2,
                     const std::vector<SteinerPt>& adj3);
  void populateSides(const SteinerPt from, const SteinerPt to,
		     const SteinerPt adj,
                     const std::vector<SteinerPt>& adj1,
                     const std::vector<SteinerPt>& adj2,
                     const std::vector<SteinerPt>& adj3);
  int distance(SteinerPt& from, SteinerPt& to) const;

  // "Accessors" for SteinerPts.
  const char *name(SteinerPt pt,
                   const Network *network);
  const PinSeq *pins(SteinerPt pt) const;
  const Pin *pin(SteinerPt pt) const;
  Point location(SteinerPt pt) const;
  void setTree(const stt::Tree& tree,
               const dbNetwork *network);
  void setHasInputPort(bool input_port);
  stt::Tree &fluteTree() { return tree_; }
  void createSteinerPtToPinMap();

  static SteinerPt null_pt;

protected:
  void locAddPin(Point &loc,
                 const Pin *pin);

  stt::Tree tree_;
  const Pin *drvr_pin_;
  int drvr_steiner_pt_;            // index into tree_.branch
  PinSeq pins_;                    // Initial input
  LocPinMap loc_pin_map_;          // location -> pins map
  std::vector<SteinerPt>  left_;
  std::vector<SteinerPt>  right_;
  std::vector<const Pin*> point_pin_array_;
  Resizer *resizer_;
  Logger *logger_;

  friend class Resizer;
  friend class GateCloner;
};

} // namespace
