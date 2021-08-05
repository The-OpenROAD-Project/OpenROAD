///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "search.h"

#include <tuple>
#include <utility>

#include "dbShape.h"

namespace gui {

// Build the rtree's for the block
void Search::init(odb::dbBlock* block)
{
  for (odb::dbNet* net : block->getNets()) {
    addNet(net);
    addSNet(net);
  }

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
        addInst(inst);
    }
  }

  for (odb::dbBTerm* term : block->getBTerms()) {
    for (odb::dbBPin* pin : term->getBPins()) {
      odb::dbPlacementStatus status = pin->getPlacementStatus();
      if (status == odb::dbPlacementStatus::NONE
          || status == odb::dbPlacementStatus::UNPLACED) {
        continue;
      }
      for (odb::dbBox* box : pin->getBoxes()) {
        if (!box) {
          continue;
        }
        Box bbox(Point(box->xMin(), box->yMin()),
                 Point(box->xMax(), box->yMax()));
        Polygon poly;
        bg::convert(bbox, poly);
        odb::dbTechLayer* layer = box->getTechLayer();
        shapes_[layer].insert(std::make_tuple(bbox, poly, term->getNet()));
      }
    }
  }

  for (odb::dbFill* fill : block->getFills()) {
    odb::Rect rect;
    fill->getRect(rect);
    Box box(Point(rect.xMin(), rect.yMin()), Point(rect.xMax(), rect.yMax()));
    Polygon poly;
    bg::convert(box, poly);
    fills_[fill->getTechLayer()].insert(std::make_tuple(box, poly, fill));
  }

  for (odb::dbBlockage* blockage : block->getBlockages()) {
    addBlockage(blockage);
  }

  for (odb::dbObstruction* obs : block->getObstructions()) {
    addObstruction(obs);
  }
}

void Search::addVia(odb::dbNet* net, odb::dbShape* shape, int x, int y)
{
  if (shape->getType() == odb::dbShape::TECH_VIA) {
    odb::dbTechVia* via = shape->getTechVia();
    for (odb::dbBox* box : via->getBoxes()) {
      Point ll(x + box->xMin(), y + box->yMin());
      Point ur(x + box->xMax(), y + box->yMax());
      Box bbox(ll, ur);
      Polygon poly;
      bg::convert(bbox, poly);
      shapes_[box->getTechLayer()].insert(std::make_tuple(bbox, poly, net));
    }
  } else {
    odb::dbVia* via = shape->getVia();
    for (odb::dbBox* box : via->getBoxes()) {
      Point ll(x + box->xMin(), y + box->yMin());
      Point ur(x + box->xMax(), y + box->yMax());
      Box bbox(ll, ur);
      Polygon poly;
      bg::convert(bbox, poly);
      shapes_[box->getTechLayer()].insert(std::make_tuple(bbox, poly, net));
    }
  }
}

void Search::addSNet(odb::dbNet* net)
{
  std::vector<odb::dbShape> shapes;
  for (odb::dbSWire* swire : net->getSWires()) {
    for (odb::dbSBox* box : swire->getWires()) {
      if (box->isVia()) {
        box->getViaBoxes(shapes);
        for (auto& shape : shapes) {
          Box bbox(Point(shape.xMin(), shape.yMin()),
                   Point(shape.xMax(), shape.yMax()));
          Polygon poly;
          bg::convert(bbox, poly);
          shapes_[shape.getTechLayer()].insert(
              std::make_tuple(bbox, poly, net));
        }
      } else {
        Box bbox(Point(box->xMin(), box->yMin()),
                 Point(box->xMax(), box->yMax()));
        Polygon poly;
        auto points = box->getGeomShape()->getPoints();
        for (auto point : points)
          bg::append(poly.outer(), Point(point.getX(), point.getY()));
        shapes_[box->getTechLayer()].insert(std::make_tuple(bbox, poly, net));
      }
    }
  }
}

void Search::addNet(odb::dbNet* net)
{
  odb::dbWire* wire = net->getWire();

  if (wire == NULL)
    return;

  odb::dbWireShapeItr itr;
  odb::dbShape s;

  for (itr.begin(wire); itr.next(s);) {
    if (s.isVia()) {
      addVia(net, &s, itr._prev_x, itr._prev_y);
    } else {
      Box box(Point(s.xMin(), s.yMin()), Point(s.xMax(), s.yMax()));
      Polygon poly;
      bg::convert(box, poly);
      shapes_[s.getTechLayer()].insert(std::make_tuple(box, poly, net));
    }
  }
}

void Search::addInst(odb::dbInst* inst)
{
  odb::dbBox* bbox = inst->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  Polygon poly;
  bg::convert(box, poly);
  insts_.insert(std::make_tuple(box, poly, inst));
}

void Search::addBlockage(odb::dbBlockage* blockage)
{
  odb::dbBox* bbox = blockage->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  Polygon poly;
  bg::convert(box, poly);
  blockages_.insert({box, poly, blockage});
}

void Search::addObstruction(odb::dbObstruction* obs)
{
  odb::dbBox* bbox = obs->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  Polygon poly;
  bg::convert(box, poly);
  obstructions_[bbox->getTechLayer()].insert({box, poly, obs});
}

void Search::clear()
{
  insts_.clear();
  shapes_.clear();
  blockages_.clear();
  obstructions_.clear();
}

template <typename T>
class Search::MinSizePredicate
{
 public:
  MinSizePredicate(int min_size) : min_size_(min_size) {}
  bool operator()(const std::tuple<Box, Polygon, T>& o) const
  {
    Box box = std::get<0>(o);
    const Point& ll = box.min_corner();
    const Point& ur = box.max_corner();
    int w = ur.x() - ll.x();
    int h = ur.y() - ll.y();
    return std::max(w, h) >= min_size_;
  }

 private:
  int min_size_;
};

template <typename T>
class Search::MinHeightPredicate
{
 public:
  MinHeightPredicate(int min_height) : min_height_(min_height) {}
  bool operator()(const std::tuple<Box, Polygon, T>& o) const
  {
    Box box = std::get<0>(o);
    const Point& ll = box.min_corner();
    const Point& ur = box.max_corner();
    int h = ur.y() - ll.y();
    return h >= min_height_;
  }

 private:
  int min_height_;
};

Search::ShapeRange Search::searchShapes(odb::dbTechLayer* layer,
                                        int x_lo,
                                        int y_lo,
                                        int x_hi,
                                        int y_hi,
                                        int min_size)
{
  auto it = shapes_.find(layer);
  if (it == shapes_.end()) {
    return ShapeRange();
  }

  auto& rtree = it->second;

  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_size > 0) {
    return ShapeRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbNet*>(min_size))),
        rtree.qend());
  }

  return ShapeRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::FillRange Search::searchFills(odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size)
{
  auto it = fills_.find(layer);
  if (it == fills_.end()) {
    return FillRange();
  }

  auto& rtree = it->second;
  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_size > 0) {
    return FillRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbFill*>(min_size))),
        rtree.qend());
  }

  return FillRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::InstRange Search::searchInsts(int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_height)
{
  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_height > 0) {
    return InstRange(
        insts_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbInst*>(min_height))),
        insts_.qend());
  }

  return InstRange(insts_.qbegin(bgi::intersects(query)), insts_.qend());
}

Search::BlockageRange Search::searchBlockages(int x_lo,
                                              int y_lo,
                                              int x_hi,
                                              int y_hi,
                                              int min_height)
{
  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_height > 0) {
    return BlockageRange(
        blockages_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbBlockage*>(min_height))),
            blockages_.qend());
  }

  return BlockageRange(blockages_.qbegin(bgi::intersects(query)), blockages_.qend());
}

Search::ObstructionRange Search::searchObstructions(odb::dbTechLayer* layer,
                                                    int x_lo,
                                                    int y_lo,
                                                    int x_hi,
                                                    int y_hi,
                                                    int min_size)
{
  auto it = obstructions_.find(layer);
  if (it == obstructions_.end()) {
    return ObstructionRange();
  }

  auto& rtree = it->second;
  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_size > 0) {
    return ObstructionRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbObstruction*>(min_size))),
        rtree.qend());
  }

  return ObstructionRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

}  // namespace gui
