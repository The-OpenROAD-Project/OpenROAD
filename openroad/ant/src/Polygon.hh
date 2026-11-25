// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <vector>

#include "AntennaCheckerImpl.hh"
#include "PinType.hh"
#include "ant/AntennaChecker.hh"
#include "boost/functional/hash.hpp"
#include "boost/polygon/polygon.hpp"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace ant {

namespace gtl = boost::polygon;

using Polygon = gtl::polygon_90_data<int>;
using PolygonSet = std::vector<Polygon>;
using Point = gtl::polygon_traits<Polygon>::point_type;

struct GraphNode
{
  int id;
  bool is_via;
  Polygon pol;
  std::vector<int> low_adj;
  std::set<PinType, PinTypeCmp> gates;
  GraphNode() = default;
  GraphNode(int id, bool is_via, const Polygon& pol)
      : id(id), is_via(is_via), pol(pol)
  {
  }
};

Polygon rectToPolygon(const odb::Rect& rect);
std::vector<int> findNodesWithIntersection(const GraphNodes& graph_nodes,
                                           const Polygon& pol);
void wiresToPolygonSetMap(
    odb::dbWire* wires,
    odb::PtrMap<odb::dbTechLayer, PolygonSet>& set_by_layer);
// Adds the shapes of db_net's pins to its routed metal.  A pin's own metal
// is antenna area: LEF gives a pin's metal area as its shape plus its
// ANTENNAPARTIALMETALAREA (LEF/DEF 5.8, p. 144), and the area values in LEF
// leave the pin out because the router counts it (Appendix C, p. 1079).
// A block pin without a partial area on a layer is the exception: its
// shape there is often an abstract of the block, not its metal, so only the
// routed metal over it counts.
void addPinShapes(odb::dbNet* db_net,
                  odb::PtrMap<odb::dbTechLayer, PolygonSet>& set_by_layer);

}  // namespace ant
