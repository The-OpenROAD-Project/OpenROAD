//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "techlayer.h"

#include <limits>
#include <optional>

#include "utl/Logger.h"

namespace pdn {

TechLayer::TechLayer(odb::dbTechLayer* layer) : layer_(layer), grid_({})
{
}

int TechLayer::getSpacing(int width, int length) const
{
  // get the spacing the DB would use
  const int db_spacing = layer_->getSpacing(width, length);

  // Check the two widths table for spacing assuming same width metal
  const int two_widths_spacing = layer_->findTwSpacing(width, width, length);

  return std::max(db_spacing, two_widths_spacing);
}

double TechLayer::dbuToMicron(int value) const
{
  return value / static_cast<double>(getLefUnits());
}

void TechLayer::populateGrid(odb::dbBlock* block, odb::dbTechLayerDir dir)
{
  grid_.clear();

  if (dir == odb::dbTechLayerDir::NONE) {
    dir = layer_->getDirection();
  }

  auto* tracks = block->findTrackGrid(layer_);
  if (dir == odb::dbTechLayerDir::HORIZONTAL) {
    tracks->getGridY(grid_);
  } else if (dir == odb::dbTechLayerDir::VERTICAL) {
    tracks->getGridX(grid_);
  } else {
    tracks->getGridY(grid_);
  }
}

int TechLayer::snapToGrid(int pos, int greater_than) const
{
  if (grid_.empty()) {
    return pos;
  }

  std::optional<int> delta_pos;
  int delta = std::numeric_limits<int>::max();
  for (const int grid_pos : grid_) {
    if (grid_pos < greater_than) {
      // ignore since it is lower than the minimum
      continue;
    }

    // look for smallest delta
    const int new_delta = std::abs(pos - grid_pos);
    if (new_delta < delta) {
      delta_pos = grid_pos;
      delta = new_delta;
    } else {
      break;
    }
  }

  if (delta_pos.has_value()) {
    return delta_pos.value();
  }
  return pos;
}

int TechLayer::snapToGridInterval(odb::dbBlock* block, int dist) const
{
  odb::dbTechLayerDir dir = layer_->getDirection();

  int origin = 0;
  int num = 0;
  int step = 0;
  for (auto* grid : block->getTrackGrids()) {
    if (grid->getTechLayer() != layer_) {
      continue;
    }

    if (dir == odb::dbTechLayerDir::VERTICAL) {
      if (grid->getNumGridPatternsX() < 1) {
        continue;
      }

      grid->getGridPatternX(0, origin, num, step);
    } else {
      if (grid->getNumGridPatternsY() < 1) {
        continue;
      }

      grid->getGridPatternY(0, origin, num, step);
    }
  }

  if (num == 0 || step == 0) {
    return dist;
  }

  const int count = std::max(1, dist / step);
  return count * step;
}

int TechLayer::snapToManufacturingGrid(odb::dbTech* tech,
                                       int pos,
                                       bool round_up,
                                       int grid_multiplier)
{
  if (!tech->hasManufacturingGrid()) {
    return pos;
  }

  const int grid = grid_multiplier * tech->getManufacturingGrid();

  if (pos % grid != 0) {
    int round_pos = pos / grid;
    if (round_up) {
      round_pos += 1;
    }
    pos = round_pos * grid;
  }

  return pos;
}

bool TechLayer::checkIfManufacturingGrid(odb::dbTech* tech, int value)
{
  if (!tech->hasManufacturingGrid()) {
    return true;
  }

  const int grid = tech->getManufacturingGrid();

  if (value % grid != 0) {
    return false;
  }

  return true;
}

bool TechLayer::checkIfManufacturingGrid(int value,
                                         utl::Logger* logger,
                                         const std::string& type) const
{
  auto* tech = layer_->getTech();
  if (!checkIfManufacturingGrid(tech, value)) {
    logger->error(
        utl::PDN,
        191,
        "{} of {:.4f} um does not fit the manufacturing grid of {:.4f} um.",
        type,
        dbuToMicron(value),
        dbuToMicron(tech->getManufacturingGrid()));
    return false;
  }

  return true;
}

int TechLayer::snapToManufacturingGrid(int pos,
                                       bool round_up,
                                       int grid_multiplier) const
{
  return snapToManufacturingGrid(
      layer_->getTech(), pos, round_up, grid_multiplier);
}

std::vector<TechLayer::MinCutRule> TechLayer::getMinCutRules() const
{
  std::vector<MinCutRule> rules;

  // get all the LEF55 rules
  for (auto* min_cut_rule : layer_->getMinCutRules()) {
    odb::uint numcuts;
    odb::uint rule_width;
    min_cut_rule->getMinimumCuts(numcuts, rule_width);

    rules.push_back(MinCutRule{nullptr,
                               min_cut_rule->isAboveOnly(),
                               min_cut_rule->isBelowOnly(),
                               static_cast<int>(rule_width),
                               static_cast<int>(numcuts)});
  }

  for (auto* rule : layer_->getTechLayerMinCutRules()) {
    for (const auto& [cut_class_name, cuts] : rule->getCutClassCutsMap()) {
      auto* cut_class = layer_->getLowerLayer()->findTechLayerCutClassRule(
          cut_class_name.c_str());
      if (cut_class == nullptr) {
        cut_class = layer_->getUpperLayer()->findTechLayerCutClassRule(
            cut_class_name.c_str());
      }

      rules.push_back(MinCutRule{cut_class,
                                 rule->isFromBelow(),  // same as ABOVE
                                 rule->isFromAbove(),  // same as BELOW
                                 rule->getWidth(),
                                 cuts});
    }
  }

  return rules;
}

int TechLayer::getMinIncrementStep() const
{
  if (layer_->getTech()->hasManufacturingGrid()) {
    const int grid = layer_->getTech()->getManufacturingGrid();
    return grid;
  }
  return 1;
}

odb::Rect TechLayer::adjustToMinArea(const odb::Rect& rect) const
{
  if (!layer_->hasArea()) {
    return rect;
  }

  const double min_area = layer_->getArea();
  if (min_area == 0.0) {
    return rect;
  }

  // make sure minimum area is honored
  const int dbu_per_micron = getLefUnits();
  const double area = min_area * dbu_per_micron * dbu_per_micron;

  odb::Rect new_rect = rect;

  const int width = new_rect.dx();
  const int height = new_rect.dy();
  if (width * height < area) {
    if (layer_->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      const int required_width = std::ceil(area / height);
      const double added_width = required_width - width;
      const int adjust_min = std::ceil(added_width / 2.0);
      const int new_x0
          = snapToManufacturingGrid(rect.xMin() - adjust_min, false);
      const int new_x1
          = snapToManufacturingGrid(rect.xMax() + adjust_min, true);
      new_rect.set_xlo(new_x0);
      new_rect.set_xhi(new_x1);
    } else {
      const int required_height = std::ceil(area / width);
      const double added_height = required_height - height;
      const int adjust_min = std::ceil(added_height / 2.0);
      const int new_y0
          = snapToManufacturingGrid(rect.yMin() - adjust_min, false);
      const int new_y1
          = snapToManufacturingGrid(rect.yMax() + adjust_min, true);
      new_rect.set_ylo(new_y0);
      new_rect.set_yhi(new_y1);
    }
  }

  return new_rect;
}

}  // namespace pdn
