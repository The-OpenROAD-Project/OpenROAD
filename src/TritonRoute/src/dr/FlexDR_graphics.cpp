#include <algorithm>
#include <cstdio>
#include <limits>

#include "FlexDR_graphics.h"
#include "FlexDR.h"
#include "openroad/OpenRoad.hh"

namespace fr {

FlexDRGraphics::FlexDRGraphics(frDebugSettings* settings,
                               frDesign* design)
  : worker_(nullptr),
    net_(nullptr),
    settings_(settings),
    current_iter_(-1),
    last_pt_layer_(-1),
    gui_(gui::Gui::get())
{
  assert(MAX_THREADS == 1);

  // Build the layer map between opendb & tr
  auto odb_tech = ord::OpenRoad::openRoad()->getDb()->getTech();

  layer_map.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = odb_tech->findLayer(tr_layer->getName().c_str());
    if (odb_layer) {
      layer_map[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

  gui_->register_renderer(this);
}

void FlexDRGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  if (!net_) {
    return;
  }

  frLayerNum layerNum = layer_map.at(layer->getNumber());
  if (layerNum < 0) {
    return;
  }

  painter.setPen(layer);
  painter.setBrush(layer);

  // Draw segs & vias
  {
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

  // Draw guides
  painter.setBrush(layer, /* alpha */ 50);
  for (auto& rect : net_->getOrigGuides()) {
    if (rect.getLayerNum() == layerNum) {
      frBox box;
      rect.getBBox(box);
      painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
    }
  }

  painter.setPen(layer, /* cosmetic */ true);
  for (frPoint& pt : points_by_layer_[layerNum]) {
    painter.drawLine({pt.x() - 100, pt.y() - 100},
                     {pt.x() + 100, pt.y() + 100});
    painter.drawLine({pt.x() - 100, pt.y() + 100},
                     {pt.x() + 100, pt.y() - 100});
  }

  // Draw markers
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
  for (auto& marker : worker_->getMarkers()) {
    if (marker.getLayerNum() == layerNum) {
      frBox box;
      marker.getBBox(box);
      painter.drawRect({box.left(), box.bottom(), box.right(), box.top()});
      painter.drawLine({box.left(), box.bottom()},
                       {box.right(), box.top()});
      painter.drawLine({box.left(), box.top()},
                       {box.right(), box.bottom()});
    }
  }
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
  if (current_iter_ < settings_->iter) {
    return;
  }
  status("Start worker: " + std::to_string(in->getMarkers().size())
         + " markers");

  worker_ = in;
  net_ = nullptr;

  points_by_layer_.resize(in->getTech()->getLayers().size());
  
  if (settings_->netName.empty()) {
    frBox box;
    worker_->getExtBox(box);
    gui_->zoomTo({box.left(), box.bottom(), box.right(), box.top()});
    gui_->pause();
  }
}

void FlexDRGraphics::searchNode(const FlexGridGraph* gridGraph,
                                const FlexWavefrontGrid& grid)
{
  if (!net_) {
    return;
  }

  frPoint in;
  gridGraph->getPoint(in, grid.x(), grid.y());
  frLayerNum layer = gridGraph->getLayerNum(grid.z());

  auto& pts = points_by_layer_.at(layer);
  pts.push_back(in);

  // Pause on any layer change
  if (settings_->debugMaze
      && last_pt_layer_ != layer
      && last_pt_layer_ != -1) {
    gui_->redraw();
    gui_->pause();
  }

  last_pt_layer_ = layer;
}

void FlexDRGraphics::startNet(drNet* net)
{
  net_ = nullptr;

  if (current_iter_ < settings_->iter) {
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
  gui_->pause();
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

  gui_->redraw();
  gui_->pause();

  for (auto& points : points_by_layer_) {
    points.clear();
  }
}

void FlexDRGraphics::startIter(int iter)
{
  current_iter_ = iter;
  if (iter >= settings_->iter) {
    status("Start iter: " + std::to_string(iter));
    gui_->pause();
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
