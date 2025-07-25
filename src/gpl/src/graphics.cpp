// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "graphics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nesterovBase.h"
#include "nesterovPlace.h"
#include "placerBase.h"
#include "utl/Logger.h"

namespace gpl {

Graphics::Graphics(utl::Logger* logger)
    : HeatMapDataSource(logger, "gpl", "gpl"), logger_(logger), mode_(Mbff)
{
  gui::Gui::get()->registerRenderer(this);
}

Graphics::Graphics(utl::Logger* logger,
                   std::shared_ptr<PlacerBaseCommon> pbc,
                   std::vector<std::shared_ptr<PlacerBase>>& pbVec)
    : HeatMapDataSource(logger, "gpl", "gpl"),
      pbc_(std::move(pbc)),
      pbVec_(pbVec),
      logger_(logger),
      mode_(Initial)
{
  gui::Gui::get()->registerRenderer(this);
}

Graphics::Graphics(utl::Logger* logger,
                   NesterovPlace* np,
                   std::shared_ptr<PlacerBaseCommon> pbc,
                   std::shared_ptr<NesterovBaseCommon> nbc,
                   std::vector<std::shared_ptr<PlacerBase>>& pbVec,
                   std::vector<std::shared_ptr<NesterovBase>>& nbVec,
                   bool draw_bins,
                   odb::dbInst* inst)
    : HeatMapDataSource(logger, "gpl", "gpl"),
      pbc_(std::move(pbc)),
      nbc_(std::move(nbc)),
      pbVec_(pbVec),
      nbVec_(nbVec),
      np_(np),
      draw_bins_(draw_bins),
      logger_(logger),
      mode_(Nesterov)
{
  gui::Gui* gui = gui::Gui::get();
  gui->registerRenderer(this);

  // Setup the chart
  chart_ = gui->addChart("GPL", "Iteration", {"HPWL (μm)", "Overflow"});
  chart_->setXAxisFormat("%d");
  chart_->setYAxisFormats({"%.2e", "%.2f"});
  chart_->setYAxisMin({std::nullopt, 0});

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
}

void Graphics::initHeatmap()
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

void Graphics::drawBounds(gui::Painter& painter)
{
  // draw core bounds
  auto& die = pbc_->getDie();
  painter.setPen(gui::Painter::kYellow, /* cosmetic */ true);
  painter.drawLine(die.coreLx(), die.coreLy(), die.coreUx(), die.coreLy());
  painter.drawLine(die.coreUx(), die.coreLy(), die.coreUx(), die.coreUy());
  painter.drawLine(die.coreUx(), die.coreUy(), die.coreLx(), die.coreUy());
  painter.drawLine(die.coreLx(), die.coreUy(), die.coreLx(), die.coreLy());
}

void Graphics::drawInitial(gui::Painter& painter)
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

void Graphics::drawForce(gui::Painter& painter)
{
  for (const auto& nb : nbVec_) {
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

      painter.setPen(gui::Painter::kRed, true);
      painter.drawLine(cx, cy, cx + dx, cy + dy);

      // Draw a circle at the outer end of the line
      int circle_x = static_cast<int>(cx + dx);
      int circle_y = static_cast<int>(cy + dy);
      float bin_area = bin.dx() * bin.dy();
      int circle_radius = static_cast<int>(0.05 * std::sqrt(bin_area / M_PI));
      painter.setPen(gui::Painter::kRed, true);
      painter.drawCircle(circle_x, circle_y, circle_radius);
    }
  }
}

void Graphics::drawCells(const std::vector<GCellHandle>& cells,
                         gui::Painter& painter)
{
  for (const auto& handle : cells) {
    const GCell* gCell = handle;
    drawSingleGCell(gCell, painter);
  }
}

void Graphics::drawCells(const std::vector<GCell*>& cells,
                         gui::Painter& painter)
{
  for (const auto& gCell : cells) {
    drawSingleGCell(gCell, painter);
  }
}

void Graphics::drawSingleGCell(const GCell* gCell, gui::Painter& painter)
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
      color = {255, 255, 255, 100};  // White
      break;
    case GCell::GCellChange::kTimingDriven:
      color = {180, 150, 255, 100};  // Light purple
      break;
    default:
      if (gCell->isInstance()) {
        color = gCell->isLocked() ? gui::Painter::kDarkCyan
                                  : gui::Painter::kDarkGreen;
      } else if (gCell->isFiller()) {
        color = gui::Painter::kDarkMagenta;
      }
      color.a = 180;
      break;
  }

  // Highlight selection (highest priority)
  if (selected_ != kInvalidIndex && gCell == nbc_->getGCellByIndex(selected_)) {
    color = gui::Painter::kYellow;
    color.a = 180;
  }

  painter.setBrush(color);
  painter.drawRect({xl, yl, xh, yh});

  if (gCell->isInstance()) {
    odb::dbInst* db_inst = gCell->insts()[0]->dbInst();
    if (db_inst != nullptr) {
      odb::dbBox* bbox = db_inst->getBBox();
      if (bbox != nullptr) {
        int origLx = bbox->xMin();
        int origLy = bbox->yMin();
        int origUx = bbox->xMax();
        int origUy = bbox->yMax();

        gui::Painter::Color outline = gui::Painter::kBlack;
        outline.a = 150;  // Semi-transparent

        painter.setPen(outline, /*cosmetic=*/false, /*width=*/1);
        painter.drawRect({origLx, origLy, origUx, origUy});
      }
    }
  }
}

void Graphics::drawNesterov(gui::Painter& painter)
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
  for (const auto& nb : nbVec_) {
    drawCells(nb->getGCells(), painter);
  }

  painter.setBrush(gui::Painter::Color(gui::Painter::kLightGray, 50));
  for (const auto& pb : pbVec_) {
    for (auto& inst : pb->nonPlaceInsts()) {
      painter.drawRect({inst->lx(), inst->ly(), inst->ux(), inst->uy()});
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
  }

  // Draw force direction lines
  if (draw_bins_) {
    drawForce(painter);
  }
}

void Graphics::drawMBFF(gui::Painter& painter)
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

void Graphics::drawObjects(gui::Painter& painter)
{
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

void Graphics::reportSelected()
{  // TODO: PD_FIX
  if (selected_ == kInvalidIndex) {
    return;
  }
  logger_->report("Inst: {}", nbc_->getGCellByIndex(selected_)->getName());

  if (np_) {
    auto wlCoeffX = np_->getWireLengthCoefX();
    auto wlCoeffY = np_->getWireLengthCoefY();

    logger_->report("  Wire Length Gradient");
    for (auto& gPin : nbc_->getGCellByIndex(selected_)->gPins()) {
      FloatPoint wlGrad
          = nbc_->getWireLengthGradientPinWA(gPin, wlCoeffX, wlCoeffY);
      const float weight = gPin->getGNet()->getTotalWeight();
      logger_->report("          ({:+.2e}, {:+.2e}) (weight = {}) pin {}",
                      wlGrad.x,
                      wlGrad.y,
                      weight,
                      gPin->getPbPin()->getName());
    }

    FloatPoint wlGrad = nbc_->getWireLengthGradientWA(
        nbc_->getGCellByIndex(selected_), wlCoeffX, wlCoeffY);
    logger_->report("  sum wl  ({: .2e}, {: .2e})", wlGrad.x, wlGrad.y);

    auto densityGrad
        = nbVec_[0]->getDensityGradient(nbc_->getGCellByIndex(selected_));
    float densityPenalty = nbVec_[0]->getDensityPenalty();
    logger_->report("  density ({: .2e}, {: .2e}) (penalty: {})",
                    densityPenalty * densityGrad.x,
                    densityPenalty * densityGrad.y,
                    densityPenalty);
    logger_->report("  overall ({: .2e}, {: .2e})",
                    wlGrad.x + densityPenalty * densityGrad.x,
                    wlGrad.y + densityPenalty * densityGrad.y);
  }
}

void Graphics::addIter(const int iter, const double overflow)
{
  odb::dbBlock* block = pbc_->db()->getChip()->getBlock();
  chart_->addPoint(iter, {block->dbuToMicrons(nbc_->getHpwl()), overflow});
}

void Graphics::addTimingDrivenIter(const int iter)
{
  chart_->addVerticalMarker(iter, gui::Painter::kTurquoise);
}

void Graphics::addRoutabilitySnapshot(int iter)
{
  chart_->addVerticalMarker(iter, gui::Painter::kYellow);
}

void Graphics::addRoutabilityIter(const int iter, const bool revert)
{
  gui::Painter::Color color
      = revert ? gui::Painter::kRed : gui::Painter::kGreen;
  chart_->addVerticalMarker(iter, color);
}

void Graphics::cellPlot(bool pause)
{
  gui::Gui::get()->redraw();
  if (pause) {
    reportSelected();
    gui::Gui::get()->pause();
  }
}

void Graphics::mbffMapping(const LineSegs& segs)
{
  mbff_edges_ = segs;
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
  mbff_edges_.clear();
}

void Graphics::mbffFlopClusters(const std::vector<odb::dbInst*>& ffs)
{
  mbff_cluster_ = ffs;
  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
  mbff_cluster_.clear();
}

gui::SelectionSet Graphics::select(odb::dbTechLayer* layer,
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

void Graphics::status(const std::string& message)
{
  gui::Gui::get()->status(message);
}

double Graphics::getGridXSize() const
{
  const BinGrid& grid = nbVec_[0]->getBinGrid();
  return grid.getBinSizeX() / (double) getBlock()->getDbUnitsPerMicron();
}

double Graphics::getGridYSize() const
{
  const BinGrid& grid = nbVec_[0]->getBinGrid();
  return grid.getBinSizeY() / (double) getBlock()->getDbUnitsPerMicron();
}

odb::Rect Graphics::getBounds() const
{
  return getBlock()->getCoreArea();
}

bool Graphics::populateMap()
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

void Graphics::populateXYGrid()
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

void Graphics::combineMapData(bool base_has_value,
                              double& base,
                              const double new_data,
                              const double data_area,
                              const double intersection_area,
                              const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

/* static */
bool Graphics::guiActive()
{
  return gui::Gui::enabled();
}

void Graphics::addFrameLabel(gui::Gui* gui,
                             const odb::Rect& bbox,
                             const std::string& label,
                             const std::string& label_name,
                             int image_width_px)
{
  int label_x = bbox.xMin() + 300;
  int label_y = bbox.yMin() + 300;

  gui::Painter::Color color = gui::Painter::kYellow;
  gui::Painter::Anchor anchor = gui::Painter::kBottomLeft;

  int font_size = std::clamp(image_width_px / 50, 15, 24);

  gui->addLabel(label_x, label_y, label, color, font_size, anchor, label_name);
}

void Graphics::saveLabeledImage(const std::string& path,
                                const std::string& label,
                                bool select_buffers,
                                const std::string& heatmap_control,
                                int image_width_px)
{
  gui::Gui* gui = getGuiObjectFromGraphics();
  odb::Rect bbox = pbc_->db()->getChip()->getBlock()->getBBox()->getBox();

  if (!heatmap_control.empty()) {
    gui->setDisplayControlsVisible(heatmap_control, true);
  }

  if (select_buffers) {
    gui->select("Inst", "", "Description", "Timing Repair Buffer", true, -1);
  }

  static int label_id = 0;
  std::string label_name = fmt::format("auto_label_{}", label_id++);

  addFrameLabel(gui, bbox, label, label_name, image_width_px);
  gui->saveImage(path);
  gui->deleteLabel(label_name);

  if (!heatmap_control.empty()) {
    gui->setDisplayControlsVisible(heatmap_control, false);
  }

  gui->clearSelections();
}

}  // namespace gpl
