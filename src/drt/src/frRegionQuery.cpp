/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "frRegionQuery.h"

#include <boost/polygon/polygon.hpp>
#include <iostream>

#include "frDesign.h"
#include "frRTree.h"
#include "global.h"
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

  frDesign* design_;
  Logger* logger_;
  RouterConfiguration* router_cfg_;
  // only for pin shapes, obs and snet
  RTreesByLayer<frBlockObject*> shapes_;
  RTreesByLayer<frGuide*> guides_;
  RTreesByLayer<frNet*> origGuides_;  // non-processed guides;
  RTree<frBlockObject*> grPins_;
  RTreesByLayer<frRPin*> rpins_;  // only for rpins
  // only for gr objs, via only in via layer
  RTreesByLayer<grBlockObject*> grObjs_;
  // only for dr objs, via only in via layer
  RTreesByLayer<frBlockObject*> drObjs_;
  RTreesByLayer<frMarker*> markers_;  // use init()

  Impl() = default;
  void init();
  void initOrigGuide(
      std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& tmpGuides);
  void initGuide();
  void initRPin();
  void initGRPin(std::vector<std::pair<frBlockObject*, Point>>& in);
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
                             Logger* logger,
                             RouterConfiguration* router_cfg)
    : impl_(std::make_unique<Impl>())
{
  impl_->design_ = design;
  impl_->logger_ = logger;
  impl_->router_cfg_ = router_cfg;
}

frRegionQuery::frRegionQuery() : impl_(nullptr)
{
}

frRegionQuery::~frRegionQuery() = default;

frDesign* frRegionQuery::getDesign() const
{
  return impl_->design_;
}

void frRegionQuery::Impl::add(frShape* shape,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
    Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).push_back(std::make_pair(frb, shape));
  } else {
    logger_->error(DRT, 5, "Unsupported region query add.");
  }
}

void frRegionQuery::addDRObj(frShape* shape)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    Rect frb = shape->getBBox();
    impl_->drObjs_.at(shape->getLayerNum()).insert(std::make_pair(frb, shape));
  } else {
    impl_->logger_->error(DRT, 6, "Unsupported region query add.");
  }
}

void frRegionQuery::addMarker(frMarker* in)
{
  Rect frb = in->getBBox();
  impl_->markers_.at(in->getLayerNum()).insert(std::make_pair(frb, in));
}

void frRegionQuery::Impl::addDRObj(frShape* shape,
                                   ObjectsByLayer<frBlockObject>& allShapes)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).push_back(std::make_pair(frb, shape));
  } else {
    logger_->error(DRT, 7, "Unsupported region query add.");
  }
}

void frRegionQuery::removeDRObj(frShape* shape)
{
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect
      || shape->typeId() == frcPatchWire) {
    Rect frb = shape->getBBox();
    impl_->drObjs_.at(shape->getLayerNum()).remove(std::make_pair(frb, shape));
  } else {
    impl_->logger_->error(DRT, 31, "Unsupported region query add.");
  }
}

std::vector<std::pair<frBlockObject*, Rect>> frRegionQuery::getVias(
    frLayerNum layer_num)
{
  std::vector<std::pair<frBlockObject*, Rect>> result;
  result.reserve(impl_->shapes_.at(layer_num).size()
                 + impl_->drObjs_.at(layer_num).size());
  for (auto [box, obj] : impl_->shapes_.at(layer_num)) {
    result.emplace_back(obj, box);
  }
  for (auto [box, obj] : impl_->drObjs_.at(layer_num)) {
    result.emplace_back(obj, box);
  }
  return result;
}

void frRegionQuery::addBlockObj(frBlockObject* obj)
{
  switch (obj->typeId()) {
    case frcInstTerm: {
      auto instTerm = static_cast<frInstTerm*>(obj);
      dbTransform xform = instTerm->getInst()->getDBTransform();
      for (auto& pin : instTerm->getTerm()->getPins()) {
        for (auto& uFig : pin->getFigs()) {
          auto shape = uFig.get();
          Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
              .insert(std::make_pair(frb, instTerm));
        }
      }
      break;
    }
    case frcInstBlockage: {
      auto instBlk = static_cast<frInstBlockage*>(obj);
      dbTransform xform = instBlk->getInst()->getDBTransform();
      auto blk = instBlk->getBlockage();
      auto pin = blk->getPin();
      for (auto& uFig : pin->getFigs()) {
        auto shape = uFig.get();
        if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
          Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
              .insert(std::make_pair(frb, instBlk));
        } else if (shape->typeId() == frcPolygon) {
          // Decompose the polygon to rectangles and store those
          // Convert the frPolygon to a Boost polygon
          std::vector<gtl::point_data<frCoord>> points;
          for (Point pt : ((frPolygon*) shape)->getPoints()) {
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
            Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
            impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
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
      impl_->logger_->error(DRT, 513, "Unsupported region addBlockObj");
  }
}

void frRegionQuery::removeBlockObj(frBlockObject* obj)
{
  switch (obj->typeId()) {
    case frcInstTerm: {
      auto instTerm = static_cast<frInstTerm*>(obj);
      dbTransform xform = instTerm->getInst()->getDBTransform();
      for (auto& pin : instTerm->getTerm()->getPins()) {
        for (auto& uFig : pin->getFigs()) {
          auto shape = uFig.get();
          Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
              .remove(std::make_pair(frb, instTerm));
        }
      }
      break;
    }
    case frcInstBlockage: {
      auto instBlk = static_cast<frInstBlockage*>(obj);
      dbTransform xform = instBlk->getInst()->getDBTransform();
      auto blk = instBlk->getBlockage();
      auto pin = blk->getPin();
      for (auto& uFig : pin->getFigs()) {
        auto shape = uFig.get();
        if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
          Rect frb = shape->getBBox();
          xform.apply(frb);
          impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
              .remove(std::make_pair(frb, instBlk));
        } else if (shape->typeId() == frcPolygon) {
          // Decompose the polygon to rectangles and store those
          // Convert the frPolygon to a Boost polygon
          std::vector<gtl::point_data<frCoord>> points;
          for (Point pt : ((frPolygon*) shape)->getPoints()) {
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
            Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
            impl_->shapes_.at(static_cast<frShape*>(shape)->getLayerNum())
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
      impl_->logger_->error(DRT, 512, "Unsupported region removeBlockObj");
  }
}

void frRegionQuery::addGRObj(grShape* shape)
{
  impl_->addGRObj(shape);
}

void frRegionQuery::Impl::addGRObj(grShape* shape)
{
  if (shape->typeId() == grcPathSeg) {
    Rect frb = shape->getBBox();
    grObjs_.at(shape->getLayerNum()).insert(std::make_pair(frb, shape));
  } else {
    logger_->error(DRT, 8, "Unsupported region query add.");
  }
}

void frRegionQuery::Impl::addGRObj(grVia* via,
                                   ObjectsByLayer<grBlockObject>& allShapes)
{
  Rect frb = via->getBBox();
  allShapes.at(via->getViaDef()->getCutLayerNum())
      .push_back(std::make_pair(frb, via));
}

void frRegionQuery::removeGRObj(grVia* via)
{
  Rect frb = via->getBBox();
  impl_->grObjs_.at(via->getViaDef()->getCutLayerNum())
      .remove(std::make_pair(frb, via));
}

void frRegionQuery::Impl::addGRObj(grShape* shape,
                                   ObjectsByLayer<grBlockObject>& allShapes)
{
  if (shape->typeId() == grcPathSeg) {
    Rect frb = shape->getBBox();
    allShapes.at(shape->getLayerNum()).push_back(std::make_pair(frb, shape));
  } else {
    logger_->error(DRT, 9, "Unsupported region query add.");
  }
}

void frRegionQuery::removeGRObj(grShape* shape)
{
  if (shape->typeId() == grcPathSeg) {
    Rect frb = shape->getBBox();
    impl_->grObjs_.at(shape->getLayerNum()).remove(std::make_pair(frb, shape));
  } else {
    impl_->logger_->error(DRT, 10, "Unsupported region query add.");
  }
}

void frRegionQuery::removeMarker(frMarker* in)
{
  Rect frb = in->getBBox();
  impl_->markers_.at(in->getLayerNum()).remove(std::make_pair(frb, in));
}

void frRegionQuery::Impl::add(frVia* via,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  dbTransform xform = via->getTransform();
  for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getLayer1Num())
          .push_back(std::make_pair(frb, via));
    } else {
      logger_->error(DRT, 11, "Unsupported region query add.");
    }
  }
  for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getLayer2Num())
          .push_back(std::make_pair(frb, via));
    } else {
      logger_->error(DRT, 12, "Unsupported region query add.");
    }
  }
  for (auto& uShape : via->getViaDef()->getCutFigs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(via->getViaDef()->getCutLayerNum())
          .push_back(std::make_pair(frb, via));
    } else {
      logger_->error(DRT, 13, "Unsupported region query add.");
    }
  }
}

void frRegionQuery::addDRObj(frVia* via)
{
  Rect frb = via->getBBox();
  impl_->drObjs_.at(via->getViaDef()->getCutLayerNum())
      .insert(std::make_pair(frb, via));
}

void frRegionQuery::Impl::addDRObj(frVia* via,
                                   ObjectsByLayer<frBlockObject>& allShapes)
{
  Rect frb = via->getBBox();
  allShapes.at(via->getViaDef()->getCutLayerNum())
      .push_back(std::make_pair(frb, via));
}

void frRegionQuery::removeDRObj(frVia* via)
{
  Rect frb = via->getBBox();
  impl_->drObjs_.at(via->getViaDef()->getCutLayerNum())
      .remove(std::make_pair(frb, via));
}

void frRegionQuery::addGRObj(grVia* via)
{
  Rect frb = via->getBBox();
  impl_->grObjs_.at(via->getViaDef()->getCutLayerNum())
      .insert(std::make_pair(frb, via));
}

void frRegionQuery::Impl::add(frInstTerm* instTerm,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  dbTransform xform = instTerm->getInst()->getDBTransform();

  for (auto& pin : instTerm->getTerm()->getPins()) {
    for (auto& uFig : pin->getFigs()) {
      auto shape = uFig.get();
      if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .push_back(std::make_pair(frb, instTerm));
      } else {
        logger_->error(DRT, 14, "Unsupported region query add.");
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
        Rect frb = shape->getBBox();
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .push_back(std::make_pair(frb, term));
      } else {
        logger_->error(DRT, 15, "Unsupported region query add.");
      }
    }
  }
}

void frRegionQuery::Impl::add(frInstBlockage* instBlk,
                              ObjectsByLayer<frBlockObject>& allShapes)
{
  dbTransform xform = instBlk->getInst()->getDBTransform();
  auto blk = instBlk->getBlockage();
  auto pin = blk->getPin();
  for (auto& uFig : pin->getFigs()) {
    auto shape = uFig.get();
    if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
      Rect frb = shape->getBBox();
      xform.apply(frb);
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
          .push_back(std::make_pair(frb, instBlk));
    } else if (shape->typeId() == frcPolygon) {
      // Decompose the polygon to rectangles and store those
      // Convert the frPolygon to a Boost polygon
      std::vector<gtl::point_data<frCoord>> points;
      for (Point pt : ((frPolygon*) shape)->getPoints()) {
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
        Rect box(xl(rect), yl(rect), xh(rect), yh(rect));
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
            .push_back(std::make_pair(box, instBlk));
      }
    } else {
      logger_->error(DRT,
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
      Rect frb = shape->getBBox();
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum())
          .push_back(std::make_pair(frb, blk));
    } else {
      logger_->error(DRT, 17, "Unsupported region query add.");
    }
  }
}

void frRegionQuery::Impl::addGuide(frGuide* guide,
                                   ObjectsByLayer<frGuide>& allShapes)
{
  Rect frb = guide->getBBox();
  for (int i = guide->getBeginLayerNum(); i <= guide->getEndLayerNum(); i++) {
    allShapes.at(i).push_back(std::make_pair(frb, guide));
  }
}

void frRegionQuery::Impl::addRPin(frRPin* rpin,
                                  ObjectsByLayer<frRPin>& allRPins)
{
  frLayerNum layerNum = rpin->getLayerNum();
  Rect frb = rpin->getBBox();
  allRPins.at(layerNum).push_back(std::make_pair(frb, rpin));
}

void frRegionQuery::Impl::addOrigGuide(frNet* net,
                                       const frRect& rect,
                                       ObjectsByLayer<frNet>& allShapes)
{
  Rect frb = rect.getBBox();
  allShapes.at(rect.getLayerNum()).push_back(std::make_pair(frb, net));
}

void frRegionQuery::query(const box_t& boostb,
                          const frLayerNum layerNum,
                          Objects<frBlockObject>& result) const
{
  impl_->shapes_.at(layerNum).query(bgi::intersects(boostb),
                                    back_inserter(result));
}

void frRegionQuery::query(const Rect& box,
                          const frLayerNum layerNum,
                          Objects<frBlockObject>& result) const
{
  impl_->shapes_.at(layerNum).query(bgi::intersects(box),
                                    back_inserter(result));
}

void frRegionQuery::queryRPin(const Rect& box,
                              const frLayerNum layerNum,
                              Objects<frRPin>& result) const
{
  impl_->rpins_.at(layerNum).query(bgi::intersects(box), back_inserter(result));
}

void frRegionQuery::queryGuide(const Rect& box,
                               const frLayerNum layerNum,
                               Objects<frGuide>& result) const
{
  impl_->guides_.at(layerNum).query(bgi::intersects(box),
                                    back_inserter(result));
}

void frRegionQuery::queryGuide(const Rect& box,
                               const frLayerNum layerNum,
                               std::vector<frGuide*>& result) const
{
  Objects<frGuide> temp;
  queryGuide(box, layerNum, temp);
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryGuide(const Rect& box,
                               std::vector<frGuide*>& result) const
{
  Objects<frGuide> temp;
  for (auto& m : impl_->guides_) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryOrigGuide(const Rect& box,
                                   const frLayerNum layerNum,
                                   Objects<frNet>& result) const
{
  impl_->origGuides_.at(layerNum).query(bgi::intersects(box),
                                        back_inserter(result));
}

void frRegionQuery::queryGRPin(const Rect& box,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  impl_->grPins_.query(bgi::intersects(box), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryDRObj(const box_t& boostb,
                               const frLayerNum layerNum,
                               Objects<frBlockObject>& result) const
{
  impl_->drObjs_.at(layerNum).query(bgi::intersects(boostb),
                                    back_inserter(result));
}

void frRegionQuery::queryDRObj(const Rect& box,
                               const frLayerNum layerNum,
                               Objects<frBlockObject>& result) const
{
  impl_->drObjs_.at(layerNum).query(bgi::intersects(box),
                                    back_inserter(result));
}

void frRegionQuery::queryDRObj(const Rect& box,
                               const frLayerNum layerNum,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  impl_->drObjs_.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryDRObj(const Rect& box,
                               std::vector<frBlockObject*>& result) const
{
  Objects<frBlockObject> temp;
  for (auto& m : impl_->drObjs_) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryGRObj(const Rect& box,
                               std::vector<grBlockObject*>& result) const
{
  Objects<grBlockObject> temp;
  for (auto& m : impl_->grObjs_) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryMarker(const Rect& box,
                                const frLayerNum layerNum,
                                std::vector<frMarker*>& result) const
{
  Objects<frMarker> temp;
  impl_->markers_.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::queryMarker(const Rect& box,
                                std::vector<frMarker*>& result) const
{
  Objects<frMarker> temp;
  for (auto& m : impl_->markers_) {
    m.query(bgi::intersects(box), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void frRegionQuery::init()
{
  impl_->init();
}

void frRegionQuery::Impl::init()
{
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  shapes_.clear();
  shapes_.resize(numLayers);

  markers_.clear();
  markers_.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  int cnt = 0;
  for (auto& inst : design_->getTopBlock()->getInsts()) {
    for (auto& instTerm : inst->getInstTerms()) {
      add(instTerm.get(), allShapes);
    }
    for (auto& instBlk : inst->getInstBlockages()) {
      add(instBlk.get(), allShapes);
    }
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt < 1000000) {
        if (cnt % 100000 == 0) {
          logger_->info(DRT, 18, "  Complete {} insts.", cnt);
        }
      } else {
        if (cnt % 1000000 == 0) {
          logger_->info(DRT, 19, "  Complete {} insts.", cnt);
        }
      }
    }
  }
  cnt = 0;
  for (auto& term : design_->getTopBlock()->getTerms()) {
    add(term.get(), allShapes);
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger_->info(DRT, 20, "  Complete {} terms.", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger_->info(DRT, 21, "  Complete {} terms.", cnt);
        }
      }
    }
  }

  cnt = 0;
  for (auto& net : design_->getTopBlock()->getSNets()) {
    for (auto& shape : net->getShapes()) {
      add(shape.get(), allShapes);
    }
    for (auto& via : net->getVias()) {
      add(via.get(), allShapes);
    }
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger_->info(DRT, 22, "  Complete {} snets.", cnt);
      }
    }
  }

  cnt = 0;
  for (auto& blk : design_->getTopBlock()->getBlockages()) {
    add(blk.get(), allShapes);
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger_->info(DRT, 23, "  Complete {} blockages.", cnt);
      }
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    shapes_.at(i) = boost::move(RTree<frBlockObject*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    24,
                    "  Complete {}.",
                    design_->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initOrigGuide(
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& tmpGuides)
{
  impl_->initOrigGuide(tmpGuides);
}

void frRegionQuery::Impl::initOrigGuide(
    std::map<frNet*, std::vector<frRect>, frBlockObjectComp>& tmpGuides)
{
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  origGuides_.clear();
  origGuides_.resize(numLayers);
  ObjectsByLayer<frNet> allShapes(numLayers);

  int cnt = 0;
  for (auto& [net, rects] : tmpGuides) {
    for (auto& rect : rects) {
      addOrigGuide(net, rect, allShapes);
      cnt++;
      if (router_cfg_->VERBOSE > 0) {
        if (cnt < 1000000) {
          if (cnt % 100000 == 0) {
            logger_->info(DRT, 26, "  Complete {} origin guides.", cnt);
          }
        } else {
          if (cnt % 1000000 == 0) {
            logger_->info(DRT, 27, "  Complete {} origin guides.", cnt);
          }
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    origGuides_.at(i) = boost::move(RTree<frNet*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    28,
                    "  Complete {}.",
                    design_->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGuide()
{
  impl_->initGuide();
}

void frRegionQuery::Impl::initGuide()
{
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  guides_.clear();
  guides_.resize(numLayers);
  ObjectsByLayer<frGuide> allGuides(numLayers);

  int cnt = 0;
  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& guide : net->getGuides()) {
      addGuide(guide.get(), allGuides);
    }
    cnt++;
    if (router_cfg_->VERBOSE > 0) {
      if (cnt < 1000000) {
        if (cnt % 100000 == 0) {
          logger_->info(DRT, 29, "  Complete {} nets (guide).", cnt);
        }
      } else {
        if (cnt % 1000000 == 0) {
          logger_->info(DRT, 30, "  Complete {} nets (guide).", cnt);
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    guides_.at(i) = boost::move(RTree<frGuide*>(allGuides.at(i)));
    allGuides.at(i).clear();
    allGuides.at(i).shrink_to_fit();
    if (router_cfg_->VERBOSE > 0) {
      logger_->info(DRT,
                    35,
                    "  Complete {} (guide).",
                    design_->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGRPin(std::vector<std::pair<frBlockObject*, Point>>& in)
{
  impl_->initGRPin(in);
}

void frRegionQuery::Impl::initGRPin(
    std::vector<std::pair<frBlockObject*, Point>>& in)
{
  grPins_.clear();
  Objects<frBlockObject> allGRPins;
  for (auto& [obj, pt] : in) {
    Rect frb(pt.x(), pt.y(), pt.x(), pt.y());
    allGRPins.push_back(std::make_pair(frb, obj));
  }
  in.clear();
  in.shrink_to_fit();
  grPins_ = boost::move(RTree<frBlockObject*>(allGRPins));
}

void frRegionQuery::initRPin()
{
  impl_->initRPin();
}

void frRegionQuery::Impl::initRPin()
{
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  rpins_.clear();
  rpins_.resize(numLayers);

  ObjectsByLayer<frRPin> allRPins(numLayers);

  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& rpin : net->getRPins()) {
      addRPin(rpin.get(), allRPins);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    rpins_.at(i) = boost::move(RTree<frRPin*>(allRPins.at(i)));
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
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  drObjs_.clear();
  drObjs_.shrink_to_fit();
  drObjs_.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  for (auto& net : design_->getTopBlock()->getNets()) {
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
    drObjs_.at(i) = boost::move(RTree<frBlockObject*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

void frRegionQuery::Impl::initGRObj()
{
  const frLayerNum numLayers = design_->getTech()->getLayers().size();
  grObjs_.clear();
  grObjs_.shrink_to_fit();
  grObjs_.resize(numLayers);

  ObjectsByLayer<grBlockObject> allShapes(numLayers);

  for (auto& net : design_->getTopBlock()->getNets()) {
    for (auto& shape : net->getGRShapes()) {
      addGRObj(shape.get(), allShapes);
    }
    for (auto& via : net->getGRVias()) {
      addGRObj(via.get(), allShapes);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    grObjs_.at(i) = boost::move(RTree<grBlockObject*>(allShapes.at(i)));
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
  auto& layers = impl_->design_->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger_->info(DRT,
                         32,
                         "{} grObj region query size = {}.",
                         layerName,
                         impl_->grObjs_.at(i).size());
  }
}

void frRegionQuery::print()
{
  auto& layers = impl_->design_->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger_->info(DRT,
                         33,
                         "{} shape region query size = {}.",
                         layerName,
                         impl_->shapes_.at(i).size());
  }
}

void frRegionQuery::printGuide()
{
  auto& layers = impl_->design_->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger_->info(DRT,
                         36,
                         "{} guide region query size = {}.",
                         layerName,
                         impl_->guides_.at(i).size());
  }
}

void frRegionQuery::printDRObj()
{
  auto& layers = impl_->design_->getTech()->getLayers();
  for (auto [i, layer] : enumerate(layers)) {
    frString layerName;
    layer->getName(layerName);
    impl_->logger_->info(DRT,
                         34,
                         "{} drObj region query size = {}.",
                         layerName,
                         impl_->drObjs_.at(i).size());
  }
}

void frRegionQuery::clearGuides()
{
  for (auto& m : impl_->guides_) {
    m.clear();
  }
}

}  // namespace drt
