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

Search::~Search()
{
  if (block_ != nullptr) {
    removeOwner();  // unregister as a callback object
  }
}

void Search::inDbNetDestroy(odb::dbNet* net)
{
  clearShapes();
}

void Search::inDbInstDestroy(odb::dbInst* inst)
{
  if (inst->isPlaced()) {
    clearInsts();
  }
}

void Search::inDbInstSwapMasterAfter(odb::dbInst* inst)
{
  if (inst->isPlaced()) {
    clearInsts();
  }
}

void Search::inDbInstPlacementStatusBefore(odb::dbInst* inst,
                                           const odb::dbPlacementStatus& status)
{
  if (inst->getPlacementStatus().isPlaced() != status.isPlaced()) {
    clearInsts();
  }
}

void Search::inDbPostMoveInst(odb::dbInst* inst)
{
  if (inst->isPlaced()) {
    clearInsts();
  }
}

void Search::inDbBPinDestroy(odb::dbBPin* pin)
{
  clearShapes();
}

void Search::inDbFillCreate(odb::dbFill* fill)
{
  clearFills();
}

void Search::inDbWireCreate(odb::dbWire* wire)
{
  clearShapes();
}

void Search::inDbWireDestroy(odb::dbWire* wire)
{
  clearShapes();
}

void Search::inDbSWireCreate(odb::dbSWire* wire)
{
  clearShapes();
}

void Search::inDbSWireDestroy(odb::dbSWire* wire)
{
  clearShapes();
}

void Search::inDbSWireAddSBox(odb::dbSBox* box)
{
  clearShapes();
}

void Search::inDbSWireRemoveSBox(odb::dbSBox* box)
{
  clearShapes();
}

void Search::inDbBlockageCreate(odb::dbBlockage* blockage)
{
  clearBlockages();
}

void Search::inDbObstructionCreate(odb::dbObstruction* obs)
{
  clearObstructions();
}

void Search::inDbObstructionDestroy(odb::dbObstruction* obs)
{
  clearObstructions();
}

void Search::inDbBlockSetDieArea(odb::dbBlock* block)
{
  setBlock(block);
}

void Search::inDbRegionAddBox(odb::dbRegion*, odb::dbBox*)
{
  emit modified();
}

void Search::inDbRegionDestroy(odb::dbRegion* region)
{
  emit modified();
}

void Search::inDbRowCreate(odb::dbRow* row)
{
  clearRows();
}

void Search::inDbRowDestroy(odb::dbRow* row)
{
  clearRows();
}

void Search::setBlock(odb::dbBlock* block)
{
  if (block_ != block) {
    clear();

    if (block_ != nullptr) {
      removeOwner();
    }

    addOwner(block);  // register as a callback object
  }

  block_ = block;

  emit newBlock(block);
}

void Search::announceModified(bool& flag)
{
  const bool current_flag = flag;

  flag = false;

  if (flag != current_flag) {
    emit modified();
  }
}

void Search::clear()
{
  clearShapes();
  clearFills();
  clearInsts();
  clearBlockages();
  clearObstructions();
  clearRows();
}

void Search::clearShapes()
{
  announceModified(shapes_init_);
}

void Search::clearFills()
{
  announceModified(fills_init_);
}

void Search::clearInsts()
{
  announceModified(insts_init_);
}

void Search::clearBlockages()
{
  announceModified(blockages_init_);
}

void Search::clearObstructions()
{
  announceModified(obstructions_init_);
}

void Search::clearRows()
{
  announceModified(rows_init_);
}

void Search::updateShapes()
{
  box_shapes_.clear();
  polygon_shapes_.clear();

  for (odb::dbNet* net : block_->getNets()) {
    addNet(net);
    addSNet(net);
  }

  for (odb::dbBTerm* term : block_->getBTerms()) {
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
        odb::dbTechLayer* layer = box->getTechLayer();
        box_shapes_[layer].insert({bbox, term->getNet()});
      }
    }
  }

  shapes_init_ = true;
}

void Search::updateFills()
{
  fills_.clear();

  for (odb::dbFill* fill : block_->getFills()) {
    odb::Rect rect;
    fill->getRect(rect);
    Box box(Point(rect.xMin(), rect.yMin()), Point(rect.xMax(), rect.yMax()));
    fills_[fill->getTechLayer()].insert({box, fill});
  }

  fills_init_ = true;
}

void Search::updateInsts()
{
  insts_.clear();

  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isPlaced()) {
      addInst(inst);
    }
  }

  insts_init_ = true;
}

void Search::updateBlockages()
{
  blockages_.clear();

  for (odb::dbBlockage* blockage : block_->getBlockages()) {
    addBlockage(blockage);
  }

  blockages_init_ = true;
}

void Search::updateObstructions()
{
  obstructions_.clear();

  for (odb::dbObstruction* obs : block_->getObstructions()) {
    addObstruction(obs);
  }

  obstructions_init_ = true;
}

void Search::updateRows()
{
  rows_.clear();

  for (odb::dbRow* row : block_->getRows()) {
    addRow(row);
  }

  rows_init_ = true;
}

void Search::addVia(odb::dbNet* net, odb::dbShape* shape, int x, int y)
{
  if (shape->getType() == odb::dbShape::TECH_VIA) {
    odb::dbTechVia* via = shape->getTechVia();
    for (odb::dbBox* box : via->getBoxes()) {
      Point ll(x + box->xMin(), y + box->yMin());
      Point ur(x + box->xMax(), y + box->yMax());
      Box bbox(ll, ur);
      box_shapes_[box->getTechLayer()].insert({bbox, net});
    }
  } else {
    odb::dbVia* via = shape->getVia();
    for (odb::dbBox* box : via->getBoxes()) {
      Point ll(x + box->xMin(), y + box->yMin());
      Point ur(x + box->xMax(), y + box->yMax());
      Box bbox(ll, ur);
      box_shapes_[box->getTechLayer()].insert({bbox, net});
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
          box_shapes_[shape.getTechLayer()].insert({bbox, net});
        }
      } else {
        Box bbox(Point(box->xMin(), box->yMin()),
                 Point(box->xMax(), box->yMax()));
        std::vector<odb::Point> points;
        if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
          points = box->getOct().getPoints();
        } else {
          odb::Rect rect = box->getBox();
          points = rect.getPoints();
        }
        Polygon poly;
        for (const auto& point : points) {
          bg::append(poly.outer(), Point(point.getX(), point.getY()));
        }
        polygon_shapes_[box->getTechLayer()].insert({bbox, poly, net});
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
      box_shapes_[s.getTechLayer()].insert({box, net});
    }
  }
}

void Search::addInst(odb::dbInst* inst)
{
  odb::dbBox* bbox = inst->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  insts_.insert({box, inst});
}

void Search::addBlockage(odb::dbBlockage* blockage)
{
  odb::dbBox* bbox = blockage->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  blockages_.insert({box, blockage});
}

void Search::addObstruction(odb::dbObstruction* obs)
{
  odb::dbBox* bbox = obs->getBBox();
  Point ll(bbox->xMin(), bbox->yMin());
  Point ur(bbox->xMax(), bbox->yMax());
  Box box(ll, ur);
  obstructions_[bbox->getTechLayer()].insert({box, obs});
}

void Search::addRow(odb::dbRow* row)
{
  odb::Rect bbox = row->getBBox();
  Box box = convertRect(bbox);
  rows_.insert({box, row});
}

Search::Box Search::convertRect(const odb::Rect& box) const
{
  Point ll(box.xMin(), box.yMin());
  Point ur(box.xMax(), box.yMax());
  return Box(ll, ur);
}

template <typename T>
class Search::MinSizePredicate
{
 public:
  MinSizePredicate(int min_size) : min_size_(min_size) {}
  bool operator()(const std::tuple<Box, Polygon, T>& o) const
  {
    return operator()({std::get<0>(o), std::get<2>(o)});
  }

  bool operator()(const std::tuple<Box, T>& o) const
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
    return operator()({std::get<0>(o), std::get<2>(o)});
  }

  bool operator()(const std::tuple<Box, T>& o) const
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

Search::BoxRange Search::searchBoxShapes(odb::dbTechLayer* layer,
                                         int x_lo,
                                         int y_lo,
                                         int x_hi,
                                         int y_hi,
                                         int min_size)
{
  if (!shapes_init_) {
    updateShapes();
  }

  auto it = box_shapes_.find(layer);
  if (it == box_shapes_.end()) {
    return BoxRange();
  }

  auto& rtree = it->second;

  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_size > 0) {
    return BoxRange(rtree.qbegin(bgi::intersects(query)
                                 && bgi::satisfies(
                                     MinSizePredicate<odb::dbNet*>(min_size))),
                    rtree.qend());
  }

  return BoxRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::PolygonRange Search::searchPolygonShapes(odb::dbTechLayer* layer,
                                                 int x_lo,
                                                 int y_lo,
                                                 int x_hi,
                                                 int y_hi,
                                                 int min_size)
{
  if (!shapes_init_) {
    updateShapes();
  }

  auto it = polygon_shapes_.find(layer);
  if (it == polygon_shapes_.end()) {
    return PolygonRange();
  }

  auto& rtree = it->second;

  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_size > 0) {
    return PolygonRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbNet*>(min_size))),
        rtree.qend());
  }

  return PolygonRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::FillRange Search::searchFills(odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size)
{
  if (!fills_init_) {
    updateFills();
  }

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
  if (!insts_init_) {
    updateInsts();
  }

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
  if (!blockages_init_) {
    updateBlockages();
  }

  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_height > 0) {
    return BlockageRange(
        blockages_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(
                MinHeightPredicate<odb::dbBlockage*>(min_height))),
        blockages_.qend());
  }

  return BlockageRange(blockages_.qbegin(bgi::intersects(query)),
                       blockages_.qend());
}

Search::ObstructionRange Search::searchObstructions(odb::dbTechLayer* layer,
                                                    int x_lo,
                                                    int y_lo,
                                                    int x_hi,
                                                    int y_hi,
                                                    int min_size)
{
  if (!obstructions_init_) {
    updateObstructions();
  }

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

Search::RowRange Search::searchRows(int x_lo,
                                    int y_lo,
                                    int x_hi,
                                    int y_hi,
                                    int min_height)
{
  if (!rows_init_) {
    updateRows();
  }

  Box query(Point(x_lo, y_lo), Point(x_hi, y_hi));
  if (min_height > 0) {
    return RowRange(
        rows_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbRow*>(min_height))),
        rows_.qend());
  }

  return RowRange(rows_.qbegin(bgi::intersects(query)), rows_.qend());
}

}  // namespace gui
