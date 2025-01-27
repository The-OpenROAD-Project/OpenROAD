///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////
#include "graphics.h"

#include <vector>

#include "object.h"
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
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::startCoarse()
{
  active_ = coarse_;
}

void Graphics::startFine()
{
  active_ = fine_;
}

void Graphics::startSA()
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

  logger_->report("------ Start ------");
  best_norm_cost_ = std::numeric_limits<float>::max();
  skipped_ = 0;
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
  logger_->report("------ End ------");
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
void Graphics::report(const char* name, const std::optional<T>& value)
{
  if (value) {
    auto penalty = value.value();
    logger_->report(
        "{:25}(Norm penalty {:>8.4f}) * (weight {:>8.4f}) = cost {:>8.4f}",
        name,
        penalty.norm_penalty,
        penalty.weight,
        penalty.norm_penalty * penalty.weight);
  }
}

void Graphics::report(const float norm_cost)
{
  report("Area", area_penalty_);
  report("Outline Penalty", outline_penalty_);
  report("Wirelength", wirelength_penalty_);
  report("Fence Penalty", fence_penalty_);
  report("Guidance Penalty", guidance_penalty_);
  report("Boundary Penalty", boundary_penalty_);
  report("Macro Blockage Penalty", macro_blockage_penalty_);
  report("Notch Penalty", notch_penalty_);
  report("Normalized Cost", std::optional<Penalty>({1.0f, norm_cost}));
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
  Rect outline = parent->getBBox();
  odb::Rect dbu_outline(block_->micronsToDbu(outline.xMin()),
                        block_->micronsToDbu(outline.yMin()),
                        block_->micronsToDbu(outline.xMax()),
                        block_->micronsToDbu(outline.yMax()));
  outlines[level].push_back(dbu_outline);

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

  if (is_skipping_) {
    return;
  }

  if (target_cluster_id_ != -1 && !isTargetCluster()) {
    return;
  }

  bool drawing_last_step = skip_steps_ && !is_skipping_;

  if (norm_cost < best_norm_cost_ || drawing_last_step) {
    logger_->report("------ Penalty ------");
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

void Graphics::setNotchPenalty(const Penalty& penalty)
{
  notch_penalty_ = penalty;
}

void Graphics::setMacroBlockagePenalty(const Penalty& penalty)
{
  macro_blockage_penalty_ = penalty;
}

void Graphics::setBoundaryPenalty(const Penalty& penalty)
{
  boundary_penalty_ = penalty;
}

void Graphics::setFencePenalty(const Penalty& penalty)
{
  fence_penalty_ = penalty;
}

void Graphics::setGuidancePenalty(const Penalty& penalty)
{
  guidance_penalty_ = penalty;
}

void Graphics::setAreaPenalty(const Penalty& penalty)
{
  area_penalty_ = penalty;
}

void Graphics::setOutlinePenalty(const Penalty& penalty)
{
  outline_penalty_ = penalty;
}

void Graphics::setWirelengthPenalty(const Penalty& penalty)
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
  setXMarksSizeAndPosition(tree->blocked_boundaries);
}

void Graphics::setXMarksSizeAndPosition(
    const std::set<Boundary>& blocked_boundaries)
{
  const odb::Rect die = block_->getDieArea();

  // Not too big/small
  x_mark_size_ = (die.dx() + die.dy()) * 0.03;

  for (Boundary boundary : blocked_boundaries) {
    odb::Point x_mark_point;

    switch (boundary) {
      case L: {
        x_mark_point = odb::Point(die.xMin(), die.yCenter());
        break;
      }
      case R: {
        x_mark_point = odb::Point(die.xMax(), die.yCenter());
        break;
      }
      case B: {
        x_mark_point = odb::Point(die.xCenter(), die.yMin());
        break;
      }
      case T: {
        x_mark_point = odb::Point(die.xCenter(), die.yMax());
        break;
      }
      case NONE:
        break;
    }

    blocked_boundary_to_mark_[boundary] = x_mark_point;
  }
}

void Graphics::drawCluster(Cluster* cluster, gui::Painter& painter)
{
  const int lx = block_->micronsToDbu(cluster->getX());
  const int ly = block_->micronsToDbu(cluster->getY());
  const int ux = lx + block_->micronsToDbu(cluster->getWidth());
  const int uy = ly + block_->micronsToDbu(cluster->getHeight());
  odb::Rect bbox(lx, ly, ux, uy);

  painter.drawRect(bbox);

  for (auto& child : cluster->getChildren()) {
    drawCluster(child.get(), painter);
  }
}

void Graphics::drawAllBlockages(gui::Painter& painter)
{
  if (!macro_blockages_.empty()) {
    painter.setPen(gui::Painter::gray, true);
    painter.setBrush(gui::Painter::gray, gui::Painter::DIAGONAL);

    for (const auto& blockage : macro_blockages_) {
      drawOffsetRect(blockage, "", painter);
    }
  }

  if (!placement_blockages_.empty()) {
    painter.setPen(gui::Painter::green, true);
    painter.setBrush(gui::Painter::green, gui::Painter::DIAGONAL);

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
                   gui::Painter::DIAGONAL);
  painter.setPen(gui::Painter::dark_yellow, true);

  for (const auto& [macro_id, fence] : fences_) {
    drawOffsetRect(fence, std::to_string(macro_id), painter);
  }
}

void Graphics::drawOffsetRect(const Rect& rect,
                              const std::string& center_text,
                              gui::Painter& painter)
{
  const int lx = block_->micronsToDbu(rect.xMin());
  const int ly = block_->micronsToDbu(rect.yMin());
  const int ux = block_->micronsToDbu(rect.xMax());
  const int uy = block_->micronsToDbu(rect.yMax());

  odb::Rect rect_bbox(lx, ly, ux, uy);
  rect_bbox.moveDelta(outline_.xMin(), outline_.yMin());
  painter.drawRect(rect_bbox);

  if (!center_text.empty()) {
    painter.drawString(rect_bbox.xCenter(),
                       rect_bbox.yCenter(),
                       gui::Painter::CENTER,
                       center_text);
  }
}

// We draw the shapes of SoftMacros, HardMacros and blockages based
// on the outline's origin.
void Graphics::drawObjects(gui::Painter& painter)
{
  if (root_ && !only_final_result_) {
    painter.setPen(gui::Painter::red, true);
    painter.setBrush(gui::Painter::transparent);
    drawCluster(root_, painter);
  }

  // Draw blockages only during SA for SoftMacros
  if (!soft_macros_.empty()) {
    drawAllBlockages(painter);
  }

  painter.setPen(gui::Painter::white, true);

  int i = 0;
  for (const auto& macro : soft_macros_) {
    Cluster* cluster = macro.getCluster();

    if (!cluster) {  // fixed terminals
      continue;
    }

    if (cluster->isIOCluster()) {
      continue;
    }

    setSoftMacroBrush(painter, macro);

    const int lx = block_->micronsToDbu(macro.getX());
    const int ly = block_->micronsToDbu(macro.getY());
    const int ux = lx + block_->micronsToDbu(macro.getWidth());
    const int uy = ly + block_->micronsToDbu(macro.getHeight());
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
                       gui::Painter::CENTER,
                       cluster_id_string);
  }

  painter.setPen(gui::Painter::white, true);
  painter.setBrush(gui::Painter::dark_red);

  i = 0;
  for (const auto& macro : hard_macros_) {
    const int lx = block_->micronsToDbu(macro.getX());
    const int ly = block_->micronsToDbu(macro.getY());
    const int width = block_->micronsToDbu(macro.getWidth());
    const int height = block_->micronsToDbu(macro.getHeight());
    const int ux = lx + width;
    const int uy = ly + height;
    odb::Rect bbox(lx, ly, ux, uy);

    bbox.moveDelta(outline_.xMin(), outline_.yMin());

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::CENTER,
                       std::to_string(i++));
    switch (macro.getOrientation()) {
      case odb::dbOrientType::R0: {
        painter.drawLine(bbox.xMin(),
                         bbox.yMin() + 0.1 * height,
                         bbox.xMin() + 0.1 * width,
                         bbox.yMin());
        break;
      }
      case odb::dbOrientType::MX: {
        painter.drawLine(bbox.xMin(),
                         bbox.yMax() - 0.1 * height,
                         bbox.xMin() + 0.1 * width,
                         bbox.yMax());
        break;
      }
      case odb::dbOrientType::MY: {
        painter.drawLine(bbox.xMax(),
                         bbox.yMin() + 0.1 * height,
                         bbox.xMax() - 0.1 * width,
                         bbox.yMin());
        break;
      }
      case odb::dbOrientType::R180: {
        painter.drawLine(bbox.xMax(),
                         bbox.yMax() - 0.1 * height,
                         bbox.xMax() - 0.1 * width,
                         bbox.yMax());
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
    painter.setPen(gui::Painter::yellow, true);

    if (!hard_macros_.empty()) {
      drawBundledNets(painter, hard_macros_);
    }
    if (!soft_macros_.empty()) {
      drawBundledNets(painter, soft_macros_);
    }
  }

  drawBlockedBoundariesIndication(painter);

  painter.setBrush(gui::Painter::transparent);
  if (only_final_result_) {
    // Draw all outlines. Same level outlines have the same color.
    for (int level = 0; level < outlines_.size(); ++level) {
      gui::Painter::Color level_color = gui::Painter::highlightColors[level];
      // Remove transparency
      level_color.a = 255;

      painter.setPen(level_color, true, 3 /*width*/);
      for (const odb::Rect& outline : outlines_[level]) {
        painter.drawRect(outline);
      }
    }
  } else {
    // Hightlight current outline so we see where SA is working
    painter.setPen(gui::Painter::cyan, true);
    painter.drawRect(outline_);

    drawGuides(painter);
    drawFences(painter);
  }
}

// Draw guidance regions for macros.
void Graphics::drawGuides(gui::Painter& painter)
{
  painter.setPen(gui::Painter::green, true);

  for (const auto& [macro_id, guidance_region] : guides_) {
    odb::Rect guide(block_->micronsToDbu(guidance_region.xMin()),
                    block_->micronsToDbu(guidance_region.yMin()),
                    block_->micronsToDbu(guidance_region.xMax()),
                    block_->micronsToDbu(guidance_region.yMax()));
    guide.moveDelta(outline_.xMin(), outline_.yMin());

    painter.drawRect(guide);
    painter.drawString(guide.xCenter(),
                       guide.yCenter(),
                       gui::Painter::Anchor::CENTER,
                       std::to_string(macro_id),
                       false /* rotate 90 */);
  }
}

void Graphics::drawBlockedBoundariesIndication(gui::Painter& painter)
{
  painter.setPen(gui::Painter::red, true);
  painter.setBrush(gui::Painter::transparent);

  for (const auto [boundary, x_mark_point] : blocked_boundary_to_mark_) {
    painter.drawX(x_mark_point.getX(), x_mark_point.getY(), x_mark_size_);
  }
}

template <typename T>
void Graphics::drawBundledNets(gui::Painter& painter,
                               const std::vector<T>& macros)
{
  for (const auto& bundled_net : bundled_nets_) {
    const T& source = macros[bundled_net.terminals.first];
    const T& target = macros[bundled_net.terminals.second];

    if (target.isIOCluster()) {
      drawDistToIoConstraintBoundary(painter, source, target);
      continue;
    }

    const int x1 = block_->micronsToDbu(source.getPinX());
    const int y1 = block_->micronsToDbu(source.getPinY());
    odb::Point from(x1, y1);

    const int x2 = block_->micronsToDbu(target.getPinX());
    const int y2 = block_->micronsToDbu(target.getPinY());
    odb::Point to(x2, y2);

    addOutlineOffsetToLine(from, to);
    painter.drawLine(from, to);
  }
}

template <typename T>
void Graphics::drawDistToIoConstraintBoundary(gui::Painter& painter,
                                              const T& macro,
                                              const T& io)
{
  if (isOutsideTheOutline(macro)) {
    return;
  }

  Cluster* io_cluster = io.getCluster();

  const int x1 = block_->micronsToDbu(macro.getPinX());
  const int y1 = block_->micronsToDbu(macro.getPinY());
  odb::Point from(x1, y1);

  odb::Point to;
  Boundary constraint_boundary = io_cluster->getConstraintBoundary();

  if (constraint_boundary == Boundary::L
      || constraint_boundary == Boundary::R) {
    const int x2 = block_->micronsToDbu(io.getPinX());
    const int y2 = block_->micronsToDbu(macro.getPinY());
    to.setX(x2);
    to.setY(y2);
  } else if (constraint_boundary == Boundary::B
             || constraint_boundary == Boundary::T) {
    const int x2 = block_->micronsToDbu(macro.getPinX());
    const int y2 = block_->micronsToDbu(io.getPinY());
    to.setX(x2);
    to.setY(y2);
  } else {
    // For NONE, the shape of the io cluster is the die area.
    const Rect die = io_cluster->getBBox();
    Boundary closest_unblocked_boundary
        = getClosestUnblockedBoundary(macro, die);

    to = getClosestBoundaryPoint(macro, die, closest_unblocked_boundary);
  }

  addOutlineOffsetToLine(from, to);

  painter.drawLine(from, to);
  painter.drawString(to.getX(),
                     to.getY(),
                     gui::Painter::CENTER,
                     toString(constraint_boundary));
}

template <typename T>
bool Graphics::isOutsideTheOutline(const T& macro) const
{
  return block_->micronsToDbu(macro.getPinX()) > outline_.dx()
         || block_->micronsToDbu(macro.getPinY()) > outline_.dy();
}

// Here, we have to manually decompensate the offset of the
// coordinates that come from the cluster.
template <typename T>
odb::Point Graphics::getClosestBoundaryPoint(const T& macro,
                                             const Rect& die,
                                             Boundary closest_boundary)
{
  odb::Point to;

  if (closest_boundary == Boundary::L) {
    to.setX(block_->micronsToDbu(die.xMin()));
    to.setY(block_->micronsToDbu(macro.getPinY()));
    to.addX(-outline_.xMin());
  } else if (closest_boundary == Boundary::R) {
    to.setX(block_->micronsToDbu(die.xMax()));
    to.setY(block_->micronsToDbu(macro.getPinY()));
    to.addX(-outline_.xMin());
  } else if (closest_boundary == Boundary::B) {
    to.setX(block_->micronsToDbu(macro.getPinX()));
    to.setY(block_->micronsToDbu(die.yMin()));
    to.addY(-outline_.yMin());
  } else {  // Top
    to.setX(block_->micronsToDbu(macro.getPinX()));
    to.setY(block_->micronsToDbu(die.yMax()));
    to.addY(-outline_.yMin());
  }

  return to;
}

void Graphics::addOutlineOffsetToLine(odb::Point& from, odb::Point& to)
{
  from.addX(outline_.xMin());
  from.addY(outline_.yMin());
  to.addX(outline_.xMin());
  to.addY(outline_.yMin());
}

template <typename T>
Boundary Graphics::getClosestUnblockedBoundary(const T& macro, const Rect& die)
{
  const float macro_x = macro.getPinX();
  const float macro_y = macro.getPinY();

  float shortest_distance = std::numeric_limits<float>::max();
  Boundary closest_boundary = Boundary::NONE;

  if (!isBlockedBoundary(Boundary::L)) {
    const float dist_to_left = std::abs(macro_x - die.xMin());
    if (dist_to_left < shortest_distance) {
      shortest_distance = dist_to_left;
      closest_boundary = Boundary::L;
    }
  }

  if (!isBlockedBoundary(Boundary::R)) {
    const float dist_to_right = std::abs(macro_x - die.xMax());
    if (dist_to_right < shortest_distance) {
      shortest_distance = dist_to_right;
      closest_boundary = Boundary::R;
    }
  }

  if (!isBlockedBoundary(Boundary::B)) {
    const float dist_to_bottom = std::abs(macro_y - die.yMin());
    if (dist_to_bottom < shortest_distance) {
      shortest_distance = dist_to_bottom;
      closest_boundary = Boundary::B;
    }
  }

  if (!isBlockedBoundary(Boundary::T)) {
    const float dist_to_top = std::abs(macro_y - die.yMax());
    if (dist_to_top < shortest_distance) {
      closest_boundary = Boundary::T;
    }
  }

  return closest_boundary;
}

bool Graphics::isBlockedBoundary(Boundary boundary)
{
  return blocked_boundary_to_mark_.find(boundary)
         != blocked_boundary_to_mark_.end();
}

// Give some transparency to mixed and hard so we can see overlap with
// macro blockages.
void Graphics::setSoftMacroBrush(gui::Painter& painter,
                                 const SoftMacro& soft_macro)
{
  if (soft_macro.getCluster()->getClusterType() == StdCellCluster) {
    painter.setBrush(gui::Painter::dark_blue);
  } else if (soft_macro.getCluster()->getClusterType() == HardMacroCluster) {
    // dark red
    painter.setBrush(gui::Painter::Color(0x80, 0x00, 0x00, 150));
  } else {
    // dark purple
    painter.setBrush(gui::Painter::Color(0x80, 0x00, 0x80, 150));
  }
}

void Graphics::setMacroBlockages(const std::vector<mpl::Rect>& macro_blockages)
{
  macro_blockages_ = macro_blockages;
}

void Graphics::setPlacementBlockages(
    const std::vector<mpl::Rect>& placement_blockages)
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

void Graphics::setBundledNets(const std::vector<BundledNet>& bundled_nets)
{
  bundled_nets_ = bundled_nets;
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

void Graphics::setGuides(const std::map<int, Rect>& guides)
{
  guides_ = guides;
}

void Graphics::setFences(const std::map<int, Rect>& fences)
{
  fences_ = fences;
}

void Graphics::eraseDrawing()
{
  // Ensure we don't try to access the clusters after they were deleted
  root_ = nullptr;

  soft_macros_.clear();
  hard_macros_.clear();
  macro_blockages_.clear();
  placement_blockages_.clear();
  bundled_nets_.clear();
  outline_.reset(0, 0, 0, 0);
  outlines_.clear();
  blocked_boundary_to_mark_.clear();
  guides_.clear();
}

}  // namespace mpl
