/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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

#include <string>
#include "sta/Hash.hh"
#include "sta/UnorderedMap.hh"
#include "opendb/geom.h"
#include "db_sta/dbNetwork.hh"

#define FLUTE_DTYPE int
#include "flute.h"

namespace sta {

using odb::Point;

class SteinerTree;

typedef int SteinerPt;
typedef Vector<SteinerPt> SteinerPtSeq;

// Returns nullptr if net has less than 2 pins or any pin is not placed.
SteinerTree *
makeSteinerTree(const Net *net,
		bool find_left_rights,
		dbNetwork *network);

class PointHash
{
public:
  size_t operator()(const Point &pt) const
  {
    size_t hash = hash_init_value;
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

// Wrapper for Flute::Tree
class SteinerTree
{
public:
  SteinerTree();
  ~SteinerTree();
  PinSeq &pins() { return pins_; }
  int pinCount() const { return pins_.size(); }
  int branchCount() const;
  void branch(int index,
	      // Return values.
	      Point &pt1,
	      Pin *&pin1,
	      int &steiner_pt1,
	      Point &pt2,
	      Pin *&pin2,
	      int &steiner_pt2,
	      int &wire_length);
  void report(const Network *network);
  // Return a pin in the same location as the steiner pt if it exists.
  Pin *steinerPtAlias(SteinerPt pt);
  // Return the steiner pt connected to the driver pin.
  SteinerPt drvrPt(const Network *network) const;

  // "Accessors" for SteinerPts.
  const char *name(SteinerPt pt,
		   const Network *network);
  Pin *pin(SteinerPt pt) const;
  SteinerPt steinerPt(Pin *pin) const;
  bool isLoad(SteinerPt pt,
	      const Network *network);
  Point location(SteinerPt pt) const;
  SteinerPt left(SteinerPt pt);
  SteinerPt right(SteinerPt pt);
  void findLeftRights(const Network *network);
  void setTree(Flute::Tree tree,
	       const dbNetwork *network);
  void setHasInputPort(bool input_port);
  void writeSVG(const Network *network,
		const char *filename);
  static SteinerPt null_pt;

protected:
  void findLeftRights(SteinerPt from,
		      SteinerPt to,
		      SteinerPtSeq &adj1,
		      SteinerPtSeq &adj2,
		      SteinerPtSeq &adj3);
  void findLeftRights(SteinerPt from,
		      SteinerPt to,
		      SteinerPt adj,
		      SteinerPtSeq &adj1,
		      SteinerPtSeq &adj2,
		      SteinerPtSeq &adj3);
  void checkSteinerPt(SteinerPt pt) const;

  Flute::Tree tree_;
  PinSeq pins_;
  // Flute steiner pt index -> pin index.
  Vector<Pin*> steiner_pt_pin_map_;
  // pin -> steiner pt index
  UnorderedMap<Pin*, int> pin_steiner_pt_map_;
  // location -> pin (any pin if there are multiple at the location).
  UnorderedMap<Point, Pin*, PointHash, PointEqual> loc_pin_map_;
  SteinerPtSeq left_;
  SteinerPtSeq right_;
};

} // namespace
