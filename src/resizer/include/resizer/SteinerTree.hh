// Resizer, LEF/DEF gate resizer
// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include "Hash.hh"
#include "UnorderedMap.hh"
#include "opendb/geom.h"
#include "db_sta/dbNetwork.hh"

#define FLUTE_DTYPE int
#include "flute.h"

namespace sta {

using odb::Point;

class SteinerTree;

typedef int SteinerPt;
typedef Vector<SteinerPt> SteinerPtSeq;

// Returns nullptr if net has less than 2 pins.
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
  void setTree(Flute::Tree tree,
	       const dbNetwork *network);
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
  bool isPlaced(const dbNetwork *network) const;

  // "Accessors" for SteinerPts.
  const char *name(SteinerPt pt,
		   const Network *network);
  Pin *pin(SteinerPt pt) const;
  bool isLoad(SteinerPt pt,
	      const Network *network);
  Point location(SteinerPt pt) const;
  SteinerPt left(SteinerPt pt);
  SteinerPt right(SteinerPt pt);
  void findLeftRights(const Network *network);
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
  // location -> pin (any pin if there are multiple at the location).
  UnorderedMap<Point, Pin*, PointHash, PointEqual> loc_pin_map_;
  SteinerPtSeq left_;
  SteinerPtSeq right_;
};

} // namespace
