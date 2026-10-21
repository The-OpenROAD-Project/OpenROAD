// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "Polygon.hh"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "AntennaCheckerImpl.hh"
#include "boost/polygon/polygon.hpp"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

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
  using gtl::operators::operator+=;
  using gtl::operators::operator&;

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

void wiresToPolygonSetMap(
    odb::dbWire* wires,
    odb::PtrMap<odb::dbTechLayer, PolygonSet>& set_by_layer)
{
  using gtl::operators::operator+=;

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

void addPinShapes(odb::dbNet* db_net,
                  odb::PtrMap<odb::dbTechLayer, PolygonSet>& set_by_layer)
{
  using gtl::operators::operator+=;

  for (odb::dbITerm* iterm : db_net->getITerms()) {
    odb::dbMTerm* mterm = iterm->getMTerm();
    const bool is_block = mterm->getMaster()->getType().isBlock();
    std::vector<std::pair<double, odb::dbTechLayer*>> partial_areas;
    if (is_block) {
      mterm->getPartialMetalArea(partial_areas);
    }
    auto hasPartialArea = [&](const odb::dbTechLayer* layer) {
      return std::ranges::any_of(partial_areas, [&](const auto& entry) {
        return entry.second == nullptr || entry.second == layer;
      });
    };
    const odb::dbTransform transform = iterm->getInst()->getTransform();
    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::dbTechLayer* tech_layer = box->getTechLayer();
        if (tech_layer == nullptr
            || tech_layer->getType() != odb::dbTechLayerType::ROUTING) {
          continue;
        }
        if (is_block && !hasPartialArea(tech_layer)) {
          continue;
        }
        odb::Rect pin_rect = box->getBox();
        transform.apply(pin_rect);
        set_by_layer[tech_layer] += rectToPolygon(pin_rect);
      }
    }
  }
}

}  // namespace ant
