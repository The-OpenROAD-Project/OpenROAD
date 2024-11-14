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

#include "Polygon.hh"

#include "odb/dbShape.h"

namespace ant {

Polygon rectToPolygon(const odb::Rect& rect)
{
  std::vector<Point> points{{gtl::construct<Point>(rect.xMin(), rect.yMin()),
                             gtl::construct<Point>(rect.xMin(), rect.yMax()),
                             gtl::construct<Point>(rect.xMax(), rect.yMax()),
                             gtl::construct<Point>(rect.xMax(), rect.yMin())}};
  Polygon pol;
  gtl::set_points(pol, points.begin(), points.end());
  return pol;
}

// used to find the indeces of the elements which intersect with the element pol
std::vector<int> findNodesWithIntersection(const GraphNodes& graph_nodes,
                                           const Polygon& pol)
{
  // expand object by 1
  PolygonSet obj;
  obj += pol;
  obj += 1;
  Polygon& scaled_pol = obj[0];
  int index = 0;
  std::vector<int> ids;
  for (const auto& node : graph_nodes) {
    if (gtl::area(node->pol & scaled_pol) > 0) {
      ids.push_back(index);
    }
    index++;
  }
  return ids;
}

void wiresToPolygonSetMap(odb::dbWire* wires,
                          std::map<odb::dbTechLayer*, PolygonSet>& set_by_layer)
{
  odb::dbShape shape;
  odb::dbWireShapeItr shapes_it;
  std::vector<odb::dbShape> via_boxes;

  // Add information on polygon sets
  for (shapes_it.begin(wires); shapes_it.next(shape);) {
    odb::dbTechLayer* layer;

    // Get rect of the wire
    odb::Rect wire_rect = shape.getBox();

    if (shape.isVia()) {
      // Get three polygon upper_cut - via - lower_cut
      odb::dbShape::getViaBoxes(shape, via_boxes);
      for (const odb::dbShape& box : via_boxes) {
        layer = box.getTechLayer();
        odb::Rect via_rect = box.getBox();
        Polygon via_pol = rectToPolygon(via_rect);
        set_by_layer[layer] += via_pol;
      }
    } else {
      layer = shape.getTechLayer();
      // polygon set is used to join polygon on same layer with intersection
      Polygon wire_pol = rectToPolygon(wire_rect);
      set_by_layer[layer] += wire_pol;
    }
  }
}

void avoidPinIntersection(odb::dbNet* db_net,
                          std::map<odb::dbTechLayer*, PolygonSet>& set_by_layer)
{
  // iterate all instance pin
  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    odb::dbInst* inst = iterm->getInst();
    const odb::dbTransform transform = inst->getTransform();
    for (odb::dbMPin* mterm : mterm->getMPins()) {
      for (odb::dbBox* box : mterm->getGeometry()) {
        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }

        odb::Rect pin_rect = box->getBox();
        transform.apply(pin_rect);
        // convert rect -> polygon
        Polygon pin_pol = rectToPolygon(pin_rect);
        // Remove the area with intersection of the polygon set
        set_by_layer[tech_layer] -= pin_pol;
      }
    }
  }
}

}  // namespace ant