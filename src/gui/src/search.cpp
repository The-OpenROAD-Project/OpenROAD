// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "search.h"

#include <atomic>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "boost/geometry/geometry.hpp"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace gui {

Search::~Search()
{
  if (top_chip_ != nullptr) {
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

void Search::inDbBPinAddBox(odb::dbBox* box)
{
  clearShapes();
  clearBPins();
}

void Search::inDbBPinRemoveBox(odb::dbBox* box)
{
  clearShapes();
  clearBPins();
}

void Search::inDbBPinDestroy(odb::dbBPin* pin)
{
  clearShapes();
  clearBPins();
}

void Search::inDbBPinPlacementStatusBefore(odb::dbBPin* pin,
                                           const odb::dbPlacementStatus& status)
{
  clearShapes();
  clearBPins();
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

void Search::inDbBlockageDestroy(odb::dbBlockage* blockage)
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
  setTopChip(block->getChip());
}

void Search::inDbBlockSetCoreArea(odb::dbBlock* block)
{
  emit modified();
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

void Search::setTopChip(odb::dbChip* chip)
{
  odb::dbBlock* block = chip->getBlock();
  if (top_chip_ != chip) {
    clear();

    if (top_chip_ != nullptr) {
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

  top_chip_ = chip;

  emit newChip(chip);
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
  clearBPins();
}

void Search::clearShapes()
{
  announceModified(top_block_data_.shapes_init);
}

void Search::clearFills()
{
  announceModified(top_block_data_.fills_init);
}

void Search::clearInsts()
{
  announceModified(top_block_data_.insts_init);
}

void Search::clearBlockages()
{
  announceModified(top_block_data_.blockages_init);
}

void Search::clearObstructions()
{
  announceModified(top_block_data_.obstructions_init);
}

void Search::clearRows()
{
  announceModified(top_block_data_.rows_init);
}

void Search::clearBPins()
{
  announceModified(top_block_data_.bpins_init);
}

Search::BlockData& Search::getData(odb::dbBlock* block)
{
  return block->getChip() == top_chip_ ? top_block_data_
                                       : child_block_data_[block];
}

void Search::updateShapes(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.shapes_init_mutex);
  if (data.shapes_init) {
    return;  // already done by another thread
  }

  data.box_shapes.clear();
  data.snet_via_shapes.clear();
  data.snet_shapes.clear();

  LayerMap<std::vector<SNetValue<odb::dbNet*>>> snet_shapes;
  LayerMap<std::vector<SNetDBoxValue<odb::dbNet*>>> snet_net_via_shapes;
  for (odb::dbNet* net : block->getNets()) {
    addSNet(net, snet_shapes, snet_net_via_shapes);
  }
  for (const auto& [layer, layer_shapes] : snet_shapes) {
    data.snet_shapes[layer] = RtreeSNetShapes<odb::dbNet*>(layer_shapes.begin(),
                                                           layer_shapes.end());
  }
  snet_shapes.clear();
  for (const auto& [layer, layer_shapes] : snet_net_via_shapes) {
    data.snet_via_shapes[layer] = RtreeSNetDBoxShapes<odb::dbNet*>(
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
        net_shapes[layer].emplace_back(box->getBox(), BTERM, term->getNet());
      }
    }
  }
  for (const auto& [layer, layer_shapes] : net_shapes) {
    data.box_shapes[layer] = RtreeRoutingShapes<odb::dbNet*>(
        layer_shapes.begin(), layer_shapes.end());
  }

  data.shapes_init = true;
}

void Search::updateBPins(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.bpins_init_mutex);
  if (data.bpins_init) {
    return;  // already done by another thread
  }

  data.bpins.clear();

  LayerMap<std::vector<BoxValue<odb::dbBPin*>>> shapes;

  for (odb::dbBTerm* term : block->getBTerms()) {
    for (odb::dbBPin* pin : term->getBPins()) {
      odb::dbPlacementStatus status = pin->getPlacementStatus();
      if (!status.isPlaced()) {
        continue;
      }
      for (odb::dbBox* box : pin->getBoxes()) {
        if (!box) {
          continue;
        }
        odb::dbTechLayer* layer = box->getTechLayer();
        shapes[layer].emplace_back(box, pin);
      }
    }
  }
  for (const auto& [layer, layer_shapes] : shapes) {
    data.bpins[layer]
        = RtreeBox<odb::dbBPin*>(layer_shapes.begin(), layer_shapes.end());
  }

  data.bpins_init = true;
}

void Search::updateFills(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.fills_init_mutex);
  if (data.fills_init) {
    return;  // already done by another thread
  }

  data.fills.clear();

  LayerMap<std::vector<odb::dbFill*>> fills;
  for (odb::dbFill* fill : block->getFills()) {
    fills[fill->getTechLayer()].push_back(fill);
  }
  for (const auto& [layer, layer_fill] : fills) {
    data.fills[layer] = RtreeFill(layer_fill.begin(), layer_fill.end());
  }

  data.fills_init = true;
}

void Search::updateInsts(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.insts_init_mutex);
  if (data.insts_init) {
    return;  // already done by another thread
  }

  data.insts.clear();

  std::vector<odb::dbInst*> insts;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
      insts.push_back(inst);
    }
  }
  data.insts = RtreeDBox<odb::dbInst*>(insts.begin(), insts.end());

  data.insts_init = true;
}

void Search::updateBlockages(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.blockages_init_mutex);
  if (data.blockages_init) {
    return;  // already done by another thread
  }

  data.blockages.clear();

  std::vector<odb::dbBlockage*> blockages;
  for (odb::dbBlockage* blockage : block->getBlockages()) {
    if (blockage->isSystemReserved()) {
      continue;
    }
    blockages.push_back(blockage);
  }
  data.blockages
      = RtreeDBox<odb::dbBlockage*>(blockages.begin(), blockages.end());

  data.blockages_init = true;
}

void Search::updateObstructions(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.obstructions_init_mutex);
  if (data.obstructions_init) {
    return;  // already done by another thread
  }

  data.obstructions.clear();

  LayerMap<std::vector<odb::dbObstruction*>> obstructions;
  for (odb::dbObstruction* obs : block->getObstructions()) {
    if (obs->isSystemReserved()) {
      continue;
    }
    odb::dbBox* bbox = obs->getBBox();
    obstructions[bbox->getTechLayer()].push_back(obs);
  }
  for (const auto& [layer, layer_obs] : obstructions) {
    data.obstructions[layer]
        = RtreeDBox<odb::dbObstruction*>(layer_obs.begin(), layer_obs.end());
  }

  data.obstructions_init = true;
}

void Search::updateRows(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  absl::MutexLock lock(&data.rows_init_mutex);
  if (data.rows_init) {
    return;  // already done by another thread
  }

  data.rows.clear();

  std::vector<RectValue<odb::dbRow*>> rows;
  for (odb::dbRow* row : block->getRows()) {
    rows.emplace_back(row->getBBox(), row);
  }
  data.rows = RtreeRect<odb::dbRow*>(rows.begin(), rows.end());

  data.rows_init = true;
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
      tree_shapes[box->getTechLayer()].emplace_back(bbox, VIA, net);
    }
  } else {
    odb::dbVia* via = shape->getVia();
    for (odb::dbBox* box : via->getBoxes()) {
      odb::Rect bbox = box->getBox();
      bbox.moveDelta(x, y);
      tree_shapes[box->getTechLayer()].emplace_back(bbox, VIA, net);
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
      addVia(net, &s, itr.prev_x_, itr.prev_y_, tree_shapes);
    } else {
      tree_shapes[s.getTechLayer()].emplace_back(s.getBox(), WIRE, net);
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

  bool operator()(const BoxValue<T>& o) const
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
  if (!data.shapes_init) {
    updateShapes(block);
  }

  auto it = data.box_shapes.find(layer);
  if (it == data.box_shapes.end()) {
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
  if (!data.shapes_init) {
    updateShapes(block);
  }

  auto it = data.snet_via_shapes.find(layer);
  if (it == data.snet_via_shapes.end()) {
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
  if (!data.shapes_init) {
    updateShapes(block);
  }

  auto it = data.snet_shapes.find(layer);
  if (it == data.snet_shapes.end()) {
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
  if (!data.fills_init) {
    updateFills(block);
  }

  auto it = data.fills.find(layer);
  if (it == data.fills.end()) {
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
  if (!data.insts_init) {
    updateInsts(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return InstRange(
        data.insts.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbInst*>(min_height))),
        data.insts.qend());
  }

  return InstRange(data.insts.qbegin(bgi::intersects(query)),
                   data.insts.qend());
}

Search::BlockageRange Search::searchBlockages(odb::dbBlock* block,
                                              int x_lo,
                                              int y_lo,
                                              int x_hi,
                                              int y_hi,
                                              int min_height)
{
  BlockData& data = getData(block);
  if (!data.blockages_init) {
    updateBlockages(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return BlockageRange(
        data.blockages.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(
                MinHeightPredicate<odb::dbBlockage*>(min_height))),
        data.blockages.qend());
  }

  return BlockageRange(data.blockages.qbegin(bgi::intersects(query)),
                       data.blockages.qend());
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
  if (!data.obstructions_init) {
    updateObstructions(block);
  }

  auto it = data.obstructions.find(layer);
  if (it == data.obstructions.end()) {
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
  if (!data.rows_init) {
    updateRows(block);
  }

  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_height > 0) {
    return RowRange(
        data.rows.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinHeightPredicate<odb::dbRow*>(min_height))),
        data.rows.qend());
  }

  return RowRange(data.rows.qbegin(bgi::intersects(query)), data.rows.qend());
}

Search::BPinRange Search::searchBPins(odb::dbBlock* block,
                                      odb::dbTechLayer* layer,
                                      int x_lo,
                                      int y_lo,
                                      int x_hi,
                                      int y_hi,
                                      int min_size)
{
  BlockData& data = getData(block);
  if (!data.bpins_init) {
    updateBPins(block);
  }

  auto it = data.bpins.find(layer);
  if (it == data.bpins.end()) {
    return BPinRange();
  }

  auto& rtree = it->second;
  const odb::Rect query(x_lo, y_lo, x_hi, y_hi);
  if (min_size > 0) {
    return BPinRange(
        rtree.qbegin(
            bgi::intersects(query)
            && bgi::satisfies(MinSizePredicate<odb::dbBPin*>(min_size))),
        rtree.qend());
  }

  return BPinRange(rtree.qbegin(bgi::intersects(query)), rtree.qend());
}

}  // namespace gui
