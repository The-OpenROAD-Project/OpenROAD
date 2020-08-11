///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include <utility>

#include "dbShape.h"
#include "search.h"

namespace gui {

// Build the rtree's for the block
void Search::init(odb::dbBlock* block)
{
  for (odb::dbNet* net : block->getNets()) {
    addNet(net);
    addSNet(net);
  }

  for (odb::dbInst* inst : block->getInsts()) {
    odb::dbPlacementStatus status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::NONE
        || status == odb::dbPlacementStatus::UNPLACED) {
      continue;
    }
    addInst(inst);
  }
}

void Search::addVia(odb::dbShape* shape, int shapeId, int x, int y)
{
  if (shape->getType() == odb::dbShape::TECH_VIA) {
    odb::dbTechVia* via = shape->getTechVia();
    for (odb::dbBox* box : via->getBoxes()) {
      point_t ll(x + box->xMin(), y + box->yMin());
      point_t ur(x + box->xMax(), y + box->yMax());
      box_t   bbox(ll, ur);
      shapes_[box->getTechLayer()].insert(std::make_pair(bbox, shapeId));
    }
  } else {
    odb::dbVia* via = shape->getVia();
    for (odb::dbBox* box : via->getBoxes()) {
      point_t ll(x + box->xMin(), y + box->yMin());
      point_t ur(x + box->xMax(), y + box->yMax());
      box_t   bbox(ll, ur);
      shapes_[box->getTechLayer()].insert(std::make_pair(bbox, shapeId));
    }
  }
}

void Search::addSNet(odb::dbNet* net)
{
  odb::dbSet<odb::dbSWire> swires = net->getSWires();

  for (auto itr = swires.begin(); itr != swires.end(); ++itr) {
    odb::dbSWire*                     swire = *itr;
    odb::dbSet<odb::dbSBox>           wires = swire->getWires();
    odb::dbSet<odb::dbSBox>::iterator box_itr;

    for (box_itr = wires.begin(); box_itr != wires.end(); ++box_itr) {
      odb::dbSBox* box     = *box_itr;
      uint         shapeId = box->getId();
      if (box->isVia()) {
        continue;
      }

      box_t bbox(point_t(box->xMin(), box->yMin()),
                 point_t(box->xMax(), box->yMax()));
      shapes_[box->getTechLayer()].insert(std::make_pair(bbox, shapeId));
    }
  }
}

void Search::addNet(odb::dbNet* net)
{
  odb::dbWire* wire = net->getWire();

  if (wire == NULL)
    return;

  odb::dbWireShapeItr itr;
  odb::dbShape        s;

  for (itr.begin(wire); itr.next(s);) {
    int shapeId = itr.getShapeId();
    if (s.isVia()) {
      addVia(&s, shapeId, itr._prev_x, itr._prev_y);
    } else {
      box_t box(point_t(s.xMin(), s.yMin()), point_t(s.xMax(), s.yMax()));
      shapes_[s.getTechLayer()].insert(std::make_pair(box, shapeId));
    }
  }
}

void Search::addInst(odb::dbInst* inst)
{
  odb::dbBox* bbox = inst->getBBox();
  point_t     ll(bbox->xMin(), bbox->yMin());
  point_t     ur(bbox->xMax(), bbox->yMax());
  box_t       box(ll, ur);
  insts_.insert(std::make_pair(box, inst));
}

template <typename T>
class Search::MinSizePredicate
{
 public:
  MinSizePredicate(int min_size) : min_size_(min_size) {}
  bool operator()(const std::pair<box_t, T>& o) const
  {
    const box_t&   box = o.first;
    const point_t& ll  = box.min_corner();
    const point_t& ur  = box.max_corner();
    int            w   = ur.x() - ll.x();
    int            h   = ur.y() - ll.y();
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
  bool operator()(const std::pair<box_t, T>& o) const
  {
    const box_t&   box = o.first;
    const point_t& ll  = box.min_corner();
    const point_t& ur  = box.max_corner();
    int            h   = ur.y() - ll.y();
    return h >= min_height_;
  }

 private:
  int min_height_;
};

Search::ShapeRange Search::search_shapes(odb::dbTechLayer* layer,
                                         int               xLo,
                                         int               yLo,
                                         int               xHi,
                                         int               yHi,
                                         int               minSize)
{
  auto it = shapes_.find(layer);
  if (it == shapes_.end()) {
    return ShapeRange();
  }

  auto& rtree = it->second;
  box_t query(point_t(xLo, yLo), point_t(xHi, yHi));
  if (minSize > 0) {
    return ShapeRange(
        rtree.qbegin(bgi::intersects(query)
                     && bgi::satisfies(MinSizePredicate<int>(minSize))),
        rtree.qend());
  }

  return ShapeRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::InstRange Search::search_insts(int xLo,
                                       int yLo,
                                       int xHi,
                                       int yHi,
                                       int minHeight)
{
  box_t query(point_t(xLo, yLo), point_t(xHi, yHi));
  if (minHeight > 0) {
    return InstRange(
        insts_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbInst*>(minHeight))),
        insts_.qend());
  }

  return InstRange(insts_.qbegin(bgi::intersects(query)), insts_.qend());
}

}  // namespace gui
