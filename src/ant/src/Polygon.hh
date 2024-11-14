// BSD 3-Clause License
//
// Copyright (c) 2020, MICL, DD-Lab, University of Michigan
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

#pragma once

#include <boost/functional/hash.hpp>
#include <boost/polygon/polygon.hpp>

#include "PinType.hh"
#include "ant/AntennaChecker.hh"

namespace ant {

namespace gtl = boost::polygon;
using namespace gtl::operators;

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