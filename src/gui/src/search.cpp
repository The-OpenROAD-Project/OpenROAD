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

#include "odb/dbShape.h"

namespace gui {

Search::~Search()
{
  if (top_block_ != nullptr) {
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

void Search::inDbBPinCreate(odb::dbBPin* pin)
{
  clearShapes();
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
  setTopBlock(block);
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

void Search::inDbWirePostModify(odb::dbWire* wire)
{
  clearShapes();
}

void Search::setTopBlock(odb::dbBlock* block)
{
  if (top_block_ != block) {
    clear();

    if (top_block_ != nullptr) {
      removeOwner();
    }

    addOwner(block);  // register as a callback object

    // Pre-populate children so we don't have to lock access to
    // child_block_data_ later
    if (block) {
      for (auto child : block->getChildren()) {
        child_block_data_[child];
      }
    }
  }

  top_block_ = block;

  emit newBlock(block);
}

void Search::announceModified(std::atomic_bool& flag)
{
  const bool prev_flag = flag.exchange(false);

  if (prev_flag) {
    emit modified();
  }
}

void Search::clear()
{
  child_block_data_.clear();
  clearShapes();
  clearFills();
  clearInsts();
  clearBlockages();
  clearObstructions();
  clearRows();
}

void Search::clearShapes()
{
  announceModified(top_block_data_.shapes_init_);
}

void Search::clearFills()
{
  announceModified(top_block_data_.fills_init_);
}

void Search::clearInsts()
{
  announceModified(top_block_data_.insts_init_);
}

void Search::clearBlockages()
{
  announceModified(top_block_data_.blockages_init_);
}

void Search::clearObstructions()
{
  announceModified(top_block_data_.obstructions_init_);
}

void Search::clearRows()
{
  announceModified(top_block_data_.rows_init_);
}

Search::BlockData& Search::getData(odb::dbBlock* block)
{
  return block == top_block_ ? top_block_data_ : child_block_data_[block];
}

void Search::updateShapes(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.shapes_init_mutex_);
  if (data.shapes_init_) {
    return;  // already done by another thread
  }

  data.box_shapes_.clear();
  data.snet_via_shapes_.clear();
  data.snet_shapes_.clear();

  LayerMap<std::vector<SNetValue<odb::dbNet*>>> snet_shapes;
  LayerMap<std::vector<SNetDBoxValue<odb::dbNet*>>> snet_net_via_shapes;
  for (odb::dbNet* net : block->getNets()) {
    addSNet(net, snet_shapes, snet_net_via_shapes);
  }
  for (const auto& [layer, layer_shapes] : snet_shapes) {
    data.snet_shapes_[layer] = RtreeSNetShapes<odb::dbNet*>(
        layer_shapes.begin(), layer_shapes.end());
  }
  snet_shapes.clear();
  for (const auto& [layer, layer_shapes] : snet_net_via_shapes) {
    data.snet_via_shapes_[layer] = RtreeSNetDBoxShapes<odb::dbNet*>(
        layer_shapes.begin(), layer_shapes.end());
  }
  snet_net_via_shapes.clear();

  LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>> net_shapes;
  for (odb::dbNet* net : block->getNets()) {
    addNet(net, net_shapes);
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
        odb::dbTechLayer* layer = box->getTechLayer();
        net_shapes[layer].emplace_back(box->getBox(), false, term->getNet());
      }
    }
  }
  for (const auto& [layer, layer_shapes] : net_shapes) {
    data.box_shapes_[layer] = RtreeRoutingShapes<odb::dbNet*>(
        layer_shapes.begin(), layer_shapes.end());
  }

  data.shapes_init_ = true;
}

void Search::updateFills(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.fills_init_mutex_);
  if (data.fills_init_) {
    return;  // already done by another thread
  }

  data.fills_.clear();

  LayerMap<std::vector<odb::dbFill*>> fills;
  for (odb::dbFill* fill : block->getFills()) {
    fills[fill->getTechLayer()].push_back(fill);
  }
  for (const auto& [layer, layer_fill] : fills) {
    data.fills_[layer] = RtreeFill(layer_fill.begin(), layer_fill.end());
  }

  data.fills_init_ = true;
}

void Search::updateInsts(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.insts_init_mutex_);
  if (data.insts_init_) {
    return;  // already done by another thread
  }

  data.insts_.clear();

  std::vector<odb::dbInst*> insts;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
      insts.push_back(inst);
    }
  }
  data.insts_ = RtreeDBox<odb::dbInst*>(insts.begin(), insts.end());

  data.insts_init_ = true;
}

void Search::updateBlockages(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.blockages_init_mutex_);
  if (data.blockages_init_) {
    return;  // already done by another thread
  }

  data.blockages_.clear();

  std::vector<odb::dbBlockage*> blockages;
  for (odb::dbBlockage* blockage : block->getBlockages()) {
    blockages.push_back(blockage);
  }
  data.blockages_
      = RtreeDBox<odb::dbBlockage*>(blockages.begin(), blockages.end());

  data.blockages_init_ = true;
}

void Search::updateObstructions(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.obstructions_init_mutex_);
  if (data.obstructions_init_) {
    return;  // already done by another thread
  }

  data.obstructions_.clear();

  LayerMap<std::vector<odb::dbObstruction*>> obstructions;
  for (odb::dbObstruction* obs : block->getObstructions()) {
    odb::dbBox* bbox = obs->getBBox();
    obstructions[bbox->getTechLayer()].push_back(obs);
  }
  for (const auto& [layer, layer_obs] : obstructions) {
    data.obstructions_[layer]
        = RtreeDBox<odb::dbObstruction*>(layer_obs.begin(), layer_obs.end());
  }

  data.obstructions_init_ = true;
}

void Search::updateRows(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.rows_init_mutex_);
  if (data.rows_init_) {
    return;  // already done by another thread
  }

  data.rows_.clear();

  std::vector<RectValue<odb::dbRow*>> rows;
  for (odb::dbRow* row : block->getRows()) {
    rows.emplace_back(row->getBBox(), row);
  }
  data.rows_ = RtreeRect<odb::dbRow*>(rows.begin(), rows.end());

  data.rows_init_ = true;
}

void Search::addVia(
    odb::dbNet* net,
    odb::dbShape* shape,
    int x,
    int y,
    LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>>& tree_shapes)
{
  if (shape->getType() == odb::dbShape::TECH_VIA) {
    odb::dbTechVia* via = shape->getTechVia();
    for (odb::dbBox* box : via->getBoxes()) {
      odb::Rect bbox = box->getBox();
      bbox.moveDelta(x, y);
      tree_shapes[box->getTechLayer()].emplace_back(bbox, true, net);
    }
  } else {
    odb::dbVia* via = shape->getVia();
    for (odb::dbBox* box : via->getBoxes()) {
      odb::Rect bbox = box->getBox();
      bbox.moveDelta(x, y);
      tree_shapes[box->getTechLayer()].emplace_back(bbox, true, net);
    }
  }
}

void Search::addSNet(
    odb::dbNet* net,
    LayerMap<std::vector<SNetValue<odb::dbNet*>>>& net_shapes,
    LayerMap<std::vector<SNetDBoxValue<odb::dbNet*>>>& via_shapes)
{
  for (odb::dbSWire* swire : net->getSWires()) {
    for (odb::dbSBox* box : swire->getWires()) {
      if (box->isVia()) {
        odb::dbTechLayer* layer;
        if (auto via = box->getTechVia()) {
          layer = via->getBottomLayer()->getUpperLayer();
        } else {
          auto block_via = box->getBlockVia();
          layer = block_via->getBottomLayer()->getUpperLayer();
        }
        via_shapes[layer].emplace_back(box, net);
      } else {
        if (box->getDirection() == odb::dbSBox::OCTILINEAR) {
          net_shapes[box->getTechLayer()].emplace_back(box, box->getOct(), net);
        } else {
          net_shapes[box->getTechLayer()].emplace_back(box, box->getBox(), net);
        }
      }
    }
  }
}

void Search::addNet(
    odb::dbNet* net,
    LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>>& tree_shapes)
{
  odb::dbWire* wire = net->getWire();

  if (wire == nullptr) {
    return;
  }

  odb::dbWireShapeItr itr;
  odb::dbShape s;

  for (itr.begin(wire); itr.next(s);) {
    if (s.isVia()) {
      addVia(net, &s, itr._prev_x, itr._prev_y, tree_shapes);
    } else {
      tree_shapes[s.getTechLayer()].emplace_back(s.getBox(), false, net);
    }
  }
}

template <typename T>
class Search::MinSizePredicate
{
 public:
  MinSizePredicate(int min_size) : min_size_(min_size) {}
  bool operator()(const SNetValue<T>& o) const
  {
    return checkBox(std::get<0>(o)->getBox());
  }

  bool operator()(const RectValue<T>& o) const { return checkBox(o.first); }

  bool operator()(const RouteBoxValue<T>& o) const
  {
    return checkBox(std::get<0>(o));
  }

  bool operator()(const SNetDBoxValue<T>& o) const
  {
    return checkBox(o.first->getBox());
  }

  bool operator()(odb::dbObstruction* o) const
  {
    return checkBox(o->getBBox()->getBox());
  }

  bool operator()(odb::dbFill* o) const
  {
    odb::Rect fill;
    o->getRect(fill);
    return checkBox(fill);
  }

  bool checkBox(const odb::Rect& box) const
  {
    return box.maxDXDY() >= min_size_;
  }

 private:
  int min_size_;
};

template <typename T>
class Search::PolygonIntersectPredicate
{
 public:
  PolygonIntersectPredicate(const odb::Rect& region) : region_(region) {}
  bool operator()(const SNetValue<T>& o) const
  {
    return checkPolygon(std::get<1>(o));
  }

  bool operator()(const RectValue<T>& o) const { return checkPolygon(o.first); }

  bool operator()(const RouteBoxValue<T>& o) const
  {
    return checkPolygon(std::get<0>(o));
  }

  bool checkPolygon(const odb::Polygon& poly) const
  {
    return boost::geometry::intersects(region_, poly);
  }

 private:
  odb::Rect region_;
};

template <typename T>
class Search::MinHeightPredicate
{
 public:
  MinHeightPredicate(int min_height) : min_height_(min_height) {}
  bool operator()(const SNetValue<T>& o) const
  {
    return checkBox(std::get<0>(o));
  }

  bool operator()(const RouteBoxValue<T>& o) const
  {
    return checkBox(std::get<0>(o));
  }

  bool operator()(const RectValue<T>& o) const { return checkBox(o.first); }

  bool operator()(odb::dbInst* o) const
  {
    return checkBox(o->getBBox()->getBox());
  }

  bool operator()(odb::dbBlockage* o) const
  {
    return checkBox(o->getBBox()->getBox());
  }

  bool checkBox(const odb::Rect& box) const { return box.dy() >= min_height_; }

 private:
  int min_height_;
};

Search::RoutingRange Search::searchBoxShapes(odb::dbBlock* block,
                                             odb::dbTechLayer* layer,
                                             int x_lo,
                                             int y_lo,
                                             int x_hi,
                                             int y_hi,
                                             int min_size)
{
  BlockData& data = getData(block);
  if (!data.shapes_init_) {
    updateShapes(block);
  }

  auto it = data.box_shapes_.find(layer);
  if (it == data.box_shapes_.end()) {
    return RoutingRange();
  }

  auto& rtree = it->second;

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return RoutingRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbNet*>(min_size))),
        rtree.qend());
  }

  return RoutingRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::SNetSBoxRange Search::searchSNetViaShapes(odb::dbBlock* block,
                                                  odb::dbTechLayer* layer,
                                                  int x_lo,
                                                  int y_lo,
                                                  int x_hi,
                                                  int y_hi,
                                                  int min_size)
{
  BlockData& data = getData(block);
  if (!data.shapes_init_) {
    updateShapes(block);
  }

  auto it = data.snet_via_shapes_.find(layer);
  if (it == data.snet_via_shapes_.end()) {
    return SNetSBoxRange();
  }

  auto& rtree = it->second;

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return SNetSBoxRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbNet*>(min_size))),
        rtree.qend());
  }

  return SNetSBoxRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::SNetShapeRange Search::searchSNetShapes(odb::dbBlock* block,
                                                odb::dbTechLayer* layer,
                                                int x_lo,
                                                int y_lo,
                                                int x_hi,
                                                int y_hi,
                                                int min_size)
{
  BlockData& data = getData(block);
  if (!data.shapes_init_) {
    updateShapes(block);
  }

  auto it = data.snet_shapes_.find(layer);
  if (it == data.snet_shapes_.end()) {
    return SNetShapeRange();
  }

  auto& rtree = it->second;

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return SNetShapeRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbNet*>(min_size))
            && bgi::satisfies(PolygonIntersectPredicate<odb::dbNet*>(query))),
        rtree.qend());
  }

  return SNetShapeRange(
      rtree.qbegin(
          bgi::intersects(query)
          && bgi::satisfies(PolygonIntersectPredicate<odb::dbNet*>(query))),
      rtree.qend());
}

Search::FillRange Search::searchFills(odb::dbBlock* block,
                                      odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size)
{
  BlockData& data = getData(block);
  if (!data.fills_init_) {
    updateFills(block);
  }

  auto it = data.fills_.find(layer);
  if (it == data.fills_.end()) {
    return FillRange();
  }

  auto& rtree = it->second;
  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return FillRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbFill*>(min_size))),
        rtree.qend());
  }

  return FillRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::InstRange Search::searchInsts(odb::dbBlock* block,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_height)
{
  BlockData& data = getData(block);
  if (!data.insts_init_) {
    updateInsts(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return InstRange(
        data.insts_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbInst*>(min_height))),
        data.insts_.qend());
  }

  return InstRange(data.insts_.qbegin(bgi::intersects(query)),
                   data.insts_.qend());
}

Search::BlockageRange Search::searchBlockages(odb::dbBlock* block,
                                              int x_lo,
                                              int y_lo,
                                              int x_hi,
                                              int y_hi,
                                              int min_height)
{
  BlockData& data = getData(block);
  if (!data.blockages_init_) {
    updateBlockages(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return BlockageRange(
        data.blockages_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(
                MinHeightPredicate<odb::dbBlockage*>(min_height))),
        data.blockages_.qend());
  }

  return BlockageRange(data.blockages_.qbegin(bgi::intersects(query)),
                       data.blockages_.qend());
}

Search::ObstructionRange Search::searchObstructions(odb::dbBlock* block,
                                                    odb::dbTechLayer* layer,
                                                    int x_lo,
                                                    int y_lo,
                                                    int x_hi,
                                                    int y_hi,
                                                    int min_size)
{
  BlockData& data = getData(block);
  if (!data.obstructions_init_) {
    updateObstructions(block);
  }

  auto it = data.obstructions_.find(layer);
  if (it == data.obstructions_.end()) {
    return ObstructionRange();
  }

  auto& rtree = it->second;
  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return ObstructionRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbObstruction*>(min_size))),
        rtree.qend());
  }

  return ObstructionRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

Search::RowRange Search::searchRows(odb::dbBlock* block,
                                    int x_lo,
                                    int y_lo,
                                    int x_hi,
                                    int y_hi,
                                    int min_height)
{
  BlockData& data = getData(block);
  if (!data.rows_init_) {
    updateRows(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return RowRange(
        data.rows_.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbRow*>(min_height))),
        data.rows_.qend());
  }

  return RowRange(data.rows_.qbegin(bgi::intersects(query)), data.rows_.qend());
}

}  // namespace gui
