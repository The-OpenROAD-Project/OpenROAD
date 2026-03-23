// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2026, The OpenROAD Authors

#include "search.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "tile_generator.h"
#include "utl/Logger.h"

namespace web {

// CountdownLatch replacement for compilers that lack <latch> (e.g. GCC 10).
class CountdownLatch
{
 public:
  explicit CountdownLatch(int count) : count_(count) {}

  void count_down()
  {
    std::lock_guard<std::mutex> lock(mu_);
    if (--count_ <= 0) {
      cv_.notify_all();
    }
  }

  void wait()
  {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [this] { return count_ <= 0; });
  }

 private:
  std::mutex mu_;
  std::condition_variable cv_;
  int count_;
};

Search::Search(utl::Logger* logger) : logger_(logger)
{
}

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
  // emit modified();
}

void Search::inDbRegionAddBox(odb::dbRegion*, odb::dbBox*)
{
  // emit modified();
}

void Search::inDbRegionDestroy(odb::dbRegion* region)
{
  // emit modified();
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
  if (!chip) {
    return;
  }
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

  // emit newChip(chip);
}

void Search::announceModified(std::atomic_bool& flag)
{
  const bool prev_flag = flag.exchange(false);

  if (prev_flag) {
    // emit modified();
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

void Search::eagerInit(odb::dbBlock* block)
{
  const auto t0 = std::chrono::steady_clock::now();

  CountdownLatch done(6);
  auto run = [&](auto fn) {
    boost::asio::post(pool_, [&done, fn] {
      fn();
      done.count_down();
    });
  };
  run([&] { updateShapes(block); });
  run([&] { updateInsts(block); });
  run([&] { updateFills(block); });
  run([&] { updateBlockages(block); });
  run([&] { updateObstructions(block); });
  run([&] { updateRows(block); });
  done.wait();

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  debugPrint(
      logger_, utl::WEB, "timing", 1, "Search init took {}ms (parallel)", ms);
}

Search::BlockData& Search::getData(odb::dbBlock* block)
{
  return block->getChip() == top_chip_ ? top_block_data_
                                       : child_block_data_[block];
}

void Search::updateShapes(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.shapes_init_mutex);
  if (data.shapes_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

  data.box_shapes.clear();
  data.snet_via_shapes.clear();
  data.snet_shapes.clear();

  // Single pass over all nets to collect both special and routing shapes.
  LayerMap<std::vector<SNetValue<odb::dbNet*>>> snet_shapes;
  LayerMap<std::vector<SNetDBoxValue<odb::dbNet*>>> snet_net_via_shapes;
  LayerMap<std::vector<RouteBoxValue<odb::dbNet*>>> net_shapes;

  for (odb::dbNet* net : block->getNets()) {
    addSNet(net, snet_shapes, snet_net_via_shapes);
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

  const auto t_collect = std::chrono::steady_clock::now();
  debugPrint(
      logger_,
      utl::WEB,
      "timing",
      1,
      "updateShapes collect took {}ms",
      std::chrono::duration_cast<std::chrono::milliseconds>(t_collect - t0)
          .count());

  // Pre-populate map keys so pool tasks only write to existing entries.
  for (const auto& [layer, _] : snet_shapes) {
    data.snet_shapes[layer];
  }
  for (const auto& [layer, _] : snet_net_via_shapes) {
    data.snet_via_shapes[layer];
  }
  for (const auto& [layer, _] : net_shapes) {
    data.box_shapes[layer];
  }

  // Build R-trees in parallel — one pool task per layer per map.
  const auto num_tasks
      = snet_shapes.size() + snet_net_via_shapes.size() + net_shapes.size();
  CountdownLatch rtree_done(num_tasks);

  for (auto& [layer, layer_shapes] : snet_shapes) {
    boost::asio::post(pool_, [&data, layer, &layer_shapes, &rtree_done] {
      data.snet_shapes[layer] = RtreeSNetShapes<odb::dbNet*>(
          layer_shapes.begin(), layer_shapes.end());
      rtree_done.count_down();
    });
  }
  for (auto& [layer, layer_shapes] : snet_net_via_shapes) {
    boost::asio::post(pool_, [&data, layer, &layer_shapes, &rtree_done] {
      data.snet_via_shapes[layer] = RtreeSNetDBoxShapes<odb::dbNet*>(
          layer_shapes.begin(), layer_shapes.end());
      rtree_done.count_down();
    });
  }
  for (auto& [layer, layer_shapes] : net_shapes) {
    boost::asio::post(pool_, [&data, layer, &layer_shapes, &rtree_done] {
      data.box_shapes[layer] = RtreeRoutingShapes<odb::dbNet*>(
          layer_shapes.begin(), layer_shapes.end());
      rtree_done.count_down();
    });
  }
  rtree_done.wait();

  const auto t_rtree = std::chrono::steady_clock::now();

  data.shapes_init = true;

  const auto rtree_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            t_rtree - t_collect)
                            .count();
  debugPrint(logger_,
             utl::WEB,
             "timing",
             1,
             "updateShapes rtree took {}ms ({} tasks)",
             rtree_ms,
             num_tasks);

  const auto total_ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t_rtree - t0)
            .count();
  debugPrint(
      logger_, utl::WEB, "timing", 1, "updateShapes took {}ms", total_ms);
}

void Search::updateFills(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.fills_init_mutex);
  if (data.fills_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

  data.fills.clear();

  LayerMap<std::vector<odb::dbFill*>> fills;
  for (odb::dbFill* fill : block->getFills()) {
    fills[fill->getTechLayer()].push_back(fill);
  }

  // Pre-populate map keys, then build R-trees in parallel.
  for (const auto& [layer, _] : fills) {
    data.fills[layer];
  }
  CountdownLatch done(fills.size());
  for (auto& [layer, layer_fill] : fills) {
    boost::asio::post(pool_, [&data, layer, &layer_fill, &done] {
      data.fills[layer] = RtreeFill(layer_fill.begin(), layer_fill.end());
      done.count_down();
    });
  }
  done.wait();

  data.fills_init = true;

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  if (ms > 10) {
    logger_->info(utl::WEB, 12, "Search::updateFills took {}ms", ms);
  }
}

void Search::updateInsts(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.insts_init_mutex);
  if (data.insts_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

  data.insts.clear();

  std::vector<odb::dbInst*> insts;
  for (odb::dbInst* inst : block->getInsts()) {
    if (inst->isPlaced()) {
      insts.push_back(inst);
    }
  }
  data.insts = RtreeDBox<odb::dbInst*>(insts.begin(), insts.end());

  data.insts_init = true;

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  logger_->info(utl::WEB, 13, "Search::updateInsts took {}ms", ms);
}

void Search::updateBlockages(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.blockages_init_mutex);
  if (data.blockages_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

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

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  if (ms > 10) {
    logger_->info(utl::WEB, 14, "Search::updateBlockages took {}ms", ms);
  }
}

void Search::updateObstructions(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.obstructions_init_mutex);
  if (data.obstructions_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

  data.obstructions.clear();

  LayerMap<std::vector<odb::dbObstruction*>> obstructions;
  for (odb::dbObstruction* obs : block->getObstructions()) {
    if (obs->isSystemReserved()) {
      continue;
    }
    odb::dbBox* bbox = obs->getBBox();
    obstructions[bbox->getTechLayer()].push_back(obs);
  }
  // Pre-populate map keys, then build R-trees in parallel.
  for (const auto& [layer, _] : obstructions) {
    data.obstructions[layer];
  }
  CountdownLatch done(obstructions.size());
  for (auto& [layer, layer_obs] : obstructions) {
    boost::asio::post(pool_, [&data, layer, &layer_obs, &done] {
      data.obstructions[layer]
          = RtreeDBox<odb::dbObstruction*>(layer_obs.begin(), layer_obs.end());
      done.count_down();
    });
  }
  done.wait();

  data.obstructions_init = true;

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  if (ms > 10) {
    logger_->info(utl::WEB, 15, "Search::updateObstructions took {}ms", ms);
  }
}

void Search::updateRows(odb::dbBlock* block)
{
  BlockData& data = getData(block);
  std::lock_guard<std::mutex> lock(data.rows_init_mutex);
  if (data.rows_init) {
    return;  // already done by another thread
  }

  const auto t0 = std::chrono::steady_clock::now();

  data.rows.clear();

  std::vector<RectValue<odb::dbRow*>> rows;
  for (odb::dbRow* row : block->getRows()) {
    rows.emplace_back(row->getBBox(), row);
  }
  data.rows = RtreeRect<odb::dbRow*>(rows.begin(), rows.end());

  data.rows_init = true;

  const auto t1 = std::chrono::steady_clock::now();
  const auto ms
      = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  if (ms > 10) {
    logger_->info(utl::WEB, 16, "Search::updateRows took {}ms", ms);
  }
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

// ─── Snap (nearest edge search) ─────────────────────────────────────────────

namespace {

using Edge = std::pair<odb::Point, odb::Point>;

int edgeToPointDistance(const odb::Point& pt, const Edge& edge)
{
  using BPoint = boost::geometry::model::d2::point_xy<int>;
  using BLine = boost::geometry::model::linestring<BPoint>;

  const BPoint bpt(pt.x(), pt.y());
  BLine bline;
  bline.push_back(BPoint(edge.first.x(), edge.first.y()));
  bline.push_back(BPoint(edge.second.x(), edge.second.y()));

  return static_cast<int>(boost::geometry::distance(bpt, bline));
}

// Prefer shorter edges when distances are equal.
bool isShorterEdge(const Edge& lhs, const Edge& rhs)
{
  const int64_t lhs_len = static_cast<int64_t>(lhs.first.x() - lhs.second.x())
                              * (lhs.first.x() - lhs.second.x())
                          + static_cast<int64_t>(lhs.first.y() - lhs.second.y())
                                * (lhs.first.y() - lhs.second.y());
  const int64_t rhs_len = static_cast<int64_t>(rhs.first.x() - rhs.second.x())
                              * (rhs.first.x() - rhs.second.x())
                          + static_cast<int64_t>(rhs.first.y() - rhs.second.y())
                                * (rhs.first.y() - rhs.second.y());
  return lhs_len < rhs_len;
}

}  // namespace

Search::SnapResult Search::searchNearestEdge(
    odb::dbBlock* block,
    odb::Point pt,
    int search_radius,
    int point_snap_threshold,
    bool horizontal,
    bool vertical,
    const TileVisibility& vis,
    const std::set<std::string>& visible_layers)
{
  if (!block) {
    return {};
  }

  Edge closest_edge;
  int edge_distance = std::numeric_limits<int>::max();

  auto check_rect = [&](const odb::Rect& rect) {
    const odb::Point ll(rect.xMin(), rect.yMin());
    const odb::Point lr(rect.xMax(), rect.yMin());
    const odb::Point ul(rect.xMin(), rect.yMax());
    const odb::Point ur(rect.xMax(), rect.yMax());

    auto try_edge = [&](const Edge& e) {
      const int dist = edgeToPointDistance(pt, e);
      if (dist < edge_distance
          || (dist == edge_distance && isShorterEdge(e, closest_edge))) {
        edge_distance = dist;
        closest_edge = e;
      }
    };

    if (horizontal) {
      try_edge({ul, ur});  // top
      try_edge({ll, lr});  // bottom
    }
    if (vertical) {
      try_edge({ll, ul});  // left
      try_edge({lr, ur});  // right
    }
  };

  // Build directional search box.
  odb::Rect search_box;
  if (horizontal && !vertical) {
    search_box = odb::Rect(
        pt.x(), pt.y() - search_radius, pt.x(), pt.y() + search_radius);
  } else if (vertical && !horizontal) {
    search_box = odb::Rect(
        pt.x() - search_radius, pt.y(), pt.x() + search_radius, pt.y());
  } else {
    search_box = odb::Rect(pt.x() - search_radius,
                           pt.y() - search_radius,
                           pt.x() + search_radius,
                           pt.y() + search_radius);
  }

  // Die area.
  check_rect(block->getDieArea());

  // Instances.
  for (auto* inst : searchInsts(block,
                                search_box.xMin(),
                                search_box.yMin(),
                                search_box.xMax(),
                                search_box.yMax())) {
    if (vis.isInstVisible(inst, nullptr)) {
      check_rect(inst->getBBox()->getBox());
    }
  }

  // Layer-based shapes.
  odb::dbTech* tech = block->getTech();
  if (tech) {
    for (auto* layer : tech->getLayers()) {
      if (!visible_layers.contains(layer->getName())) {
        continue;
      }

      // Routing shapes (wires, vias, bterms).
      if (vis.routing || vis.pins) {
        for (const auto& [box, type, net] :
             searchBoxShapes(block,
                             layer,
                             search_box.xMin(),
                             search_box.yMin(),
                             search_box.xMax(),
                             search_box.yMax())) {
          if (!vis.routing && type == WIRE) {
            continue;
          }
          if (!vis.routing && type == VIA) {
            continue;
          }
          if (!vis.pins && type == BTERM) {
            continue;
          }
          if (vis.isNetVisible(net)) {
            check_rect(box);
          }
        }
      }

      // Special net shapes.
      if (vis.special_nets) {
        for (const auto& [sbox, poly, net] :
             searchSNetShapes(block,
                              layer,
                              search_box.xMin(),
                              search_box.yMin(),
                              search_box.xMax(),
                              search_box.yMax())) {
          if (vis.isNetVisible(net)) {
            check_rect(sbox->getBox());
          }
        }

        // Special net vias.
        for (const auto& [sbox, net] : searchSNetViaShapes(block,
                                                           layer,
                                                           search_box.xMin(),
                                                           search_box.yMin(),
                                                           search_box.xMax(),
                                                           search_box.yMax())) {
          if (vis.isNetVisible(net)) {
            check_rect(sbox->getBox());
          }
        }
      }

      // Fills.
      for (auto* fill : searchFills(block,
                                    layer,
                                    search_box.xMin(),
                                    search_box.yMin(),
                                    search_box.xMax(),
                                    search_box.yMax())) {
        odb::Rect fill_rect;
        fill->getRect(fill_rect);
        check_rect(fill_rect);
      }

      // Obstructions.
      if (vis.routing_obstructions) {
        for (auto* obs : searchObstructions(block,
                                            layer,
                                            search_box.xMin(),
                                            search_box.yMin(),
                                            search_box.xMax(),
                                            search_box.yMax())) {
          check_rect(obs->getBBox()->getBox());
        }
      }
    }
  }

  // Blockages.
  if (vis.placement_blockages) {
    for (auto* blk : searchBlockages(block,
                                     search_box.xMin(),
                                     search_box.yMin(),
                                     search_box.xMax(),
                                     search_box.yMax())) {
      check_rect(blk->getBBox()->getBox());
    }
  }

  // Rows.
  if (vis.rows) {
    for (const auto& [row_rect, row] : searchRows(block,
                                                  search_box.xMin(),
                                                  search_box.yMin(),
                                                  search_box.xMax(),
                                                  search_box.yMax())) {
      check_rect(row_rect);
    }
  }

  if (edge_distance == std::numeric_limits<int>::max()) {
    return {};
  }

  // Point snap: if cursor is close to an endpoint or midpoint, snap to it.
  const std::array<odb::Point, 3> snap_points
      = {closest_edge.first,
         odb::Point((closest_edge.first.x() + closest_edge.second.x()) / 2,
                    (closest_edge.first.y() + closest_edge.second.y()) / 2),
         closest_edge.second};

  for (const auto& snap_pt : snap_points) {
    if (std::abs(snap_pt.x() - pt.x()) < point_snap_threshold
        && std::abs(snap_pt.y() - pt.y()) < point_snap_threshold) {
      closest_edge.first = snap_pt;
      closest_edge.second = snap_pt;
      break;
    }
  }

  SnapResult result;
  result.edge = closest_edge;
  result.distance = edge_distance;
  result.found = true;
  return result;
}

}  // namespace web
