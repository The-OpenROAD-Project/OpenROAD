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

#include "connect.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Enclosure::Enclosure(int x, int y, odb::dbTechLayer* layer) : x_(x), y_(y)
{
  if (layer->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    if (x_ > y_) {
      std::swap(x_, y_);
    }
  } else {
    if (y_ > x_) {
      std::swap(x_, y_);
    }
  }
}

//////////

DbVia::ViaLayerShape DbVia::getLayerShapes(odb::dbSBox* box) const
{
  std::vector<odb::dbShape> shapes;
  box->getViaBoxes(shapes);

  std::map<int, std::set<odb::Rect>> layer_rects;
  for (auto& shape : shapes) {
    auto* layer = shape.getTechLayer();
    if (layer->getType() == odb::dbTechLayerType::ROUTING) {
      odb::Rect box_shape;
      shape.getBox(box_shape);
      layer_rects[layer->getRoutingLevel()].insert(box_shape);
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

void DbVia::combineLayerShapes(const ViaLayerShape& other,
                               ViaLayerShape& shapes) const
{
  shapes.bottom.insert(other.bottom.begin(), other.bottom.end());
  shapes.top.insert(other.top.begin(), other.top.end());
}

odb::Rect DbVia::adjustToMinArea(odb::dbTechLayer* layer, const odb::Rect& rect) const
{
  odb::Rect new_rect = rect;

  if (!layer->hasArea()) {
    return new_rect;
  }

  const double min_area = layer->getArea();
  if (min_area == 0.0) {
    return new_rect;
  }

  // make sure minimum area is honored
  const int dbu_per_micron = layer->getTech()->getLefUnits();
  const double area
      = min_area * dbu_per_micron * dbu_per_micron;

  const TechLayer techlayer(layer);

  const int width = new_rect.dx();
  const int height = new_rect.dy();
  if (width * height < area) {
    if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
      const int required_width = std::ceil(area / height);
      const int added_width = (required_width - width) / 2;
      const int new_x0 = techlayer.snapToManufacturingGrid(rect.xMin() - added_width, false);
      const int new_x1 = techlayer.snapToManufacturingGrid(rect.xMax() + added_width, true);
      new_rect.set_xlo(new_x0);
      new_rect.set_xhi(new_x1);
    } else {
      const int required_height = std::ceil(area / width);
      const int added_height = (required_height - height) / 2;
      const int new_y0 = techlayer.snapToManufacturingGrid(rect.yMin() - added_height, false);
      const int new_y1 = techlayer.snapToManufacturingGrid(rect.yMax() + added_height, true);
      new_rect.set_ylo(new_y0);
      new_rect.set_yhi(new_y1);
    }
  }

  return new_rect;
}

////////////

DbTechVia::DbTechVia(odb::dbTechVia* via,
                     int rows,
                     int row_pitch,
                     int cols,
                     int col_pitch)
    : via_(via),
      rows_(rows),
      row_pitch_(row_pitch),
      cols_(cols),
      col_pitch_(col_pitch)
{
  via_rect_.mergeInit();
  enc_bottom_rect_.mergeInit();
  enc_top_rect_.mergeInit();

  for (auto* box : via_->getBoxes()) {
    auto* layer = box->getTechLayer();
    if (layer == nullptr) {
      continue;
    }

    odb::Rect rect;
    box->getBox(rect);

    if (layer->getType() == odb::dbTechLayerType::CUT) {
      via_rect_.merge(rect);
    } else {
      if (layer == via->getBottomLayer()) {
        enc_bottom_rect_.merge(rect);
      } else {
        enc_top_rect_.merge(rect);
      }
    }
  }
}

DbVia::ViaLayerShape DbTechVia::generate(odb::dbBlock* block,
                                         odb::dbSWire* wire,
                                         odb::dbWireShapeType type,
                                         int x,
                                         int y) const
{
  ViaLayerShape via_shapes;

  odb::Rect via_rect(0, 0, (cols_ - 1) * col_pitch_, (rows_ - 1) * row_pitch_);
  via_rect.moveTo(x - via_rect.dx() / 2, y - via_rect.dy() / 2);

  int row = via_rect.yMin();
  for (int r = 0; r < rows_; r++) {
    int col = via_rect.xMin();
    for (int c = 0; c < cols_; c++) {
      auto shapes
          = getLayerShapes(odb::dbSBox::create(wire, via_, col, row, type));
      combineLayerShapes(shapes, via_shapes);

      col += col_pitch_;
    }

    row += row_pitch_;
  }

  return via_shapes;
}

const odb::Rect DbTechVia::getViaRect(bool include_enclosure, bool include_bottom, bool include_top) const
{
  if (include_enclosure) {
    odb::Rect enc;
    enc.mergeInit();
    if (include_bottom) {
      enc.merge(enc_bottom_rect_);
    }
    if (include_top) {
      enc.merge(enc_top_rect_);
    }
    return enc;
  } else {
    return via_rect_;
  }
}

///////////

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

const odb::Rect DbGenerateVia::getViaRect(bool include_enclosure, bool /* include_bottom */, bool /* include_top */) const
{
  const int y_enc = std::max(bottom_enclosure_y_, top_enclosure_y_);
  const int height = (rows_ - 1) * cut_pitch_y_ + cut_rect_.dy();
  const int x_enc = std::max(bottom_enclosure_x_, top_enclosure_x_);
  const int width = (columns_ - 1) * cut_pitch_x_ + cut_rect_.dx();

  const int height_enc = include_enclosure ? y_enc : 0;
  const int width_enc = include_enclosure ? x_enc : 0;

  const int height_half = height / 2;
  const int width_half = width / 2;

  return {-width_half - width_enc, -height_half - height_enc,
	      width_half + width_enc, height_half + height_enc};
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

DbArrayVia::DbArrayVia(DbBaseVia* core_via,
                       DbBaseVia* end_of_row,
                       DbBaseVia* end_of_column,
                       DbBaseVia* end_of_row_column,
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

DbVia::ViaLayerShape DbArrayVia::generate(odb::dbBlock* block,
                                          odb::dbSWire* wire,
                                          odb::dbWireShapeType type,
                                          int x,
                                          int y) const
{
  const odb::Rect core_via_rect = core_via_->getViaRect(false);
  ViaLayerShape via_shapes;

  int array_y = array_start_y_ + y;
  for (int row = 0; row < rows_; row++) {
    odb::Rect last_via_rect;

    int array_x = array_start_x_ + x;
    for (int col = 0; col < columns_; col++) {
      DbBaseVia* via = core_via_.get();
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
      combineLayerShapes(shapes, via_shapes);

      last_via_rect = via->getViaRect(false);

      array_x
          += (core_via_rect.dx() + last_via_rect.dx()) / 2 + array_spacing_x_;
    }
    array_y += (core_via_rect.dy() + last_via_rect.dy()) / 2 + array_spacing_y_;
  }
  return via_shapes;
}

/////////////

DbSplitCutVia::DbSplitCutVia(DbBaseVia* via,
                             int rows,
                             int row_pitch,
                             int cols,
                             int col_pitch,
                             odb::dbBlock* block,
                             odb::dbTechLayer* bottom,
                             bool snap_bottom,
                             odb::dbTechLayer* top,
                             bool snap_top)
    : bottom_(std::make_unique<TechLayer>(bottom)),
      top_(std::make_unique<TechLayer>(top)),
      via_(std::unique_ptr<DbBaseVia>(via)),
      rows_(rows),
      row_pitch_(row_pitch),
      cols_(cols),
      col_pitch_(col_pitch)
{
  if (snap_bottom) {
    bottom_->populateGrid(block);
  }
  if (snap_top) {
    top_->populateGrid(block);
  }
}

DbSplitCutVia::~DbSplitCutVia()
{
}

DbVia::ViaLayerShape DbSplitCutVia::generate(odb::dbBlock* block,
                                             odb::dbSWire* wire,
                                             odb::dbWireShapeType type,
                                             int x,
                                             int y) const
{
  TechLayer* horizontal = nullptr;
  TechLayer* vertical = nullptr;
  if (bottom_->getLayer()->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
    horizontal = bottom_.get();
    vertical = top_.get();
  } else {
    horizontal = top_.get();
    vertical = bottom_.get();
  }

  ViaLayerShape via_shapes;

  odb::Rect via_rect(0, 0, (cols_ - 1) * col_pitch_, (rows_ - 1) * row_pitch_);
  via_rect.moveTo(x - via_rect.dx() / 2, y - via_rect.dy() / 2);

  int row = via_rect.yMin();
  for (int r = 0; r < rows_; r++) {
    const int row_pos = horizontal->snapToGrid(row);

    int col = via_rect.xMin();
    for (int c = 0; c < cols_; c++) {
      const int col_pos = vertical->snapToGrid(col);

      auto shapes = via_->generate(block, wire, type, col_pos, row_pos);
      combineLayerShapes(shapes, via_shapes);

      col = col_pos + col_pitch_;
    }

    row = row_pos + row_pitch_;
  }

  return via_shapes;
}

/////////////

DbGenerateStackedVia::DbGenerateStackedVia(
    const std::vector<DbVia*>& vias,
    odb::dbTechLayer* bottom,
    odb::dbBlock* block,
    const std::set<odb::dbTechLayer*>& ongrid)
{
  for (auto* via : vias) {
    vias_.push_back(std::unique_ptr<DbVia>(via));
  }

  int bottom_layer = bottom->getRoutingLevel();
  auto* tech = bottom->getTech();
  for (int i = 0; i < vias.size() + 1; i++) {
    auto layer
        = std::make_unique<TechLayer>(tech->findRoutingLayer(bottom_layer + i));
    if (ongrid.find(layer->getLayer()) != ongrid.end()) {
      layer->populateGrid(block);
    }
    layers_.push_back(std::move(layer));
  }
}

DbGenerateStackedVia::~DbGenerateStackedVia()
{
}

DbVia::ViaLayerShape DbGenerateStackedVia::generate(odb::dbBlock* block,
                                                    odb::dbSWire* wire,
                                                    odb::dbWireShapeType type,
                                                    int x,
                                                    int y) const
{
  using namespace boost::polygon::operators;
  using Rectangle = boost::polygon::rectangle_data<int>;
  using Polygon90 = boost::polygon::polygon_90_with_holes_data<int>;
  using Polygon90Set = boost::polygon::polygon_90_set_data<int>;
  using Pt = Polygon90::point_type;

  auto rect_to_poly = [](const odb::Rect& rect) -> Polygon90 {
    std::array<Pt, 4> pts = {Pt(rect.xMin(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMin()),
                             Pt(rect.xMax(), rect.yMax()),
                             Pt(rect.xMin(), rect.yMax())};

    Polygon90 poly;
    poly.set(pts.begin(), pts.end());
    return poly;
  };

  ViaLayerShape via_shapes;

  DbVia* prev_via = nullptr;
  Polygon90Set top_of_previous;
  for (size_t i = 0; i < vias_.size(); i++) {
    const auto& via = vias_[i];
    const auto& layer_lower = layers_[i];
    const auto& layer_upper = layers_[i + 1];
    int layer_x = x;
    int layer_y = y;

    if (layer_lower->getLayer()->getDirection()
        == odb::dbTechLayerDir::HORIZONTAL) {
      layer_x = layer_upper->snapToGrid(layer_x);
      layer_y = layer_lower->snapToGrid(layer_y);
    } else {
      layer_x = layer_lower->snapToGrid(layer_x);
      layer_y = layer_upper->snapToGrid(layer_y);
    }

    auto shapes = via->generate(block, wire, type, layer_x, layer_y);
    if (i == 0) {
      via_shapes.bottom = shapes.bottom;
    }
    via_shapes.top = shapes.top;

    Polygon90Set patch_shapes;
    odb::dbTechLayer* add_to_layer = layer_lower->getLayer();
    if (prev_via != nullptr) {
      Polygon90Set bottom_of_current;
      for (const auto& shape : shapes.bottom) {
        bottom_of_current += rect_to_poly(shape);
      }

      // create a single set of shapes for the layer
      Polygon90Set combine_layer = top_of_previous + bottom_of_current;

      if (prev_via->requiresPatch() || via->requiresPatch()) {
        Rectangle patch_shape;
        combine_layer.extents(patch_shape);
        patch_shapes.clear();
        patch_shapes += patch_shape;
      } else {
        patch_shapes = combine_layer;
      }

      std::vector<Polygon90> patches;
      combine_layer.get_polygons(patches);

      // extract the rectangles that will patch the layer
      for (const auto& patch : patches) {
        Rectangle patch_shape;
        extents(patch_shape, patch);

        patch_shapes += patch_shape;
      }

      // ensure patches are minimum area
      std::vector<Rectangle> patch_rects;
      patch_shapes.get_rectangles(patch_rects);
      patch_shapes.clear();
      for (const auto& patch : patch_rects) {
        const odb::Rect patch_rect(xl(patch), yl(patch), xh(patch), yh(patch));
        odb::Rect min_area_shape = adjustToMinArea(add_to_layer, patch_rect);
        patch_shapes += rect_to_poly(min_area_shape);
      }

      // find shapes that touch "left-over" shapes from the xor
      patch_shapes = patch_shapes.interact(patch_shapes ^ combine_layer);
    }

    if (!patch_shapes.empty()) {
      std::vector<Rectangle> patches;
      patch_shapes.get_rectangles(patches);

      for (const auto& patch : patches) {
        // add patch metal on layers between the bottom and top of the via stack
        odb::dbSBox::create(wire,
                            add_to_layer,
                            xl(patch),
                            yl(patch),
                            xh(patch),
                            yh(patch),
                            type);
      }
    }

    prev_via = via.get();
    top_of_previous.clear();
    for (const auto& shape : shapes.top) {
      top_of_previous += rect_to_poly(shape);
    }
  }

  return via_shapes;
}

/////////////

DbGenerateDummyVia::DbGenerateDummyVia(utl::Logger* logger,
                                       const odb::Rect& shape,
                                       odb::dbTechLayer* bottom,
                                       odb::dbTechLayer* top)
    : logger_(logger), shape_(shape), bottom_(bottom), top_(top)
{
}

DbVia::ViaLayerShape DbGenerateDummyVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType /* type */,
    int x,
    int y) const
{
  odb::dbTransform xfm({x, y});

  odb::Rect via_area = shape_;
  xfm.apply(via_area);
  logger_->warn(utl::PDN,
                110,
                "No via inserted between {} and {} at {} on {}",
                bottom_->getName(),
                top_->getName(),
                Shape::getRectText(via_area, block->getDbUnitsPerMicron()),
                wire->getNet()->getName());

  return {};
}

/////////////

ViaGenerator::ViaGenerator(utl::Logger* logger,
                           const odb::Rect& lower_rect,
                           const odb::Rect& upper_rect)
    : logger_(logger),
      lower_rect_(lower_rect),
      upper_rect_(upper_rect),
      intersection_rect_(),
      cut_(),
      cutclass_(nullptr),
      cut_pitch_x_(0),
      cut_pitch_y_(0),
      max_rows_(0),
      max_cols_(0),
      core_row_(0),
      core_col_(0),
      end_row_(0),
      end_col_(0),
      split_cuts_bottom_(false),
      split_cuts_top_(false),
      array_spacing_x_(0),
      array_spacing_y_(0),
      array_core_x_(1),
      array_core_y_(1),
      bottom_x_enclosure_(0),
      bottom_y_enclosure_(0),
      top_x_enclosure_(0),
      top_y_enclosure_(0)
{
  lower_rect_.intersection(upper_rect_, intersection_rect_);
}

void ViaGenerator::setCut(const odb::Rect& cut)
{
  cut_ = cut;
  determineCutClass();
}

int ViaGenerator::getTotalCuts() const
{
  return getRows() * getColumns();
}

int ViaGenerator::getCutArea() const
{
  return cut_.area() * getTotalCuts();
}

int ViaGenerator::getCuts(int width,
                          int cut,
                          int bot_enc,
                          int top_enc,
                          int pitch,
                          int max_cuts) const
{
  const int max_enc = std::max(bot_enc, top_enc);
  int available_width = width - 2 * max_enc;
  if (available_width < 0) {
    return 0;
  }

  available_width -= cut;
  if (available_width < 0) {
    return 0;
  }

  if (pitch == 0) {
    return 1;
  }

  const int additional_cuts = available_width / pitch;
  const int cuts = additional_cuts + 1;
  if (max_cuts != 0) {
    return std::min(cuts, max_cuts);
  }
  return cuts;
}

int ViaGenerator::getCutsWidth(int cuts,
                               int cut_width,
                               int spacing,
                               int enc) const
{
  if (cuts == 0) {
    return 0;
  }
  return cut_width * cuts + spacing * (cuts - 1) + 2 * enc;
}

void ViaGenerator::determineCutClass()
{
  auto* layer = getCutLayer();
  for (auto* rule : layer->getTechLayerCutClassRules()) {
    const int rule_width = rule->getWidth();
    int rule_length = rule_width;
    if (rule->isLengthValid()) {
      rule_length = rule->getLength();
    }

    if ((cut_.dx() == rule_length && cut_.dy() == rule_width) ||
        (cut_.dx() == rule_width && cut_.dy() == rule_length)) {
      cutclass_ = rule;
      return;
    }
  }
}

bool ViaGenerator::checkConstraints() const
{
  if (getTotalCuts() == 0) {
    return false;
  }

  if (!checkMinCuts()) {
    debugPrint(logger_,
               utl::PDN,
               "Via",
               2,
               "Violates minimum cut rules: {}",
               getTotalCuts());

    return false;
  }

  if (!checkMinEnclosure()) {
    const double dbu_microns = getTech()->getLefUnits();
    debugPrint(logger_,
               utl::PDN,
               "Via",
               2,
               "Violates minimum enclosure rules: {} ({:.4f} {:.4f}) width {:.4f} - {} "
               "({:.4f} {:.4f}) width {:.4f}",
               getBottomLayer()->getName(),
               bottom_x_enclosure_ / dbu_microns,
               bottom_y_enclosure_ / dbu_microns,
               getLowerWidth() / dbu_microns,
               getTopLayer()->getName(),
               top_x_enclosure_ / dbu_microns,
               top_y_enclosure_ / dbu_microns,
               getUpperWidth() / dbu_microns);

    return false;
  }

  return true;
}

bool ViaGenerator::checkMinCuts() const
{
  const bool lower = checkMinCuts(getBottomLayer(), getLowerWidth());
  const bool upper = checkMinCuts(getTopLayer(), getUpperWidth());

  return lower && upper;
}

bool ViaGenerator::checkMinCuts(odb::dbTechLayer* layer, int width) const
{
  const int total_cuts = getTotalCuts();
  const bool is_below = getBottomLayer() == layer;

  // find all the rules that apply
  using MinCutRules = std::vector<TechLayer::MinCutRule>;
  std::map<int, MinCutRules> min_rules;
  const TechLayer tech_layer(layer);
  for (const auto& min_cut_rule : tech_layer.getMinCutRules()) {
    if (!isCutClass(min_cut_rule.cut_class)) {
      // cut classes do not match
     continue;
    }

    bool use_rule = false;
    if (min_cut_rule.below) {
      if (is_below) {
        use_rule = true;
      }
    } else if (min_cut_rule.above) {
      if (!is_below) {
        use_rule = true;
      }
    } else {
      use_rule = true;
    }
    if (!use_rule) {
      continue;
    }

    min_rules[min_cut_rule.width].push_back(min_cut_rule);
  }

  MinCutRules* min_rules_use = nullptr;
  for (auto& [rule_width, rules] : min_rules) {
    if (rule_width < width) {
      min_rules_use = &rules;
    }
  }

  debugPrint(logger_, utl::PDN, "MinCut", 1, "Layer {} of width {:.4f} has {} min cut rules.",
      layer->getName(),
      tech_layer.dbuToMicron(width),
      min_rules_use == nullptr ? 0 : min_rules_use->size());

  if (min_rules_use == nullptr) {
    // no rules apply so assume valid
    return true;
  }

  bool is_valid = false;
  for (const auto& min_cut_rule : *min_rules_use) {
    const bool pass = min_cut_rule.cuts <= total_cuts;

    debugPrint(logger_, utl::PDN, "MinCut", 2, "Rule width {:.4f} above ({}) or below ({}) requires {} vias: {}.",
        tech_layer.dbuToMicron(min_cut_rule.width),
        min_cut_rule.above,
        min_cut_rule.below,
        min_cut_rule.cuts,
        pass);

    is_valid |= pass;
  }

  return is_valid;
}

bool ViaGenerator::checkMinEnclosure() const
{
  auto check_overhang
      = [](int overhang1, int overhang2, int enc1, int enc2) -> bool {
    if (enc1 > enc2) {
      std::swap(enc1, enc2);
    }
    if (overhang1 > overhang2) {
      std::swap(overhang1, overhang2);
    }

    if (enc1 < overhang1) {
      return false;
    }

    if (enc2 < overhang2) {
      return false;
    }

    return true;
  };

  const double dbu = getTech()->getLefUnits();

  const int bottom_width = getLowerWidth(false);
  const auto bottom_rules = getCutMinimumEnclosureRules(bottom_width, false);
  const bool bottom_has_rules = !bottom_rules.empty();
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Bottom layer {} with width {:.4f} has {} rules and enclosures of {:4f} and {:4f}.",
             getBottomLayer()->getName(),
             bottom_width / dbu,
             bottom_rules.size(),
             bottom_x_enclosure_ / dbu,
             bottom_y_enclosure_ / dbu);
  bool bottom_passed = false;
  for (auto* rule : bottom_rules) {
    const bool pass = check_overhang(rule->getFirstOverhang(), rule->getSecondOverhang(), bottom_x_enclosure_, bottom_y_enclosure_);
    debugPrint(logger_,
               utl::PDN,
               "ViaEnclosure",
               2,
               "Bottom rule enclosures {:4f} and {:4f} -> {}.",
               rule->getFirstOverhang() / dbu,
               rule->getSecondOverhang() / dbu,
               pass)
    bottom_passed |= pass;
  }

  const int top_width = getUpperWidth(false);
  const auto top_rules = getCutMinimumEnclosureRules(top_width, true);
  const bool top_has_rules = !top_rules.empty();
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Top layer {} with width {:.4f} has {} rules and enclosures of {:4f} and {:4f}.",
             getTopLayer()->getName(),
             top_width / dbu,
             top_rules.size(),
             top_x_enclosure_ / dbu,
             top_y_enclosure_ / dbu);
  bool top_passed = false;
  for (auto* rule : top_rules) {
    const bool pass = check_overhang(rule->getFirstOverhang(), rule->getSecondOverhang(), top_x_enclosure_, top_y_enclosure_);
    debugPrint(logger_,
               utl::PDN,
               "ViaEnclosure",
               2,
               "Top rule enclosures {:4f} and {:4f} -> {}.",
               rule->getFirstOverhang() / dbu,
               rule->getSecondOverhang() / dbu,
               pass)
    top_passed |= pass;
  }

  return (!bottom_has_rules || bottom_passed) && (!top_has_rules || top_passed);
}

bool ViaGenerator::isCutClass(odb::dbTechLayerCutClassRule* cutclass) const
{
  if (cutclass_ != nullptr && cutclass != nullptr) {
    return cutclass_ == cutclass;
  }

  return true;
}

bool ViaGenerator::isSetupValid(odb::dbTechLayer* lower,
                                odb::dbTechLayer* upper) const
{
  return appliesToLayers(lower, upper);
}

bool ViaGenerator::appliesToLayers(odb::dbTechLayer* lower,
                                   odb::dbTechLayer* upper) const
{
  return getBottomLayer() == lower && getTopLayer() == upper;
}

void ViaGenerator::getMinimumEnclosures(std::vector<Enclosure>& bottom, std::vector<Enclosure>& top) const
{
  auto populate_enc = [this](odb::dbTechLayer* layer, int width, bool above, std::vector<Enclosure>& encs) {
    for (auto* rule : getCutMinimumEnclosureRules(width, above)) {
      encs.emplace_back(rule->getFirstOverhang(),
                        rule->getSecondOverhang(),
                        layer);
    }
    if (encs.empty()) {
      // assume zero enclosure when nothing is available
      encs.emplace_back(0, 0, layer);
    }
  };

  populate_enc(getBottomLayer(),
               getLowerWidth(false),
               false,
               bottom);
  populate_enc(getTopLayer(),
               getUpperWidth(false),
               true,
               top);
}

bool ViaGenerator::build(bool use_bottom_min_enclosure,
                         bool use_top_min_enclosure)
{
  std::vector<Enclosure> bottom_enclosures;
  std::vector<Enclosure> top_enclosures;
  getMinimumEnclosures(bottom_enclosures, top_enclosures);

  const bool bottom_is_horizontal = getBottomLayer()->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  const bool top_is_horizontal = getTopLayer()->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

  int best_cuts = 0;
  Enclosure* best_bot_enc = nullptr;
  Enclosure* best_top_enc = nullptr;
  for (auto& bottom_enc : bottom_enclosures) {
    for (auto& top_enc : top_enclosures) {
      determineRowsAndColumns(use_bottom_min_enclosure,
                              use_top_min_enclosure,
                              bottom_enc,
                              top_enc);

      if (!checkConstraints()) {
        continue;
      }

      bool save = best_cuts == 0;

      const int cuts = getTotalCuts();
      if (best_cuts == cuts) {
        // if same cut area, pick smaller enclosure
        if (best_bot_enc == nullptr) {
          save = true;
        } else if (bottom_is_horizontal) {
          if (best_bot_enc->getY() > bottom_enc.getY()) {
            save = true;
          }
        } else {
          if (best_bot_enc->getX() > bottom_enc.getX()) {
            save = true;
          }
        }
        if (best_top_enc == nullptr) {
          save = true;
        } else if (top_is_horizontal) {
          if (best_top_enc->getY() > top_enc.getY()) {
            save = true;
          }
        } else {
          if (best_top_enc->getX() > top_enc.getX()) {
            save = true;
          }
        }
      } else if (best_cuts < cuts) {
        save = true;
      }

      if (save) {
        best_bot_enc = &bottom_enc;
        best_top_enc = &top_enc;
        best_cuts = cuts;
      }
    }
  }

  if (best_cuts == 0) {
    return false;
  }

  // rebuild best
  determineRowsAndColumns(use_bottom_min_enclosure,
                          use_top_min_enclosure,
                          *best_bot_enc,
                          *best_top_enc);

  return true;
}

void ViaGenerator::determineRowsAndColumns(bool use_bottom_min_enclosure,
                                           bool use_top_min_enclosure,
                                           const Enclosure& bottom_min_enclosure,
                                           const Enclosure& top_min_enclosure)
{
  const double dbu_to_microns = getTech()->getLefUnits();

  const odb::Rect& cut = getCut();
  const int cut_width = cut.dx();
  const int cut_height = cut.dy();

  // determine enclosure needed
  const odb::Rect& intersection = getIntersectionRect();
  const int width = intersection.dx();
  const int height = intersection.dy();

  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Bottom layer {} with width {:.4f} minimum enclosures {:.4f} and {:.4f}.",
             getBottomLayer()->getName(),
             getLowerWidth(false) / dbu_to_microns,
             bottom_min_enclosure.getX() / dbu_to_microns,
             bottom_min_enclosure.getY() / dbu_to_microns);
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Top layer {} with width {:.4f} minimum enclosures {:.4f} and {:.4f}.",
             getTopLayer()->getName(),
             getUpperWidth(false) / dbu_to_microns,
             top_min_enclosure.getX() / dbu_to_microns,
             top_min_enclosure.getY() / dbu_to_microns);

  // determine rows and columns possible
  int cols;
  int rows;
  if (isSplitCutArray()) {
    cols = 1;
    rows = 1;
  } else {
    cols = getCuts(width,
                   cut_width,
                   bottom_min_enclosure.getX(),
                   top_min_enclosure.getX(),
                   getCutPitchX(),
                   getMaxColumns());

    rows = getCuts(height,
                   cut_height,
                   bottom_min_enclosure.getY(),
                   top_min_enclosure.getY(),
                   getCutPitchY(),
                   getMaxRows());
  }

  debugPrint(logger_,
             utl::PDN,
             "Via",
             2,
             "Initial via setup for {} {}: Cut size {:.4f}x{:.4f} on {} with pitch x={:.4f} and y={:.4f} yielding {} rows and {} columns.",
             Shape::getRectText(intersection, dbu_to_microns),
             isSplitCutArray(),
             cut.dx() / dbu_to_microns,
             cut.dy() / dbu_to_microns,
             getCutLayer()->getName(),
             getCutPitchX() / dbu_to_microns,
             getCutPitchY() / dbu_to_microns,
             rows,
             cols);

  bool used_array = false;

  const int array_size = std::max(rows, cols);
  if (array_size >= 2) {
    // if array rules might apply
    const int array_area_x
        = width
          - 2 * std::max(bottom_min_enclosure.getX(), top_min_enclosure.getX());
    const int array_area_y
        = height
          - 2 * std::max(bottom_min_enclosure.getY(), top_min_enclosure.getY());
    int max_cut_area = 0;
    const TechLayer layer(getCutLayer());
    for (const auto& spacing : layer.getArraySpacing()) {
      if (spacing.width != 0 && spacing.width > width) {
        // this rule is ignored due to width
        continue;
      }

      if (spacing.cuts > array_size) {
        // this rule is ignored due to cuts
        continue;
      }

      int cut_spacing_x = getCutPitchX() - cut_width;
      int cut_spacing_y = getCutPitchY() - cut_height;
      if (spacing.cut_spacing != 0) {
        // reset spacing based on rule
        cut_spacing_x = spacing.cut_spacing;
        cut_spacing_y = spacing.cut_spacing;
      }
      // determine new rows and columns for array segments
      int x_cuts = getCuts(width,
                           cut_width,
                           bottom_min_enclosure.getX(),
                           top_min_enclosure.getX(),
                           cut_spacing_x + cut_width,
                           getMaxColumns());
      // if long array allowed,  leave x alone
      x_cuts = spacing.longarray ? x_cuts : std::min(x_cuts, spacing.cuts);
      int y_cuts = getCuts(height,
                           cut_height,
                           bottom_min_enclosure.getY(),
                           top_min_enclosure.getY(),
                           cut_spacing_y + cut_height,
                           getMaxRows());
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

      // determine how many via row and columns are needed in the last segments
      // of the array if any
      int last_rows = 0;
      int last_cols = 0;
      const int remainder_x = array_area_x - full_arrays_x * array_pitch_x;
      if (remainder_x != 0) {
        last_cols = getCuts(remainder_x,
                            cut_width,
                            0,
                            0,
                            cut_spacing_x + cut_width,
                            getMaxColumns());
      }
      const int remainder_y = array_area_y - full_arrays_y * array_pitch_y;
      if (remainder_y != 0) {
        last_rows = getCuts(remainder_y,
                            cut_height,
                            0,
                            0,
                            cut_spacing_y + cut_height,
                            getMaxRows());
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

        setCutPitchX(cut_spacing_x + cut_width);
        setCutPitchY(cut_spacing_y + cut_height);

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
          bottom_x_enclosure_ = bottom_min_enclosure.getX();
          bottom_y_enclosure_ = bottom_min_enclosure.getY();
        } else {
          bottom_x_enclosure_ = double_enc_x / 2;
          bottom_y_enclosure_ = double_enc_y / 2;
        }
        if (use_top_min_enclosure) {
          top_x_enclosure_ = top_min_enclosure.getX();
          top_y_enclosure_ = top_min_enclosure.getY();
        } else {
          top_x_enclosure_ = double_enc_x / 2;
          top_y_enclosure_ = double_enc_y / 2;
        }

        max_cut_area = total_cut_area;
      }
    }

    used_array = max_cut_area != 0;
  }

  if (!used_array) {
    array_core_x_ = 1;
    array_core_y_ = 1;
    core_col_ = std::max(1, cols);
    core_row_ = std::max(1, rows);
    end_col_ = 0;
    end_row_ = 0;

    const int via_width_x
        = getCutsWidth(core_col_, cut_width, getCutPitchX() - cut_width, 0);
    const int double_enc_x = width - via_width_x;
    const int via_width_y
        = getCutsWidth(core_row_, cut_height, getCutPitchY() - cut_height, 0);
    const int double_enc_y = height - via_width_y;

    if (use_bottom_min_enclosure || isSplitCutArray()) {
      bottom_x_enclosure_ = bottom_min_enclosure.getX();
      bottom_y_enclosure_ = bottom_min_enclosure.getY();
    } else {
      bottom_x_enclosure_ = double_enc_x / 2;
      bottom_y_enclosure_ = double_enc_y / 2;
    }
    if (use_top_min_enclosure || isSplitCutArray()) {
      top_x_enclosure_ = top_min_enclosure.getX();
      top_y_enclosure_ = top_min_enclosure.getY();
    } else {
      top_x_enclosure_ = double_enc_x / 2;
      top_y_enclosure_ = double_enc_y / 2;
    }

    if (isSplitCutArray()) {
      array_core_x_ = std::max(width / getCutPitchX(), 1);
      if (getMaxColumns() != 0) {
        array_core_x_ = std::min(getMaxColumns(), array_core_x_);
      }
      array_core_y_ = std::max(height / getCutPitchY(), 1);
      if (getMaxRows() != 0) {
        array_core_y_ = std::min(getMaxRows(), array_core_y_);
      }
    }
  }

  auto* tech = getTech();
  bottom_x_enclosure_ = TechLayer::snapToManufacturingGrid(tech, bottom_x_enclosure_);
  bottom_y_enclosure_ = TechLayer::snapToManufacturingGrid(tech, bottom_y_enclosure_);
  top_x_enclosure_ = TechLayer::snapToManufacturingGrid(tech, top_x_enclosure_);
  top_y_enclosure_ = TechLayer::snapToManufacturingGrid(tech, top_y_enclosure_);
}

std::vector<odb::dbTechLayerCutEnclosureRule*> ViaGenerator::getCutMinimumEnclosureRules(int width, bool above) const
{
  using CutRules = std::vector<odb::dbTechLayerCutEnclosureRule*>;
  std::map<int, CutRules> rules_map;

  for (auto* enc_rule : getCutLayer()->getTechLayerCutEnclosureRules()) {
    if (!isCutClass(enc_rule->getCutClass())) {
      // cut classes do not match
      continue;
    }

    bool check_top = enc_rule->isAbove();
    bool check_bot = enc_rule->isBelow();
    if (!check_top && !check_bot) {
      check_top = true;
      check_bot = true;
    }

    if ((above && check_top) || (!above && check_bot)) {
      int min_width = 0;
      if (enc_rule->isWidthValid()) {
        min_width = enc_rule->getMinWidth();
      }

      rules_map[min_width].push_back(enc_rule);
    }
  }

  // rules with 0 width apply to all widths, so copy those rules to all other widths
  CutRules* applies_to_all = nullptr;
  auto find_itr = rules_map.find(0);
  if (find_itr != rules_map.end()) {
    applies_to_all = &find_itr->second;
  }
  if (applies_to_all != nullptr) {
    for (auto& [min_width, width_rules] : rules_map) {
      if (min_width == 0) {
        continue;
      }

      width_rules.insert(width_rules.end(), applies_to_all->begin(), applies_to_all->end());
    }
  }

  CutRules* rules = nullptr;
  for (auto& [min_width, width_rules] : rules_map) {
    if (min_width <= width) {
      rules = &width_rules;
    }
  }

  if (rules == nullptr) {
    return {};
  } else {
    return *rules;
  }
}

void ViaGenerator::setSplitCutArray(bool split_cuts_bot, bool split_cuts_top)
{
  split_cuts_bottom_ = split_cuts_bot;
  split_cuts_top_ = split_cuts_top;
}

int ViaGenerator::getRows() const
{
  int rows = 0;

  rows += array_core_y_ * core_row_;
  rows += end_row_;

  return rows;
}

int ViaGenerator::getColumns() const
{
  int columns = 0;

  columns += array_core_x_ * core_col_;
  columns += end_col_;

  return columns;
}

odb::dbTech* ViaGenerator::getTech() const
{
  return getBottomLayer()->getTech();
}

int ViaGenerator::getLowerWidth(bool only_real) const
{
  if (!only_real && isSplitCutArray()) {
    return 0;
  }
  return lower_rect_.minDXDY();
}

int ViaGenerator::getUpperWidth(bool only_real) const
{
  if (!only_real && isSplitCutArray()) {
    return 0;
  }
  return upper_rect_.minDXDY();
}

DbVia* ViaGenerator::generate(odb::dbBlock* block) const
{
  if (isSplitCutArray()) {
    return new DbSplitCutVia(makeBaseVia(getViaCoreRows(),
                                         getCutPitchY(),
                                         getViaCoreColumns(),
                                         getCutPitchX()),
                             getArrayCoresY(),
                             getCutPitchY(),
                             getArrayCoresX(),
                             getCutPitchX(),
                             block,
                             getBottomLayer(),
                             false,
                             getTopLayer(),
                             false);
  } else if (!isCutArray()) {
    return makeBaseVia(
        getViaCoreRows(), getCutPitchY(), getViaCoreColumns(), getCutPitchX());
  } else {
    DbBaseVia* core_via = makeBaseVia(
        getViaCoreRows(), getCutPitchY(), getViaCoreColumns(), getCutPitchX());
    DbBaseVia* end_of_row = nullptr;
    DbBaseVia* end_of_column = nullptr;
    DbBaseVia* end_of_row_column = nullptr;
    const bool need_end_of_row = hasViaLastRows();
    if (need_end_of_row) {
      end_of_row = makeBaseVia(getViaLastRows(),
                               getCutPitchY(),
                               getViaCoreColumns(),
                               getCutPitchX());
    }
    const bool need_end_of_column = hasViaLastColumns();
    if (need_end_of_column) {
      end_of_column = makeBaseVia(getViaCoreRows(),
                                  getCutPitchY(),
                                  getViaLastColumns(),
                                  getCutPitchX());
    }
    if (need_end_of_row || need_end_of_column) {
      const int rows = need_end_of_row ? getViaLastRows() : getViaCoreRows();
      const int cols
          = need_end_of_column ? getViaLastColumns() : getViaCoreColumns();
      end_of_row_column
          = makeBaseVia(rows, getCutPitchY(), cols, getCutPitchX());
    }
    return new DbArrayVia(core_via,
                          end_of_row,
                          end_of_column,
                          end_of_row_column,
                          getArrayCoresY(),
                          getArrayCoresX(),
                          getArraySpacingX(),
                          getArraySpacingY());
  }
}

void ViaGenerator::determineCutSpacing()
{
  const auto cut = getCut();

  auto* cut_layer = getCutLayer();
  int cut_spacing = cut_layer->getSpacing();
  if (cut_spacing != 0) {
    setCutPitchX(cut.dx() + cut_spacing);
    setCutPitchY(cut.dy() + cut_spacing);
  }

  if (hasCutClass()) {
    const std::string cut_class = getCutClass()->getName();

    int max_spacing = 0;
    for (auto* rule : cut_layer->getTechLayerCutSpacingTableDefRules()) {
      if (rule->getSecondLayer() != nullptr) {
        continue;
      }

      // look for the largest / smallest spacing
      int rule_pitch = rule->getMaxSpacing(cut_class, cut_class, odb::dbTechLayerCutSpacingTableDefRule::MIN);
      int rule_spacing = rule_pitch;
      if (rule->isCenterAndEdge(cut_class, cut_class)) {
        // already spacing
      } else if (rule->isCenterToCenter(cut_class, cut_class)) {
        rule_spacing -= cut.dx();
      } else {
        // already spacing
      }

      if (rule->isSameNet()) {
        // use this rule since it's the same net
        max_spacing = rule_spacing;
        break;
      } else {
        // keep looking for spacings
        max_spacing = std::max(max_spacing, rule_spacing);
      }
    }
    if (max_spacing != 0) {
      setCutPitchX(cut.dx() + max_spacing);
      setCutPitchY(cut.dy() + max_spacing);
    }
  }
}

/////////////

GenerateViaGenerator::GenerateViaGenerator(utl::Logger* logger,
                                           odb::dbTechViaGenerateRule* rule,
                                           const odb::Rect& lower_rect,
                                           const odb::Rect& upper_rect)
    : ViaGenerator(logger, lower_rect, upper_rect), rule_(rule)
{
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
  odb::Rect rect;
  if (cut_rule->hasRect()) {
    cut_rule->getRect(rect);
  }
  setCut(rect);

  determineCutSpacing();

  if (cut_rule->hasSpacing() && (getCutPitchX() == 0 || getCutPitchY() == 0)) {
    int cut_x, cut_y;
    cut_rule->getSpacing(cut_x, cut_y);
    setCutPitchX(cut_x);
    setCutPitchY(cut_y);
  }
}

const std::string GenerateViaGenerator::getName() const
{
  const std::string seperator = "_";
  std::string name = "via";
  // name after layers connected
  name += std::to_string(getBottomLayer()->getRoutingLevel());
  name += seperator;
  name += std::to_string(getTopLayer()->getRoutingLevel());
  // name after area
  name += seperator;
  const auto& intersection = getIntersectionRect();
  name += std::to_string(intersection.dx());
  name += seperator;
  name += std::to_string(intersection.dy());
  // name after rows and columns
  name += seperator;
  name += std::to_string(getRows());
  name += seperator;
  name += std::to_string(getColumns());
  // name after pitch
  name += seperator;
  name += std::to_string(getCutPitchX());
  name += seperator;
  name += std::to_string(getCutPitchY());

  return name;
}

const std::string GenerateViaGenerator::getRuleName() const
{
  return rule_->getName();
}

odb::dbTechViaLayerRule* GenerateViaGenerator::getBottomLayerRule() const
{
  return rule_->getViaLayerRule(layers_[0]);
}

odb::dbTechLayer* GenerateViaGenerator::getBottomLayer() const
{
  return getBottomLayerRule()->getLayer();
}

odb::dbTechViaLayerRule* GenerateViaGenerator::getCutLayerRule() const
{
  return rule_->getViaLayerRule(layers_[1]);
}

odb::dbTechLayer* GenerateViaGenerator::getCutLayer() const
{
  return getCutLayerRule()->getLayer();
}

odb::dbTechViaLayerRule* GenerateViaGenerator::getTopLayerRule() const
{
  return rule_->getViaLayerRule(layers_[2]);
}

odb::dbTechLayer* GenerateViaGenerator::getTopLayer() const
{
  return getTopLayerRule()->getLayer();
}

bool GenerateViaGenerator::isBottomValidForWidth(int width) const
{
  return isLayerValidForWidth(getBottomLayerRule(), width);
}

bool GenerateViaGenerator::isTopValidForWidth(int width) const
{
  return isLayerValidForWidth(getTopLayerRule(), width);
}

bool GenerateViaGenerator::isLayerValidForWidth(odb::dbTechViaLayerRule* rule,
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

bool GenerateViaGenerator::getLayerEnclosureRule(odb::dbTechViaLayerRule* rule,
                                                 int& dx,
                                                 int& dy) const
{
  if (rule->hasEnclosure()) {
    rule->getEnclosure(dx, dy);
  } else {
    return false;
  }

  return true;
}

DbBaseVia* GenerateViaGenerator::makeBaseVia(int rows,
                                             int row_pitch,
                                             int cols,
                                             int col_pitch) const
{
  return new DbGenerateVia(getIntersectionRect(),
                           rule_,
                           rows,
                           cols,
                           col_pitch,
                           row_pitch,
                           getBottomEnclosureX(),
                           getBottomEnclosureY(),
                           getTopEnclosureX(),
                           getTopEnclosureY(),
                           getBottomLayer(),
                           getCutLayer(),
                           getTopLayer());
}

bool GenerateViaGenerator::isSetupValid(odb::dbTechLayer* lower,
                                        odb::dbTechLayer* upper) const
{
  if (!ViaGenerator::isSetupValid(lower, upper)) {
    return false;
  }

  return isBottomValidForWidth(getLowerWidth())
         && isTopValidForWidth(getUpperWidth());
}

/////////

TechViaGenerator::TechViaGenerator(utl::Logger* logger,
                                   odb::dbTechVia* via,
                                   const odb::Rect& lower_rect,
                                   const odb::Rect& upper_rect)
    : ViaGenerator(logger, lower_rect, upper_rect),
      via_(via),
      cuts_(0),
      bottom_(via_->getBottomLayer()),
      cut_(nullptr),
      top_(via_->getTopLayer())
{
  for (auto* box : via_->getBoxes()) {
    auto* layer = box->getTechLayer();
    if (layer == bottom_ || layer == top_ || layer == nullptr) {
      continue;
    }

    if (cuts_ == 0) {
      cut_ = layer;

      odb::Rect cut;
      box->getBox(cut);
      setCut(cut);
    }

    cuts_++;
  }

  determineCutSpacing();
}

int TechViaGenerator::getCutArea() const
{
  return ViaGenerator::getCutArea() * cuts_;
}

DbBaseVia* TechViaGenerator::makeBaseVia(int rows,
                                         int row_pitch,
                                         int cols,
                                         int col_pitch) const
{
  return new DbTechVia(via_, rows, row_pitch, cols, col_pitch);
}

bool TechViaGenerator::isSetupValid(odb::dbTechLayer* lower,
                                    odb::dbTechLayer* upper) const
{
  if (!ViaGenerator::isSetupValid(lower, upper)) {
    return false;
  }

  return fitsShapes();
}

bool TechViaGenerator::fitsShapes() const
{
  const auto& lower_rect = getLowerRect();
  const auto& upper_rect = getUpperRect();
  const auto& intersection = getIntersectionRect();
  odb::dbTransform transform(
      odb::Point(0.5 * (intersection.xMin() + intersection.xMax()),
                 0.5 * (intersection.yMin() + intersection.yMax())));

  const DbTechVia via(via_, 1, 0, 1, 0);
  odb::Rect bottom_rect = via.getViaRect(true, true, false);
  odb::Rect top_rect = via.getViaRect(true, false, true);

  transform.apply(bottom_rect);
  if (!mostlyContains(lower_rect, bottom_rect)) {
    return false;
  }

  transform.apply(top_rect);
  if (!mostlyContains(upper_rect, top_rect)) {
    return false;
  }

  return true;
}

// check if shape is contains on three sides
bool TechViaGenerator::mostlyContains(const odb::Rect& full_shape,
                                      const odb::Rect& small_shape) const
{
  const bool inside_top = full_shape.yMax() >= small_shape.yMax();
  const bool inside_right = full_shape.xMax() >= small_shape.xMax();
  const bool inside_bottom = full_shape.yMin() <= small_shape.yMin();
  const bool inside_left = full_shape.xMin() <= small_shape.xMin();

  int contains = 0;
  if (inside_top) {
    contains++;
  }
  if (inside_right) {
    contains++;
  }
  if (inside_bottom) {
    contains++;
  }
  if (inside_left) {
    contains++;
  }

  return contains > 2;
}

void TechViaGenerator::getMinimumEnclosures(std::vector<Enclosure>& bottom, std::vector<Enclosure>& top) const
{
  ViaGenerator::getMinimumEnclosures(bottom, top);

  const DbTechVia via(via_, 1, 0, 1, 0);

  const odb::Rect via_rect = via.getViaRect(false);
  const odb::Rect enc_bottom_rect = via.getViaRect(true, true, false);
  const odb::Rect enc_top_rect = via.getViaRect(true, false, true);

  debugPrint(getLogger(),
             utl::PDN,
             "ViaEnclosure",
             3,
             "Tech via: {}, via rect {}, enc bottom rect {}, enc top rect {}",
             via_->getName(),
             Shape::getRectText(via_rect, getTech()->getLefUnits()),
             Shape::getRectText(enc_bottom_rect, getTech()->getLefUnits()),
             Shape::getRectText(enc_top_rect, getTech()->getLefUnits()));

  const bool use_via = !via_rect.isInverted() && !enc_bottom_rect.isInverted() && !enc_top_rect.isInverted();

  int odx = 0;
  int ody = 0;
  if (use_via) {
    odx = std::max(via_rect.xMin() - enc_bottom_rect.xMin(),
                   enc_bottom_rect.xMax() - via_rect.xMax());
    ody = std::max(via_rect.yMin() - enc_bottom_rect.yMin(),
                   enc_bottom_rect.yMax() - via_rect.yMax());
  }
  // remove rules that do not fit the tech vias enclosures.
  bottom.erase(std::remove_if(bottom.begin(), bottom.end(), [&] (const auto& enc) {
    return enc.getX() < odx && enc.getY() < ody;
  }), bottom.end());
  // "fix" rules to use the tech via enclosure
  for (auto& enc : bottom) {
    if (enc.getX() < odx) {
      enc.setX(odx);
    }
    if (enc.getY() < ody) {
      enc.setY(ody);
    }
  }
  bottom.emplace_back(odx, ody, getBottomLayer());

  odx = 0;
  ody = 0;
  if (use_via) {
    odx = std::max(via_rect.xMin() - enc_top_rect.xMin(),
                   enc_top_rect.xMax() - via_rect.xMax());
    ody = std::max(via_rect.yMin() - enc_top_rect.yMin(),
                   enc_top_rect.yMax() - via_rect.yMax());
  }
  // remove rules that do not fit the tech vias enclosures.
  top.erase(std::remove_if(top.begin(), top.end(), [&] (const auto& enc) {
    return enc.getX() < odx && enc.getY() < ody;
  }), top.end());
  // "fix" rules to use the tech via enclosure
  for (auto& enc : top) {
    if (enc.getX() < odx) {
      enc.setX(odx);
    }
    if (enc.getY() < ody) {
      enc.setY(ody);
    }
  }
  top.emplace_back(odx, ody, getTopLayer());
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

  DbVia::ViaLayerShape shapes;
  connect_->makeVia(wire, lower_->getRect(), upper_->getRect(), type, shapes);

  auto check_shapes = [this](const ShapePtr& shape, const std::set<odb::Rect>& via_shapes) {
    const odb::Rect& rect = shape->getRect();
    odb::Rect new_shape = rect;
    for (const auto& via_shape : via_shapes) {
      new_shape.merge(via_shape);
    }
    if (new_shape != rect) {
      auto* layer = shape->getLayer();

      debugPrint(getLogger(),
                 utl::PDN,
                 "Via",
                 3,
                 "{} shape changed {}: {} -> {}",
                 shape->getNet()->getName(),
                 layer->getName(),
                 Shape::getRectText(shape->getRect(), layer->getTech()->getLefUnits()),
                 Shape::getRectText(new_shape, layer->getTech()->getLefUnits()));
      bool valid_change = shape->isModifiable();
      if (layer->getDirection() == odb::dbTechLayerDir::HORIZONTAL) {
        if (new_shape.yMin() != rect.yMin() || new_shape.yMax() != rect.yMax()) {
          valid_change = false;
        }
      } else {
        if (new_shape.xMin() != rect.xMin() || new_shape.xMax() != rect.xMax()) {
          valid_change = false;
        }
      }

      if (valid_change) {
        shape->setRect(new_shape);
      } else {
        getLogger()->warn(utl::PDN,
                          195,
                          "{} shape change required in a non-preferred direction to fit via {}: {} -> {}",
                          shape->getNet()->getName(),
                          layer->getName(),
                          Shape::getRectText(shape->getRect(), layer->getTech()->getLefUnits()),
                          Shape::getRectText(new_shape, layer->getTech()->getLefUnits()));
      }
    }

  };

  check_shapes(lower_, shapes.bottom);
  check_shapes(upper_, shapes.top);
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

utl::Logger* Via::getLogger() const
{
  return getGrid()->getLogger();
}

}  // namespace pdn
