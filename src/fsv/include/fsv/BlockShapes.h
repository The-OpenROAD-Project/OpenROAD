// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The conductor shapes of a block as boxes on routing and cut layers, with
// where each came from. The field solver reads a net's window from them,
// and the extractor reads the whole block, so both see the same metal.

#pragma once

#include <vector>

#include "fsv/Geometry.h"
#include "odb/geom.h"

namespace odb {
class dbBlock;
class dbBTerm;
class dbITerm;
class dbNet;
class dbTechLayer;
}  // namespace odb

namespace fsv {

// A conductor shape in database units.
struct BlockShape
{
  enum class Source
  {
    kWire,            // a segment or via of the net's dbWire
    kSpecialWire,     // a box or via of a special wire
    kITermPin,        // a box of an instance pin on the net
    kBTermPin,        // a box of a block pin on the net
    kObstruction,     // a cell obstruction (no net)
    kUnconnectedPin,  // an instance pin on no net
    kFill,            // a fill shape (no net)
  };

  odb::dbTechLayer* layer;
  odb::Rect rect;
  Source source;
  // kWire: the dbWire shape id (dbWireShapeItr::getShapeId); the boxes of
  // one via share it. -1 otherwise.
  int shape_id = -1;
  odb::dbITerm* iterm = nullptr;  // kITermPin and kUnconnectedPin
  odb::dbBTerm* bterm = nullptr;  // kBTermPin
};

// Every shape of a net: its routed wires and vias, special wires, and the
// pins of its instance and block terminals, in that order. A via expands to
// the boxes of its layers. Throws std::invalid_argument for a non-Manhattan
// special wire and for an unplaced instance or block pin, whose shapes would
// sit at the origin.
std::vector<BlockShape> netShapes(odb::dbNet* net);

// The shapes of no net: cell obstructions, instance pins on no net, and
// fill. Throws std::invalid_argument for an unplaced instance.
std::vector<BlockShape> unconnectedShapes(odb::dbBlock* block);

// A shape of no net, in µm, which joins the grounded net once trimmed
// clear of every net's shapes.
struct Obstruction
{
  Box box;
  double spacing;    // the layer's minimum spacing (or width)
  double min_width;  // pieces narrower than this are not metal
};

// The obstruction of a shape of no net, given as `box` in µm (its layer's z
// extent, possibly clipped), with its layer's spacing and width.
Obstruction obstruction(const BlockShape& shape, const Box& box, double dbu);

// Obstructions trimmed clear of net shapes: a LEF obstruction is often a
// routing blockage over a whole macro, its own pins included. Each one
// loses, on its layer (the same z extent), every net box grown by the
// layer's spacing, which is as close as real metal could come; leftovers
// narrower than the layer's width are dropped. Counts the obstructions
// that changed in `trimmed`.
std::vector<Box> trimObstructions(const std::vector<Box>& net_boxes,
                                  const std::vector<Obstruction>& obstructions,
                                  int& trimmed);

}  // namespace fsv
