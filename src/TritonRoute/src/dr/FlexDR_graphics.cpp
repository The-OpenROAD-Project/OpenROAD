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

#include <algorithm>
#include <cstdio>
#include <limits>
#include <qt5/QtGui/qcolor.h>

#include "FlexDR_graphics.h"
#include "FlexDR.h"
#include "../gc/FlexGC.h"
#include "openroad/OpenRoad.hh"

namespace fr {

FlexDRGraphics::FlexDRGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db)
  : worker_(nullptr),
    net_(nullptr),
    settings_(settings),
    current_iter_(-1),
    last_pt_layer_(-1),
    gui_(gui::Gui::get())
{
  assert(MAX_THREADS == 1);

  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();

  layer_map_.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = odb_tech->findLayer(tr_layer->getName().c_str());
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

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
  if (gui_->areRoutingObjsVisible()){
    auto& rq = worker_->getWorkerRegionQuery();
    frBox box;
    worker_->getRouteBox(box);
    std::vector<drConnFig*> figs;
    rq.query(box, layerNum, figs);
    for (auto& fig : figs) {
      switch (fig->typeId()) {
      case drcPathSeg: {
        auto seg = (drPathSeg *) fig;
        if (seg->getLayerNum() == layerNum) {
          seg->getBBox(box);
          painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
        }
        break;
      }
      case drcVia: {
        auto via = (drVia *) fig;
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
        auto patch = (drPatchWire *) fig;
        if (patch->getLayerNum() == layerNum) {
          patch->getBBox(box);
          painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
        }
        break;
      }

      default:
        printf("unknown %d\n", (int)fig->typeId());
      }
    }
  }

  if (gui_->areRouteGuidesVisible()){
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
    painter.drawLine({pt.x() - 20, pt.y() - 20},
                     {pt.x() + 20, pt.y() + 20});
    painter.drawLine({pt.x() - 20, pt.y() + 20},
                     {pt.x() + 20, pt.y() - 20});
  }

  // Draw graphs
  if (grid_graph_ && layer->getType() == odb::dbTechLayerType::ROUTING) {
    frMIdx z = grid_graph_->getMazeZIdx(layerNum);
    if (gui_->isGridGraphVisible()){
        int of = 50;
        bool prefIsVert = layer->getDirection().getValue() == layer->getDirection().VERTICAL;
        frMIdx x_dim, y_dim, z_dim;
        grid_graph_->getDim(x_dim, y_dim, z_dim);
        for (frMIdx x = 0; x < x_dim; ++x) {
          for (frMIdx y = 0; y < y_dim; ++y) {
            frPoint pt;
            grid_graph_->getPoint(pt, x, y);

            if (x != x_dim-1 && (/*!grid_graph_->hasEdge(x, y, z, frDirEnum::E) || */
                    grid_graph_->isBlocked(x, y, z, frDirEnum::E) || !prefIsVert && grid_graph_->hasGridCostE(x, y, z))) {
              frPoint pt2;
              grid_graph_->getPoint(pt2, x + 1, y);
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }

            if (y != y_dim-1 && (/*!grid_graph_->hasEdge(x, y, z, frDirEnum::N) || */
                    grid_graph_->isBlocked(x, y, z, frDirEnum::N) || prefIsVert && grid_graph_->hasGridCostN(x, y, z))) {
              frPoint pt2;
              grid_graph_->getPoint(pt2, x, y + 1);
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }
            if (grid_graph_->hasAnyPlanarCost(x, y, z))
                painter.drawRect({grid_graph_->xCoord(x)-of, grid_graph_->yCoord(y)-of, grid_graph_->xCoord(x)+of, grid_graph_->yCoord(y)+of});
          }
        }
    }
   
  // Draw markers
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
  for (auto& marker : worker_->getGCWorker()->getMarkers()) { //getDesign()->getTopBlock()->getMarkers()
    if (marker->getLayerNum() == layerNum) {
      frBox box;
      marker->getBBox(box);
      painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
      painter.drawLine({box.left(), box.bottom()},
                       {box.right(), box.top()});
      painter.drawLine({box.left(), box.top()},
                       {box.right(), box.bottom()});
    }
  }
 }
}
  
void FlexDRGraphics::update(){
    if (settings_->draw) gui_->redraw();
}
  
void FlexDRGraphics::pause(drNet* net){
    if (!settings_->allowPause || net && !settings_->netName.empty() &&
        net->getFrNet()->getName() != settings_->netName) {
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
  if (settings_->gcellX >= 0 &&
      !gcellBox.contains(frPoint(settings_->gcellX, settings_->gcellY))) {
    return;
  }

  frPoint origin;
  in->getDesign()->getTopBlock()->getGCellIdx(in->getRouteBox().lowerLeft(),
                                              origin);
  status("Start worker: gcell origin ("
         + std::to_string(origin.x()) + ", " + std::to_string(origin.y()) + ") "
         + std::to_string(in->getMarkers().size()) + " markers");

  worker_ = in;
  net_ = nullptr;
  grid_graph_ = nullptr;

  points_by_layer_.resize(in->getTech()->getLayers().size());
  
  if (settings_->netName.empty()) {
    frBox box;
    worker_->getExtBox(box);
    gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
    if (settings_->allowPause) gui_->pause();
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
  if (settings_->debugMaze
      && last_pt_layer_ != layer
      && last_pt_layer_ != -1) {
    if (settings_->draw) gui_->redraw();
    if (settings_->allowPause) gui_->pause();
  }

  last_pt_layer_ = layer;
}

void FlexDRGraphics::startNet(drNet* net)
{
  net_ = nullptr;

  if (!worker_) {
    return;
  }

  if (!settings_->netName.empty() &&
      net->getFrNet()->getName() != settings_->netName) {
    return;
  }

  status("Start net: " + net->getFrNet()->getName());
  net_ = net;
  last_pt_layer_ = -1;
  
  frBox box;
  worker_->getExtBox(box);
  gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
  if (settings_->allowPause) gui_->pause();
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

  if (settings_->draw) gui_->redraw();
  if (settings_->allowPause) gui_->pause();

  for (auto& points : points_by_layer_) {
    points.clear();
  }
}

void FlexDRGraphics::startIter(int iter)
{
  current_iter_ = iter;
  if (iter >= settings_->iter) {
    status("Start iter: " + std::to_string(iter));
    if (settings_->allowPause) gui_->pause();
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
