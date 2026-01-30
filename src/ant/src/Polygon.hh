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
  bool isVia;
  Polygon pol;
  std::vector<int> low_adj;
  std::set<PinType, PinTypeCmp> gates;
  GraphNode() = default;
  GraphNode(int id_, bool isVia_, const Polygon& pol_)
  {
    id = id_;
    isVia = isVia_;
    pol = pol_;
  }
};

Polygon rectToPolygon(const odb::Rect& rect);
std::vector<int> findNodesWithIntersection(const GraphNodes& graph_nodes,
                                           const Polygon& pol);
void wiresToPolygonSetMap(
    odb::dbWire* wires,
    std::map<odb::dbTechLayer*, PolygonSet>& set_by_layer);
void avoidPinIntersection(
    odb::dbNet* db_net,
    std::map<odb::dbTechLayer*, PolygonSet>& set_by_layer);

}  // namespace ant
