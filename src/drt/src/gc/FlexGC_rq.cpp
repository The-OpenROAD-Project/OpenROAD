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

#include <iostream>

#include "frRTree.h"
#include "gc/FlexGC_impl.h"

using namespace std;
using namespace fr;

struct FlexGCWorkerRegionQuery::Impl
{
  void addPolygonEdge(
      gcSegment* edge,
      std::vector<std::vector<std::pair<segment_t, gcSegment*>>>& allShapes);
  void addMaxRectangle(
      gcRect* rect,
      std::vector<std::vector<rq_box_value_t<gcRect*>>>& allShapes);
  void addSpcRectangle(
      gcRect* rect,
      std::vector<std::vector<rq_box_value_t<gcRect>>>& allShapes);
  void init(int numLayers);

  FlexGCWorker* gcWorker_;
  std::vector<bgi::rtree<std::pair<segment_t, gcSegment*>, bgi::quadratic<16>>>
      polygon_edges_;  // merged
  std::vector<bgi::rtree<rq_box_value_t<gcRect*>, bgi::quadratic<16>>>
      max_rectangles_;  // merged
  std::vector<bgi::rtree<rq_box_value_t<gcRect>, bgi::quadratic<16>>>
      spc_rectangles_;  // rects that require nondefault spacing that intersects
                        // tapered max rects
};

FlexGCWorkerRegionQuery::FlexGCWorkerRegionQuery(FlexGCWorker* in)
    : impl_(make_unique<Impl>())
{
  impl_->gcWorker_ = in;
}
FlexGCWorkerRegionQuery::~FlexGCWorkerRegionQuery() = default;

FlexGCWorker* FlexGCWorkerRegionQuery::getGCWorker() const
{
  return impl_->gcWorker_;
}

void FlexGCWorkerRegionQuery::addPolygonEdge(gcSegment* edge)
{
  segment_t boosts(point_t(edge->low().x(), edge->low().y()),
                   point_t(edge->high().x(), edge->high().y()));
  impl_->polygon_edges_[edge->getLayerNum()].insert(make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::Impl::addPolygonEdge(
    gcSegment* edge,
    vector<vector<pair<segment_t, gcSegment*>>>& allShapes)
{
  segment_t boosts(point_t(edge->low().x(), edge->low().y()),
                   point_t(edge->high().x(), edge->high().y()));
  allShapes[edge->getLayerNum()].push_back(make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::addMaxRectangle(gcRect* rect)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  impl_->max_rectangles_[rect->getLayerNum()].insert(make_pair(boostb, rect));
}

void FlexGCWorkerRegionQuery::addSpcRectangle(gcRect* rect)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  impl_->spc_rectangles_[rect->getLayerNum()].insert(make_pair(boostb, *rect));
}

void FlexGCWorkerRegionQuery::Impl::addMaxRectangle(
    gcRect* rect,
    vector<vector<rq_box_value_t<gcRect*>>>& allShapes)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  allShapes[rect->getLayerNum()].push_back(make_pair(boostb, rect));
}

void FlexGCWorkerRegionQuery::Impl::addSpcRectangle(
    gcRect* rect,
    vector<vector<rq_box_value_t<gcRect>>>& allShapes)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  allShapes[rect->getLayerNum()].push_back(make_pair(boostb, *rect));
}

void FlexGCWorkerRegionQuery::removePolygonEdge(gcSegment* edge)
{
  segment_t boosts(point_t(edge->low().x(), edge->low().y()),
                   point_t(edge->high().x(), edge->high().y()));
  impl_->polygon_edges_[edge->getLayerNum()].remove(make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::removeMaxRectangle(gcRect* rect)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  impl_->max_rectangles_[rect->getLayerNum()].remove(make_pair(boostb, rect));
}

void FlexGCWorkerRegionQuery::removeSpcRectangle(gcRect* rect)
{
  box_t boostb(point_t(gtl::xl(*rect), gtl::yl(*rect)),
               point_t(gtl::xh(*rect), gtl::yh(*rect)));
  impl_->spc_rectangles_[rect->getLayerNum()].remove(make_pair(boostb, *rect));
}

void FlexGCWorkerRegionQuery::queryPolygonEdge(
    const box_t& box,
    const frLayerNum layerNum,
    vector<pair<segment_t, gcSegment*>>& result) const
{
  impl_->polygon_edges_[layerNum].query(bgi::intersects(box),
                                        back_inserter(result));
}

void FlexGCWorkerRegionQuery::queryPolygonEdge(
    const frBox& box,
    const frLayerNum layerNum,
    vector<pair<segment_t, gcSegment*>>& result) const
{
  box_t boostb(point_t(box.left(), box.bottom()),
               point_t(box.right(), box.top()));
  queryPolygonEdge(boostb, layerNum, result);
}

void FlexGCWorkerRegionQuery::queryMaxRectangle(
    const box_t& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<gcRect*>>& result) const
{
  impl_->max_rectangles_[layerNum].query(bgi::intersects(box),
                                         back_inserter(result));
}

void FlexGCWorkerRegionQuery::querySpcRectangle(
    const box_t& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<gcRect>>& result) const
{
  impl_->spc_rectangles_[layerNum].query(bgi::intersects(box),
                                         back_inserter(result));
}

void FlexGCWorkerRegionQuery::queryMaxRectangle(
    const frBox& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<gcRect*>>& result) const
{
  box_t boostb(point_t(box.left(), box.bottom()),
               point_t(box.right(), box.top()));
  queryMaxRectangle(boostb, layerNum, result);
}

void FlexGCWorkerRegionQuery::queryMaxRectangle(
    const gtl::rectangle_data<frCoord>& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<gcRect*>>& result) const
{
  box_t boostb(point_t(gtl::xl(box), gtl::yl(box)),
               point_t(gtl::xh(box), gtl::yh(box)));
  queryMaxRectangle(boostb, layerNum, result);
}

void FlexGCWorkerRegionQuery::init(int numLayers)
{
  impl_->init(numLayers);
}

void FlexGCWorkerRegionQuery::Impl::init(int numLayers)
{
  polygon_edges_.clear();
  polygon_edges_.resize(numLayers);
  max_rectangles_.clear();
  max_rectangles_.resize(numLayers);
  spc_rectangles_.clear();
  spc_rectangles_.resize(numLayers);
  vector<vector<pair<segment_t, gcSegment*>>> allPolygonEdges(numLayers);
  vector<vector<rq_box_value_t<gcRect*>>> allMaxRectangles(numLayers);
  vector<vector<rq_box_value_t<gcRect>>> allSpcRectangles(numLayers);

  int cntPolygonEdge = 0;
  int cntMaxRectangle = 0;
  for (auto& net : gcWorker_->getNets()) {
    for (auto& pins : net->getPins()) {
      for (auto& pin : pins) {
        for (auto& edges : pin->getPolygonEdges()) {
          for (auto& edge : edges) {
            addPolygonEdge(edge.get(), allPolygonEdges);
            cntPolygonEdge++;
          }
        }
        for (auto& rect : pin->getMaxRectangles()) {
          addMaxRectangle(rect.get(), allMaxRectangles);
          cntMaxRectangle++;
        }
      }
    }
    for (auto& spcRect : net->getSpecialSpcRects())
      addSpcRectangle(spcRect.get(), allSpcRectangles);
  }

  int cntRTPolygonEdge = 0;
  int cntRTMaxRectangle = 0;
  for (int i = 0; i < numLayers; i++) {
    polygon_edges_[i] = boost::move(
        bgi::rtree<pair<segment_t, gcSegment*>, bgi::quadratic<16>>(
            allPolygonEdges[i]));
    max_rectangles_[i]
        = boost::move(bgi::rtree<rq_box_value_t<gcRect*>, bgi::quadratic<16>>(
            allMaxRectangles[i]));
    spc_rectangles_[i]
        = boost::move(bgi::rtree<rq_box_value_t<gcRect>, bgi::quadratic<16>>(
            allSpcRectangles[i]));
    cntRTPolygonEdge += polygon_edges_[i].size();
    cntRTMaxRectangle += max_rectangles_[i].size();
  }
}

void FlexGCWorkerRegionQuery::addToRegionQuery(gcNet* net)
{
  for (auto& pins : net->getPins()) {
    for (auto& pin : pins) {
      for (auto& edges : pin->getPolygonEdges()) {
        for (auto& edge : edges) {
          addPolygonEdge(edge.get());
        }
      }
      for (auto& rect : pin->getMaxRectangles()) {
        addMaxRectangle(rect.get());
      }
    }
  }
  for (auto& spcR : net->getSpecialSpcRects()) {
      addSpcRectangle(spcR.get());
  }
}

void FlexGCWorkerRegionQuery::removeFromRegionQuery(gcNet* net)
{
  for (auto& pins : net->getPins()) {
    for (auto& pin : pins) {
      for (auto& edges : pin->getPolygonEdges()) {
        for (auto& edge : edges) {
          removePolygonEdge(edge.get());
        }
      }
      for (auto& rect : pin->getMaxRectangles()) {
        removeMaxRectangle(rect.get());
      }
    }
  }
  for (auto& spcR: net->getSpecialSpcRects()) {
      removeSpcRectangle(spcR.get());
  }
}
