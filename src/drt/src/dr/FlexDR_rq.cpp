// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "db/drObj/drFig.h"
#include "db/drObj/drShape.h"
#include "db/drObj/drVia.h"
#include "dr/FlexDR.h"
#include "frBaseTypes.h"
#include "frRTree.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {

// I believe it is safe not to sort the query results here despite the
// use of the serializer.  Most of the use of the query is in init()
// which we call pre-serialization.  The only post-serialization use I
// see is in the graphics which don't care about order (it's just
// drawing the shapes).

struct FlexDRWorkerRegionQuery::Impl
{
  FlexDRWorker* drWorker;
  std::vector<RTree<drConnFig*>> shapes;  // only for drXXX in dr worker

  static void add(
      drConnFig* connFig,
      std::vector<std::vector<rq_box_value_t<drConnFig*>>>& allShapes);

 private:
};

FlexDRWorkerRegionQuery::FlexDRWorkerRegionQuery(FlexDRWorker* in)
    : impl_(std::make_unique<Impl>())
{
  impl_->drWorker = in;
}

FlexDRWorkerRegionQuery::~FlexDRWorkerRegionQuery() = default;

void FlexDRWorkerRegionQuery::cleanup()
{
  impl_->shapes.clear();
  impl_->shapes.shrink_to_fit();
}

void FlexDRWorkerRegionQuery::add(drConnFig* connFig)
{
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    impl_->shapes.at(obj->getLayerNum()).insert(std::make_pair(frb, obj));
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    odb::dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getLayer1Num())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getLayer2Num())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getCutLayerNum())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
  } else {
    std::cout << "Error: unsupported region query add\n";
  }
}

void FlexDRWorkerRegionQuery::Impl::add(
    drConnFig* connFig,
    std::vector<std::vector<rq_box_value_t<drConnFig*>>>& allShapes)
{
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    allShapes.at(obj->getLayerNum()).emplace_back(frb, obj);
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    odb::dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getLayer1Num()).emplace_back(frb, via);
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getLayer2Num()).emplace_back(frb, via);
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getCutLayerNum()).emplace_back(frb, via);
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
  } else {
    std::cout << "Error: unsupported region query add\n";
  }
}

void FlexDRWorkerRegionQuery::remove(drConnFig* connFig)
{
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    impl_->shapes.at(obj->getLayerNum()).remove(std::make_pair(frb, obj));
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    odb::dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getLayer1Num())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getLayer2Num())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove\n";
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        impl_->shapes.at(via->getViaDef()->getCutLayerNum())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove\n";
      }
    }
  } else {
    std::cout << "Error: unsupported region query remove\n";
  }
}

void FlexDRWorkerRegionQuery::query(const odb::Rect& box,
                                    const frLayerNum layerNum,
                                    std::vector<drConnFig*>& result) const
{
  std::vector<rq_box_value_t<drConnFig*>> temp;
  impl_->shapes.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  result.reserve(temp.size());
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void FlexDRWorkerRegionQuery::query(
    const odb::Rect& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<drConnFig*>>& result) const
{
  impl_->shapes.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void FlexDRWorkerRegionQuery::init()
{
  int numLayers = impl_->drWorker->getTech()->getLayers().size();
  impl_->shapes.clear();
  impl_->shapes.resize(numLayers);
  std::vector<std::vector<rq_box_value_t<drConnFig*>>> allShapes(numLayers);
  for (auto& net : impl_->drWorker->getNets()) {
    for (auto& connFig : net->getRouteConnFigs()) {
      Impl::add(connFig.get(), allShapes);
    }
    for (auto& connFig : net->getExtConnFigs()) {
      Impl::add(connFig.get(), allShapes);
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    impl_->shapes.at(i) = boost::move(RTree<drConnFig*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

bool FlexDRWorkerRegionQuery::isEmpty() const
{
  return impl_->shapes.empty();
}

}  // namespace drt
