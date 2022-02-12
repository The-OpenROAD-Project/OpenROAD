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

#include "via.h"

#include <boost/polygon/polygon.hpp>
#include <map>

#include "grid.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

DbVia::ViaLayerShape DbVia::getLayerShapes(odb::dbSBox* box) const
{
  std::vector<odb::dbShape> shapes;
  box->getViaBoxes(shapes);

  std::map<int, odb::Rect> layer_rects;
  for (auto& shape : shapes) {
    auto* layer = shape.getTechLayer();
    if (layer->getType() == odb::dbTechLayerType::ROUTING) {
      shape.getBox(layer_rects[layer->getRoutingLevel()]);
    }
  }

  ViaLayerShape via_shapes;
  bool first = true;
  for (const auto& [level, rect] : layer_rects) {
    if (first) {
      via_shapes.bottom = rect;
    } else {
      via_shapes.top = rect;
    }
    first = false;
  }

  return via_shapes;
}

DbTechVia::DbTechVia(odb::dbTechVia* via) : via_(via)
{
}

DbVia::ViaLayerShape DbTechVia::generate(odb::dbBlock* block,
                                         odb::dbSWire* wire,
                                         odb::dbWireShapeType type,
                                         int x,
                                         int y) const
{
  return getLayerShapes(odb::dbSBox::create(wire, via_, x, y, type));
}

DbGenerateVia::DbGenerateVia(const odb::Rect& rect,
                             odb::dbTechViaGenerateRule* rule,
                             int rows,
                             int columns,
                             int cut_pitch_x,
                             int cut_pitch_y,
                             int bottom_enclosure_x,
                             int bottom_enclosure_y,
                             int top_enclosure_x,
                             int top_enclosure_y,
                             odb::dbTechLayer* bottom,
                             odb::dbTechLayer* cut,
                             odb::dbTechLayer* top)
    : rect_(rect),
      cut_rect_(),
      rule_(rule),
      rows_(rows),
      columns_(columns),
      cut_pitch_x_(cut_pitch_x),
      cut_pitch_y_(cut_pitch_y),
      bottom_enclosure_x_(bottom_enclosure_x),
      bottom_enclosure_y_(bottom_enclosure_y),
      top_enclosure_x_(top_enclosure_x),
      top_enclosure_y_(top_enclosure_y),
      bottom_(bottom),
      cut_(cut),
      top_(top)
{
  for (uint l = 0; rule_->getViaLayerRuleCount(); l++) {
    auto* layer_rule = rule_->getViaLayerRule(l);
    if (layer_rule->getLayer() == cut_) {
      layer_rule->getRect(cut_rect_);
      break;
    }
  }
}

const std::string DbGenerateVia::getName() const
{
  const std::string seperator = "_";
  std::string name = "via";
  // name after layers connected
  name += std::to_string(bottom_->getRoutingLevel());
  name += seperator;
  name += std::to_string(top_->getRoutingLevel());
  // name after area
  name += seperator;
  name += std::to_string(rect_.dx());
  name += seperator;
  name += std::to_string(rect_.dy());
  // name after rows and columns
  name += seperator;
  name += std::to_string(rows_);
  name += seperator;
  name += std::to_string(columns_);
  // name after pitch
  name += seperator;
  name += std::to_string(cut_pitch_x_);
  name += seperator;
  name += std::to_string(cut_pitch_y_);

  return name;
}

const odb::Rect DbGenerateVia::getViaRect(bool include_enclosure) const
{
  const int y_enc = std::max(bottom_enclosure_y_, top_enclosure_y_);
  const int height = (rows_ - 1) * cut_pitch_y_ + cut_rect_.dy();
  const int x_enc = std::max(bottom_enclosure_x_, top_enclosure_x_);
  const int width = (columns_ - 1) * cut_pitch_x_ + cut_rect_.dx();

  const int height_enc = include_enclosure ? 2 * y_enc : 0;
  const int width_enc = include_enclosure ? 2 * x_enc : 0;

  return {0, 0, width + width_enc, height + height_enc};
}

DbVia::ViaLayerShape DbGenerateVia::generate(odb::dbBlock* block,
                                             odb::dbSWire* wire,
                                             odb::dbWireShapeType type,
                                             int x,
                                             int y) const
{
  const std::string via_name = getName();
  auto* via = block->findVia(via_name.c_str());

  if (via == nullptr) {
    const int cut_width = cut_rect_.dx();
    const int cut_height = cut_rect_.dy();
    via = odb::dbVia::create(block, via_name.c_str());

    via->setViaGenerateRule(rule_);

    odb::dbViaParams params;
    via->getViaParams(params);

    params.setBottomLayer(bottom_);
    params.setCutLayer(cut_);
    params.setTopLayer(top_);

    params.setXCutSize(cut_width);
    params.setYCutSize(cut_height);

    params.setXCutSpacing(cut_pitch_x_ - cut_width);
    params.setYCutSpacing(cut_pitch_y_ - cut_height);

    params.setXBottomEnclosure(bottom_enclosure_x_);
    params.setYBottomEnclosure(bottom_enclosure_y_);
    params.setXTopEnclosure(top_enclosure_x_);
    params.setYTopEnclosure(top_enclosure_y_);

    params.setNumCutRows(rows_);
    params.setNumCutCols(columns_);

    via->setViaParams(params);
  }

  return getLayerShapes(odb::dbSBox::create(wire, via, x, y, type));
}

//////////////

DbGenerateArrayVia::DbGenerateArrayVia(DbGenerateVia* core_via,
                                       DbGenerateVia* end_of_row,
                                       DbGenerateVia* end_of_column,
                                       DbGenerateVia* end_of_row_column,
                                       int core_rows,
                                       int core_columns,
                                       int array_spacing_x,
                                       int array_spacing_y)
    : core_via_(core_via),
      end_of_row_(end_of_row),
      end_of_column_(end_of_column),
      end_of_row_column_(end_of_row_column),
      rows_(core_rows),
      columns_(core_columns),
      array_spacing_x_(array_spacing_x),
      array_spacing_y_(array_spacing_y),
      array_start_x_(0),
      array_start_y_(0)
{
  if (end_of_row != nullptr) {
    rows_++;
  }
  if (end_of_column != nullptr) {
    columns_++;
  }

  const odb::Rect core_via_rect = core_via->getViaRect(false);

  // determine the via array offset from the center

  int total_width
      = (columns_ - 1)
        * (array_spacing_x_ + core_via_rect.dx());  // all spacing + core vias
  int x_offset = 0;
  if (end_of_column != nullptr) {
    const odb::Rect end_rect = end_of_column_->getViaRect(false);
    total_width += end_rect.dx();
    x_offset = end_rect.dx() / 2;
  } else {
    total_width += core_via_rect.dx();
    x_offset = core_via_rect.dx() / 2;
  }

  int total_height
      = (rows_ - 1)
        * (array_spacing_y_ + core_via_rect.dy());  // all spacing + core vias
  int y_offset = 0;
  if (end_of_row_ != nullptr) {
    const odb::Rect end_rect = end_of_row_->getViaRect(false);
    total_height += end_rect.dy();
    y_offset = end_rect.dy() / 2;
  } else {
    total_height += core_via_rect.dy();
    y_offset = core_via_rect.dy() / 2;
  }

  // set the via array offset
  array_start_x_ = -total_width / 2 + x_offset;
  array_start_y_ = -total_height / 2 + y_offset;
}

DbVia::ViaLayerShape DbGenerateArrayVia::generate(odb::dbBlock* block,
                                                  odb::dbSWire* wire,
                                                  odb::dbWireShapeType type,
                                                  int x,
                                                  int y) const
{
  const odb::Rect core_via_rect = core_via_->getViaRect(false);
  ViaLayerShape via_shapes;
  via_shapes.bottom.mergeInit();
  via_shapes.top.mergeInit();

  int array_y = array_start_y_ + y;
  for (int row = 0; row < rows_; row++) {
    odb::Rect last_via_rect;

    int array_x = array_start_x_ + x;
    for (int col = 0; col < columns_; col++) {
      DbGenerateVia* via = core_via_.get();
      if (row == 0 && col == 0) {
        if (end_of_row_column_ != nullptr) {
          via = end_of_row_column_.get();
        }
      } else if (row == 0) {
        if (end_of_row_ != nullptr) {
          via = end_of_row_.get();
        }
      } else if (col == 0) {
        if (end_of_column_ != nullptr) {
          via = end_of_column_.get();
        }
      }

      auto shapes = via->generate(block, wire, type, array_x, array_y);
      via_shapes.bottom.merge(shapes.bottom);
      via_shapes.top.merge(shapes.top);

      last_via_rect = via->getViaRect(false);

      array_x
          += (core_via_rect.dx() + last_via_rect.dx()) / 2 + array_spacing_x_;
    }
    array_y += (core_via_rect.dy() + last_via_rect.dy()) / 2 + array_spacing_y_;
  }
  return via_shapes;
}

/////////////

GenerateVia::GenerateVia(odb::dbTechViaGenerateRule* rule,
                         const odb::Rect& lower_rect,
                         const odb::Rect& upper_rect)
    : rule_(rule),
      lower_rect_(lower_rect),
      upper_rect_(upper_rect),
      intersection_rect_({}),
      cutclass_(nullptr),
      core_row_(0),
      core_col_(0),
      end_row_(0),
      end_col_(0),
      array_spacing_x_(0),
      array_spacing_y_(0),
      array_core_x_(1),
      array_core_y_(1),
      cut_pitch_x_(0),
      cut_pitch_y_(0),
      bottom_x_enclosure_(0),
      bottom_y_enclosure_(0),
      top_x_enclosure_(0),
      top_y_enclosure_(0)
{
  lower_rect_.intersection(upper_rect_, intersection_rect_);

  const uint layer_count = rule_->getViaLayerRuleCount();

  std::map<odb::dbTechLayer*, uint> layer_map;
  std::vector<odb::dbTechLayer*> layers;
  for (uint l = 0; l < layer_count; l++) {
    odb::dbTechLayer* layer = rule_->getViaLayerRule(l)->getLayer();
    layer_map[layer] = l;
    layers.push_back(layer);
  }

  std::sort(layers.begin(),
            layers.end(),
            [](odb::dbTechLayer* l, odb::dbTechLayer* r) {
              return l->getNumber() < r->getNumber();
            });

  for (int i = 0; i < 3; i++) {
    layers_[i] = layer_map[layers[i]];
  }

  auto* cut_rule = getCutLayerRule();
  if (cut_rule->hasSpacing()) {
    cut_rule->getSpacing(cut_pitch_x_, cut_pitch_y_);
  }
}

int GenerateVia::getRows() const
{
  int rows = 0;

  rows += array_core_y_ * core_row_;
  rows += end_row_;

  return rows;
}

int GenerateVia::getColumns() const
{
  int columns = 0;

  columns += array_core_x_ * core_col_;
  columns += end_col_;

  return columns;
}

const std::string GenerateVia::getName() const
{
  const std::string seperator = "_";
  std::string name = "via";
  // name after layers connected
  name += std::to_string(getBottomLayer()->getRoutingLevel());
  name += seperator;
  name += std::to_string(getTopLayer()->getRoutingLevel());
  // name after area
  name += seperator;
  name += std::to_string(intersection_rect_.dx());
  name += seperator;
  name += std::to_string(intersection_rect_.dy());
  // name after rows and columns
  name += seperator;
  name += std::to_string(getRows());
  name += seperator;
  name += std::to_string(getColumns());
  // name after pitch
  name += seperator;
  name += std::to_string(cut_pitch_x_);
  name += seperator;
  name += std::to_string(cut_pitch_y_);

  return name;
}

const std::string GenerateVia::getRuleName() const
{
  return rule_->getName();
}

odb::dbTechViaLayerRule* GenerateVia::getBottomLayerRule() const
{
  return rule_->getViaLayerRule(layers_[0]);
}

odb::dbTechLayer* GenerateVia::getBottomLayer() const
{
  return getBottomLayerRule()->getLayer();
}

odb::dbTechViaLayerRule* GenerateVia::getCutLayerRule() const
{
  return rule_->getViaLayerRule(layers_[1]);
}

odb::dbTechLayer* GenerateVia::getCutLayer() const
{
  return getCutLayerRule()->getLayer();
}

odb::dbTechViaLayerRule* GenerateVia::getTopLayerRule() const
{
  return rule_->getViaLayerRule(layers_[2]);
}

odb::dbTechLayer* GenerateVia::getTopLayer() const
{
  return getTopLayerRule()->getLayer();
}

bool GenerateVia::isBottomValidForWidth(int width) const
{
  return isLayerValidForWidth(getBottomLayerRule(), width);
}

bool GenerateVia::isTopValidForWidth(int width) const
{
  return isLayerValidForWidth(getTopLayerRule(), width);
}

bool GenerateVia::isLayerValidForWidth(odb::dbTechViaLayerRule* rule,
                                       int width) const
{
  if (!rule->hasWidth()) {
    // rule does not require width, so true
    return true;
  }

  int min, max;
  rule->getWidth(min, max);

  // check if width is within rule
  return min <= width && width <= max;
}

const odb::Rect GenerateVia::getCut() const
{
  odb::Rect rect;
  auto* rule = getCutLayerRule();
  if (rule->hasRect()) {
    rule->getRect(rect);
  }
  return rect;
}

int GenerateVia::getTotalCuts() const
{
  return getRows() * getColumns();
}

int GenerateVia::getCutArea() const
{
  return getCut().area() * getTotalCuts();
}

void GenerateVia::getLayerEnclosureRule(odb::dbTechViaLayerRule* rule,
                                        int& dx,
                                        int& dy) const
{
  if (rule->hasEnclosure()) {
    rule->getEnclosure(dx, dy);
  } else if (rule->hasOverhang()) {
    dx = dy = rule->getOverhang();
  } else if (rule->hasMetalOverhang()) {
    dx = dy = rule->getMetalOverhang();
  } else {
    dx = 0;
    dy = 0;
  }
}

void GenerateVia::getMinimumEnclosure(odb::dbTechLayer* layer,
                                      int width,
                                      int& dx,
                                      int& dy) const
{
  dx = 0;
  dy = 0;
  if (layer == getBottomLayer()) {
    getLayerEnclosureRule(getBottomLayerRule(), dx, dy);
  } else if (layer == getTopLayer()) {
    getLayerEnclosureRule(getTopLayerRule(), dx, dy);
  }
}

void GenerateVia::determineRowsAndColumns(bool use_bottom_min_enclosure,
                                          bool use_top_min_enclosure)
{
  const odb::Rect cut = getCut();
  const int cut_width = cut.dx();
  const int cut_height = cut.dy();

  // determine enclosure needed
  const int width = intersection_rect_.dx();
  const int height = intersection_rect_.dy();
  int minimum_enclosure_bottom_x, minimum_enclosure_bottom_y;
  getMinimumEnclosure(getBottomLayer(),
                      intersection_rect_.minDXDY(),
                      minimum_enclosure_bottom_x,
                      minimum_enclosure_bottom_y);
  int minimum_enclosure_top_x, minimum_enclosure_top_y;
  getMinimumEnclosure(getTopLayer(),
                      intersection_rect_.minDXDY(),
                      minimum_enclosure_top_x,
                      minimum_enclosure_top_y);

  // determine rows and columns possible
  int cols;
  getCuts(width,
          cut_width,
          minimum_enclosure_bottom_x,
          minimum_enclosure_top_x,
          cut_pitch_x_,
          cols);
  int rows;
  getCuts(height,
          cut_height,
          minimum_enclosure_bottom_y,
          minimum_enclosure_top_y,
          cut_pitch_y_,
          rows);

  const int array_size = std::max(rows, cols);
  if (array_size >= 2) {
    // if array rules might apply
    const int array_area_x
        = width
          - 2 * std::max(minimum_enclosure_bottom_x, minimum_enclosure_top_x);
    const int array_area_y
        = height
          - 2 * std::max(minimum_enclosure_bottom_y, minimum_enclosure_top_y);
    int max_cut_area = 0;
    for (const auto& spacing : getArraySpacing()) {
      if (spacing.width != 0 && spacing.width > width) {
        // this rule is ignored due to width
        continue;
      }

      if (spacing.cuts > array_size) {
        // this rule is ignored due to cuts
        continue;
      }

      int cut_spacing_x = cut_pitch_x_ - cut_width;
      int cut_spacing_y = cut_pitch_y_ - cut_height;
      if (spacing.cut_spacing != 0) {
        // reset spacing based on rule
        cut_spacing_x = spacing.cut_spacing;
        cut_spacing_y = spacing.cut_spacing;
      }
      // determine new rows and columns for array segments
      int x_cuts;
      getCuts(width,
              cut_width,
              minimum_enclosure_bottom_x,
              minimum_enclosure_top_x,
              cut_spacing_x + cut_width,
              x_cuts);
      // if long array allowed,  leave x alone
      x_cuts = spacing.longarray ? x_cuts : std::min(x_cuts, spacing.cuts);
      int y_cuts;
      getCuts(height,
              cut_height,
              minimum_enclosure_bottom_y,
              minimum_enclosure_top_y,
              cut_spacing_y + cut_height,
              y_cuts);
      y_cuts = std::min(y_cuts, spacing.cuts);
      const int array_width_x
          = getCutsWidth(x_cuts, cut_width, cut_spacing_x, 0);
      const int array_width_y
          = getCutsWidth(y_cuts, cut_height, cut_spacing_y, 0);

      const int array_pitch_x = array_width_x + spacing.array_spacing;
      const int array_pitch_y = array_width_y + spacing.array_spacing;

      const int full_arrays_x
          = (array_area_x - array_width_x) / array_pitch_x + 1;
      const int full_arrays_y
          = (array_area_y - array_width_y) / array_pitch_y + 1;

      // determine how many via row and columns are needed in the last segments of the array
      // if any
      int last_rows = 0;
      int last_cols = 0;
      const int remainder_x = array_area_x - full_arrays_x * array_pitch_x;
      if (remainder_x != 0) {
        getCuts(
            remainder_x, cut_width, 0, 0, cut_spacing_x + cut_width, last_cols);
      }
      const int remainder_y = array_area_y - full_arrays_y * array_pitch_y;
      if (remainder_y != 0) {
        getCuts(remainder_y,
                cut_width,
                0,
                0,
                cut_spacing_y + cut_height,
                last_rows);
      }

      const int total_cut_area = cut.area()
                                 * (full_arrays_x * x_cuts + last_cols)
                                 * (full_arrays_y * y_cuts + last_rows);
      if (max_cut_area < total_cut_area) {
        // new array contains a greater cut area, so save this
        array_core_x_ = full_arrays_x;
        array_core_y_ = full_arrays_y;
        core_col_ = x_cuts;
        core_row_ = y_cuts;
        end_col_ = last_cols;
        end_row_ = last_rows;

        cut_pitch_x_ = cut_spacing_x + cut_width;
        cut_pitch_y_ = cut_spacing_y + cut_height;

        array_spacing_x_ = spacing.array_spacing;
        array_spacing_y_ = spacing.array_spacing;

        const int via_width_x
            = full_arrays_x * getCutsWidth(x_cuts, cut_width, cut_spacing_x, 0)
              + (full_arrays_x - 1) * array_spacing_x_
              + getCutsWidth(last_cols, cut_width, cut_spacing_x, 0)
              + (last_cols > 0 ? array_spacing_x_ : 0);
        const int double_enc_x = width - via_width_x;
        const int via_width_y
            = full_arrays_y * getCutsWidth(y_cuts, cut_height, cut_spacing_y, 0)
              + (full_arrays_y - 1) * array_spacing_y_
              + getCutsWidth(last_rows, cut_height, cut_spacing_y, 0)
              + (last_rows > 0 ? array_spacing_y_ : 0);
        const int double_enc_y = height - via_width_y;

        if (use_bottom_min_enclosure) {
          bottom_x_enclosure_ = minimum_enclosure_bottom_x;
          bottom_y_enclosure_ = minimum_enclosure_bottom_y;
        } else {
          bottom_x_enclosure_ = double_enc_x / 2;
          bottom_y_enclosure_ = double_enc_y / 2;
        }
        if (use_top_min_enclosure) {
          top_x_enclosure_ = minimum_enclosure_top_x;
          top_y_enclosure_ = minimum_enclosure_top_y;
        } else {
          top_x_enclosure_ = double_enc_x / 2;
          top_y_enclosure_ = double_enc_y / 2;
        }

        max_cut_area = total_cut_area;
      }
    }

    if (max_cut_area != 0) {
      // found an array, so done
      determineCutClass();
      return;
    }
  }

  array_core_x_ = 1;
  array_core_y_ = 1;
  core_col_ = cols;
  core_row_ = rows;
  end_col_ = 0;
  end_row_ = 0;

  const int via_width_x
      = getCutsWidth(cols, cut_width, cut_pitch_x_ - cut_width, 0);
  const int double_enc_x = width - via_width_x;
  const int via_width_y
      = getCutsWidth(rows, cut_height, cut_pitch_y_ - cut_height, 0);
  const int double_enc_y = height - via_width_y;

  if (use_bottom_min_enclosure) {
    bottom_x_enclosure_ = minimum_enclosure_bottom_x;
    bottom_y_enclosure_ = minimum_enclosure_bottom_y;
  } else {
    bottom_x_enclosure_ = double_enc_x / 2;
    bottom_y_enclosure_ = double_enc_y / 2;
  }
  if (use_top_min_enclosure) {
    top_x_enclosure_ = minimum_enclosure_top_x;
    top_y_enclosure_ = minimum_enclosure_top_y;
  } else {
    top_x_enclosure_ = double_enc_x / 2;
    top_y_enclosure_ = double_enc_y / 2;
  }

  determineCutClass();
}

void GenerateVia::getCuts(int width,
                          int cut,
                          int bot_enc,
                          int top_enc,
                          int pitch,
                          int& cuts) const
{
  const int max_enc = std::max(bot_enc, top_enc);
  const int available_width = width - 2 * max_enc;
  if (available_width < 0) {
    cuts = 0;
    return;
  }

  int additional_cuts = (available_width - cut) / pitch;
  if (additional_cuts < 0) {
    additional_cuts = 0;
  }

  cuts = additional_cuts + 1;
}

int GenerateVia::getCutsWidth(int cuts,
                              int cut_width,
                              int spacing,
                              int enc) const
{
  if (cuts == 0) {
    return 0;
  }
  return cut_width * cuts + spacing * (cuts - 1) + 2 * enc;
}

void GenerateVia::determineCutClass()
{
  const odb::Rect cut = getCut();

  auto* layer = getCutLayer();
  for (auto* rule : layer->getTechLayerCutClassRules()) {
    if (cut.dx() != rule->getWidth()) {
      continue;
    }
    if (rule->isLengthValid()) {
      if (cut.dy() != rule->getLength()) {
        continue;
      }
    } else {
      if (cut.dy() != rule->getWidth()) {
        continue;
      }
    }

    cutclass_ = rule;
    break;
  }
}

std::vector<GenerateVia::ArraySpacing> GenerateVia::getArraySpacing() const
{
  const TechLayer tech_layer(getCutLayer());
  const std::vector<std::string> tokenized
      = tech_layer.tokenizeStringProperty("LEF58_ARRAYSPACING");
  if (tokenized.empty()) {
    return {};
  }

  // get cut spacing
  int cut_spacing = 0;
  auto cut_spacing_find
      = std::find(tokenized.begin(), tokenized.end(), "CUTSPACING");
  if (cut_spacing_find != tokenized.end()) {
    cut_spacing_find++;

    cut_spacing = tech_layer.micronToDbu(*cut_spacing_find);
  }
  int width = 0;
  auto width_find = std::find(tokenized.begin(), tokenized.end(), "WIDTH");
  if (width_find != tokenized.end()) {
    width_find++;

    width = tech_layer.micronToDbu(*width_find);
  }
  bool longarray = false;
  auto longarray_find
      = std::find(tokenized.begin(), tokenized.end(), "LONGARRAY");
  if (longarray_find != tokenized.end()) {
    longarray = true;
  }

  // get cuts
  std::vector<ArraySpacing> spacing;
  ArraySpacing props{0, false, 0, 0, 0};
  for (auto itr = tokenized.begin(); itr != tokenized.end(); itr++) {
    if (*itr == "ARRAYCUTS") {
      if (props.cuts != 0) {
        spacing.push_back(props);
      }
      itr++;
      const int cuts = std::stoi(*itr);
      props = ArraySpacing{width, longarray, cut_spacing, cuts, 0};
      continue;
    }
    if (*itr == "SPACING") {
      itr++;
      const int spacing = tech_layer.micronToDbu(*itr);
      props.array_spacing = spacing;
      continue;
    }
  }

  return spacing;
}

DbVia* GenerateVia::generate(odb::dbBlock* block) const
{
  if (!isCutArray()) {
    return new DbGenerateVia(intersection_rect_,
                             rule_,
                             core_row_,
                             core_col_,
                             cut_pitch_x_,
                             cut_pitch_y_,
                             bottom_x_enclosure_,
                             bottom_y_enclosure_,
                             top_x_enclosure_,
                             top_y_enclosure_,
                             getBottomLayer(),
                             getCutLayer(),
                             getTopLayer());
  } else {
    DbGenerateVia* core_via = new DbGenerateVia(intersection_rect_,
                                                rule_,
                                                core_row_,
                                                core_col_,
                                                cut_pitch_x_,
                                                cut_pitch_y_,
                                                bottom_x_enclosure_,
                                                bottom_y_enclosure_,
                                                top_x_enclosure_,
                                                top_y_enclosure_,
                                                getBottomLayer(),
                                                getCutLayer(),
                                                getTopLayer());
    DbGenerateVia* end_of_row = nullptr;
    DbGenerateVia* end_of_column = nullptr;
    DbGenerateVia* end_of_row_column = nullptr;
    const bool need_end_of_row = end_row_ != 0;
    if (need_end_of_row) {
      end_of_row = new DbGenerateVia(intersection_rect_,
                                     rule_,
                                     end_row_,
                                     core_col_,
                                     cut_pitch_x_,
                                     cut_pitch_y_,
                                     bottom_x_enclosure_,
                                     bottom_y_enclosure_,
                                     top_x_enclosure_,
                                     top_y_enclosure_,
                                     getBottomLayer(),
                                     getCutLayer(),
                                     getTopLayer());
    }
    const bool need_end_of_column = end_col_ != 0;
    if (need_end_of_column) {
      end_of_column = new DbGenerateVia(intersection_rect_,
                                        rule_,
                                        core_row_,
                                        end_col_,
                                        cut_pitch_x_,
                                        cut_pitch_y_,
                                        bottom_x_enclosure_,
                                        bottom_y_enclosure_,
                                        top_x_enclosure_,
                                        top_y_enclosure_,
                                        getBottomLayer(),
                                        getCutLayer(),
                                        getTopLayer());
    }
    if (need_end_of_row || need_end_of_column) {
      const int rows = need_end_of_row ? end_row_ : core_row_;
      const int cols = need_end_of_column ? end_col_ : core_col_;
      end_of_row_column = new DbGenerateVia(intersection_rect_,
                                            rule_,
                                            rows,
                                            cols,
                                            cut_pitch_x_,
                                            cut_pitch_y_,
                                            bottom_x_enclosure_,
                                            bottom_y_enclosure_,
                                            top_x_enclosure_,
                                            top_y_enclosure_,
                                            getBottomLayer(),
                                            getCutLayer(),
                                            getTopLayer());
    }
    return new DbGenerateArrayVia(core_via,
                                  end_of_row,
                                  end_of_column,
                                  end_of_row_column,
                                  array_core_y_,
                                  array_core_x_,
                                  array_spacing_x_,
                                  array_spacing_y_);
  }
}

bool GenerateVia::checkMinCuts() const
{
  const int total_cuts = getTotalCuts();
  if (cutclass_ != nullptr) {
    if (cutclass_->isCutsValid()) {
      if (total_cuts < cutclass_->getNumCuts()) {
        // fails the cutclass
        return false;
      }
    }
  }

  bool is_valid = false;
  bool has_rules = false;

  for (auto* min_cut_rule : getCutLayer()->getMinCutRules()) {
    has_rules = true;

    uint numcuts;
    uint width;
    min_cut_rule->getMinimumCuts(numcuts, width);
    if (numcuts > total_cuts) {
      continue;
    }

    is_valid = true;
  }

  return is_valid | !has_rules;
}

bool GenerateVia::checkMinEnclosure() const
{
  bool is_valid = false;
  bool has_rules = false;

  for (auto* enc_rule : getCutLayer()->getTechLayerCutEnclosureRules()) {
    auto* enc_rule_cut_class = enc_rule->getCutClass();
    if (cutclass_ != nullptr && enc_rule_cut_class != nullptr) {
      if (cutclass_ != enc_rule_cut_class) {
        // cut classes do not match
        continue;
      }
    }

    bool check_top = enc_rule->isAbove();
    bool check_bot = enc_rule->isBelow();
    if (!check_top && !check_bot) {
      check_top = true;
      check_bot = true;
    }

    if (check_bot) {
      const int width = lower_rect_.minDXDY();

      if (enc_rule->isWidthValid() && width > enc_rule->getMinWidth()) {
        continue;
      }
      has_rules = true;

      if (bottom_x_enclosure_ < enc_rule->getFirstOverhang()) {
        continue;
      }

      if (bottom_y_enclosure_ < enc_rule->getSecondOverhang()) {
        continue;
      }
    }

    if (check_top) {
      const int width = upper_rect_.minDXDY();
      if (enc_rule->isWidthValid() && width > enc_rule->getMinWidth()) {
        continue;
      }
      has_rules = true;

      if (top_x_enclosure_ < enc_rule->getFirstOverhang()) {
        continue;
      }

      if (top_y_enclosure_ < enc_rule->getSecondOverhang()) {
        continue;
      }
    }

    is_valid = true;
  }

  return is_valid | !has_rules;
}

/////////

Via::Via(Connect* connect,
         odb::dbNet* net,
         const odb::Rect& area,
         const ShapePtr& lower,
         const ShapePtr& upper)
    : net_(net), area_(area), lower_(lower), upper_(upper), connect_(connect)
{
}

odb::dbTechLayer* Via::getLowerLayer() const
{
  return connect_->getLowerLayer();
}

odb::dbTechLayer* Via::getUpperLayer() const
{
  return connect_->getUpperLayer();
}

const Box Via::getBox() const
{
  return Box(Point(area_.xMin(), area_.yMin()),
             Point(area_.xMax(), area_.yMax()));
}

Via* Via::copy() const
{
  return new Via(connect_, net_, area_, lower_, upper_);
}

bool Via::overlaps(const ViaPtr& via) const
{
  return connect_->overlaps(via->getConnect());
}

bool Via::startsBelow(const ViaPtr& via) const
{
  return connect_->startsBelow(via->getConnect());
}

void Via::writeToDb(odb::dbSWire* wire, odb::dbBlock* block) const
{
  odb::dbWireShapeType type = lower_->getType();

  if (lower_->getType() != upper_->getType()) {
    // If both shapes are not the same, use stripe
    type = odb::dbWireShapeType::STRIPE;
  }

  if (connect_->isTaperedVia(lower_->getRect(), upper_->getRect())) {
    connect_->getGrid()->getLogger()->warn(
        utl::PDN,
        111,
        "Tapered via required between {} and {}, which will not be added.",
        getLowerLayer()->getName(),
        getUpperLayer()->getName());
  } else {
    connect_->makeVia(wire, lower_->getRect(), upper_->getRect(), type);
  }
}

const std::string Via::getDisplayText() const
{
  const std::string seperator = ":";
  std::string text;

  text += net_->getName() + seperator;
  text += getLowerLayer()->getName() + seperator;
  text += getUpperLayer()->getName() + seperator;
  if (getGrid() != nullptr) {
    text += getGrid()->getName();
  }

  return text;
}

bool Via::isValid() const
{
  return lower_ != nullptr && upper_ != nullptr;
}

void Via::removeShape(Shape* shape)
{
  if (lower_.get() == shape) {
    lower_ = nullptr;
  } else if (upper_.get() == shape) {
    upper_ = nullptr;
  }
}

bool Via::containsIntermediateLayer(odb::dbTechLayer* layer) const
{
  return connect_->containsIntermediateLayer(layer);
}

Grid* Via::getGrid() const
{
  return connect_->getGrid();
}

}  // namespace pdn
