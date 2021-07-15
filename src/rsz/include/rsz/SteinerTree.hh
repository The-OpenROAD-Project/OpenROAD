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

#include "utl/Logger.h"

#include "sta/Hash.hh"
#include "sta/UnorderedMap.hh"

#include "opendb/geom.h"

#include "db_sta/dbNetwork.hh"

#include "stt/SteinerTreeBuilder.h"

namespace rsz {

using utl::Logger;

using odb::Point;

using sta::UnorderedMap;
using sta::Vector;
using sta::Network;
using sta::dbNetwork;
using sta::Net;
using sta::Pin;
using sta::PinSeq;
using sta::hashIncr;

using stt::SteinerTreeBuilder;

class SteinerTree;

class PointHash
{
public:
  size_t operator()(const Point &pt) const
  {
    size_t hash = sta::hash_init_value;
    hashIncr(hash, pt.x());
    hashIncr(hash, pt.y());
    return hash;
  }
};

class PointEqual
{
public:
  bool operator()(const Point &pt1,
                  const Point &pt2) const
  {
    return pt1.x() == pt2.x()
      && pt1.y() == pt2.y();
  }
};

typedef int SteinerPt;
typedef Vector<SteinerPt> SteinerPtSeq;
typedef UnorderedMap<Point, PinSeq, PointHash, PointEqual> LocPinMap;

// Returns nullptr if net has less than 2 pins or any pin is not placed.
SteinerTree *
makeSteinerTree(const Pin *drvr_pin,
                bool find_left_rights,
                dbNetwork *network,
                Logger *logger,
                SteinerTreeBuilder *stt_builder);

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
  SteinerTree(const Pin *drvr_pin);
  ~SteinerTree();
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
  void report(Logger *logger,
              const Network *network);
  // Return the steiner pt connected to the driver pin.
  SteinerPt drvrPt(const Network *network) const;

  // "Accessors" for SteinerPts.
  const char *name(SteinerPt pt,
                   const Network *network);
  const PinSeq *pins(SteinerPt pt) const;
  Point location(SteinerPt pt) const;
  SteinerPt left(SteinerPt pt);
  SteinerPt right(SteinerPt pt);
  void findLeftRights(const Network *network,
                      Logger *logger);
  void setTree(stt::Tree tree,
               const dbNetwork *network);
  void setHasInputPort(bool input_port);
  stt::Tree &fluteTree() { return tree_; }

  static SteinerPt null_pt;

protected:
  void findLeftRights(SteinerPt from,
                      SteinerPt to,
                      SteinerPtSeq &adj1,
                      SteinerPtSeq &adj2,
                      SteinerPtSeq &adj3,
                      Logger *logger);
  void findLeftRights(SteinerPt from,
                      SteinerPt to,
                      SteinerPt adj,
                      SteinerPtSeq &adj1,
                      SteinerPtSeq &adj2,
                      SteinerPtSeq &adj3,
                      Logger *logger);
  void locAddPin(Point &loc,
                 Pin *pin);

  stt::Tree tree_;
  const Pin *drvr_pin_;
  int drvr_steiner_pt_;
  PinSeq pins_;
  // location -> pins
  LocPinMap loc_pin_map_;
  SteinerPtSeq left_;
  SteinerPtSeq right_;

  friend SteinerTree *makeSteinerTree(const Pin *drvr_pin,
                                      bool find_left_rights,
                                      dbNetwork *network,
                                      Logger *logger,
                                      SteinerTreeBuilder *stt_builder);
};

} // namespace
