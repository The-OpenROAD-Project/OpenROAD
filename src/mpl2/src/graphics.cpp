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

Graphics::Graphics(int dbu, utl::Logger* logger) : dbu_(dbu), logger_(logger)
{
  gui::Gui::get()->registerRenderer(this);
}

void Graphics::startSA()
{
  logger_->report("------ Start ------");
  best_norm_cost_ = std::numeric_limits<float>::max();
  skipped_ = 0;
}

void Graphics::endSA()
{
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
  outline_width_.reset();
  outline_height_.reset();
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

void Graphics::setOutlinePenalty(float outline_penalty,
                                 float outline_width,
                                 float outline_height)
{
  outline_penalty_ = outline_penalty;
  outline_width_ = outline_width;
  outline_height_ = outline_height;
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

void Graphics::drawObjects(gui::Painter& painter)
{
  if (root_) {
    painter.setPen(gui::Painter::red, true);
    painter.setBrush(gui::Painter::transparent);
    drawCluster(root_, painter);
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

    painter.drawRect(bbox);
    painter.drawString(bbox.xCenter(),
                       bbox.yCenter(),
                       gui::Painter::CENTER,
                       std::to_string(i++));
    switch (macro.getOrientation()) {
      case odb::dbOrientType::R0: {
        painter.drawLine(lx, ly + 0.1 * height, lx + 0.1 * width, ly);
        break;
      }
      case odb::dbOrientType::MX: {
        painter.drawLine(lx, uy - 0.1 * height, lx + 0.1 * width, uy);
        break;
      }
      case odb::dbOrientType::MY: {
        painter.drawLine(ux, ly + 0.1 * height, ux - 0.1 * width, ly);
        break;
      }
      case odb::dbOrientType::R180: {
        painter.drawLine(ux, uy - 0.1 * height, ux - 0.1 * width, uy);
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

  if (outline_width_.has_value()) {
    odb::Rect bbox(
        0, 0, dbu_ * outline_width_.value(), dbu_ * outline_height_.value());

    painter.setPen(gui::Painter::red, true);
    painter.setBrush(gui::Painter::transparent);
    painter.drawRect(bbox);
  }
}

}  // namespace mpl2
