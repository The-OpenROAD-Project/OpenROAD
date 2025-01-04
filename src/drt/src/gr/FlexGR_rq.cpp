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

namespace drt {

frDesign* FlexGRWorkerRegionQuery::getDesign() const
{
  return grWorker_->getDesign();
}

void FlexGRWorkerRegionQuery::add(grConnFig* connFig)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    Rect frb = obj->getBBox();
    Rect boostr = Rect(frb.xMin(), frb.yMin(), frb.xMax(), frb.yMax());
    shapes_.at(obj->getLayerNum()).insert(std::make_pair(boostr, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer1Num())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add" << std::endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer2Num())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add" << std::endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .insert(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add" << std::endl;
      }
    }
  } else {
    std::cout << "Error: unsupported region query add" << std::endl;
  }
}

void FlexGRWorkerRegionQuery::add(
    grConnFig* connFig,
    std::vector<std::vector<rq_box_value_t<grConnFig*>>>& allShapes)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    Rect frb = obj->getBBox();
    allShapes.at(obj->getLayerNum()).push_back(std::make_pair(frb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getLayer1Num())
            .push_back(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query add" << std::endl;
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
        std::cout << "Error: unsupported region query add" << std::endl;
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
        std::cout << "Error: unsupported region query add" << std::endl;
      }
    }
  } else {
    std::cout << "Error: unsupported region query add" << std::endl;
  }
}

void FlexGRWorkerRegionQuery::remove(grConnFig* connFig)
{
  if (connFig->typeId() == grcPathSeg) {
    auto obj = static_cast<grShape*>(connFig);
    Rect frb = obj->getBBox();
    shapes_.at(obj->getLayerNum()).remove(std::make_pair(frb, obj));
  } else if (connFig->typeId() == grcVia) {
    auto via = static_cast<grVia*>(connFig);
    dbTransform xform = via->getTransform();
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer1Num())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove" << std::endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getLayer2Num())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove" << std::endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        Rect frb = shape->getBBox();
        xform.apply(frb);
        shapes_.at(via->getViaDef()->getCutLayerNum())
            .remove(std::make_pair(frb, via));
      } else {
        std::cout << "Error: unsupported region query remove" << std::endl;
      }
    }
  } else {
    std::cout << "Error: unsupported region query remove" << std::endl;
  }
}

void FlexGRWorkerRegionQuery::query(const Rect& box,
                                    const frLayerNum layerNum,
                                    std::vector<grConnFig*>& result) const
{
  std::vector<rq_box_value_t<grConnFig*>> temp;
  box_t boostb
      = box_t(point_t(box.xMin(), box.yMin()), point_t(box.xMax(), box.yMax()));
  shapes_.at(layerNum).query(bgi::intersects(boostb), back_inserter(temp));
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void FlexGRWorkerRegionQuery::query(
    const Rect& box,
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
