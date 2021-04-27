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

#include "gr/FlexGR.h"

using namespace std;
using namespace fr;

frDesign* FlexGRWorkerRegionQuery::getDesign() const
{
  return grWorker_->getDesign();
}

void FlexGRWorkerRegionQuery::add(grConnFig* connFig)
{
  frBox frb;
  box_t boostb;
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    obj->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()),
                   point_t(frb.right(), frb.top()));
    shapes_.at(obj->getLayerNum()).insert(make_pair(boostb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    frTransform xform;
    frPoint origin;
    via->getOrigin(origin);
    xform.set(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getLayer1Num())
            .insert(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getLayer2Num())
            .insert(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .insert(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexGRWorkerRegionQuery::add(
    grConnFig* connFig,
    vector<vector<rq_box_value_t<grConnFig*>>>& allShapes)
{
  frBox frb;
  box_t boostb;
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    obj->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()),
                   point_t(frb.right(), frb.top()));
    allShapes.at(obj->getLayerNum()).push_back(make_pair(boostb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    frTransform xform;
    frPoint origin;
    via->getOrigin(origin);
    xform.set(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        allShapes.at(via->getViaDef()->getLayer1Num())
            .push_back(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        allShapes.at(via->getViaDef()->getLayer2Num())
            .push_back(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        allShapes.at(via->getViaDef()->getCutLayerNum())
            .push_back(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexGRWorkerRegionQuery::remove(grConnFig* connFig)
{
  frBox frb;
  box_t boostb;
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    obj->getBBox(frb);
    boostb = box_t(point_t(frb.left(), frb.bottom()),
                   point_t(frb.right(), frb.top()));
    shapes_.at(obj->getLayerNum()).remove(make_pair(boostb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    frTransform xform;
    frPoint origin;
    via->getOrigin(origin);
    xform.set(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getLayer1Num())
            .remove(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getLayer2Num())
            .remove(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        frb.transform(xform);
        boostb = box_t(point_t(frb.left(), frb.bottom()),
                       point_t(frb.right(), frb.top()));
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .remove(make_pair(boostb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query remove" << endl;
  }
}

void FlexGRWorkerRegionQuery::query(const frBox& box,
                                    const frLayerNum layerNum,
                                    vector<grConnFig*>& result) const
{
  vector<rq_box_value_t<grConnFig*>> temp;
  box_t boostb = box_t(point_t(box.left(), box.bottom()),
                       point_t(box.right(), box.top()));
  shapes_.at(layerNum).query(bgi::intersects(boostb), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void FlexGRWorkerRegionQuery::query(const frBox& box,
                                    const frLayerNum layerNum,
                                    vector<rq_box_value_t<grConnFig*>>& result) const
{
  box_t boostb = box_t(point_t(box.left(), box.bottom()),
                       point_t(box.right(), box.top()));
  shapes_.at(layerNum).query(bgi::intersects(boostb), back_inserter(result));
}

void FlexGRWorkerRegionQuery::init(bool includeExt)
{
  int numLayers = getDesign()->getTech()->getLayers().size();
  shapes_.clear();
  shapes_.resize(numLayers);
  vector<vector<rq_box_value_t<grConnFig*>>> allShapes(numLayers);
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
    shapes_.at(i) = boost::move(
        bgi::rtree<rq_box_value_t<grConnFig*>, bgi::quadratic<16>>(
            allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}
