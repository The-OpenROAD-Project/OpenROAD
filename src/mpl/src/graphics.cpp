// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "graphics.h"

#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clusterEngine.h"
#include "gui/gui.h"
#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {

Graphics::Graphics(bool coarse,
                   bool fine,
                   odb::dbBlock* block,
                   utl::Logger* logger)
    : coarse_(coarse),
      fine_(fine),
      show_bundled_nets_(false),
      show_clusters_ids_(false),
      skip_steps_(false),
      is_skipping_(false),
      only_final_result_(false),
      block_(block),
      logger_(logger)
{
  gui::Gui* gui = gui::Gui::get();
  gui->registerRenderer(this);

  // Setup the chart
  chart_ = gui->addChart("MPL", "Iteration", {"Area", "Outline", "WireLength"});
  chart_->setXAxisFormat("%d");
  chart_->setYAxisFormats({"%.2e", "%.2e", "%.2e"});
  chart_->setYAxisMin({0, 0, 0});
}

void Graphics::startCoarse()
{
  active_ = coarse_;
}

void Graphics::startFine()
{
  active_ = fine_;
}

void Graphics::startSA(const char* type,
                       const int max_num_step,
                       const int num_perturb_per_step)
{
  if (!active_) {
    return;
  }

  if (skip_steps_) {
    return;
  }

  if (target_cluster_id_ != -1 && !isTargetCluster()) {
    return;
  }

  logger_->report("------ Start {} ------", type);
  logger_->report("max_num_step = {} num_perturb_per_step = {}",
                  max_num_step,
                  num_perturb_per_step);
  best_norm_cost_ = std::numeric_limits<float>::max();
  skipped_ = 0;
  chart_->clearPoints();
  iter_ = 0;
}

void Graphics::endSA(const float norm_cost)
{
  if (!active_) {
    return;
  }

  if (skip_steps_) {
    return;
  }

  if (target_cluster_id_ != -1 && !isTargetCluster()) {
    return;
  }

  if (skipped_ > 0) {
    logger_->report("Skipped to end: {}", skipped_);
  }
  logger_->report("------ End (Iter {}) ------", iter_);
  report(norm_cost);
  gui::Gui::get()->pause();
}

bool Graphics::isTargetCluster()
{
  return current_cluster_->getId() == target_cluster_id_;
}

void Graphics::saStep(const std::vector<SoftMacro>& macros)
{
  resetPenalties();
  soft_macros_ = macros;
  hard_macros_.clear();
}

void Graphics::saStep(const std::vector<HardMacro>& macros)
{
  resetPenalties();
  hard_macros_ = macros;
  soft_macros_.clear();
}

template <typename T>
void Graphics::report(const std::optional<T>& value)
{
  if (value) {
    const PenaltyData& penalty_data = value.value();
    const float normalized_penalty
        = penalty_data.value / penalty_data.normalization_factor;

    logger_->report(
        "{:25}(Norm penalty {:>8.4f}) * (weight {:>8.4f}) = cost {:>8.4f}",
        penalty_data.name,
        normalized_penalty,
        penalty_data.weight,
        normalized_penalty * penalty_data.weight);
  }
}

void Graphics::report(const float norm_cost)
{
  report(area_penalty_);
  report(outline_penalty_);
  report(wirelength_penalty_);
  report(fence_penalty_);
  report(guidance_penalty_);
  report(boundary_penalty_);
  report(macro_blockage_penalty_);
  report(fixed_macros_penalty_);
  report(notch_penalty_);
  report(std::optional<PenaltyData>({"Total", 1.0f, norm_cost, 1.0f}));
}

void Graphics::drawResult()
{
  if (max_level_) {
    std::vector<std::vector<odb::Rect>> outlines(max_level_.value() + 1);
    int level = 0;
    fetchSoftAndHard(root_, hard_macros_, soft_macros_, outlines, level);
    outlines_ = std::move(outlines);
  }

  gui::Gui::get()->redraw();
  gui::Gui::get()->pause();
}

void Graphics::fetchSoftAndHard(Cluster* parent,
                                std::vector<HardMacro>& hard,
                                std::vector<SoftMacro>& soft,
                                std::vector<std::vector<odb::Rect>>& outlines,
                                int level)
{
  odb::Rect outline = parent->getBBox();
  outlines[level].push_back(outline);

  for (auto& child : parent->getChildren()) {
    switch (child->getClusterType()) {
      case HardMacroCluster: {
        std::vector<mpl::HardMacro*> hard_macros = child->getHardMacros();
        for (HardMacro* hard_macro : hard_macros) {
          hard.push_back(*hard_macro);
        }
        break;
      }
      case StdCellCluster: {
        if (child->isLeaf()) {
          soft.push_back(*child->getSoftMacro());
        } else {
          fetchSoftAndHard(child.get(), hard, soft, outlines, (level + 1));
        }
        break;
      }
      case MixedCluster: {
        fetchSoftAndHard(child.get(), hard, soft, outlines, (level + 1));
        break;
      }
    }
  }
}

void Graphics::penaltyCalculated(float norm_cost)
{
  if (!active_) {
    return;
  }
  iter_++;

  if (is_skipping_) {
    return;
  }

  if (target_cluster_id_ != -1 && !isTargetCluster()) {
    return;
  }

  bool drawing_last_step = skip_steps_ && !is_skipping_;

  if (norm_cost < best_norm_cost_ || drawing_last_step) {
    const float area = area_penalty_ ? area_penalty_.value().value : 0;
    const float outline = outline_penalty_ ? outline_penalty_.value().value : 0;
    const float wirelength
        = wirelength_penalty_ ? wirelength_penalty_.value().value : 0;
    chart_->addPoint(iter_, {area, outline, wirelength});

    logger_->report("------ Penalty (Iter {}) ------", iter_);
    report(norm_cost);

    if (skipped_ > 0) {
      logger_->report("Skipped: {}", skipped_);
      skipped_ = 0;
    }

    const char* type = !soft_macros_.empty() ? "SoftMacro" : "HardMacro";
    gui::Gui::get()->status(type);
    gui::Gui::get()->redraw();
    if (norm_cost < 0.99 * best_norm_cost_ || drawing_last_step) {
      gui::Gui::get()->pause();
    }
    best_norm_cost_ = norm_cost;

    if (drawing_last_step) {
      is_skipping_ = true;
    }
  } else {
    ++skipped_;
  }
}

void Graphics::resetPenalties()
{
  area_penalty_.reset();
  outline_penalty_.reset();
  wirelength_penalty_.reset();
  fence_penalty_.reset();
  guidance_penalty_.reset();
  boundary_penalty_.reset();
  macro_blockage_penalty_.reset();
  notch_penalty_.reset();
}

void Graphics::setNotchPenalty(const PenaltyData& penalty)
{
  notch_penalty_ = penalty;
}

void Graphics::setMacroBlockagePenalty(const PenaltyData& penalty)
{
  macro_blockage_penalty_ = penalty;
}

void Graphics::setFixedMacrosPenalty(const PenaltyData& penalty)
{
  fixed_macros_penalty_ = penalty;
}

void Graphics::setBoundaryPenalty(const PenaltyData& penalty)
{
  boundary_penalty_ = penalty;
}

void Graphics::setFencePenalty(const PenaltyData& penalty)
{
  fence_penalty_ = penalty;
}

void Graphics::setGuidancePenalty(const PenaltyData& penalty)
{
  guidance_penalty_ = penalty;
}

void Graphics::setAreaPenalty(const PenaltyData& penalty)
{
  area_penalty_ = penalty;
}

void Graphics::setOutlinePenalty(const PenaltyData& penalty)
{
  outline_penalty_ = penalty;
}

void Graphics::setWirelengthPenalty(const PenaltyData& penalty)
{
  wirelength_penalty_ = penalty;
}

void Graphics::setMaxLevel(const int max_level)
{
  max_level_ = max_level;
}

void Graphics::finishedClustering(PhysicalHierarchy* tree)
{
  root_ = tree->root.get();
  setXMarksSize();
}

// Mark to indicate blocked regions for pins.
void Graphics::setXMarksSize()
{
  const odb::Rect& die = block_->getDieArea();

  // Not too big/small
  x_mark_size_ = (die.dx() + die.dy()) * 0.02;
}

void Graphics::drawCluster(Cluster* cluster, gui::Painter& painter)
{
  const int lx = cluster->getX();
  const int ly = cluster->getY();
  const int ux = lx + cluster->getWidth();
  const int uy = ly + cluster->getHeight();
  odb::Rect bbox(lx, ly, ux, uy);

  painter.drawRect(bbox);

  for (auto& child : cluster->getChildren()) {
    drawCluster(child.get(), painter);
  }
}

void Graphics::drawAllBlockages(gui::Painter& painter)
{
  if (!macro_blockages_.empty()) {
    painter.setPen(gui::Painter::kGray, true);
    painter.setBrush(gui::Painter::kGray, gui::Painter::kDiagonal);

    for (const auto& blockage : macro_blockages_) {
      drawOffsetRect(blockage, "", painter);
    }
  }

  if (!placement_blockages_.empty()) {
    painter.setPen(gui::Painter::kGreen, true);
    painter.setBrush(gui::Painter::kGreen, gui::Painter::kDiagonal);

    for (const auto& blockage : placement_blockages_) {
      drawOffsetRect(blockage, "", painter);
    }
  }
}

void Graphics::drawFences(gui::Painter& painter)
{
  if (fences_.empty()) {
    return;
  }

  // slightly transparent dark yellow
  painter.setBrush(gui::Painter::Color(0x80, 0x80, 0x00, 150),
                   gui::Painter::kDiagonal);
  painter.setPen(gui::Painter::kDarkYellow, true);

  for (const auto& [macro_id, fence] : fences_) {
    drawOffsetRect(fence, std::to_string(macro_id), painter);
  }
}

void Graphics::drawOffsetRect(const odb::Rect& rect,
                              const std::string& center_text,
                              gui::Painter& painter)
{
  odb::Rect movable_rect = rect;
  movable_rect.moveDelta(outline_.xMin(), outline_.yMin());
  painter.drawRect(movable_rect);

  if (!center_text.empty()) {
    painter.drawString(movable_rect.xCenter(),
                       movable_rect.yCenter(),
                       gui::Painter::kCenter,
                       center_text);
  }
}

// We draw the shapes of SoftMacros, HardMacros and blockages based
// on the outline's origin.
void Graphics::drawObjects(gui::Painter& painter)
{
  if (root_ && !only_final_result_) {
    painter.setPen(gui::Painter::kRed, true);
    painter.setBrush(gui::Painter::kTransparent);
    drawCluster(root_, painter);
  }

  // Draw blockages and notches only during SA for SoftMacros
  if (!soft_macros_.empty()) {
    drawAllBlockages(painter);
  }

  painter.setPen(gui::Painter::kWhite, true);

  int i = 0;
  for (const auto& macro : soft_macros_) {
    if (!macro.isBlockage() && isSkippable(macro)) {
      continue;
    }

    setSoftMacroBrush(painter, macro);

    const int lx = macro.getX();
    const int ly = macro.getY();
    const int ux = lx + macro.getWidth();
    const int uy = ly + macro.getHeight();
    odb::Rect bbox(lx, ly, ux, uy);

    bbox.moveDelta(outline_.xMin(), outline_.yMin());

    std::string cluster_id_string;
    if (show_clusters_ids_) {
      Cluster* cluster = macro.getCluster();
      cluster_id_string = cluster ? std::to_string(cluster->getId()) : "fixed";
    } else {
      // Use the ID of the sequence pair itself.
      cluster_id_string = std::to_string(i++);
    }

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::kCenter,
                       cluster_id_string);
  }

  painter.setPen(gui::Painter::kWhite, true);

  i = 0;
  for (const auto& macro : hard_macros_) {
    if (isSkippable(macro)) {
      continue;
    }

    const int width = macro.getRealWidth();
    const int height = macro.getRealHeight();

    odb::Rect halo_bbox(macro.getX(),
                        macro.getY(),
                        macro.getX() + macro.getWidth(),
                        macro.getY() + macro.getHeight());
    odb::Rect macro_bbox(macro.getRealX(),
                         macro.getRealY(),
                         macro.getRealX() + width,
                         macro.getRealY() + height);

    halo_bbox.moveDelta(outline_.xMin(), outline_.yMin());
    macro_bbox.moveDelta(outline_.xMin(), outline_.yMin());

    painter.setBrush(gui::Painter::kDarkRed);
    painter.drawRect(halo_bbox);

    painter.setBrush(gui::Painter::kRed);
    painter.drawRect(macro_bbox);

    painter.drawString(macro_bbox.xCenter(),
                       macro_bbox.yCenter(),
                       gui::Painter::kCenter,
                       std::to_string(i++));
    switch (macro.getOrientation().getValue()) {
      case odb::dbOrientType::R0: {
        painter.drawLine(macro_bbox.xMin(),
                         macro_bbox.yMin() + 0.1 * height,
                         macro_bbox.xMin() + 0.1 * width,
                         macro_bbox.yMin());
        break;
      }
      case odb::dbOrientType::MX: {
        painter.drawLine(macro_bbox.xMin(),
                         macro_bbox.yMax() - 0.1 * height,
                         macro_bbox.xMin() + 0.1 * width,
                         macro_bbox.yMax());
        break;
      }
      case odb::dbOrientType::MY: {
        painter.drawLine(macro_bbox.xMax(),
                         macro_bbox.yMin() + 0.1 * height,
                         macro_bbox.xMax() - 0.1 * width,
                         macro_bbox.yMin());
        break;
      }
      case odb::dbOrientType::R180: {
        painter.drawLine(macro_bbox.xMax(),
                         macro_bbox.yMax() - 0.1 * height,
                         macro_bbox.xMax() - 0.1 * width,
                         macro_bbox.yMax());
        break;
      }
      case odb::dbOrientType::R90:
      case odb::dbOrientType::R270:
      case odb::dbOrientType::MYR90:
      case odb::dbOrientType::MXR90: {
        logger_->error(utl::MPL, 15, "Unexpected orientation");
      }
    }
  }

  if (show_bundled_nets_) {
    if (!hard_macros_.empty()) {
      drawBundledNets(painter, hard_macros_);
    }
    if (!soft_macros_.empty()) {
      drawBundledNets(painter, soft_macros_);
    }
  }

  drawBlockedRegionsIndication(painter);

  painter.setBrush(gui::Painter::kTransparent);
  if (only_final_result_) {
    // Draw all outlines. Same level outlines have the same color.
    for (int level = 0; level < outlines_.size(); ++level) {
      gui::Painter::Color level_color = gui::Painter::kHighlightColors[level];
      // Remove transparency
      level_color.a = 255;

      painter.setPen(level_color, true, 3 /*width*/);
      for (const odb::Rect& outline : outlines_[level]) {
        painter.drawRect(outline);
      }
    }
  } else {
    // Hightlight current outline so we see where SA is working
    painter.setPen(gui::Painter::kCyan, true);
    painter.drawRect(outline_);

    drawGuides(painter);
    drawFences(painter);
  }

  if (!soft_macros_.empty()) {
    drawNotches(painter);
  }
}

template <typename T>
bool Graphics::isSkippable(const T& macro)
{
  Cluster* cluster = macro.getCluster();
  return !cluster || cluster->isClusterOfUnplacedIOPins();
}

// Draw guidance regions for macros.
void Graphics::drawGuides(gui::Painter& painter)
{
  painter.setPen(gui::Painter::kGreen, true);

  for (const auto& [macro_id, guidance_region] : guides_) {
    odb::Rect guide = guidance_region;
    guide.moveDelta(outline_.xMin(), outline_.yMin());

    painter.drawRect(guide);
    painter.drawString(guide.xCenter(),
                       guide.yCenter(),
                       gui::Painter::Anchor::kCenter,
                       std::to_string(macro_id),
                       false /* rotate 90 */);
  }
}

void Graphics::drawNotches(gui::Painter& painter)
{
  painter.setPen(gui::Painter::kYellow, true);

  for (const auto& notch : notches_) {
    odb::Rect rect = notch;
    rect.moveDelta(outline_.xMin(), outline_.yMin());

    painter.setBrush(gui::Painter::kYellow, gui::Painter::kDiagonal);

    painter.drawRect(rect);
  }
}

void Graphics::drawBlockedRegionsIndication(gui::Painter& painter)
{
  painter.setPen(gui::Painter::kRed, true);
  painter.setBrush(gui::Painter::kTransparent);

  for (const odb::Rect& region : blocked_regions_for_pins_) {
    painter.drawX(region.xCenter(), region.yCenter(), x_mark_size_);
  }
}

template <typename T>
void Graphics::drawBundledNets(gui::Painter& painter,
                               const std::vector<T>& macros)
{
  painter.setPen(gui::Painter::kYellow, true);

  for (const auto& net : nets_) {
    drawBundledNet(painter, macros, net);
  }
}

template <typename T>
void Graphics::drawBundledNet(gui::Painter& painter,
                              const std::vector<T>& macros,
                              const BundledNet& net)
{
  const T& source = macros[net.terminals.first];
  const T& target = macros[net.terminals.second];

  if (target.isClusterOfUnplacedIOPins()) {
    drawDistToRegion(painter, source, target);
    return;
  }

  odb::Point from(source.getPinX(), source.getPinY());
  odb::Point to(target.getPinX(), target.getPinY());

  addOutlineOffsetToLine(from, to);
  painter.drawLine(from, to);
}

template <typename T>
void Graphics::drawDistToRegion(gui::Painter& painter,
                                const T& macro,
                                const T& io)
{
  if (isOutsideTheOutline(macro)) {
    return;
  }

  odb::Point from(macro.getPinX(), macro.getPinY());
  from.addX(outline_.xMin());
  from.addY(outline_.yMin());

  odb::Point to;
  if (io.getCluster()->isClusterOfUnconstrainedIOPins()) {
    computeDistToNearestRegion(
        from, available_regions_for_unconstrained_pins_, &to);
  } else {
    computeDistToNearestRegion(
        from, {io_cluster_to_constraint_.at(io.getCluster())}, &to);
  }

  painter.drawLine(from, to);
  painter.drawString(to.getX(), to.getY(), gui::Painter::kCenter, io.getName());
}

template <typename T>
bool Graphics::isOutsideTheOutline(const T& macro) const
{
  return macro.getPinX() > outline_.dx() || macro.getPinY() > outline_.dy();
}

void Graphics::addOutlineOffsetToLine(odb::Point& from, odb::Point& to)
{
  from.addX(outline_.xMin());
  from.addY(outline_.yMin());
  to.addX(outline_.xMin());
  to.addY(outline_.yMin());
}

// Give some transparency to mixed and hard so we can see overlap with
// macro blockages.
void Graphics::setSoftMacroBrush(gui::Painter& painter,
                                 const SoftMacro& soft_macro)
{
  if (soft_macro.isBlockage()) {
    painter.setBrush(gui::Painter::kDarkGreen);
    return;
  }

  if (soft_macro.getCluster()->getClusterType() == StdCellCluster) {
    painter.setBrush(gui::Painter::kDarkBlue);
  } else if (soft_macro.getCluster()->getClusterType() == HardMacroCluster) {
    // dark red
    painter.setBrush(gui::Painter::Color(0x80, 0x00, 0x00, 150));
  } else {
    // dark purple
    painter.setBrush(gui::Painter::Color(0x80, 0x00, 0x80, 150));
  }
}

void Graphics::setMacroBlockages(const std::vector<odb::Rect>& macro_blockages)
{
  macro_blockages_ = macro_blockages;
}

void Graphics::setPlacementBlockages(
    const std::vector<odb::Rect>& placement_blockages)
{
  placement_blockages_ = placement_blockages;
}

void Graphics::setShowBundledNets(bool show_bundled_nets)
{
  show_bundled_nets_ = show_bundled_nets;
}

void Graphics::setShowClustersIds(bool show_clusters_ids)
{
  show_clusters_ids_ = show_clusters_ids;
}

void Graphics::setSkipSteps(bool skip_steps)
{
  skip_steps_ = skip_steps;

  if (skip_steps_) {
    is_skipping_ = true;
  }
}

void Graphics::doNotSkip()
{
  if (skip_steps_) {
    is_skipping_ = false;
  }
}

void Graphics::setOnlyFinalResult(bool only_final_result)
{
  only_final_result_ = only_final_result;
}

void Graphics::setNets(const BundledNetList& nets)
{
  nets_ = nets;
}

void Graphics::setTargetClusterId(const int target_cluster_id)
{
  target_cluster_id_ = target_cluster_id;
}

void Graphics::setOutline(const odb::Rect& outline)
{
  outline_ = outline;
}

void Graphics::setCurrentCluster(Cluster* current_cluster)
{
  current_cluster_ = current_cluster;
}

void Graphics::setGuides(const std::map<int, odb::Rect>& guides)
{
  guides_ = guides;
}

void Graphics::setFences(const std::map<int, odb::Rect>& fences)
{
  fences_ = fences;
}

void Graphics::addNotch(const odb::Rect& notch)
{
  notches_.emplace_back(notch);
}

void Graphics::clearNotches()
{
  notches_.clear();
}

void Graphics::setIOConstraintsMap(
    const ClusterToBoundaryRegionMap& io_cluster_to_constraint)
{
  io_cluster_to_constraint_ = io_cluster_to_constraint;
}

void Graphics::setBlockedRegionsForPins(
    const std::vector<odb::Rect>& blocked_regions_for_pins)
{
  blocked_regions_for_pins_ = blocked_regions_for_pins;
}

void Graphics::setAvailableRegionsForUnconstrainedPins(
    const BoundaryRegionList& regions)
{
  available_regions_for_unconstrained_pins_ = regions;
}

void Graphics::eraseDrawing()
{
  // Ensure we don't try to access the clusters after they were deleted
  root_ = nullptr;

  soft_macros_.clear();
  hard_macros_.clear();
  macro_blockages_.clear();
  placement_blockages_.clear();
  nets_.clear();
  outline_.reset(0, 0, 0, 0);
  outlines_.clear();
  blocked_regions_for_pins_.clear();
  guides_.clear();
}

}  // namespace mpl
