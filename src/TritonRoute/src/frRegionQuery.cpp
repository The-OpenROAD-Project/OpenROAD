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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <boost/polygon/polygon.hpp>
#include "global.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "frRTree.h"
using namespace std;
using namespace fr;
namespace gtl = boost::polygon;

struct frRegionQuery::Impl
{
    template<typename T>
    using rtree = bgi::rtree<rq_box_value_t<T*>, bgi::quadratic<16>>;

    template<typename T>
    using ObjectsByLayer = std::vector<Objects<T>>;

    frDesign*         design;
    Logger*           logger;
    std::vector<rtree<frBlockObject>> shapes; // only for pin shapes, obs and snet
    std::vector<rtree<frGuide>>       guides;
    std::vector<rtree<frNet>>         origGuides; // non-processed guides;
    rtree<frBlockObject>              grPins;
    std::vector<rtree<frRPin>>        rpins; // only for rpins
    std::vector<rtree<grBlockObject>> grObjs; // only for gr objs, via only in via layer
    std::vector<rtree<frBlockObject>> drObjs; // only for dr objs, via only in via layer
    std::vector<rtree<frMarker>>      markers; // use init()  

    void init(frLayerNum numLayers);
    void initOrigGuide(frLayerNum numLayers, map<frNet*, vector<frRect>, frBlockObjectComp> &tmpGuides);
    void initGuide(frLayerNum numLayers);
    void initRPin(frLayerNum numLayers);
    void initGRPin(vector<pair<frBlockObject*, frPoint> > &in);
    void initDRObj(frLayerNum numLayers);
    void initGRObj(frLayerNum numLayers);
  
    void add(frShape* in,    ObjectsByLayer<frBlockObject> &allShapes);
    void add(frVia* in,      ObjectsByLayer<frBlockObject> &allShapes);
    void add(frInstTerm* in, ObjectsByLayer<frBlockObject> &allShapes);
    void add(frTerm* in,     ObjectsByLayer<frBlockObject> &allShapes);
    void add(frBlockage* in, ObjectsByLayer<frBlockObject> &allShapes);
    void add(frInstBlockage* in, ObjectsByLayer<frBlockObject> &allShapes);
    void addGuide(frGuide* in, ObjectsByLayer<frGuide> &allShapes);
    void addOrigGuide(frNet* net, const frRect &rect, ObjectsByLayer<frNet> &allShapes);
    void addDRObj(frShape* in, ObjectsByLayer<frBlockObject> &allShapes);
    void addDRObj(frVia* in,   ObjectsByLayer<frBlockObject> &allShapes);
    void addRPin(frRPin *rpin, std::vector<std::vector<rq_box_value_t<frRPin*>>> &allRPins);
    void addGRObj(grShape* in, std::vector<std::vector<rq_box_value_t<grBlockObject*>>> &allShapes);
    void addGRObj(grVia* in, std::vector<std::vector<rq_box_value_t<grBlockObject*>>> &allShapes);
    void addGRObj(grShape* in);
    void addGRObj(grVia* in);
};

frRegionQuery::frRegionQuery(frDesign* designIn, Logger* logger)
  : impl_(make_unique<Impl>())
{
  impl_->design = designIn;
  impl_->logger = logger;
}
frRegionQuery::~frRegionQuery() = default;

frDesign* frRegionQuery::getDesign() const {
  return impl_->design;
}

void frRegionQuery::Impl::add(frShape* shape, ObjectsByLayer<frBlockObject> &allShapes) {
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
    frBox frb;
    shape->getBBox(frb);
    allShapes.at(shape->getLayerNum()).push_back(make_pair(frb, shape));
  } else {
    logger->error(DRT, 5, "Unsupported region query add");
  }
}

void frRegionQuery::addDRObj(frShape* shape) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect || shape->typeId() == frcPatchWire) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    impl_->drObjs.at(shape->getLayerNum()).insert(make_pair(boostb, shape));
  } else {
    impl_->logger->error(DRT, 6, "Unsupported region query add");
  }
}

void frRegionQuery::addMarker(frMarker* in) {
  frBox frb;
  box_t boostb;
  in->getBBox(frb);
  boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->markers.at(in->getLayerNum()).insert(make_pair(boostb, in));
}

void frRegionQuery::Impl::addDRObj(frShape* shape, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect || shape->typeId() == frcPatchWire) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    allShapes.at(shape->getLayerNum()).push_back(make_pair(boostb, shape));
  } else {
    logger->error(DRT, 7, "Unsupported region query add");
  }
}

void frRegionQuery::removeDRObj(frShape* shape) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect || shape->typeId() == frcPatchWire) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    impl_->drObjs.at(shape->getLayerNum()).remove(make_pair(boostb, shape));
  } else {
    impl_->logger->error(DRT, 31, "Unsupported region query add");
  }
}

void frRegionQuery::addGRObj(grShape* shape) {
  impl_->addGRObj(shape);
}

void frRegionQuery::Impl::addGRObj(grShape* shape) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == grcPathSeg) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    grObjs.at(shape->getLayerNum()).insert(make_pair(boostb, shape));
  } else {
    logger->error(DRT, 8, "Unsupported region query add");
  }
}

void frRegionQuery::Impl::addGRObj(grVia *via, vector<vector<rq_box_value_t<grBlockObject*>>> &allShapes) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  allShapes.at(via->getViaDef()->getCutLayerNum()).push_back(make_pair(boostb, via));
}

void frRegionQuery::removeGRObj(grVia* via) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->grObjs.at(via->getViaDef()->getCutLayerNum()).remove(make_pair(boostb, via));
}

void frRegionQuery::Impl::addGRObj(grShape* shape, vector<vector<rq_box_value_t<grBlockObject*>>> &allShapes) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == grcPathSeg) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    allShapes.at(shape->getLayerNum()).push_back(make_pair(boostb, shape));
  } else {
    logger->error(DRT, 9, "Unsupported region query add");
  }
}

void frRegionQuery::removeGRObj(grShape* shape) {
  frBox frb;
  box_t boostb;
  if (shape->typeId() == grcPathSeg) {
    shape->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
    impl_->grObjs.at(shape->getLayerNum()).remove(make_pair(boostb, shape));
  } else {
    impl_->logger->error(DRT, 10, "Unsupported region query add");
  }
}

void frRegionQuery::removeMarker(frMarker* in) {
  frBox frb;
  box_t boostb;
  in->getBBox(frb);
  boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->markers.at(in->getLayerNum()).remove(make_pair(boostb, in));
}

void frRegionQuery::Impl::add(frVia* via, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  frTransform xform;
  frPoint origin;
  via->getOrigin(origin);
  xform.set(origin);
  box_t boostb;
  for (auto &uShape: via->getViaDef()->getLayer1Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      shape->getBBox(frb);
      frb.transform(xform);
      boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
      allShapes.at(via->getViaDef()->getLayer1Num()).push_back(make_pair(boostb, via));
    } else {
      logger->error(DRT, 11, "Unsupported region query add");
    }
  }
  for (auto &uShape: via->getViaDef()->getLayer2Figs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      shape->getBBox(frb);
      frb.transform(xform);
      boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
      allShapes.at(via->getViaDef()->getLayer2Num()).push_back(make_pair(boostb, via));
    } else {
      logger->error(DRT, 12, "Unsupported region query add");
    }
  }
  for (auto &uShape: via->getViaDef()->getCutFigs()) {
    auto shape = uShape.get();
    if (shape->typeId() == frcRect) {
      shape->getBBox(frb);
      frb.transform(xform);
      boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
      allShapes.at(via->getViaDef()->getCutLayerNum()).push_back(make_pair(boostb, via));
    } else {
      logger->error(DRT, 13, "Unsupported region query add");
    }
  }
}

void frRegionQuery::addDRObj(frVia* via) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->drObjs.at(via->getViaDef()->getCutLayerNum()).insert(make_pair(boostb, via));
}

void frRegionQuery::Impl::addDRObj(frVia* via, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  allShapes.at(via->getViaDef()->getCutLayerNum()).push_back(make_pair(boostb, via));
}

void frRegionQuery::removeDRObj(frVia* via) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->drObjs.at(via->getViaDef()->getCutLayerNum()).remove(make_pair(boostb, via));
}

void frRegionQuery::addGRObj(grVia* via) {
  frBox frb;
  via->getBBox(frb);
  box_t boostb(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  impl_->grObjs.at(via->getViaDef()->getCutLayerNum()).insert(make_pair(boostb, via));
}

void frRegionQuery::Impl::add(frInstTerm* instTerm, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  box_t boostb;

  frTransform xform;
  instTerm->getInst()->getUpdatedXform(xform);

  for (auto &pin: instTerm->getTerm()->getPins()) {
    for (auto &uFig: pin->getFigs()) {
      auto shape = uFig.get();
      if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum()).push_back(make_pair(boostb, instTerm));
      } else {
        logger->error(DRT, 14, "Unsupported region query add");
      }
    }
  }
}

void frRegionQuery::Impl::add(frTerm* term, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  box_t boostb;
  for (auto &pin: term->getPins()) {
    for (auto &uFig: pin->getFigs()) {
      auto shape = uFig.get();
      if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
        shape->getBBox(frb);
        boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum()).push_back(make_pair(boostb, term));
      } else {
        logger->error(DRT, 15, "Unsupported region query add");
      }
    }
  }
}

void frRegionQuery::Impl::add(frInstBlockage* instBlk, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  box_t boostb;

  frTransform xform;
  instBlk->getInst()->getUpdatedXform(xform);
  auto blk = instBlk->getBlockage();
  auto pin = blk->getPin();
  for (auto &uFig: pin->getFigs()) {
    auto shape = uFig.get();
    if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
      shape->getBBox(frb);
      frb.transform(xform);
      boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum()).push_back(make_pair(boostb, instBlk));
    } else if (shape->typeId() == frcPolygon) {
      // Decompose the polygon to rectangles and store those
      // Convert the frPolygon to a Boost polygon
      vector<gtl::point_data<frCoord>> points;
      for (auto pt: ((frPolygon *) shape)->getPoints()) {
        pt.transform(xform);
        points.push_back({pt.x(), pt.y()});
      }
      gtl::polygon_90_data<frCoord> poly;
      poly.set(points.begin(), points.end());
      // Add the polygon to a polygon set
      gtl::polygon_90_set_data<frCoord> polySet;
      {
        using namespace boost::polygon::operators;
        polySet += poly;
      }
      // Decompose the polygon set to rectanges
      vector<gtl::rectangle_data<frCoord>> rects;
      polySet.get_rectangles(rects);
      // Store the rectangles with this blockage
      for (auto& rect : rects) {
        frBox box(xl(rect), yl(rect), xh(rect), yh(rect));
        allShapes.at(static_cast<frShape*>(shape)->getLayerNum()).push_back(make_pair(box, instBlk));
      }
    } else {
      logger->error(DRT, 16,
                    "Unsupported region query add of blockage in instance {}",
                    instBlk->getInst()->getName());
    }
  }
}

void frRegionQuery::Impl::add(frBlockage* blk, ObjectsByLayer<frBlockObject> &allShapes) {
  frBox frb;
  box_t boostb;
  auto pin = blk->getPin();
  for (auto &uFig: pin->getFigs()) {
    auto shape = uFig.get();
    if (shape->typeId() == frcPathSeg || shape->typeId() == frcRect) {
      shape->getBBox(frb);
      boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
      allShapes.at(static_cast<frShape*>(shape)->getLayerNum()).push_back(make_pair(boostb, blk));
    } else {
      logger->error(DRT, 17, "Unsupported region query add");
    }
  }
}

void frRegionQuery::Impl::addGuide(frGuide* guide, ObjectsByLayer<frGuide> &allShapes) {
  frBox frb;
  box_t boostb;
  guide->getBBox(frb);
  boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  for (int i = guide->getBeginLayerNum(); i <= guide->getEndLayerNum(); i++) {
    allShapes.at(i).push_back(make_pair(boostb, guide));
  }
}

void frRegionQuery::Impl::addRPin(frRPin *rpin, vector<vector<rq_box_value_t<frRPin*>>> &allRPins) {
  frBox frb;
  box_t boostb;
  frLayerNum layerNum = rpin->getLayerNum();
  rpin->getBBox(frb);
  boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  allRPins.at(layerNum).push_back(make_pair(boostb, rpin));
}

void frRegionQuery::Impl::addOrigGuide(frNet* net, const frRect &rect, ObjectsByLayer<frNet> &allShapes) {
  frBox frb;
  rect.getBBox(frb);
  box_t boostb;
  boostb = box_t(point_t(frb.left(), frb.bottom()), point_t(frb.right(), frb.top()));
  allShapes.at(rect.getLayerNum()).push_back(make_pair(boostb, net));
}

void frRegionQuery::query(const frBox &box, frLayerNum layerNum, Objects<frBlockObject> &result) {
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->shapes.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void frRegionQuery::queryRPin(const frBox &box, frLayerNum layerNum, Objects<frRPin> &result) {
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->rpins.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void frRegionQuery::queryGuide(const frBox &box, frLayerNum layerNum, Objects<frGuide> &result) {
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->guides.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void frRegionQuery::queryGuide(const frBox &box, frLayerNum layerNum, vector<frGuide*> &result) {
  Objects<frGuide> temp;
  queryGuide(box, layerNum, temp);
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryGuide(const frBox &box, vector<frGuide*> &result) {
  Objects<frGuide> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  for (auto &m: impl_->guides) {
    m.query(bgi::intersects(boostb), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryOrigGuide(const frBox &box, frLayerNum layerNum, Objects<frNet> &result) {
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->origGuides.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void frRegionQuery::queryGRPin(const frBox &box, vector<frBlockObject*> &result) {
  Objects<frBlockObject> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->grPins.query(bgi::intersects(boostb), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryDRObj(const frBox &box, frLayerNum layerNum, Objects<frBlockObject> &result) {
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->drObjs.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void frRegionQuery::queryDRObj(const frBox &box, frLayerNum layerNum, vector<frBlockObject*> &result) {
  Objects<frBlockObject> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->drObjs.at(layerNum).query(bgi::intersects(boostb), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryDRObj(const frBox &box, vector<frBlockObject*> &result) {
  Objects<frBlockObject> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  for (auto &m: impl_->drObjs) {
    m.query(bgi::intersects(boostb), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryGRObj(const frBox &box, vector<grBlockObject*> &result) {
  Objects<grBlockObject> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  for (auto &m: impl_->grObjs) {
    m.query(bgi::intersects(boostb), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryMarker(const frBox &box, frLayerNum layerNum, vector<frMarker*> &result) {
  Objects<frMarker> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  impl_->markers.at(layerNum).query(bgi::intersects(boostb), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::queryMarker(const frBox &box, vector<frMarker*> &result) {
  Objects<frMarker> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()), point_t(box.right(), box.top()));
  for (auto &m: impl_->markers) {
    m.query(bgi::intersects(boostb), back_inserter(temp));
  }
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto &kv) {return kv.second;});
}

void frRegionQuery::init(frLayerNum numLayers) {
  impl_->init(numLayers);
}

void frRegionQuery::Impl::init(frLayerNum numLayers) {
  shapes.clear();
  shapes.resize(numLayers);

  markers.clear();
  markers.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  int cnt = 0;
  for (auto &inst: design->getTopBlock()->getInsts()) {
    for (auto &instTerm: inst->getInstTerms()) {
      add(instTerm.get(), allShapes);
    }
    for (auto &instBlk: inst->getInstBlockages()) {
      add(instBlk.get(), allShapes);
    }
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger->info(DRT, 18, "  complete {} insts", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 19, "  complete {} insts", cnt);
        }
      }
    }
  }
  cnt = 0;
  for (auto &term: design->getTopBlock()->getTerms()) {
    add(term.get(), allShapes);
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger->info(DRT, 20, "  complete {} terms", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 21, "  complete {} terms", cnt);
        }
      }
    }
  }
  /*
  cnt = 0;
  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &shape: net->getShapes()) {
      add(shape.get(), allShapes);
    }
    for (auto &via: net->getVias()) {
      add(via.get(), allShapes);
    }
    cnt++;
    if (VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        cout <<"  complete " <<cnt <<" nets" <<endl;
      }
    }
  }
  */
  cnt = 0;
  for (auto &net: design->getTopBlock()->getSNets()) {
    for (auto &shape: net->getShapes()) {
      add(shape.get(), allShapes);
    }
    for (auto &via: net->getVias()) {
      add(via.get(), allShapes);
    }
    cnt++;
    if (VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger->info(DRT, 22, "  complete {} snets", cnt);
      }
    }
  }

  cnt = 0;
  for (auto &blk: design->getTopBlock()->getBlockages()) {
    add(blk.get(), allShapes);
    cnt++;
    if (VERBOSE > 0) {
      if (cnt % 10000 == 0) {
        logger->info(DRT, 23, "  complete {} blockages", cnt);
      }
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    shapes.at(i) = boost::move(rtree<frBlockObject>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (VERBOSE > 0) {
      logger->info(DRT, 24, "  complete {}", 
                   design->getTech()->getLayer(i)->getName());
    }
  }

}

void frRegionQuery::initOrigGuide(frLayerNum numLayers, map<frNet*, vector<frRect>, frBlockObjectComp> &tmpGuides) {
  impl_->initOrigGuide(numLayers, tmpGuides);
}

void frRegionQuery::Impl::initOrigGuide(frLayerNum numLayers, map<frNet*, vector<frRect>, frBlockObjectComp> &tmpGuides) {
  origGuides.clear();
  origGuides.resize(numLayers);
  ObjectsByLayer<frNet> allShapes(numLayers);

  int cnt = 0;
  for (auto &[net, rects]: tmpGuides) {
    for (auto &rect: rects) {
      addOrigGuide(net, rect, allShapes);
      cnt++;
      if (VERBOSE > 0) {
        if (cnt < 100000) {
          if (cnt % 10000 == 0) {
            logger->info(DRT, 26, "  complete {} orig guides", cnt);
          }
        } else {
          if (cnt % 100000 == 0) {
            logger->info(DRT, 27, "  complete {} orig guides", cnt);
          }
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    origGuides.at(i) = boost::move(rtree<frNet>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
    if (VERBOSE > 0) {
      logger->info(DRT, 28, "  complete {}", 
                   design->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGuide(frLayerNum numLayers) {
  impl_->initGuide(numLayers);
}

void frRegionQuery::Impl::initGuide(frLayerNum numLayers) {
  guides.clear();
  guides.resize(numLayers);
  ObjectsByLayer<frGuide> allGuides(numLayers);

  int cnt = 0;
  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &guide: net->getGuides()) {
      addGuide(guide.get(), allGuides);
    }
    cnt++;
    if (VERBOSE > 0) {
      if (cnt < 100000) {
        if (cnt % 10000 == 0) {
          logger->info(DRT, 29, "  complete {} nets (guide)", cnt);
        }
      } else {
        if (cnt % 100000 == 0) {
          logger->info(DRT, 30, "  complete {} nets (guide)", cnt);
        }
      }
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    guides.at(i) = boost::move(rtree<frGuide>(allGuides.at(i)));
    allGuides.at(i).clear();
    allGuides.at(i).shrink_to_fit();
    if (VERBOSE > 0) {
      logger->info(DRT, 35, "  complete {} (guide)",
                   design->getTech()->getLayer(i)->getName());
    }
  }
}

void frRegionQuery::initGRPin(vector<pair<frBlockObject*, frPoint> > &in) {
  impl_->initGRPin(in);
}

void frRegionQuery::Impl::initGRPin(vector<pair<frBlockObject*, frPoint> > &in) {
  grPins.clear();
  Objects<frBlockObject> allGRPins;
  box_t boostb;
  for (auto &[obj, pt]: in) {
    boostb = box_t(point_t(pt.x(), pt.y()), point_t(pt.x(), pt.y()));
    allGRPins.push_back(make_pair(boostb, obj));
  }
  in.clear();
  in.shrink_to_fit();
  grPins = boost::move(rtree<frBlockObject>(allGRPins));
}

void frRegionQuery::initRPin(frLayerNum numLayers) {
  impl_->initRPin(numLayers);
}

void frRegionQuery::Impl::initRPin(frLayerNum numLayers) {
  rpins.clear();
  rpins.resize(numLayers);

  vector<vector<rq_box_value_t<frRPin*>>> allRPins(numLayers);

  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &rpin: net->getRPins()) {
      addRPin(rpin.get(), allRPins);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    rpins.at(i) = boost::move(rtree<frRPin>(allRPins.at(i)));
    allRPins.at(i).clear();
    allRPins.at(i).shrink_to_fit();
  }
}

void frRegionQuery::initDRObj(frLayerNum numLayers) {
  impl_->initDRObj(numLayers);
}

void frRegionQuery::Impl::initDRObj(frLayerNum numLayers) {
  drObjs.clear();
  drObjs.resize(numLayers);

  ObjectsByLayer<frBlockObject> allShapes(numLayers);

  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &shape: net->getShapes()) {
      addDRObj(shape.get(), allShapes);
    }
    for (auto &via: net->getVias()) {
      addDRObj(via.get(), allShapes);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    drObjs.at(i) = boost::move(rtree<frBlockObject>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }

}

void frRegionQuery::Impl::initGRObj(frLayerNum numLayers) {
  grObjs.clear();
  grObjs.shrink_to_fit();
  grObjs.resize(numLayers);

  vector<vector<rq_box_value_t<grBlockObject*>>> allShapes(numLayers);

  for (auto &net: design->getTopBlock()->getNets()) {
    for (auto &shape: net->getGRShapes()) {
      addGRObj(shape.get(), allShapes);
    }
    for (auto &via: net->getGRVias()) {
      addGRObj(via.get(), allShapes);
    }
  }

  for (auto i = 0; i < numLayers; i++) {
    grObjs.at(i) = boost::move(bgi::rtree<rq_box_value_t<grBlockObject*>, bgi::quadratic<16> >(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

void frRegionQuery::initGRObj(frLayerNum numLayers) {
  impl_->initGRObj(numLayers);
}

void frRegionQuery::printGRObj() {
  for (int i = 0; i < (int)(impl_->design->getTech()->getLayers().size()); i++) {
    frString layerName;
    impl_->design->getTech()->getLayers().at(i)->getName(layerName);
    impl_->logger->info(DRT, 32, "{} grObj region query size = {}", layerName,
                        impl_->grObjs.at(i).size());
  }
}

void frRegionQuery::print() {
  auto& layers = impl_->design->getTech()->getLayers();
  for (int i = 0; i < (int)(layers.size()); i++) {
    frString layerName;
    layers.at(i)->getName(layerName);
    impl_->logger->info(DRT, 33, "{} shape region query size = {}", layerName,
                        impl_->shapes.at(i).size());
  }
}

void frRegionQuery::printGuide() {
  auto& layers = impl_->design->getTech()->getLayers();
  for (int i = 0; i < (int)(layers.size()); i++) {
    frString layerName;
    layers.at(i)->getName(layerName);
    impl_->logger->info(DRT, 36, "{} guide region query size = {}", layerName,
                        impl_->guides.at(i).size());
  }
}

void frRegionQuery::printDRObj() {
  auto& layers = impl_->design->getTech()->getLayers();
  for (int i = 0; i < (int)(layers.size()); i++) {
    frString layerName;
    layers.at(i)->getName(layerName);
    impl_->logger->info(DRT, 34, "{} drObj region query size = {}", layerName,
                        impl_->drObjs.at(i).size());
  }
}

void frRegionQuery::clearGuides() {
  for (auto &m: impl_->guides) {
    m.clear();
  }
}
