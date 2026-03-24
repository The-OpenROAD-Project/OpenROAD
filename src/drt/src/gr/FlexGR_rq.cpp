// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "db/grObj/grFig.h"
#include "db/grObj/grShape.h"
#include "db/grObj/grVia.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRTree.h"
#include "gr/FlexGR.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"

namespace drt {

frDesign* FlexGRWorkerRegionQuery::getDesign() const
{
  return grWorker_->getDesign();
}

void FlexGRWorkerRegionQuery::add(grConnFig* connFig)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    odb::Rect boostr
        = odb::Rect(frb.xMin(), frb.yMin(), frb.xMax(), frb.yMax());
    shapes_.at(obj->getLayerNum()).insert(std::make_pair(boostr, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    odb::dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer1Num())
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
        shapes_.at(via->getViaDef()->getLayer2Num())
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
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add\n";
      }
    }
  } else {
    std::cout << "Error: unsupported region query add\n";
  }
}

void FlexGRWorkerRegionQuery::add(
    grConnFig* connFig,
    std::vector<std::vector<rq_box_value_t<grConnFig*>>>& allShapes)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    allShapes.at(obj->getLayerNum()).emplace_back(frb, obj);
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
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

void FlexGRWorkerRegionQuery::remove(grConnFig* connFig)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    odb::Rect frb = obj->getBBox();
    shapes_.at(obj->getLayerNum()).remove(std::make_pair(frb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    odb::dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer1Num())
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
        shapes_.at(via->getViaDef()->getLayer2Num())
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
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove\n";
      }
    }
  } else {
    std::cout << "Error: unsupported region query remove\n";
  }
}

void FlexGRWorkerRegionQuery::query(const odb::Rect& box,
                                    const frLayerNum layerNum,
                                    std::vector<grConnFig*>& result) const
{
  std::vector<rq_box_value_t<grConnFig*>> temp;
  box_t boostb
      = box_t(point_t(box.xMin(), box.yMin()), point_t(box.xMax(), box.yMax()));
  shapes_.at(layerNum).query(bgi::intersects(boostb), back_inserter(temp));
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void FlexGRWorkerRegionQuery::query(
    const odb::Rect& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<grConnFig*>>& result) const
{
  box_t boostb
      = box_t(point_t(box.xMin(), box.yMin()), point_t(box.xMax(), box.yMax()));
  shapes_.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void FlexGRWorkerRegionQuery::init(bool includeExt)
{
  int numLayers = getDesign()->getTech()->getLayers().size();
  shapes_.clear();
  shapes_.resize(numLayers);
  std::vector<std::vector<rq_box_value_t<grConnFig*>>> allShapes(numLayers);
  for (auto& net : getGRWorker()->getNets()) {
    for (auto& connFig : net->getRouteConnFigs()) {
      add(connFig.get(), allShapes);
    }
    if (includeExt) {
      for (auto& connFig : net->getExtConnFigs()) {
        add(connFig.get(), allShapes);
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    shapes_.at(i) = boost::move(RTree<grConnFig*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

}  // namespace drt
