// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "dr/FlexDR_graphics.h"

#include <any>
#include <cassert>
#include <cstdio>
#include <functional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "../gc/FlexGC.h"
#include "db/drObj/drFig.h"
#include "db/obj/frShape.h"
#include "db/obj/frVia.h"
#include "dr/FlexDR.h"
#include "frBaseTypes.h"
#include "frRegionQuery.h"
#include "gui/gui.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"

namespace drt {

// Descriptor for Grid Graph nodes and their edges
class GridGraphDescriptor : public gui::Descriptor
{
 public:
  struct Data
  {
    const FlexGridGraph* graph;
    const frMIdx x;
    const frMIdx y;
    const frMIdx z;
    const frDesign* design;
  };

  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void highlight(const std::any& object, gui::Painter& painter) const override;

  Properties getProperties(const std::any& object) const override;
  gui::Selected makeSelected(const std::any& object) const override;
  bool lessThan(const std::any& l, const std::any& r) const override;

  void visitAllObjects(
      const std::function<void(const gui::Selected&)>& func) const override;
};

std::string GridGraphDescriptor::getName(const std::any& object) const
{
  auto data = std::any_cast<Data>(object);
  return "<" + std::to_string(data.x) + ", " + std::to_string(data.y) + ", "
         + std::to_string(data.z) + ">";
}

std::string GridGraphDescriptor::getTypeName() const
{
  return "Grid Graph Node";
}

bool GridGraphDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  auto data = std::any_cast<Data>(object);
  auto* graph = data.graph;
  auto x = graph->xCoord(data.x);
  auto y = graph->yCoord(data.y);
  bbox.init(x, y, x, y);
  return true;
}

void GridGraphDescriptor::highlight(const std::any& object,
                                    gui::Painter& painter) const
{
  odb::Rect bbox;
  getBBox(object, bbox);
  auto x = bbox.xMin();
  auto y = bbox.yMin();
  bbox.init(x - 20, y - 20, x + 20, y + 20);
  painter.drawRect(bbox);
}

gui::Descriptor::Properties GridGraphDescriptor::getProperties(
    const std::any& object) const
{
  auto data = std::any_cast<Data>(object);
  auto* graph = data.graph;
  auto tech = data.design->getTech();
  double dbu_per_uu = tech->getDBUPerUU();

  auto x = data.x;
  auto y = data.y;
  auto z = data.z;
  auto layer = tech->getLayer(graph->getLayerNum(z));

  Properties props({{"X", graph->xCoord(x) / dbu_per_uu},
                    {"Y", graph->yCoord(y) / dbu_per_uu},
                    {"Layer", layer->getName()}});
  auto gui = gui::Gui::get();

  // put these after the edges so they are always in the same spot for
  // faster navigation.
  Properties costs;

  for (const auto dir : frDirEnumAll) {
    // Find neighbor's coordinate & name
    frMIdx nx = x;
    frMIdx ny = y;
    frMIdx nz = z;
    std::string name;
    switch (dir) {
      case frDirEnum::UNKNOWN: /* can't happen */
        name = "ERR";
        break;
      case frDirEnum::D:
        name = "Down";
        nz -= 1;
        break;
      case frDirEnum::S:
        name = "South";
        ny -= 1;
        break;
      case frDirEnum::W:
        name = "West";
        nx -= 1;
        break;
      case frDirEnum::E:
        name = "East";
        nx += 1;
        break;
      case frDirEnum::N:
        name = "North";
        ny += 1;
        break;
      case frDirEnum::U:
        name = "Up";
        nz += 1;
        break;
    }

    if (!graph->hasEdge(x, y, z, dir)) {
      props.push_back({std::move(name), "<none>"});
      continue;
    }

    GridGraphDescriptor::Data neighbor{graph, nx, ny, nz, data.design};
    props.push_back({name, gui->makeSelected(neighbor)});
    if (graph->isBlocked(x, y, z, frDirEnum::W)) {
      costs.push_back({name + " blocked", true});
    }
    if (graph->hasGridCost(x, y, z, dir)) {
      costs.push_back({name + " grid cost", true});
    }
    if (graph->hasRouteShapeCostAdj(x, y, z, dir)) {
      costs.push_back({name + " route shape cost", true});
    }
    if (graph->hasMarkerCostAdj(x, y, z, dir)) {
      costs.push_back({name + " marker cost", true});
    }
    if (graph->hasFixedShapeCostAdj(x, y, z, dir)) {
      costs.push_back({name + " fixed shape cost", true});
    }
    if (!graph->hasGuide(x, y, z, dir)) {
      costs.push_back({name + " has guide", false});
    }
    costs.push_back(
        {name + " edge length", graph->getEdgeLength(x, y, z, dir)});
    costs.push_back(
        {name + " total cost",
         graph->getCosts(
             x, y, z, dir, layer, data.graph->getNDR() != nullptr, false)});
  }
  props.insert(props.end(), costs.begin(), costs.end());
  return props;
}

gui::Selected GridGraphDescriptor::makeSelected(const std::any& object) const
{
  if (auto data = std::any_cast<Data>(&object)) {
    return gui::Selected(*data, this);
  }
  return gui::Selected();
}

bool GridGraphDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_grid = std::any_cast<Data>(l);
  auto r_grid = std::any_cast<Data>(r);

  assert(l_grid.graph == r_grid.graph);

  return std::tie(l_grid.x, l_grid.y, l_grid.z)
         < std::tie(r_grid.x, r_grid.y, r_grid.z);
}

void GridGraphDescriptor::visitAllObjects(
    const std::function<void(const gui::Selected&)>& func) const
{
}

//////////////////////////////////////////////////

const char* FlexDRGraphics::graph_edges_visible_ = "Graph Edges";
const char* FlexDRGraphics::grid_cost_edges_visible_ = "Grid Cost Edges";
const char* FlexDRGraphics::blocked_edges_visible_ = "Blocked Edges";
const char* FlexDRGraphics::route_guides_visible_ = "Route Guides";
const char* FlexDRGraphics::routing_objs_visible_ = "Routing Objects";
const char* FlexDRGraphics::route_shape_cost_visible_ = "Route Shape Cost";
const char* FlexDRGraphics::marker_cost_visible_ = "Marker Cost";
const char* FlexDRGraphics::fixed_shape_cost_visible_ = "Fixed Shape Cost";
const char* FlexDRGraphics::maze_search_visible_ = "Maze Search";
const char* FlexDRGraphics::current_net_only_visible_ = "Current Net Only";

static std::string workerOrigin(FlexDRWorker* worker)
{
  odb::Point origin = worker->getRouteBox().ll();
  return "(" + std::to_string(origin.x()) + ", " + std::to_string(origin.y())
         + ")";
}

FlexDRGraphics::FlexDRGraphics(frDebugSettings* settings,
                               frDesign* design,
                               odb::dbDatabase* db,
                               utl::Logger* logger)
    : worker_(nullptr),
      design_(design),
      net_(nullptr),
      grid_graph_(nullptr),
      settings_(settings),
      current_iter_(-1),
      last_pt_layer_(-1),
      gui_(gui::Gui::get()),
      logger_(logger)
{
  // Build the layer map between opendb & tr
  auto odb_tech = db->getTech();
  dbu_per_uu_ = odb_tech->getDbUnitsPerMicron();

  layer_map_.resize(odb_tech->getLayerCount(), -1);

  for (auto& tr_layer : design->getTech()->getLayers()) {
    auto odb_layer = tr_layer->getDbLayer();
    if (odb_layer) {
      layer_map_[odb_layer->getNumber()] = tr_layer->getLayerNum();
    }
  }

  addDisplayControl(graph_edges_visible_);
  addDisplayControl(grid_cost_edges_visible_);
  addDisplayControl(blocked_edges_visible_);
  addDisplayControl(route_shape_cost_visible_);
  addDisplayControl(marker_cost_visible_);
  addDisplayControl(fixed_shape_cost_visible_);
  addDisplayControl(route_guides_visible_, true);
  addDisplayControl(routing_objs_visible_, true);
  addDisplayControl(maze_search_visible_, true);
  addDisplayControl(current_net_only_visible_, false);

  gui_->registerRenderer(this);
}

const char* FlexDRGraphics::getDisplayControlGroupName()
{
  return "FlexDR";
}

void FlexDRGraphics::drawLayer(odb::dbTechLayer* layer, gui::Painter& painter)
{
  if (layer_map_.empty()) {
    return;
  }
  frLayerNum layerNum = layer_map_.at(layer->getNumber());
  if (layerNum < 0) {
    return;
  }

  painter.setPen(layer);
  painter.setBrush(layer);

  // Draw segs & vias
  const bool draw_current_net_only_
      = checkDisplayControl(current_net_only_visible_);
  if (checkDisplayControl(routing_objs_visible_)) {
    if (drawWholeDesign_) {
      odb::Rect box = design_->getTopBlock()->getDieBox();
      frRegionQuery::Objects<frBlockObject> figs;
      design_->getRegionQuery()->queryDRObj(box, layerNum, figs);
      for (auto& fig : figs) {
        drawObj(fig.second, painter, layerNum);
      }
    } else if (worker_) {
      odb::Rect box;
      worker_->getExtBox(box);
      std::vector<drConnFig*> figs;
      worker_->getWorkerRegionQuery().query(box, layerNum, figs);
      for (auto& fig : figs) {
        if (draw_current_net_only_ && fig->getNet() != net_) {
          continue;
        }
        drawObj(fig, painter, layerNum);
      }
    }
  }

  if (net_ && checkDisplayControl(route_guides_visible_)) {
    // Draw guides
    painter.setBrush(layer, /* alpha */ 90);
    for (auto& rect : net_->getOrigGuides()) {
      if (rect.getLayerNum() == layerNum) {
        odb::Rect box = rect.getBBox();
        painter.drawRect({box.xMin(), box.yMin(), box.xMax(), box.yMax()});
      }
    }
  }
  painter.setPen(layer, /* cosmetic */ true);
  if (checkDisplayControl(maze_search_visible_) && !points_by_layer_.empty()) {
    for (odb::Point& pt : points_by_layer_[layerNum]) {
      painter.drawX(pt.x(), pt.y(), 20);
    }
  }
  // Draw graphs
  const bool draw_drc = checkDisplayControl(route_shape_cost_visible_);
  const bool draw_marker = checkDisplayControl(marker_cost_visible_);
  const bool draw_shape = checkDisplayControl(fixed_shape_cost_visible_);
  const bool draw_edges = checkDisplayControl(graph_edges_visible_);
  const bool draw_gCostEdges = checkDisplayControl(grid_cost_edges_visible_);
  const bool draw_blockedEdges = checkDisplayControl(blocked_edges_visible_);
  if (grid_graph_ && layer->getType() == odb::dbTechLayerType::ROUTING
      && (draw_edges || draw_drc || draw_marker || draw_shape || draw_gCostEdges
          || draw_blockedEdges)) {
    const frMIdx z = grid_graph_->getMazeZIdx(layerNum);
    const int offset = design_->getTech()->getLayer(2)->getPitch() / 8;
    frMIdx x_dim, y_dim, z_dim;
    grid_graph_->getDim(x_dim, y_dim, z_dim);

    auto color = painter.getPenColor();
    auto prevColor = color;
    color.a = 255;
    for (frMIdx x = 0; x < x_dim; ++x) {
      for (frMIdx y = 0; y < y_dim; ++y) {
        odb::Point pt;
        grid_graph_->getPoint(pt, x, y);
        // draw edges
        if (draw_edges || draw_gCostEdges || draw_blockedEdges) {
          if (x != x_dim - 1) {
            odb::Point pt2;
            grid_graph_->getPoint(pt2, x + 1, y);

            if (draw_edges && grid_graph_->hasEdge(x, y, z, frDirEnum::E)) {
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }
            if ((draw_gCostEdges && grid_graph_->hasGridCostE(x, y, z))
                || (draw_blockedEdges
                    && grid_graph_->isBlocked(x, y, z, frDirEnum::E))) {
              painter.setBrush(color);
              painter.setPen(color, true, offset / 10);
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }
            painter.setBrush(prevColor);
            painter.setPen(layer, true);
          }
          if (y != y_dim - 1) {
            odb::Point pt2;
            grid_graph_->getPoint(pt2, x, y + 1);
            if (draw_edges && grid_graph_->hasEdge(x, y, z, frDirEnum::N)) {
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }
            if ((draw_gCostEdges && grid_graph_->hasGridCostN(x, y, z))
                || (draw_blockedEdges
                    && grid_graph_->isBlocked(x, y, z, frDirEnum::N))) {
              painter.setBrush(color);
              painter.setPen(color, true, offset / 10);
              painter.drawLine({pt.x(), pt.y()}, {pt2.x(), pt2.y()});
            }
            painter.setBrush(prevColor);
            painter.setPen(layer, true);
          }
        }
        bool planar
            = (draw_drc
               && grid_graph_->hasRouteShapeCostAdj(
                   x, y, z, frDirEnum::UNKNOWN))
              || (draw_marker
                  && grid_graph_->hasMarkerCostAdj(x, y, z, frDirEnum::UNKNOWN))
              || (draw_shape
                  && grid_graph_->hasFixedShapeCostAdj(
                      x, y, z, frDirEnum::UNKNOWN));
        if (planar) {
          painter.drawRect({grid_graph_->xCoord(x) - offset,
                            grid_graph_->yCoord(y) - offset,
                            grid_graph_->xCoord(x) + offset,
                            grid_graph_->yCoord(y) + offset});
        }
        bool via
            = (draw_drc
               && grid_graph_->hasRouteShapeCostAdj(x, y, z, frDirEnum::U))
              || (draw_marker
                  && grid_graph_->hasMarkerCostAdj(x, y, z, frDirEnum::U))
              || (draw_shape
                  && grid_graph_->hasFixedShapeCostAdj(x, y, z, frDirEnum::U))
              || (draw_gCostEdges && grid_graph_->hasGridCostU(x, y, z))
              || (draw_blockedEdges
                  && grid_graph_->isBlocked(x, y, z, frDirEnum::U))
              || (draw_edges && grid_graph_->hasEdge(x, y, z, frDirEnum::U));
        if (via) {
          painter.drawCircle(
              grid_graph_->xCoord(x), grid_graph_->yCoord(y), offset / 2);
        }
      }
    }
  }
  if (!worker_) {
    return;
  }
  // Draw markers
  painter.setPen(gui::Painter::kGreen, /* cosmetic */ true);
  for (auto& marker : design_->getTopBlock()->getMarkers()) {
    if (marker->getLayerNum() == layerNum) {
      odb::Rect box = marker->getBBox();
      drawMarker(box.xMin(), box.yMin(), box.xMax(), box.yMax(), painter);
    }
  }
  painter.setPen(gui::Painter::kYellow, /* cosmetic */ true);
  for (auto& marker : worker_->getGCWorker()->getMarkers()) {
    if (marker->getLayerNum() == layerNum) {
      odb::Rect box = marker->getBBox();
      drawMarker(box.xMin(), box.yMin(), box.xMax(), box.yMax(), painter);
    }
  }
}

void FlexDRGraphics::drawObj(frBlockObject* fig,
                             gui::Painter& painter,
                             int layerNum)
{
  odb::Rect box;
  switch (fig->typeId()) {
    case frcPathSeg: {
      auto seg = (frPathSeg*) fig;
      if (seg->getLayerNum() == layerNum) {
        painter.drawRect(seg->getBBox());
      }
      break;
    }
    case drcPathSeg: {
      auto seg = (drPathSeg*) fig;
      if (seg->getLayerNum() == layerNum) {
        painter.drawRect(seg->getBBox());
      }
      break;
    }
    case frcVia: {
      auto via = (frVia*) fig;
      auto viadef = via->getViaDef();
      if (viadef->getLayer1Num() == layerNum) {
        box = via->getLayer1BBox();
      } else if (viadef->getLayer2Num() == layerNum) {
        box = via->getLayer2BBox();
      } else {
        return;
      }
      painter.drawRect(box);
      break;
    }
    case drcVia: {
      auto via = (drVia*) fig;
      auto viadef = via->getViaDef();
      if (viadef->getLayer1Num() == layerNum) {
        box = via->getLayer1BBox();
      } else if (viadef->getLayer2Num() == layerNum) {
        box = via->getLayer2BBox();
      } else {
        return;
      }
      painter.drawRect(box);
      break;
    }
    case frcPatchWire: {
      auto patch = (frPatchWire*) fig;
      if (patch->getLayerNum() == layerNum) {
        painter.drawRect(patch->getBBox());
      }
      break;
    }
    case drcPatchWire: {
      auto patch = (drPatchWire*) fig;
      if (patch->getLayerNum() == layerNum) {
        painter.drawRect(patch->getBBox());
      }
      break;
    }

    default: {
      logger_->debug(
          DRT, "gui", "Unknown fig type {} in drawLayer.", fig->typeId());
    }
  }
}

void FlexDRGraphics::drawMarker(int xl,
                                int yl,
                                int xh,
                                int yh,
                                gui::Painter& painter)
{
  painter.drawRect({xl, yl, xh, yh});
  painter.drawLine({xl, yl}, {xh, yh});
  painter.drawLine({xl, yh}, {xh, yl});
}

void FlexDRGraphics::show(bool checkStopConditions)
{
  if (checkStopConditions) {
    if (!worker_ || current_iter_ < settings_->iter
        || (!settings_->netName.empty()
            && (!net_ || net_->getFrNet()->getName() != settings_->netName))) {
      return;
    }
    const odb::Rect& rBox = worker_->getRouteBox();
    if (settings_->box != odb::Rect(-1, -1, -1, -1)
        && !rBox.intersects(settings_->box)) {
      return;
    }
  }
  update();
  pause(nullptr);
}

void FlexDRGraphics::update()
{
  if (settings_->draw) {
    gui_->redraw();
  }
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

void FlexDRGraphics::debugWholeDesign()
{
  if (!settings_->allowPause) {
    return;
  }
  drawWholeDesign_ = true;
  update();
  gui_->pause();
  drawWholeDesign_ = false;
}
void FlexDRGraphics::drawObjects(gui::Painter& painter)
{
  if (!worker_) {
    return;
  }

  painter.setBrush(gui::Painter::kTransparent);
  painter.setPen(gui::Painter::kYellow, /* cosmetic */ true);

  odb::Rect box;
  worker_->getRouteBox(box);
  painter.drawRect({box.xMin(), box.yMin(), box.xMax(), box.yMax()});

  box = worker_->getDrcBox();
  painter.drawRect({box.xMin(), box.yMin(), box.xMax(), box.yMax()});

  worker_->getExtBox(box);
  painter.drawRect({box.xMin(), box.yMin(), box.xMax(), box.yMax()});

  if (net_) {
    for (auto& pin : net_->getPins()) {
      for (auto& ap : pin->getAccessPatterns()) {
        odb::Point pt = ap->getPoint();
        painter.drawX(pt.x(), pt.y(), 100);
      }
    }
  }
}

void FlexDRGraphics::startWorker(FlexDRWorker* worker)
{
  worker_ = nullptr;

  if (current_iter_ < settings_->iter) {
    return;
  }
  const odb::Rect& rBox = worker->getRouteBox();
  if (settings_->box != odb::Rect(-1, -1, -1, -1)
      && !rBox.intersects(settings_->box)) {
    return;
  }
  status("Start worker: origin " + workerOrigin(worker) + " "
         + std::to_string(worker->getMarkers().size()) + " markers");

  worker_ = worker;
  net_ = nullptr;
  grid_graph_ = &worker_->getGridGraph();

  points_by_layer_.resize(worker->getTech()->getLayers().size());

  if (settings_->netName.empty()) {
    odb::Rect box;
    worker_->getExtBox(box);
    gui_->zoomTo({box.xMin(), box.yMin(), box.xMax(), box.yMax()});
    if (settings_->draw) {
      gui_->redraw();
    }
    if (settings_->allowPause) {
      gui_->pause();
    }
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

  odb::Point in;
  grid_graph->getPoint(in, grid.x(), grid.y());
  frLayerNum layer = grid_graph->getLayerNum(grid.z());

  auto& pts = points_by_layer_.at(layer);
  pts.push_back(in);

  // Pause on any layer change
  if (settings_->debugMaze && last_pt_layer_ != layer && last_pt_layer_ != -1) {
    if (settings_->draw) {
      gui_->redraw();
    }
    if (settings_->allowPause) {
      GridGraphDescriptor::Data data{
          grid_graph, grid.x(), grid.y(), grid.z(), design_};
      gui_->setSelected(gui_->makeSelected(data));
      gui_->pause();
    }
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
  logger_->info(
      DRT, 249, "Net {} (id = {}).", net->getFrNet()->getName(), net->getId());
  for (auto& pin : net->getPins()) {
    logger_->info(DRT, 250, "  Pin {}.", pin->getName());
    for (auto& ap : pin->getAccessPatterns()) {
      odb::Point pt = ap->getPoint();
      logger_->info(DRT,
                    275,
                    "    AP ({:.5f}, {:.5f}) (layer {}) (cost {}).",
                    pt.x() / (double) dbu_per_uu_,
                    pt.y() / (double) dbu_per_uu_,
                    ap->getBeginLayerNum(),
                    ap->getPinCost());
    }
  }
  net_ = net;
  last_pt_layer_ = -1;

  odb::Rect box;
  worker_->getExtBox(box);
  gui_->zoomTo({box.xMin(), box.yMin(), box.xMax(), box.yMax()});
  if (settings_->allowPause) {
    gui_->pause();
  }
}

void FlexDRGraphics::midNet(drNet* net)
{
  if (!net_) {
    return;
  }
  gui_->removeSelected<GridGraphDescriptor::Data>();
  assert(net == net_);
  int point_cnt = 0;
  for (auto& pts : points_by_layer_) {
    point_cnt += pts.size();
  }

  status("Mid net: " + net->getFrNet()->getName() + " searched "
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

void FlexDRGraphics::endNet(drNet* net)
{
  if (!net_) {
    return;
  }
  gui_->removeSelected<GridGraphDescriptor::Data>();
  assert(net == net_);

  status("End net: " + net->getFrNet()->getName() + " After GC");

  if (settings_->draw) {
    gui_->redraw();
  }

  if (settings_->allowPause) {
    gui_->pause();
  }
  net_ = nullptr;
}

void FlexDRGraphics::startIter(int iter, RouterConfiguration* router_cfg)
{
  current_iter_ = iter;
  if (iter >= settings_->iter) {
    if (router_cfg->MAX_THREADS > 1) {
      logger_->info(DRT, 207, "Setting MAX_THREADS=1 for use with the DR GUI.");
      router_cfg->MAX_THREADS = 1;
    }

    status("Start iter: " + std::to_string(iter));
    if (settings_->allowPause) {
      gui_->pause();
    }
  }
}

void FlexDRGraphics::endWorker(int iter)
{
  if (worker_ == nullptr) {
    return;
  }
  if (iter >= settings_->iter) {
    gui_->removeSelected<GridGraphDescriptor::Data>();
    status("End Worker: " + std::to_string(iter));
    if (settings_->draw) {
      gui_->redraw();
    }
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
  return gui::Gui::enabled();
}

void FlexDRGraphics::init()
{
  if (guiActive()) {
    gui::Gui::get()->registerDescriptor<GridGraphDescriptor::Data>(
        new GridGraphDescriptor);
  }
}

}  // namespace drt
