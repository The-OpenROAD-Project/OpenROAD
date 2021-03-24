/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
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

#include "FlexDR_graphics.h"

#include <algorithm>
#include <cstdio>
#include <limits>

#include "../gc/FlexGC.h"
#include "FlexDR.h"
#include "openroad/OpenRoad.hh"

namespace fr {

const char* FlexDRGraphics::grid_graph_visible_ = "Grid Graph";
const char* FlexDRGraphics::route_guides_visible_ = "Route Guides";
const char* FlexDRGraphics::routing_objs_visible_ = "Routing Objects";
const char* FlexDRGraphics::drc_cost_visible_ = "DRC Cost";
const char* FlexDRGraphics::marker_cost_visible_ = "Marker Cost";
const char* FlexDRGraphics::shape_cost_visible_ = "Shape Cost";

static std::string workerOrigin(FlexDRWorker* worker)
{
  frPoint ll = worker->getRouteBox().lowerLeft();
  frPoint origin;
  worker->getDesign()->getTopBlock()->getGCellIdx(ll, origin);
  return "(" + std::to_string(origin.x()) + ", " + std::to_string(origin.y())
         + ")";
}

FlexDRGraphics::FlexDRGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db,
                               Logger* logger)
    : worker_(nullptr),
      net_(nullptr),
      settings_(settings),
      current_iter_(-1),
      last_pt_layer_(-1),
      gui_(gui::Gui::get()),
      logger_(logger)
{
  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();

  layer_map_.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = odb_tech->findLayer(tr_layer->getName().c_str());
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

  gui_->addCustomVisibilityControl(grid_graph_visible_);
  gui_->addCustomVisibilityControl(drc_cost_visible_);
  gui_->addCustomVisibilityControl(marker_cost_visible_);
  gui_->addCustomVisibilityControl(shape_cost_visible_);
  gui_->addCustomVisibilityControl(route_guides_visible_, true);
  gui_->addCustomVisibilityControl(routing_objs_visible_, true);

  gui_->registerRenderer(this);
}

void FlexDRGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  if (!net_) {
    return;
  }

  frLayerNum layerNum = layer_map_.at(layer->getNumber());
  if (layerNum < 0) {
    return;
  }

  painter.setPen(layer);
  painter.setBrush(layer);

  // Draw segs & vias
  if (gui_->checkCustomVisibilityControl(routing_objs_visible_)) {
    auto& rq = worker_->getWorkerRegionQuery();
    frBox box;
    worker_->getRouteBox(box);
    std::vector<drConnFig*> figs;
    rq.query(box, layerNum, figs);
    for (auto& fig : figs) {
      switch (fig->typeId()) {
        case drcPathSeg: {
          auto seg = (drPathSeg*) fig;
          if (seg->getLayerNum() == layerNum) {
            seg->getBBox(box);
            painter.drawRect(
                {box.left(), box.bottom(), box.right(), box.top()});
          }
          break;
        }
        case drcVia: {
          auto via = (drVia*) fig;
          auto viadef = via->getViaDef();
          if (viadef->getLayer1Num() == layerNum) {
            via->getLayer1BBox(box);
          } else if (viadef->getLayer2Num() == layerNum) {
            via->getLayer2BBox(box);
          } else {
            continue;
          }
          painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
          break;
        }
        case drcPatchWire: {
          auto patch = (drPatchWire*) fig;
          if (patch->getLayerNum() == layerNum) {
            patch->getBBox(box);
            painter.drawRect(
                {box.left(), box.bottom(), box.right(), box.top()});
          }
          break;
        }

        default:
          logger_->debug(
              DRT, 1, "unknown fig type {} in drawLayer", fig->typeId());
      }
    }
  }

  if (gui_->checkCustomVisibilityControl(route_guides_visible_)) {
    // Draw guides
    painter.setBrush(layer, /* alpha */ 90);
    for (auto& rect : net_->getOrigGuides()) {
      if (rect.getLayerNum() == layerNum) {
        frBox box;
        rect.getBBox(box);
        painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
      }
    }
  }
  painter.setPen(layer, /* cosmetic */ true);
  for (frPoint& pt : points_by_layer_[layerNum]) {
    painter.drawLine({pt.x() - 20, pt.y() - 20}, {pt.x() + 20, pt.y() + 20});
    painter.drawLine({pt.x() - 20, pt.y() + 20}, {pt.x() + 20, pt.y() - 20});
  }

  // Draw graphs
  const bool draw_drc = gui_->checkCustomVisibilityControl(drc_cost_visible_);
  const bool draw_marker
      = gui_->checkCustomVisibilityControl(marker_cost_visible_);
  const bool draw_shape
      = gui_->checkCustomVisibilityControl(shape_cost_visible_);
  const bool draw_graph
      = gui_->checkCustomVisibilityControl(grid_graph_visible_);
  if (grid_graph_ && layer->getType() == odb::dbTechLayerType::ROUTING
      && (draw_graph || draw_drc || draw_marker || draw_shape)) {
    const frMIdx z = grid_graph_->getMazeZIdx(layerNum);
    const int offset = 25;
    const bool prefIsVert
        = layer->getDirection().getValue() == layer->getDirection().VERTICAL;

    frMIdx x_dim, y_dim, z_dim;
    grid_graph_->getDim(x_dim, y_dim, z_dim);

    for (frMIdx x = 0; x < x_dim; ++x) {
      for (frMIdx y = 0; y < y_dim; ++y) {
        frPoint pt;
        grid_graph_->getPoint(pt, x, y);

        if (draw_graph && x != x_dim - 1
            && (!grid_graph_->hasEdge(x, y, z, frDirEnum::E)
                || grid_graph_->isBlocked(x, y, z, frDirEnum::E)
                || (!prefIsVert && grid_graph_->hasGridCostE(x, y, z)))) {
          frPoint pt2;
          grid_graph_->getPoint(pt2, x + 1, y);
          painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
        }

        if (draw_graph && y != y_dim - 1
            && (!grid_graph_->hasEdge(x, y, z, frDirEnum::N)
                || grid_graph_->isBlocked(x, y, z, frDirEnum::N)
                || (prefIsVert && grid_graph_->hasGridCostN(x, y, z)))) {
          frPoint pt2;
          grid_graph_->getPoint(pt2, x, y + 1);
          painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
        }
        // Planar doesn't distinguish E vs N so just use one
        bool planar
            = (draw_drc && grid_graph_->hasDRCCost(x, y, z, frDirEnum::E))
              || (draw_marker
                  && grid_graph_->hasMarkerCost(x, y, z, frDirEnum::E))
              || (draw_shape
                  && grid_graph_->hasShapeCost(x, y, z, frDirEnum::E));
        if (planar) {
          painter.drawRect({grid_graph_->xCoord(x) - offset,
                            grid_graph_->yCoord(y) - offset,
                            grid_graph_->xCoord(x) + offset,
                            grid_graph_->yCoord(y) + offset});
        }
        bool via = (draw_drc && grid_graph_->hasDRCCost(x, y, z, frDirEnum::U))
                   || (draw_marker
                       && grid_graph_->hasMarkerCost(x, y, z, frDirEnum::U))
                   || (draw_shape
                       && grid_graph_->hasShapeCost(x, y, z, frDirEnum::U));
        if (via) {
          painter.drawCircle(
              grid_graph_->xCoord(x), grid_graph_->yCoord(y), offset / 2);
        }
      }
    }
  }

  // Draw markers
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
  for (auto& marker :
       worker_->getGCWorker()
           ->getMarkers()) {  // getDesign()->getTopBlock()->getMarkers()
    if (marker->getLayerNum() == layerNum) {
      frBox box;
      marker->getBBox(box);
      painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
      painter.drawLine({box.left(), box.bottom()}, {box.right(), box.top()});
      painter.drawLine({box.left(), box.top()}, {box.right(), box.bottom()});
    }
  }
}

void FlexDRGraphics::update()
{
  if (settings_->draw)
    gui_->redraw();
}

void FlexDRGraphics::pause(drNet* net)
{
  if (!settings_->allowPause
      || (net && !settings_->netName.empty()
          && net->getFrNet()->getName() != settings_->netName)) {
    return;
  }
  gui_->pause();
}
void FlexDRGraphics::drawObjects(gui::Painter& painter)
{
  if (!worker_) {
    return;
  }

  painter.setBrush(gui::Painter::transparent);
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);

  frBox box;
  worker_->getRouteBox(box);
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  box = worker_->getDrcBox();
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  worker_->getExtBox(box);
  painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});

  if (net_) {
    for (auto& pin : net_->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        frPoint pt;
        ap->getPoint(pt);
        painter.drawLine({pt.x() - 100, pt.y() - 100},
                         {pt.x() + 100, pt.y() + 100});
        painter.drawLine({pt.x() - 100, pt.y() + 100},
                         {pt.x() + 100, pt.y() - 100});
      }
    }
  }
}

void FlexDRGraphics::startWorker(FlexDRWorker* in)
{
  worker_ = nullptr;

  if (current_iter_ < settings_->iter) {
    return;
  }

  frBox gcellBox = in->getGCellBox();
  if (settings_->gcellX >= 0
      && !gcellBox.contains(frPoint(settings_->gcellX, settings_->gcellY))) {
    return;
  }

  frPoint origin;
  in->getDesign()->getTopBlock()->getGCellIdx(in->getRouteBox().lowerLeft(),
                                              origin);
  status("Start worker: gcell origin " + workerOrigin(in) + " "
         + std::to_string(in->getMarkers().size()) + " markers");

  worker_ = in;
  net_ = nullptr;
  grid_graph_ = nullptr;

  points_by_layer_.resize(in->getTech()->getLayers().size());

  if (settings_->netName.empty()) {
    frBox box;
    worker_->getExtBox(box);
    gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
    if (settings_->allowPause)
      gui_->pause();
  }
}

void FlexDRGraphics::searchNode(const FlexGridGraph* grid_graph,
                                const FlexWavefrontGrid& grid)
{
  if (!net_) {
    return;
  }

  assert(grid_graph_ == nullptr || grid_graph_ == grid_graph);
  grid_graph_ = grid_graph;

  frPoint in;
  grid_graph->getPoint(in, grid.x(), grid.y());
  frLayerNum layer = grid_graph->getLayerNum(grid.z());

  auto& pts = points_by_layer_.at(layer);
  pts.push_back(in);

  // Pause on any layer change
  if (settings_->debugMaze && last_pt_layer_ != layer && last_pt_layer_ != -1) {
    if (settings_->draw)
      gui_->redraw();
    if (settings_->allowPause)
      gui_->pause();
  }

  last_pt_layer_ = layer;
}

void FlexDRGraphics::startNet(drNet* net)
{
  net_ = nullptr;

  if (!worker_) {
    return;
  }

  if (!settings_->netName.empty()
      && net->getFrNet()->getName() != settings_->netName) {
    return;
  }

  status("Start net: " + net->getFrNet()->getName() + " "
         + workerOrigin(worker_));
  net_ = net;
  last_pt_layer_ = -1;

  frBox box;
  worker_->getExtBox(box);
  gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
  if (settings_->allowPause) {
    gui_->pause();
  }
}

void FlexDRGraphics::endNet(drNet* net)
{
  if (!net_) {
    return;
  }
  assert(net == net_);

  int point_cnt = 0;
  for (auto& pts : points_by_layer_) {
    point_cnt += pts.size();
  }

  status("End net: " + net->getFrNet()->getName() + " searched "
         + std::to_string(point_cnt) + " points");

  if (settings_->draw) {
    gui_->redraw();
  }

  if (settings_->allowPause) {
    gui_->pause();
  }

  for (auto& points : points_by_layer_) {
    points.clear();
  }
}

void FlexDRGraphics::startIter(int iter)
{
  current_iter_ = iter;
  if (iter >= settings_->iter) {
    // If you specified a gcell then only one worker can process that
    // gcell and we don't need to limit the threading.
    if (MAX_THREADS > 1 && settings_->gcellX < 0) {
      logger_->info(DRT, 207, "Setting MAX_THREADS=1 for use with the DR GUI.");
      MAX_THREADS = 1;
    }

    status("Start iter: " + std::to_string(iter));
    if (settings_->allowPause) {
      gui_->pause();
    }
  }
}

void FlexDRGraphics::status(const std::string& message)
{
  gui_->status(message);
}

/* static */
bool FlexDRGraphics::guiActive()
{
  return gui::Gui::get() != nullptr;
}

}  // namespace fr
