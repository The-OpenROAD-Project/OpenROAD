// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "frRTree.h"
#include "gc/FlexGC_impl.h"

namespace drt {

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
  std::vector<RTree<gcSegment*, segment_t>> polygon_edges_;  // merged
  std::vector<RTree<gcRect*>> max_rectangles_;               // merged
  std::vector<RTree<gcRect>>
      spc_rectangles_;  // rects that require nondefault spacing that intersects
                        // tapered max rects

 private:
};

FlexGCWorkerRegionQuery::FlexGCWorkerRegionQuery(FlexGCWorker* in)
    : impl_(std::make_unique<Impl>())
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
  impl_->polygon_edges_[edge->getLayerNum()].insert(
      std::make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::Impl::addPolygonEdge(
    gcSegment* edge,
    std::vector<std::vector<std::pair<segment_t, gcSegment*>>>& allShapes)
{
  segment_t boosts(point_t(edge->low().x(), edge->low().y()),
                   point_t(edge->high().x(), edge->high().y()));
  allShapes[edge->getLayerNum()].push_back(std::make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::addMaxRectangle(gcRect* rect)
{
  Rect r(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  impl_->max_rectangles_[rect->getLayerNum()].insert(std::make_pair(r, rect));
}

void FlexGCWorkerRegionQuery::addSpcRectangle(gcRect* rect)
{
  Rect r(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  impl_->spc_rectangles_[rect->getLayerNum()].insert(std::make_pair(r, *rect));
}

void FlexGCWorkerRegionQuery::Impl::addMaxRectangle(
    gcRect* rect,
    std::vector<std::vector<rq_box_value_t<gcRect*>>>& allShapes)
{
  Rect boostr(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  allShapes[rect->getLayerNum()].push_back(std::make_pair(boostr, rect));
}

void FlexGCWorkerRegionQuery::Impl::addSpcRectangle(
    gcRect* rect,
    std::vector<std::vector<rq_box_value_t<gcRect>>>& allShapes)
{
  Rect box(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  allShapes[rect->getLayerNum()].push_back(std::make_pair(box, *rect));
}

void FlexGCWorkerRegionQuery::removePolygonEdge(gcSegment* edge)
{
  segment_t boosts(point_t(edge->low().x(), edge->low().y()),
                   point_t(edge->high().x(), edge->high().y()));
  impl_->polygon_edges_[edge->getLayerNum()].remove(
      std::make_pair(boosts, edge));
}

void FlexGCWorkerRegionQuery::removeMaxRectangle(gcRect* rect)
{
  Rect r(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  impl_->max_rectangles_[rect->getLayerNum()].remove(std::make_pair(r, rect));
}

void FlexGCWorkerRegionQuery::removeSpcRectangle(gcRect* rect)
{
  Rect r(gtl::xl(*rect), gtl::yl(*rect), gtl::xh(*rect), gtl::yh(*rect));
  impl_->spc_rectangles_[rect->getLayerNum()].remove(std::make_pair(r, *rect));
}

void FlexGCWorkerRegionQuery::queryPolygonEdge(
    const box_t& box,
    const frLayerNum layerNum,
    std::vector<std::pair<segment_t, gcSegment*>>& result) const
{
  impl_->polygon_edges_[layerNum].query(bgi::intersects(box),
                                        back_inserter(result));
}

void FlexGCWorkerRegionQuery::queryPolygonEdge(
    const Rect& box,
    const frLayerNum layerNum,
    std::vector<std::pair<segment_t, gcSegment*>>& result) const
{
  box_t boostb(point_t(box.xMin(), box.yMin()),
               point_t(box.xMax(), box.yMax()));
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
    const Rect& box,
    const frLayerNum layerNum,
    std::vector<rq_box_value_t<gcRect*>>& result) const
{
  box_t boostb(point_t(box.xMin(), box.yMin()),
               point_t(box.xMax(), box.yMax()));
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
  std::vector<std::vector<std::pair<segment_t, gcSegment*>>> allPolygonEdges(
      numLayers);
  std::vector<std::vector<rq_box_value_t<gcRect*>>> allMaxRectangles(numLayers);
  std::vector<std::vector<rq_box_value_t<gcRect>>> allSpcRectangles(numLayers);

  for (auto& net : gcWorker_->getNets()) {
    for (auto& pins : net->getPins()) {
      for (auto& pin : pins) {
        for (auto& edges : pin->getPolygonEdges()) {
          for (auto& edge : edges) {
            addPolygonEdge(edge.get(), allPolygonEdges);
          }
        }
        for (auto& rect : pin->getMaxRectangles()) {
          addMaxRectangle(rect.get(), allMaxRectangles);
        }
      }
    }
    for (auto& spcRect : net->getSpecialSpcRects()) {
      addSpcRectangle(spcRect.get(), allSpcRectangles);
    }
  }

  for (int i = 0; i < numLayers; i++) {
    polygon_edges_[i]
        = boost::move(RTree<gcSegment*, segment_t>(allPolygonEdges[i]));
    max_rectangles_[i] = boost::move(RTree<gcRect*>(allMaxRectangles[i]));
    spc_rectangles_[i] = boost::move(RTree<gcRect>(allSpcRectangles[i]));
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
  for (auto& spcR : net->getSpecialSpcRects()) {
    removeSpcRectangle(spcR.get());
  }
}

}  // namespace drt
