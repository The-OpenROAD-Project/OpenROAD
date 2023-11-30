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

#include "object.h"
#include "utl/Logger.h"

namespace mpl2 {

Graphics::Graphics(bool coarse, bool fine, int dbu, utl::Logger* logger)
    : coarse_(coarse), fine_(fine), dbu_(dbu), logger_(logger)
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
  logger_->report("------ Start ------");
  best_norm_cost_ = std::numeric_limits<float>::max();
  skipped_ = 0;
}

void Graphics::endSA()
{
  if (!active_) {
    return;
  }
  if (skipped_ > 0) {
    logger_->report("Skipped to end: {}", skipped_);
  }
  logger_->report("------ End ------");
  gui::Gui::get()->pause();
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
    logger_->report("{:25}{:>8.4f}", name, value.value());
  }
}

void Graphics::penaltyCalculated(float norm_cost)
{
  if (!active_) {
    return;
  }
  if (norm_cost < best_norm_cost_) {
    logger_->report("------ Penalty ------");

    report("Area", area_penalty_);
    report("Outline Penalty", outline_penalty_);
    report("Wirelength", wirelength_);
    report("Fence Penalty", fence_penalty_);
    report("Guidance Penalty", guidance_penalty_);
    report("Boundary Penalty", boundary_penalty_);
    report("Macro Blockage Penalty", macro_blockage_penalty_);
    report("Notch Penalty", notch_penalty_);
    report("Normalized Cost", std::optional<float>(norm_cost));
    if (skipped_ > 0) {
      logger_->report("Skipped: {}", skipped_);
      skipped_ = 0;
    }

    const char* type = !soft_macros_.empty() ? "SoftMacro" : "HardMacro";
    gui::Gui::get()->status(type);
    gui::Gui::get()->redraw();
    if (norm_cost < 0.99 * best_norm_cost_) {
      gui::Gui::get()->pause();
    }
    best_norm_cost_ = norm_cost;
  } else {
    ++skipped_;
  }
}

void Graphics::resetPenalties()
{
  area_penalty_.reset();
  outline_penalty_.reset();
  wirelength_.reset();
  fence_penalty_.reset();
  guidance_penalty_.reset();
  boundary_penalty_.reset();
  macro_blockage_penalty_.reset();
  notch_penalty_.reset();
}

void Graphics::setNotchPenalty(float notch_penalty)
{
  notch_penalty_ = notch_penalty;
}

void Graphics::setMacroBlockagePenalty(float macro_blockage_penalty)
{
  macro_blockage_penalty_ = macro_blockage_penalty;
}

void Graphics::setBoundaryPenalty(float boundary_penalty)
{
  boundary_penalty_ = boundary_penalty;
}

void Graphics::setFencePenalty(float fence_penalty)
{
  fence_penalty_ = fence_penalty;
}

void Graphics::setGuidancePenalty(float guidance_penalty)
{
  guidance_penalty_ = guidance_penalty;
}

void Graphics::setAreaPenalty(float area_penalty)
{
  area_penalty_ = area_penalty;
}

void Graphics::setOutlinePenalty(float outline_penalty)
{
  outline_penalty_ = outline_penalty;
}

void Graphics::setWirelength(float wirelength)
{
  wirelength_ = wirelength;
}

void Graphics::finishedClustering(Cluster* root)
{
  root_ = root;
}

void Graphics::drawCluster(Cluster* cluster, gui::Painter& painter)
{
  const int lx = dbu_ * cluster->getX();
  const int ly = dbu_ * cluster->getY();
  const int ux = lx + dbu_ * cluster->getWidth();
  const int uy = ly + dbu_ * cluster->getHeight();
  odb::Rect bbox(lx, ly, ux, uy);

  painter.drawRect(bbox);

  for (Cluster* child : cluster->getChildren()) {
    drawCluster(child, painter);
  }
}

void Graphics::drawAllBlockages(gui::Painter& painter)
{
  if (!macro_blockages_.empty()) {
    painter.setPen(gui::Painter::gray, true);
    painter.setBrush(gui::Painter::gray, gui::Painter::DIAGONAL);

    for (const auto& blockage : macro_blockages_) {
      drawBlockage(blockage, painter);
    }
  }

  if (!placement_blockages_.empty()) {
    painter.setPen(gui::Painter::green, true);
    painter.setBrush(gui::Painter::green, gui::Painter::DIAGONAL);

    for (const auto& blockage : placement_blockages_) {
      drawBlockage(blockage, painter);
    }
  }
}

void Graphics::drawBlockage(const Rect& blockage, gui::Painter& painter)
{
  const int lx = dbu_ * blockage.xMin();
  const int ly = dbu_ * blockage.yMin();
  const int ux = dbu_ * blockage.xMax();
  const int uy = dbu_ * blockage.yMax();

  odb::Rect blockage_bbox(lx, ly, ux, uy);
  blockage_bbox.moveDelta(outline_.xMin(), outline_.yMin());

  painter.drawRect(blockage_bbox);
}

// We draw the shapes of SoftMacros, HardMacros and blockages based
// on the outline's origin.
void Graphics::drawObjects(gui::Painter& painter)
{
  if (root_) {
    painter.setPen(gui::Painter::red, true);
    painter.setBrush(gui::Painter::transparent);
    drawCluster(root_, painter);
  }

  // Draw blockages only during SA for SoftMacros
  if (!soft_macros_.empty()) {
    drawAllBlockages(painter);
  }

  painter.setPen(gui::Painter::yellow, true);
  painter.setBrush(gui::Painter::gray);

  int i = 0;
  for (const auto& macro : soft_macros_) {
    const int lx = dbu_ * macro.getX();
    const int ly = dbu_ * macro.getY();
    const int ux = lx + dbu_ * macro.getWidth();
    const int uy = ly + dbu_ * macro.getHeight();
    odb::Rect bbox(lx, ly, ux, uy);

    bbox.moveDelta(outline_.xMin(), outline_.yMin());

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::CENTER,
                       std::to_string(i++));
  }

  i = 0;
  for (const auto& macro : hard_macros_) {
    const int lx = dbu_ * macro.getX();
    const int ly = dbu_ * macro.getY();
    const int width = dbu_ * macro.getWidth();
    const int height = dbu_ * macro.getHeight();
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

  if (root_) {
    // Hightlight outline so we see where SA is working
    painter.setPen(gui::Painter::cyan, true);
    painter.setBrush(gui::Painter::transparent);
    painter.drawRect(outline_);
  }
}

void Graphics::setMacroBlockages(const std::vector<mpl2::Rect>& macro_blockages)
{
  macro_blockages_ = macro_blockages;
}

void Graphics::setPlacementBlockages(
    const std::vector<mpl2::Rect>& placement_blockages)
{
  placement_blockages_ = placement_blockages;
}

void Graphics::setOutline(const odb::Rect& outline)
{
  outline_ = outline;
}

void Graphics::eraseDrawing()
{
  // Ensure we don't try to access the clusters after they were deleted
  root_ = nullptr;

  soft_macros_.clear();
  hard_macros_.clear();
  macro_blockages_.clear();
  placement_blockages_.clear();
}

}  // namespace mpl2
