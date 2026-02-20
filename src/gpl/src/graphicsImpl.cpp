// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "graphicsImpl.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AbstractGraphics.h"
#include "gui/gui.h"
#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "odb/db.h"
#include "placerBase.h"
#include "point.h"
#include "utl/Logger.h"

namespace gpl {

GraphicsImpl::GraphicsImpl(utl::Logger* logger)
    : HeatMapDataSource(logger, "gpl", "gpl"), logger_(logger), mode_(Mbff)
{
  gui::Gui::get()->registerRenderer(this);
}

GraphicsImpl::~GraphicsImpl() = default;

std::unique_ptr<AbstractGraphics> GraphicsImpl::MakeNew(
    utl::Logger* logger) const
{
  return std::make_unique<GraphicsImpl>(logger);
}

void GraphicsImpl::debugForMbff()
{
  setDebugOn(true);
  mode_ = Mbff;
}

void GraphicsImpl::debugForInitialPlace(
    std::shared_ptr<PlacerBaseCommon> pbc,
    std::vector<std::shared_ptr<PlacerBase>>& pbVec)
{
  setDebugOn(true);
  pbc_ = std::move(pbc);
  pbVec_ = pbVec;
  mode_ = Initial;
}

void GraphicsImpl::debugForNesterovPlace(
    NesterovPlace* np,
    std::shared_ptr<PlacerBaseCommon> pbc,
    std::shared_ptr<NesterovBaseCommon> nbc,
    std::shared_ptr<RouteBase> rb,
    std::vector<std::shared_ptr<PlacerBase>>& pbVec,
    std::vector<std::shared_ptr<NesterovBase>>& nbVec,
    bool draw_bins,
    odb::dbInst* inst)
{
  setDebugOn(true);

  pbc_ = std::move(pbc);
  nbc_ = std::move(nbc);
  rb_ = std::move(rb);
  pbVec_ = pbVec;
  nbVec_ = nbVec;
  np_ = np;
  draw_bins_ = draw_bins;
  mode_ = Nesterov;

  if (!gui::Gui::enabled()) {
    return;
  }
  // Setup charts
  gui::Gui* gui = gui::Gui::get();
  main_chart_ = gui->addChart("GPL", "Iteration", {"HPWL (μm)", "Overflow"});
  main_chart_->setXAxisFormat("%d");
  main_chart_->setYAxisFormats({"%.2e", "%.2f"});
  main_chart_->setYAxisMin({std::nullopt, 0});

  density_chart_ = gui->addChart(
      "GPL Density Penalty", "Iteration", {"DensityPenalty", "phiCoef"});
  density_chart_->setXAxisFormat("%d");
  density_chart_->setYAxisFormats({"%.2e", "%.2f"});
  density_chart_->setYAxisMin({0.0, nbc_->getNbVars().minPhiCoef});

  stepLength_chart_ = gui->addChart(
      "GPL StepLength",
      "Iteration",
      {"StepLength", "CoordiDistance", "GradDistance", "Std area"});
  stepLength_chart_->setXAxisFormat("%d");
  stepLength_chart_->setYAxisFormats({"%.2e", "%.2f", "%.2f", "%.2f"});
  stepLength_chart_->setYAxisMin({0.0, 0.0, 0.0, 0.0});

  routing_chart_ = gui->addChart(
      "GPL Routing",
      "Iteration",
      {"avg RUDY", "Std area", "% Overflow Tiles", "Total RUDY Overflow"});
  routing_chart_->setXAxisFormat("%d");
  routing_chart_->setYAxisFormats({"%.2f", "%.2f", "%.2f", "%.2f"});
  routing_chart_->setYAxisMin({0.0, 0.0, 0.0, 0.0});

  initHeatmap();
  if (inst) {
    for (size_t idx = 0; idx < nbc_->getGCells().size(); ++idx) {
      auto cell = nbc_->getGCellByIndex(idx);
      if (cell->contains(inst)) {
        selected_ = idx;
        break;
      }
    }
  }

  for (const auto& nb : nbVec_) {
    for (size_t idx = 0; idx < nb->getGCells().size(); ++idx) {
      GCellHandle cell_handle = nb->getGCells()[idx];
      if (cell_handle->contains(inst)) {
        nb_selected_index_ = &nb - nbVec_.data();
        break;
      }
    }
  }
}

void GraphicsImpl::initHeatmap()
{
  addMultipleChoiceSetting(
      "Type",
      "Type:",
      []() {
        return std::vector<std::string>{
            "Density", "Overflow", "Overflow Normalized"};
      },
      [this]() -> std::string {
        switch (heatmap_type_) {
          case Density:
            return "Density";
          case Overflow:
            return "Overflow";
          case OverflowMinMax:
            return "Overflow Normalized";
        }
        return "Density";
      },
      [this](const std::string& value) {
        if (value == "Density") {
          heatmap_type_ = Density;
        } else if (value == "Overflow") {
          heatmap_type_ = Overflow;
        } else if (value == "Overflow Normalized") {
          heatmap_type_ = OverflowMinMax;
        } else {
          heatmap_type_ = Density;
        }
      });

  setBlock(pbc_->db()->getChip()->getBlock());
  registerHeatMap();
}

void GraphicsImpl::drawBounds(gui::Painter& painter)
{
  // draw core bounds
  auto& die = pbc_->getDie();
  painter.setPen(gui::Painter::kYellow, /* cosmetic */ true);
  painter.drawLine(die.coreLx(), die.coreLy(), die.coreUx(), die.coreLy());
  painter.drawLine(die.coreUx(), die.coreLy(), die.coreUx(), die.coreUy());
  painter.drawLine(die.coreUx(), die.coreUy(), die.coreLx(), die.coreUy());
  painter.drawLine(die.coreLx(), die.coreUy(), die.coreLx(), die.coreLy());
}

void GraphicsImpl::drawInitial(gui::Painter& painter)
{
  drawBounds(painter);

  painter.setPen(gui::Painter::kWhite, /* cosmetic */ true);
  for (auto& inst : pbc_->placeInsts()) {
    int lx = inst->lx();
    int ly = inst->ly();
    int ux = inst->ux();
    int uy = inst->uy();

    gui::Painter::Color color = gui::Painter::kDarkGreen;
    color.a = 180;
    painter.setBrush(color);
    painter.drawRect({lx, ly, ux, uy});
  }
}

void GraphicsImpl::drawForce(gui::Painter& painter)
{
  for (size_t nb_idx = 0; nb_idx < nbVec_.size(); ++nb_idx) {
    const auto& nb = nbVec_[nb_idx];
    const auto& bins = nb->getBins();
    if (bins.empty()) {
      continue;
    }
    const auto& bin = *bins.begin();
    const auto size = std::max(bin.dx(), bin.dy());
    if (size * painter.getPixelsPerDBU() < 10) {  // too small
      return;
    }
    float efMax = 0;
    int max_len = std::numeric_limits<int>::max();
    for (auto& bin : bins) {
      efMax = std::max(efMax,
                       std::hypot(bin.electroForceX(), bin.electroForceY()));
      max_len = std::min({max_len, bin.dx(), bin.dy()});
    }

    for (auto& bin : bins) {
      float fx = bin.electroForceX();
      float fy = bin.electroForceY();
      float f = std::hypot(fx, fy);
      float ratio = f / efMax;
      float dx = fx / f * max_len * ratio;
      float dy = fy / f * max_len * ratio;

      int cx = bin.cx();
      int cy = bin.cy();

      gui::Painter::Color color
          = region_colors_[nb_idx % region_colors_.size()];
      painter.setPen(color, true);
      painter.drawLine(cx, cy, cx + dx, cy + dy);

      // Draw a circle at the outer end of the line
      int circle_x = static_cast<int>(cx + dx);
      int circle_y = static_cast<int>(cy + dy);
      float bin_area = bin.dx() * bin.dy();
      int circle_radius = static_cast<int>(0.05 * std::sqrt(bin_area / M_PI));
      painter.setPen(color, true);
      painter.drawCircle(circle_x, circle_y, circle_radius);
    }
  }
}

void GraphicsImpl::drawCells(const std::vector<GCellHandle>& cells,
                             gui::Painter& painter,
                             size_t nb_index)
{
  for (const auto& handle : cells) {
    const GCell* gCell = handle;
    drawSingleGCell(gCell, painter, nb_index);
  }
}

void GraphicsImpl::drawCells(const std::vector<GCell*>& cells,
                             gui::Painter& painter)
{
  for (const auto& gCell : cells) {
    drawSingleGCell(gCell, painter);
  }
}

void GraphicsImpl::drawSingleGCell(const GCell* gCell,
                                   gui::Painter& painter,
                                   size_t nb_index)
{
  const int gcx = gCell->dCx();
  const int gcy = gCell->dCy();

  int xl = gcx - gCell->dx() / 2;
  int yl = gcy - gCell->dy() / 2;
  int xh = gcx + gCell->dx() / 2;
  int yh = gcy + gCell->dy() / 2;

  gui::Painter::Color color;
  // Highlight modified instances (overrides base color, unless selected)
  switch (gCell->changeType()) {
    case GCell::GCellChange::kRoutability:
      color = gui::Painter::kWhite;
      color.a = 75;
      break;
    case GCell::GCellChange::kNewInstance:
      color = gui::Painter::kDarkRed;
      break;
    case GCell::GCellChange::kDownsize:
      color = gui::Painter::kDarkBlue;
      break;
    case GCell::GCellChange::kUpsize:
      color = gui::Painter::kOrange;
      break;
    case GCell::GCellChange::kResizeNoChange:
      color = gui::Painter::kDarkYellow;
      break;
    default:
      if (gCell->isInstance()) {
        color = gCell->isLocked()
                    ? gui::Painter::kTurquoise
                    : instances_colors_[nb_index % instances_colors_.size()];
      } else if (gCell->isFiller()) {
        // Use different colors for each NesterovBase
        color = region_colors_[nb_index % region_colors_.size()];
      }
      color.a = 180;
      break;
  }

  // Highlight selection (highest priority)
  if (selected_ != kInvalidIndex && gCell == nbc_->getGCellByIndex(selected_)) {
    color = gui::Painter::kYellow;
    color.a = 180;
  }

  gui::Painter::Color outline = gui::Painter::kBlack;
  outline.a = 150;
  painter.setPen(outline, /*cosmetic=*/false, /*width=*/1);
  painter.setBrush(color);
  painter.drawRect({xl, yl, xh, yh});
}

void GraphicsImpl::drawNesterov(gui::Painter& painter)
{
  drawBounds(painter);
  if (draw_bins_) {
    // Draw the bins
    painter.setPen(gui::Painter::kTransparent);

    for (const auto& nb : nbVec_) {
      for (auto& bin : nb->getBins()) {
        int density = bin.getDensity() * 50 + 20;
        gui::Painter::Color color;
        if (density > 255) {
          color = {255, 165, 0, 180};  // orange = out of the range
        } else {
          density = 255 - std::max(density, 20);
          color = {density, density, density, 180};
        }

        painter.setBrush(color);
        painter.drawRect({bin.lx(), bin.ly(), bin.ux(), bin.uy()});
      }
    }
  }

  // Draw the placeable objects
  painter.setPen(gui::Painter::kWhite);
  drawCells(nbc_->getGCells(), painter);
  for (size_t nb_idx = 0; nb_idx < nbVec_.size(); ++nb_idx) {
    const auto& nb = nbVec_[nb_idx];
    drawCells(nb->getGCells(), painter, nb_idx);
  }

  // Create lighter versions of the region_colors_ with alpha 50
  std::vector<gui::Painter::Color> light_colors;
  light_colors.reserve(region_colors_.size());
  for (const auto& color : region_colors_) {
    light_colors.emplace_back(color.r, color.g, color.b, 50);
  }

  for (size_t pb_idx = 0; pb_idx < pbVec_.size(); ++pb_idx) {
    const auto& pb = pbVec_[pb_idx];
    gui::Painter::Color color = light_colors[pb_idx % light_colors.size()];
    painter.setBrush(color);

    for (auto& pb_inst : pb->nonPlaceInsts()) {
      painter.drawRect(
          {pb_inst->lx(), pb_inst->ly(), pb_inst->ux(), pb_inst->uy()});
    }
  }

  // Draw lines to neighbors
  if (selected_ != kInvalidIndex && nbc_->getGCellByIndex(selected_)) {
    painter.setPen(gui::Painter::kYellow, true);
    for (GPin* pin : nbc_->getGCellByIndex(selected_)->gPins()) {
      GNet* net = pin->getGNet();
      if (!net) {
        continue;
      }
      for (GPin* other_pin : net->getGPins()) {
        GCell* neighbor = other_pin->getGCell();
        if (neighbor == nbc_->getGCellByIndex(selected_)) {
          continue;
        }
        painter.drawLine(
            pin->cx(), pin->cy(), other_pin->cx(), other_pin->cy());
      }
    }

    // Draw gradient direction lines in the GUI from the GCell center.
    // We scale vectors to fit nicely within the cell (similar to drawForce()).
    const GCell* gcell = nbc_->getGCellByIndex(selected_);
    auto wlCoeffX = np_->getWireLengthCoefX();
    auto wlCoeffY = np_->getWireLengthCoefY();
    size_t nb_index = 0;
    if (nb_selected_index_ != kInvalidIndex) {
      nb_index = nb_selected_index_;
    } else {
      logger_->warn(
          utl::GPL, 317, "Selected instance not found in any NesterovBase");
    }
    FloatPoint densityGrad = nbVec_[nb_index]->getDensityGradient(gcell);
    FloatPoint wlGrad
        = nbc_->getWireLengthGradientWA(gcell, wlCoeffX, wlCoeffY);
    const int cx = gcell->dCx();
    const int cy = gcell->dCy();

    // Calculate the maximum length for the lines based on the GCell size
    const int max_len = std::max(1, std::min(gcell->dx(), gcell->dy()));
    const float target_len = 0.45f * static_cast<float>(max_len);

    // Determine the maximum magnitude for proper scaling
    const float wl_magnitude = std::hypot(wlGrad.x, wlGrad.y);
    const float densityPenalty = nbVec_[nb_index]->getDensityPenalty();
    const float density_magnitude = std::hypot(densityPenalty * densityGrad.x,
                                               densityPenalty * densityGrad.y);
    const float overall_x = wlGrad.x + (densityPenalty * densityGrad.x);
    const float overall_y = wlGrad.y + (densityPenalty * densityGrad.y);
    const float overall_magnitude = std::hypot(overall_x, overall_y);
    const float max_magnitude
        = std::max({wl_magnitude, density_magnitude, overall_magnitude});

    auto scaleVector = [&](float vx, float vy) -> std::pair<float, float> {
      const float magnitude = std::hypot(vx, vy);
      if (magnitude <= std::numeric_limits<float>::epsilon()) {
        return {0.0f, 0.0f};
      }
      return {vx / max_magnitude * target_len, vy / max_magnitude * target_len};
    };

    // Draw WL gradient line
    {
      auto [dx, dy] = scaleVector(wlGrad.x, wlGrad.y);
      painter.setPen(gui::Painter::kRed, true);  // Use red for WL gradient
      painter.drawLine(
          cx, cy, cx + static_cast<int>(dx), cy + static_cast<int>(dy));
    }

    // Draw Density gradient line
    {
      const float scaled_dx = densityPenalty * densityGrad.x;
      const float scaled_dy = densityPenalty * densityGrad.y;
      auto [dx, dy] = scaleVector(scaled_dx, scaled_dy);
      painter.setPen(gui::Painter::kBlue,
                     true);  // Use blue for Density gradient
      painter.drawLine(
          cx, cy, cx + static_cast<int>(dx), cy + static_cast<int>(dy));
    }

    // Draw Overall gradient line
    {
      auto [dx, dy] = scaleVector(overall_x, overall_y);
      painter.setPen(gui::Painter::kBlack,
                     true);  // Use black for Overall gradient
      painter.drawLine(
          cx, cy, cx + static_cast<int>(dx), cy + static_cast<int>(dy));
    }
  }

  // Draw force direction lines
  if (draw_bins_) {
    drawForce(painter);
  }
}

void GraphicsImpl::drawMBFF(gui::Painter& painter)
{
  painter.setPen(gui::Painter::kYellow, /* cosmetic */ true);
  for (const auto& [start, end] : mbff_edges_) {
    painter.drawLine(start, end);
  }

  for (odb::dbInst* inst : mbff_cluster_) {
    odb::Rect bbox = inst->getBBox()->getBox();
    painter.drawRect(bbox);
  }
}

void GraphicsImpl::drawObjects(gui::Painter& painter)
{
  if (!enabled()) {
    return;
  }

  switch (mode_) {
    case Mbff:
      drawMBFF(painter);
      break;
    case Nesterov:
      drawNesterov(painter);
      break;
    case Initial:
      drawInitial(painter);
      break;
  }
}

void GraphicsImpl::reportSelected()
{
  if (selected_ == kInvalidIndex) {
    return;
  }
  const GCell* gcell = nbc_->getGCellByIndex(selected_);
  logger_->report("Inst: {}", gcell->getName());

  if (np_) {
    auto wlCoeffX = np_->getWireLengthCoefX();
    auto wlCoeffY = np_->getWireLengthCoefY();

    logger_->report("  Wire Length Gradient");
    for (auto& gPin : gcell->gPins()) {
      FloatPoint wlGradPin
          = nbc_->getWireLengthGradientPinWA(gPin, wlCoeffX, wlCoeffY);
      const float weight = gPin->getGNet()->getTotalWeight();
      logger_->report("          ({:+.2e}, {:+.2e}) (weight = {}) pin {}",
                      wlGradPin.x,
                      wlGradPin.y,
                      weight,
                      gPin->getPbPin()->getName());
    }

    FloatPoint wlGrad
        = nbc_->getWireLengthGradientWA(gcell, wlCoeffX, wlCoeffY);
    logger_->report("  sum wl  ({: .2e}, {: .2e})", wlGrad.x, wlGrad.y);

    size_t nb_index = 0;
    if (nb_selected_index_ != kInvalidIndex) {
      nb_index = nb_selected_index_;
    } else {
      logger_->warn(
          utl::GPL, 318, "Selected instance not found in any NesterovBase");
    }
    FloatPoint densityGrad = nbVec_[nb_index]->getDensityGradient(gcell);
    float densityPenalty = nbVec_[nb_index]->getDensityPenalty();
    logger_->report("  density ({: .2e}, {: .2e}) (penalty: {})",
                    densityPenalty * densityGrad.x,
                    densityPenalty * densityGrad.y,
                    densityPenalty);
    logger_->report("  overall ({: .2e}, {: .2e})",
                    wlGrad.x + densityPenalty * densityGrad.x,
                    wlGrad.y + densityPenalty * densityGrad.y);
  }
}

void GraphicsImpl::addIter(const int iter, const double overflow)
{
  if (!gui::Gui::enabled()) {
    return;
  }
  odb::dbBlock* block = pbc_->db()->getChip()->getBlock();
  main_chart_->addPoint(iter, {block->dbuToMicrons(nbc_->getHpwl()), overflow});

  if (density_chart_) {
    std::vector<double> values;
    if (!nbVec_.empty() && nbVec_[0]) {
      values.push_back((static_cast<double>(nbVec_[0]->getDensityPenalty())));
      values.push_back(static_cast<double>(nbVec_[0]->getStoredPhiCoef()));
    } else {
      values.push_back(0.0);
      values.push_back(0.0);
    }
    density_chart_->addPoint(iter, values);
  }

  if (stepLength_chart_) {
    std::vector<double> values;
    if (!nbVec_.empty() && nbVec_[0]) {
      values.push_back(static_cast<double>(nbVec_[0]->getStoredStepLength()));
      values.push_back(
          static_cast<double>(nbVec_[0]->getStoredCoordiDistance()));
      values.push_back(static_cast<double>(nbVec_[0]->getStoredGradDistance()));
      values.push_back(
          block->dbuAreaToMicrons(nbVec_[0]->getNesterovInstsArea()));
    } else {
      values.push_back(0.0);
      values.push_back(0.0);
      values.push_back(0.0);
      values.push_back(0.0);
    }
    stepLength_chart_->addPoint(iter, values);
  }

  if (routing_chart_) {
    std::vector<double> values;
    if (!nbVec_.empty() && nbVec_[0]) {
      values.push_back(static_cast<double>(rb_->getRudyAverage()));
      values.push_back(
          block->dbuAreaToMicrons(nbVec_[0]->getNesterovInstsArea()));
      const double total_tiles = static_cast<double>(rb_->getTotalTilesCount());
      values.push_back(total_tiles > 0.0 ? (static_cast<double>(
                                                rb_->getOverflowedTilesCount())
                                            / total_tiles * 100.0)
                                         : 0.0);
      values.push_back((rb_->getTotalRudyOverflow()));
    } else {
      values.push_back(0.0);
      values.push_back(0.0);
      values.push_back(0.0);
      values.push_back(0.0);
      values.push_back(0.0);
    }
    routing_chart_->addPoint(iter, values);
  }
}

void GraphicsImpl::addTimingDrivenIter(const int iter)
{
  main_chart_->addVerticalMarker(iter, gui::Painter::kTurquoise);
  routing_chart_->addVerticalMarker(iter, gui::Painter::kTurquoise);
}

void GraphicsImpl::addRoutabilitySnapshot(int iter)
{
  main_chart_->addVerticalMarker(iter, gui::Painter::kYellow);
  routing_chart_->addVerticalMarker(iter, gui::Painter::kYellow);
}

void GraphicsImpl::addRoutabilityIter(const int iter, const bool revert)
{
  gui::Painter::Color color
      = revert ? gui::Painter::kRed : gui::Painter::kGreen;
  main_chart_->addVerticalMarker(iter, color);
  routing_chart_->addVerticalMarker(
      iter, rb_->isMinRc() ? gui::Painter::kMagenta : gui::Painter::kBlack);
}

void GraphicsImpl::cellPlotImpl(bool pause)
{
  gui::Gui::get()->redraw();
  if (pause) {
    reportSelected();
    gui::Gui::get()->pause();
  }
}

void GraphicsImpl::mbffMapping(const LineSegs& segs)
{
  mbff_edges_ = segs;
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
  mbff_edges_.clear();
}

void GraphicsImpl::mbffFlopClusters(const std::vector<odb::dbInst*>& ffs)
{
  mbff_cluster_ = ffs;
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
  mbff_cluster_.clear();
}

gui::SelectionSet GraphicsImpl::select(odb::dbTechLayer* layer,
                                       const odb::Rect& region)
{
  selected_ = kInvalidIndex;

  if (layer || !nbc_) {
    return gui::SelectionSet();
  }

  for (size_t idx = 0; idx < nbc_->getGCells().size(); ++idx) {
    auto cell = nbc_->getGCellByIndex(idx);
    const int gcx = cell->dCx();
    const int gcy = cell->dCy();

    int xl = gcx - cell->dx() / 2;
    int yl = gcy - cell->dy() / 2;
    int xh = gcx + cell->dx() / 2;
    int yh = gcy + cell->dy() / 2;

    if (region.xMax() < xl || region.yMax() < yl || region.xMin() > xh
        || region.yMin() > yh) {
      continue;
    }

    selected_ = idx;
    odb::dbInst* db_inst
        = cell->isInstance() ? cell->insts().front()->dbInst() : nullptr;
    if (db_inst != nullptr) {
      for (size_t nb_idx = 0; nb_idx < nbVec_.size(); ++nb_idx) {
        for (size_t gc_idx = 0; gc_idx < nbVec_[nb_idx]->getGCells().size();
             ++gc_idx) {
          GCellHandle cell_handle = nbVec_[nb_idx]->getGCells()[gc_idx];
          if (cell_handle->contains(db_inst)) {
            nb_selected_index_ = nb_idx;
            break;
          }
        }
      }
    }
    gui::Gui::get()->redraw();
    if (cell->isInstance()) {
      reportSelected();
      gui::SelectionSet selected;
      for (Instance* inst : cell->insts()) {
        selected.insert(gui::Gui::get()->makeSelected(inst->dbInst()));
      }
      return selected;
    }
  }
  return gui::SelectionSet();
}

void GraphicsImpl::status(const std::string_view message)
{
  gui::Gui::get()->status(std::string(message));
}

double GraphicsImpl::getGridXSize() const
{
  const BinGrid& grid = nbVec_[0]->getBinGrid();
  return grid.getBinSizeX() / (double) getBlock()->getDbUnitsPerMicron();
}

double GraphicsImpl::getGridYSize() const
{
  const BinGrid& grid = nbVec_[0]->getBinGrid();
  return grid.getBinSizeY() / (double) getBlock()->getDbUnitsPerMicron();
}

odb::Rect GraphicsImpl::getBounds() const
{
  return getBlock()->getCoreArea();
}

bool GraphicsImpl::populateMap()
{
  BinGrid& grid = nbVec_[0]->getBinGrid();
  odb::dbBlock* block = pbc_->db()->getChip()->getBlock();

  double min_value = std::numeric_limits<double>::max();
  double max_value = std::numeric_limits<double>::lowest();

  if (heatmap_type_ == OverflowMinMax) {
    for (const Bin& bin : grid.getBins()) {
      int64_t binArea = bin.getBinArea();
      const float scaledBinArea
          = static_cast<float>(binArea * bin.getTargetDensity());

      double value
          = std::max(0.0f,
                     static_cast<float>(bin.getInstPlacedAreaUnscaled())
                         + static_cast<float>(bin.getNonPlaceAreaUnscaled())
                         - scaledBinArea);
      value = block->dbuAreaToMicrons(value);

      min_value = std::min(min_value, value);
      max_value = std::max(max_value, value);
    }
  }

  for (const Bin& bin : grid.getBins()) {
    odb::Rect box(bin.lx(), bin.ly(), bin.ux(), bin.uy());
    double value = 0.0;

    if (heatmap_type_ == Density) {
      value = bin.getDensity() * 100.0;
    } else if (heatmap_type_ == Overflow || heatmap_type_ == OverflowMinMax) {
      int64_t binArea = bin.getBinArea();
      const float scaledBinArea
          = static_cast<float>(binArea * bin.getTargetDensity());

      double raw_value
          = std::max(0.0f,
                     static_cast<float>(bin.getInstPlacedAreaUnscaled())
                         + static_cast<float>(bin.getNonPlaceAreaUnscaled())
                         - scaledBinArea);
      raw_value = block->dbuAreaToMicrons(raw_value);

      if (heatmap_type_ == OverflowMinMax && max_value > min_value) {
        value = (raw_value - min_value) / (max_value - min_value) * 100.0;
      } else {
        value = raw_value;
      }
    }

    addToMap(box, value);
  }

  return true;
}

void GraphicsImpl::populateXYGrid()
{
  BinGrid& grid = nbVec_[0]->getBinGrid();
  std::vector<Bin>& bin = grid.getBins();
  int x_grid = grid.getBinCntX();
  int y_grid = grid.getBinCntY();

  std::vector<int> x_grid_set, y_grid_set;
  x_grid_set.reserve(x_grid + 1);
  y_grid_set.reserve(y_grid + 1);

  x_grid_set.push_back(bin[0].lx());
  y_grid_set.push_back(bin[0].ly());

  for (int x = 0; x < x_grid && x < static_cast<int>(bin.size()); x++) {
    x_grid_set.push_back(bin[x].ux());
  }

  for (int y = 0; y < y_grid; y++) {
    size_t index = static_cast<size_t>(y) * static_cast<size_t>(x_grid);
    if (index < bin.size()) {
      y_grid_set.push_back(bin[index].uy());
    }
  }
  setXYMapGrid(x_grid_set, y_grid_set);
}

void GraphicsImpl::combineMapData(bool base_has_value,
                                  double& base,
                                  const double new_data,
                                  const double data_area,
                                  const double intersection_area,
                                  const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

bool GraphicsImpl::enabled()
{
  return debug_on_ && gui::Gui::enabled();
}

void GraphicsImpl::addFrameLabelImpl(const odb::Rect& bbox,
                                     std::string_view label,
                                     std::string_view label_name,
                                     int image_width_px)
{
  gui::Gui* gui = gui::Gui::get();

  int label_x = bbox.xMin() + 300;
  int label_y = bbox.yMin() + 300;

  gui::Painter::Color color = gui::Painter::kYellow;
  gui::Painter::Anchor anchor = gui::Painter::kBottomLeft;

  int font_size = std::clamp(image_width_px / 50, 15, 24);

  gui->addLabel(label_x,
                label_y,
                std::string(label),
                color,
                font_size,
                anchor,
                std::string(label_name));
}

void GraphicsImpl::saveLabeledImageImpl(std::string_view path,
                                        std::string_view label,
                                        bool select_buffers,
                                        std::string_view heatmap_control,
                                        int image_width_px)
{
  gui::Gui* gui = gui::Gui::get();

  odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();

  if (!heatmap_control.empty()) {
    gui->setDisplayControlsVisible(std::string(heatmap_control), true);
  }

  if (select_buffers) {
    gui->select("Inst", "", "Description", "Timing Repair Buffer", true, -1);
  }

  static int label_id = 0;
  std::string label_name = fmt::format("auto_label_{}", label_id++);

  addFrameLabel(bbox, label, label_name, image_width_px);
  gui->saveImage(std::string(path));
  gui->deleteLabel(label_name);

  if (!heatmap_control.empty()) {
    gui->setDisplayControlsVisible(std::string(heatmap_control), false);
  }

  gui->clearSelections();
}

int GraphicsImpl::gifStart(std::string_view path)
{
  return gui::Gui::get()->gifStart(std::string(path));
}

void GraphicsImpl::gifAddFrameImpl(int key,
                                   const odb::Rect& region,
                                   int width_px,
                                   double dbu_per_pixel,
                                   std::optional<int> delay)
{
  gui::Gui::get()->gifAddFrame(key, region, width_px, dbu_per_pixel, delay);
}

void GraphicsImpl::deleteLabel(std::string_view label_name)
{
  gui::Gui::get()->deleteLabel(std::string(label_name));
}

void GraphicsImpl::gifEnd(int key)
{
  gui::Gui::get()->gifEnd(key);
}

void GraphicsImpl::setDisplayControl(std::string_view name, bool value)
{
  gui::Gui::get()->setDisplayControlsVisible(std::string(name), value);
}

}  // namespace gpl
