// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "frRegionQuery.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "db/obj/frBlockObject.h"
#include "db/obj/frBlockage.h"
#include "db/obj/frInstBlockage.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRTree.h"
#include "global.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"
#include "utl/algorithms.h"

namespace drt {

class FlexDR;

using utl::enumerate;
namespace gtl = boost::polygon;

struct frRegionQuery::Impl
{
  template <typename T>
  using RTreesByLayer = std::vector<RTree<T>>;

  template <typename T>
  using ObjectsByLayer = std::vector<Objects<T>>;

  frDesign* design;
  utl::Logger* logger;
  RouterConfiguration* router_cfg;
  // only for pin shapes, obs and snet
  RTreesByLayer<frBlockObject*> shapes;
  RTreesByLayer<frGuide*> guides;
  RTreesByLayer<frNet*> origGuides;  // non-processed guides;
  RTree<frBlockObject*> grPins;
  RTreesByLayer<frRPin*> rpins;  // only for rpins
  // only for gr objs, via only in via layer
  RTreesByLayer<grBlockObject*> grObjs;
  // only for dr objs, via only in via layer
  RTreesByLayer<frBlockObject*> drObjs;
  RTreesByLayer<frMarker*> markers;  // use init()

  Impl() = default;
  void init();
  void initOrigGuide(frOrderedIdMap<frNet*, std::vector<frRect>>& tmpGuides);
  void initGuide();
  void initRPin();
  void initGRPin(std::vector<std::pair<frBlockObject*, odb::Point>>& in);
  void initDRObj();
  void initGRObj();

  void add(frShape* shape, ObjectsByLayer<frBlockObject>& allShapes);
  void add(frVia* via, ObjectsByLayer<frBlockObject>& allShapes);
  void add(frInstTerm* instTerm, ObjectsByLayer<frBlockObject>& allShapes);
  void add(frBTerm* term, ObjectsByLayer<frBlockObject>& allShapes);
  void add(frBlockage* blk, ObjectsByLayer<frBlockObject>& allShapes);
  void add(frInstBlockage* instBlk, ObjectsByLayer<frBlockObject>& allShapes);
  void addGuide(frGuide* guide, ObjectsByLayer<frGuide>& allShapes);
  void addOrigGuide(frNet* net,
                    const frRect& rect,
                    ObjectsByLayer<frNet>& allShapes);
  void addDRObj(frShape* shape, ObjectsByLayer<frBlockObject>& allShapes);
  void addDRObj(frVia* via, ObjectsByLayer<frBlockObject>& allShapes);
  void addRPin(frRPin* rpin, ObjectsByLayer<frRPin>& allRPins);
  void addGRObj(grShape* shape, ObjectsByLayer<grBlockObject>& allShapes);
  void addGRObj(grVia* via, ObjectsByLayer<grBlockObject>& allShapes);
  void addGRObj(grShape* shape);
  void addGRObj(grVia* via);
};

frRegionQuery::frRegionQuery(frDesign* design,
                             utl::Logger* logger,
                             RouterConfiguration* router_cfg)
    : impl_(std::make_unique<Impl>())
{
  impl_->design = design;
  impl_->logger = logger;
  impl_->router_cfg = router_cfg;
}

frRegionQuery::frRegionQuery() : impl_(nullptr)
{
}

frRegionQuery::~frRegionQuery() = default;

frDesign* frRegionQuery::getDesign() const
{
  return impl_->design;
}

void frRegionQuery::Impl::add(frShape* shape,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
    odb::Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).emplace_back(frb, shape);
  } else {
    logger->error(DRT, 5, "Unsupported region query add.");
  }
}

void frRegionQuery::addDRObj(frShape* shape)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    odb::Rect frb = shape->getBBox();
    impl_->drObjs.at(shape->getLayerNum()).insert(std::make_pair(frb, shape));
  } else {
    impl_->logger->error(DRT, 6, "Unsupported region query add.");
  }
}

void frRegionQuery::addMarker(frMarker* in)
{
  odb::Rect frb = in->getBBox();
  impl_->markers.at(in->getLayerNum()).insert(std::make_pair(frb, in));
}

void frRegionQuery::Impl::addDRObj(frShape* shape,
                                   ObjectsByLayer<frBlockObject>& allShapes)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    odb::Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).emplace_back(frb, shape);
  } else {
    logger->error(DRT, 7, "Unsupported region query add.");
  }
}

void frRegionQuery::removeDRObj(frShape* shape)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    odb::Rect frb = shape->getBBox();
    impl_->drObjs.at(shape->getLayerNum()).remove(std::make_pair(frb, shape));
  } else {
    impl_->logger->error(DRT, 31, "Unsupported region query add.");
  }
}

std::vector<std::pair<frBlockObject*, odb::Rect>> frRegionQuery::getVias(
    frLayerNum layer_num)
{
  std::vector<std::pair<frBlockObject*, odb::Rect>> result;
  result.reserve(impl_->shapes.at(layer_num).size()
                 + impl_->drObjs.at(layer_num).size());
  for (auto [box, obj] : impl_->shapes.at(layer_num)) {
    result.emplace_back(obj, box);
  }
  for (auto [box, obj] : impl_->drObjs.at(layer_num)) {
    result.emplace_back(obj, box);
  }
  return result;
}

void frRegionQuery::addBlockObj(frBlockObject* obj)
{
  switch (obj->typeId()) {
    case frcInstTerm: {
      auto instTerm = static_cast<frInstTerm*>(obj);
      odb::dbTransform xform = instTerm->getInst()->getDBTransform();
      for (auto& pin : instTerm->getTerm()->getPins()) {
        for (auto& uFig : pin->getFigs()) {
          auto shape = uFig.get();
          odb::Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
              .insert(std::make_pair(frb, instTerm));
        }
      }
      break;
    }
    case frcInstBlockage: {
      auto instBlk = static_cast<frInstBlockage*>(obj);
      odb::dbTransform xform = instBlk->getInst()->getDBTransform();
      auto blk = instBlk->getBlockage();
      auto pin = blk->getPin();
      for (auto& uFig : pin->getFigs()) {
        auto shape = uFig.get();
        if (shape->typeId() == frcRect) {
          odb::Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
              .insert(std::make_pair(frb, instBlk));
        } else if (shape->typeId() == frcPolygon) {
          // Decompose the polygon to rectangles and store those
          // Convert the frPolygon to a Boost polygon
          std::vector<gtl::point_data<frCoord>> points;
          for (odb::Point pt : ((frPolygon*) shape)->getPoints()) {
            xform.apply(pt);
            points.emplace_back(pt.x(), pt.y());
          }
          gtl::polygon_90_data<frCoord> poly;
          poly.set(points.begin(), points.end());
          // Add the polygon to a polygon set
          gtl::polygon_90_set_data<frCoord> polySet;
          {
            using boost::polygon::operators::operator+=;
            polySet += poly;
          }
          // Decompose the polygon set to rectanges
          std::vector<gtl::rectangle_data<frCoord>> rects;
          polySet.get_rectangles(rects);
          // Store the rectangles with this blockage
          for (auto& rect : rects) {
            odb::Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
            impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
                .insert(std::make_pair(box, instBlk));
          }
        }
      }
      break;
    }
    case frcInst: {
      auto inst = static_cast<frInst*>(obj);
      for (auto& instTerm : inst->getInstTerms()) {
        addBlockObj(instTerm.get());
      }
      for (auto& blkg : inst->getInstBlockages()) {
        addBlockObj(blkg.get());
      }
      break;
    }
    default:
      impl_->logger->error(DRT, 513, "Unsupported region addBlockObj");
  }
}

void frRegionQuery::removeBlockObj(frBlockObject* obj)
{
  switch (obj->typeId()) {
    case frcInstTerm: {
      auto instTerm = static_cast<frInstTerm*>(obj);
      odb::dbTransform xform = instTerm->getInst()->getDBTransform();
      for (auto& pin : instTerm->getTerm()->getPins()) {
        for (auto& uFig : pin->getFigs()) {
          auto shape = uFig.get();
          odb::Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
              .remove(std::make_pair(frb, instTerm));
        }
      }
      break;
    }
    case frcInstBlockage: {
      auto instBlk = static_cast<frInstBlockage*>(obj);
      odb::dbTransform xform = instBlk->getInst()->getDBTransform();
      auto blk = instBlk->getBlockage();
      auto pin = blk->getPin();
      for (auto& uFig : pin->getFigs()) {
        auto shape = uFig.get();
        if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
          odb::Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
              .remove(std::make_pair(frb, instBlk));
        } else if (shape->typeId() == frcPolygon) {
          // Decompose the polygon to rectangles and store those
          // Convert the frPolygon to a Boost polygon
          std::vector<gtl::point_data<frCoord>> points;
          for (odb::Point pt : ((frPolygon*) shape)->getPoints()) {
            xform.apply(pt);
            points.emplace_back(pt.x(), pt.y());
          }
          gtl::polygon_90_data<frCoord> poly;
          poly.set(points.begin(), points.end());
          // Add the polygon to a polygon set
          gtl::polygon_90_set_data<frCoord> polySet;
          {
            using boost::polygon::operators::operator+=;
            polySet += poly;
          }
          // Decompose the polygon set to rectanges
          std::vector<gtl::rectangle_data<frCoord>> rects;
          polySet.get_rectangles(rects);
          // Store the rectangles with this blockage
          for (auto& rect : rects) {
            odb::Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
            impl_->shapes.at(static_cast<frShape*>(shape)->getLayerNum())
                .remove(std::make_pair(box, instBlk));
          }
        }
      }
      break;
    }
    case frcInst: {
      auto inst = static_cast<frInst*>(obj);
      for (auto& instTerm : inst->getInstTerms()) {
        removeBlockObj(instTerm.get());
      }
      for (auto& blkg : inst->getInstBlockages()) {
        removeBlockObj(blkg.get());
      }
      break;
    }
    default:
      impl_->logger->error(DRT, 512, "Unsupported region removeBlockObj");
  }
}

void frRegionQuery::addGRObj(grShape* shape)
{
  impl_->addGRObj(shape);
}

void frRegionQuery::Impl::addGRObj(grShape* shape)
{
  if (shape->typeId() == grcPathSeg) {
    odb::Rect frb = shape->getBBox();
    grObjs.at(shape->getLayerNum()).insert(std::make_pair(frb, shape));
  } else {
    logger->error(DRT, 8, "Unsupported region query add.");
  }
}

void frRegionQuery::Impl::addGRObj(grVia* via,
                                   ObjectsByLayer<grBlockObject>& allShapes)
{
  odb::Rect frb = via->getBBox();
  allShapes.at(via->getViaDef()->getCutLayerNum()).emplace_back(frb, via);
}

void frRegionQuery::removeGRObj(grVia* via)
{
  odb::Rect frb = via->getBBox();
  impl_->grObjs.at(via->getViaDef()->getCutLayerNum())
      .remove(std::make_pair(frb, via));
}

void frRegionQuery::Impl::addGRObj(grShape* shape,
                                   ObjectsByLayer<grBlockObject>& allShapes)
{
  if (shape->typeId() == grcPathSeg) {
    odb::Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).emplace_back(frb, shape);
  } else {
    logger->error(DRT, 9, "Unsupported region query add.");
  }
}

void frRegionQuery::removeGRObj(grShape* shape)
{
  if (shape->typeId() == grcPathSeg) {
    odb::Rect frb = shape->getBBox();
    impl_->grObjs.at(shape->getLayerNum()).remove(std::make_pair(frb, shape));
  } else {
    impl_->logger->error(DRT, 10, "Unsupported region query add.");
  }
}

void frRegionQuery::removeMarker(frMarker* in)
{
  odb::Rect frb = in->getBBox();
  impl_->markers.at(in->getLayerNum()).remove(std::make_pair(frb, in));
}

void frRegionQuery::Impl::add(frVia* via,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  odb::dbTransform xform = via->getTransform();
  for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      odb::Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getLayer1Num()).emplace_back(frb, via);
    } else {
      logger->error(DRT, 11, "Unsupported region query add.");
    }
  }
  for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      odb::Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getLayer2Num()).emplace_back(frb, via);
    } else {
      logger->error(DRT, 12, "Unsupported region query add.");
    }
  }
  for (auto& uShape : via->getViaDef()->getCutFigs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      odb::Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getCutLayerNum()).emplace_back(frb, via);
    } else {
      logger->error(DRT, 13, "Unsupported region query add.");
    }
  }
}

void frRegionQuery::addDRObj(frVia* via)
{
  odb::Rect frb = via->getBBox();
  impl_->drObjs.at(via->getViaDef()->getCutLayerNum())
      .insert(std::make_pair(frb, via));
}

void frRegionQuery::Impl::addDRObj(frVia* via,
                                   ObjectsByLayer<frBlockObject>& allShapes)
{
  odb::Rect frb = via->getBBox();
  allShapes.at(via->getViaDef()->getCutLayerNum()).emplace_back(frb, via);
}

void frRegionQuery::removeDRObj(frVia* via)
{
  odb::Rect frb = via->getBBox();
  impl_->drObjs.at(via->getViaDef()->getCutLayerNum())
      .remove(std::make_pair(frb, via));
}

void frRegionQuery::addGRObj(grVia* via)
{
  odb::Rect frb = via->getBBox();
  impl_->grObjs.at(via->getViaDef()->getCutLayerNum())
      .insert(std::make_pair(frb, via));
}

void frRegionQuery::Impl::add(frInstTerm* instTerm,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  odb::dbTransform xform = instTerm->getInst()->getDBTransform();

  for (auto& pin : instTerm->getTerm()->getPins()) {
    for (auto& uFig : pin->getFigs()) {
      auto shape = uFig.get();
      if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .emplace_back(frb, instTerm);
      } else {
        logger->error(DRT, 14, "Unsupported region query add.");
      }
    }
  }
}

void frRegionQuery::Impl::add(frBTerm* term,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  for (auto& pin : term->getPins()) {
    for (auto& uFig : pin->getFigs()) {
      auto shape = uFig.get();
      if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
        odb::Rect frb = shape->getBBox();
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .emplace_back(frb, term);
      } else {
        logger->error(DRT, 15, "Unsupported region query add.");
      }
    }
  }
}

void frRegionQuery::Impl::add(frInstBlockage* instBlk,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  odb::dbTransform xform = instBlk->getInst()->getDBTransform();
  auto blk = instBlk->getBlockage();
  auto pin = blk->getPin();
  for (auto& uFig : pin->getFigs()) {
    auto shape = uFig.get();
    if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
      odb::Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
          .emplace_back(frb, instBlk);
    } else if (shape->typeId() == frcPolygon) {
      // Decompose the polygon to rectangles and store those
      // Convert the frPolygon to a Boost polygon
      std::vector<gtl::point_data<frCoord>> points;
      for (odb::Point pt : ((frPolygon*) shape)->getPoints()) {
        xform.apply(pt);
        points.emplace_back(pt.x(), pt.y());
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      // Add the polygon to a polygon set
      gtl::polygon_90_set_data<frCoord> polySet;
      {
        using boost::polygon::operators::operator+=;
        polySet += poly;
      }
      // Decompose the polygon set to rectanges
      std::vector<gtl::rectangle_data<frCoord>> rects;
      polySet.get_rectangles(rects);
      // Store the rectangles with this blockage
      for (auto& rect : rects) {
        odb::Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .emplace_back(box, instBlk);
      }
    } else {
      logger->error(DRT,
                    16,
                    "Unsupported region query add of blockage in instance {}.",
                    instBlk->getInst()->getName());
    }
  }
}

void frRegionQuery::Impl::add(frBlockage* blk,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  auto pin = blk->getPin();
  for (auto& uFig : pin->getFigs()) {
    auto shape = uFig.get();
    if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
      odb::Rect frb = shape->getBBox();
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
          .emplace_back(frb, blk);
    } else {
      logger->error(DRT, 17, "Unsupported region query add.");
    }
  }
}

void frRegionQuery::Impl::addGuide(frGuide* guide,
                                   ObjectsByLayer<frGuide>& allShapes)
{
  odb::Rect frb = guide->getBBox();
  for (int i = guide->getBeginLayerNum(); i <= guide->getEndLayerNum(); i++) {
    allShapes.at(i).emplace_back(frb, guide);
  }
}

void frRegionQuery::Impl::addRPin(frRPin* rpin,
                                  ObjectsByLayer<frRPin>& allRPins)
{
  frLayerNum layerNum = rpin->getLayerNum();
  odb::Rect frb = rpin->getBBox();
  allRPins.at(layerNum).emplace_back(frb, rpin);
}

void frRegionQuery::Impl::addOrigGuide(frNet* net,
                                       const frRect& rect,
                                       ObjectsByLayer<frNet>& allShapes)
{
  odb::Rect frb = rect.getBBox();
  allShapes.at(rect.getLayerNum()).emplace_back(frb, net);
}

void frRegionQuery::query(const box_t& boostb,
                          const frLayerNum layerNum,
                          Objects<frBlockObject>& result) const
{
  impl_->shapes.at(layerNum).query(bgi::intersects(boostb),
                                   back_inserter(result));
}

void frRegionQuery::query(const odb::Rect& box,
                          const frLayerNum layerNum,
                          Objects<frBlockObject>& result) const
{
  impl_->shapes.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void frRegionQuery::queryRPin(const odb::Rect& box,
                              const frLayerNum layerNum,
                              Objects<frRPin>& result) const
{
  impl_->rpins.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void frRegionQuery::queryGuide(const odb::Rect& box,
                               const frLayerNum layerNum,
                               Objects<frGuide>& result) const
{
  impl_->guides.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void frRegionQuery::queryGuide(const odb::Rect& box,
                               const frLayerNum layerNum,
                               std::vector<frGuide*>& result) const
{
  Objects<frGuide> temp;
  queryGuide(box, layerNum, temp);
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryGuide(const odb::Rect& box,
                               std::vector<frGuide*>& result) const
{
  Objects<frGuide> temp;
  for (auto& m : impl_->guides) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryOrigGuide(const odb::Rect& box,
                                   const frLayerNum layerNum,
                                   Objects<frNet>& result) const
{
  impl_->origGuides.at(layerNum).query(bgi::intersects(box),
                                       back_inserter(result));
}

void frRegionQuery::queryGRPin(const odb::Rect& box,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  impl_->grPins.query(bgi::intersects(box), back_inserter(temp));
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryDRObj(const box_t& boostb,
                               const frLayerNum layerNum,
                               Objects<frBlockObject>& result) const
{
  impl_->drObjs.at(layerNum).query(bgi::intersects(boostb),
                                   back_inserter(result));
}

void frRegionQuery::queryDRObj(const odb::Rect& box,
                               const frLayerNum layerNum,
                               Objects<frBlockObject>& result) const
{
  impl_->drObjs.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void frRegionQuery::queryDRObj(const odb::Rect& box,
                               const frLayerNum layerNum,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  impl_->drObjs.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryDRObj(const odb::Rect& box,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  for (auto& m : impl_->drObjs) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryGRObj(const odb::Rect& box,
                               std::vector<grBlockObject*>& result) const
{
  Objects<grBlockObject> temp;
  for (auto& m : impl_->grObjs) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryMarker(const odb::Rect& box,
                                const frLayerNum layerNum,
                                std::vector<frMarker*>& result) const
{
  Objects<frMarker> temp;
  impl_->markers.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::queryMarker(const odb::Rect& box,
                                std::vector<frMarker*>& result) const
{
  Objects<frMarker> temp;
  for (auto& m : impl_->markers) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  std::ranges::transform(
      temp, back_inserter(result), [](auto& kv) { return kv.second; });
}

void frRegionQuery::init()
{
  impl_->init();
}

void frRegionQuery::Impl::init()
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  shapes.clear();
  shapes.resize(numLayers);

  markers.clear();
  markers.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  int cnt = 0;
  for (auto& inst : design->getTopBlock()->getInsts()) {
    for (auto& instTerm : inst->getInstTerms()) {
      add(instTerm.get(), allShapes);
    }
    for (auto& instBlk : inst->getInstBlockages()) {
      add(instBlk.get(), allShapes);
    }
    cnt++;
    if (router_cfg->VERBOSE > 0) {
      if (cnt < 1000000) {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 18, "  Complete {} insts.", cnt);
        }
      } else {
        if (cnt % 1000000 == 0) {
          logger->info(DRT, 19, "  Complete {} insts.", cnt);
        }
      }
    }
  }
  cnt = 0;
  for (auto& term : design->getTopBlock()->getTerms()) {
    add(term.get(), allShapes);
    cnt++;
    if (router_cfg->VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger->info(DRT, 20, "  Complete {} terms.", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 21, "  Complete {} terms.", cnt);
        }
      }
    }
  }

  cnt = 0;
  for (auto& net : design->getTopBlock()->getSNets()) {
    for (auto& shape : net->getShapes()) {
      add(shape.get(), allShapes);
    }
    for (auto& via : net->getVias()) {
      add(via.get(), allShapes);
    }
    cnt++;
    if (router_cfg->VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger->info(DRT, 22, "  Complete {} snets.", cnt);
      }
    }
  }

  cnt = 0;
  for (auto& blk : design->getTopBlock()->getBlockages()) {
    add(blk.get(), allShapes);
    cnt++;
    if (router_cfg->VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger->info(DRT, 23, "  Complete {} blockages.", cnt);
      }
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    shapes.at(i) = boost::move(RTree<frBlockObject*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (router_cfg->VERBOSE > 0) {
      logger->info(
          DRT, 24, "  Complete {}.", design->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initOrigGuide(
    frOrderedIdMap<frNet*, std::vector<frRect>>& tmpGuides)
{
  impl_->initOrigGuide(tmpGuides);
}

void frRegionQuery::Impl::initOrigGuide(
    frOrderedIdMap<frNet*, std::vector<frRect>>& tmpGuides)
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  origGuides.clear();
  origGuides.resize(numLayers);
  ObjectsByLayer<frNet> allShapes(numLayers);

  int cnt = 0;
  for (auto& [net, rects] : tmpGuides) {
    for (auto& rect : rects) {
      addOrigGuide(net, rect, allShapes);
      cnt++;
      if (router_cfg->VERBOSE > 0) {
        if (cnt < 1000000) {
          if (cnt % 100000 == 0) {
            logger->info(DRT, 26, "  Complete {} origin guides.", cnt);
          }
        } else {
          if (cnt % 1000000 == 0) {
            logger->info(DRT, 27, "  Complete {} origin guides.", cnt);
          }
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    origGuides.at(i) = boost::move(RTree<frNet*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (router_cfg->VERBOSE > 0) {
      logger->info(
          DRT, 28, "  Complete {}.", design->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGuide()
{
  impl_->initGuide();
}

void frRegionQuery::Impl::initGuide()
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  guides.clear();
  guides.resize(numLayers);
  ObjectsByLayer<frGuide> allGuides(numLayers);

  int cnt = 0;
  for (auto& net : design->getTopBlock()->getNets()) {
    for (auto& guide : net->getGuides()) {
      addGuide(guide.get(), allGuides);
    }
    cnt++;
    if (router_cfg->VERBOSE > 0) {
      if (cnt < 1000000) {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 29, "  Complete {} nets (guide).", cnt);
        }
      } else {
        if (cnt % 1000000 == 0) {
          logger->info(DRT, 30, "  Complete {} nets (guide).", cnt);
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    guides.at(i) = boost::move(RTree<frGuide*>(allGuides.at(i)));
    allGuides.at(i).clear();
    allGuides.at(i).shrink_to_fit();
    if (router_cfg->VERBOSE > 0) {
      logger->info(DRT,
                   35,
                   "  Complete {} (guide).",
                   design->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGRPin(
    std::vector<std::pair<frBlockObject*, odb::Point>>& in)
{
  impl_->initGRPin(in);
}

void frRegionQuery::Impl::initGRPin(
    std::vector<std::pair<frBlockObject*, odb::Point>>& in)
{
  grPins.clear();
  Objects<frBlockObject> allGRPins;
  for (auto& [obj, pt] : in) {
    odb::Rect frb(pt.x(), pt.y(), pt.x(), pt.y());
    allGRPins.emplace_back(frb, obj);
  }
  in.clear();
  in.shrink_to_fit();
  grPins = boost::move(RTree<frBlockObject*>(allGRPins));
}

void frRegionQuery::initRPin()
{
  impl_->initRPin();
}

void frRegionQuery::Impl::initRPin()
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  rpins.clear();
  rpins.resize(numLayers);

  ObjectsByLayer<frRPin> allRPins(numLayers);

  for (auto& net : design->getTopBlock()->getNets()) {
    for (auto& rpin : net->getRPins()) {
      addRPin(rpin.get(), allRPins);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    rpins.at(i) = boost::move(RTree<frRPin*>(allRPins.at(i)));
    allRPins.at(i).clear();
    allRPins.at(i).shrink_to_fit();
  }
}

void frRegionQuery::initDRObj()
{
  impl_->initDRObj();
}

void frRegionQuery::Impl::initDRObj()
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  drObjs.clear();
  drObjs.shrink_to_fit();
  drObjs.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  for (auto& net : design->getTopBlock()->getNets()) {
    for (auto& shape : net->getShapes()) {
      addDRObj(shape.get(), allShapes);
    }
    for (auto& via : net->getVias()) {
      addDRObj(via.get(), allShapes);
    }
    for (auto& pwire : net->getPatchWires()) {
      addDRObj(pwire.get(), allShapes);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    drObjs.at(i) = boost::move(RTree<frBlockObject*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

void frRegionQuery::Impl::initGRObj()
{
  const frLayerNum numLayers = design->getTech()->getLayers().size();
  grObjs.clear();
  grObjs.shrink_to_fit();
  grObjs.resize(numLayers);

  ObjectsByLayer<grBlockObject> allShapes(numLayers);

  for (auto& net : design->getTopBlock()->getNets()) {
    for (auto& shape : net->getGRShapes()) {
      addGRObj(shape.get(), allShapes);
    }
    for (auto& via : net->getGRVias()) {
      addGRObj(via.get(), allShapes);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    grObjs.at(i) = boost::move(RTree<grBlockObject*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

void frRegionQuery::initGRObj()
{
  impl_->initGRObj();
}

void frRegionQuery::printGRObj()
{
  auto& layers = impl_->design->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger->info(DRT,
                        32,
                        "{} grObj region query size = {}.",
                        layerName,
                        impl_->grObjs.at(i).size());
  }
}

void frRegionQuery::print()
{
  auto& layers = impl_->design->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger->info(DRT,
                        33,
                        "{} shape region query size = {}.",
                        layerName,
                        impl_->shapes.at(i).size());
  }
}

void frRegionQuery::printGuide()
{
  auto& layers = impl_->design->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger->info(DRT,
                        36,
                        "{} guide region query size = {}.",
                        layerName,
                        impl_->guides.at(i).size());
  }
}

void frRegionQuery::printDRObj()
{
  auto& layers = impl_->design->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger->info(DRT,
                        34,
                        "{} drObj region query size = {}.",
                        layerName,
                        impl_->drObjs.at(i).size());
  }
}

void frRegionQuery::clearGuides()
{
  for (auto& m : impl_->guides) {
    m.clear();
  }
}

}  // namespace drt
