// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// A net's resistor network, from its routed wire: nodes at junctions and
// terminals, and resistors of one or more wire shapes in series.

#pragma once

#include <unordered_map>
#include <vector>

#include "odb/geom.h"

namespace odb {
class dbNet;
}  // namespace odb

namespace pex {

class Resistance;

struct NetNode
{
  enum class Kind
  {
    kJunction,  // id is the dbWire junction id
    kITerm,     // id is the dbITerm id
    kBTerm,     // id is the dbBTerm id
  };
  Kind kind;
  int id;
  odb::Point point;  // where the wire reaches it
};

struct NetResistor
{
  int source;  // node indices, the source nearer the path's start
  int target;
  double resistance;  // ohms
};

// Where a wire shape lies in the network: on a resistor between the
// fractions t0 and t1 of its resistance from the source, the shape running
// from `start` to `end` (its path points); or, when its resistor would
// join a node to itself, on `node` alone (resistor -1).
struct ShapePlace
{
  int resistor = -1;
  int node = -1;
  double t0 = 0;
  double t1 = 0;
  odb::Point start;
  odb::Point end;
};

// A point of a shape in the network: on a resistor at the fraction t of
// its resistance from the source, or on a node (resistor -1).
struct NetPlace
{
  int resistor = -1;
  int node = -1;
  double t = 0;
};

struct Network
{
  std::vector<NetNode> nodes;
  // In path order: each path's resistors from its start.
  std::vector<NetResistor> resistors;
  std::unordered_map<int, ShapePlace> shapes;  // by dbWire shape id
  std::unordered_map<int, int> iterms;         // node by dbITerm id
  std::unordered_map<int, int> bterms;         // node by dbBTerm id
  // Resistors that would have joined a node to itself (a wire loop back
  // to where it started), whose shapes sit on that node instead.
  int loops = 0;

  // Where `point` of the wire shape `shape_id` lies: its fraction along
  // the shape from start to end, clamped to the shape, maps into the
  // shape's span of its resistor. Throws std::out_of_range for a shape
  // id the network does not have.
  NetPlace place(int shape_id, odb::Point point) const;
};

// The network of a net's dbWire, walking its paths with dbWirePathItr. A
// node is made where a path starts, at each terminal, at each junction a
// branch or a short leaves from, and where a path ends; shapes between
// nodes join in series while their resistance stays within `max_res`
// ohms (0: every shape is its own resistor). An empty network for a net
// without a wire. Reads ODB only, so nets may be built in parallel.
Network buildNetwork(odb::dbNet* net,
                     const Resistance& resistance,
                     double max_res);

}  // namespace pex
