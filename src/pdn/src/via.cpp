// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "via.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boost/geometry/geometry.hpp"
#include "boost/polygon/polygon.hpp"
#include "connect.h"
#include "grid.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "shape.h"
#include "techlayer.h"
#include "utl/Logger.h"

namespace pdn {

Enclosure::Enclosure() : x_(0), y_(0), allow_swap_(false)
{
}

Enclosure::Enclosure(int x, int y) : x_(x), y_(y), allow_swap_(false)
{
}

Enclosure::Enclosure(odb::dbTechLayerCutEnclosureRule* rule,
                     odb::dbTechLayer* layer,
                     const odb::Rect& cut,
                     const odb::dbTechLayerDir& direction)
    : x_(0), y_(0), allow_swap_(true)
{
  switch (rule->getType()) {
    case odb::dbTechLayerCutEnclosureRule::DEFAULT:
      x_ = rule->getFirstOverhang();
      y_ = rule->getSecondOverhang();
      swap(layer);
      break;
    case odb::dbTechLayerCutEnclosureRule::EOL:
      switch (direction.getValue()) {
        case odb::dbTechLayerDir::HORIZONTAL:
          x_ = rule->getFirstOverhang();
          y_ = rule->getSecondOverhang();
          break;
        case odb::dbTechLayerDir::VERTICAL:
          x_ = rule->getSecondOverhang();
          y_ = rule->getFirstOverhang();
          break;
        case odb::dbTechLayerDir::NONE:
          x_ = std::max(rule->getFirstOverhang(), rule->getSecondOverhang());
          y_ = x_;
          break;
      }
      allow_swap_ = false;
      break;
    case odb::dbTechLayerCutEnclosureRule::ENDSIDE:
      if (cut.dx() < cut.dy()) {
        // SIDE is first overhang, END is second
        x_ = rule->getSecondOverhang();
        y_ = rule->getFirstOverhang();
      } else {
        x_ = rule->getFirstOverhang();
        y_ = rule->getSecondOverhang();
      }
      allow_swap_ = false;
      break;
    case odb::dbTechLayerCutEnclosureRule::HORZ_AND_VERT:
      x_ = rule->getFirstOverhang();
      y_ = rule->getSecondOverhang();
      allow_swap_ = false;
      break;
  }
}

Enclosure::Enclosure(odb::dbTechViaLayerRule* rule, odb::dbTechLayer* layer)
    : x_(0), y_(0), allow_swap_(true)
{
  rule->getEnclosure(x_, y_);
  swap(layer);
}

bool Enclosure::check(int x, int y) const
{
  if (allow_swap_) {
    return (x_ <= x && y_ <= y) || (x_ <= y && y_ <= x);
  }

  return x_ <= x && y_ <= y;
}

void Enclosure::swap(odb::dbTechLayer* layer)
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

bool Enclosure::operator<(const Enclosure& other) const
{
  return std::tie(x_, y_) < std::tie(other.x_, other.y_);
}

bool Enclosure::operator==(const Enclosure& other) const
{
  return std::tie(x_, y_) == std::tie(other.x_, other.y_);
}

bool Enclosure::isPreferredOver(const Enclosure* other,
                                odb::dbTechLayer* layer) const
{
  return isPreferredOver(
      other, layer->getDirection() != odb::dbTechLayerDir::HORIZONTAL);
}

bool Enclosure::isPreferredOver(const Enclosure* other, bool minimize_x) const
{
  if (other == nullptr) {
    return true;
  }

  if (minimize_x) {
    if (x_ == other->x_) {
      return y_ < other->y_;
    }
    return x_ < other->x_;
  }

  if (y_ == other->y_) {
    return x_ < other->x_;
  }
  return y_ < other->y_;
}

void Enclosure::snap(odb::dbTech* tech)
{
  x_ = TechLayer::snapToManufacturingGrid(tech, x_);
  y_ = TechLayer::snapToManufacturingGrid(tech, y_);
}

void Enclosure::copy(const Enclosure* other)
{
  x_ = other->x_;
  y_ = other->y_;
  allow_swap_ = other->allow_swap_;
}

void Enclosure::copy(const Enclosure& other)
{
  copy(&other);
}

//////////

DbVia::DbVia() : generator_(nullptr)
{
}

DbVia::ViaLayerShape DbVia::getLayerShapes(odb::dbSBox* box) const
{
  std::vector<odb::dbShape> shapes;
  box->getViaBoxes(shapes);

  std::map<int, std::set<ViaLayerShape::RectBoxPair>> layer_rects;
  for (auto& shape : shapes) {
    auto* layer = shape.getTechLayer();
    if (layer->getType() == odb::dbTechLayerType::ROUTING) {
      odb::Rect box_shape = shape.getBox();
      layer_rects[layer->getRoutingLevel()].insert({box_shape, box});
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
  shapes.middle.insert(other.middle.begin(), other.middle.end());
  shapes.top.insert(other.top.begin(), other.top.end());
}

odb::Rect DbVia::adjustToMinArea(odb::dbTechLayer* layer,
                                 const odb::Rect& rect) const
{
  const TechLayer techlayer(layer);
  return techlayer.adjustToMinArea(rect);
}

void DbVia::addToViaReport(DbVia* via, ViaReport& report) const
{
  if (via == nullptr) {
    return;
  }

  for (const auto& [name, count] : via->getViaReport()) {
    report[name] += count;
  }
}

////////////

ViaReport DbBaseVia::getViaReport() const
{
  return {{getName(), count_}};
}

////////////

DbTechVia::DbTechVia(odb::dbTechVia* via,
                     int rows,
                     int row_pitch,
                     int cols,
                     int col_pitch,
                     Enclosure* required_bottom_enc,
                     Enclosure* required_top_enc)
    : via_(via),
      cut_layer_(nullptr),
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

    const odb::Rect rect = box->getBox();

    if (layer->getType() == odb::dbTechLayerType::CUT) {
      cut_layer_ = layer;
      single_via_rect_ = rect;
      via_rect_.merge(rect);
      via_centers_.insert(rect.center());
    } else {
      if (layer == via->getBottomLayer()) {
        enc_bottom_rect_.merge(rect);
      } else {
        enc_top_rect_.merge(rect);
      }
    }
  }

  if (required_bottom_enc != nullptr) {
    required_bottom_rect_
        = odb::Rect(via_rect_.xMin() - required_bottom_enc->getX(),
                    via_rect_.yMin() - required_bottom_enc->getY(),
                    via_rect_.xMax() + required_bottom_enc->getX(),
                    via_rect_.yMax() + required_bottom_enc->getY());
  } else {
    required_bottom_rect_ = enc_bottom_rect_;
  }
  if (required_top_enc != nullptr) {
    required_top_rect_ = odb::Rect(via_rect_.xMin() - required_top_enc->getX(),
                                   via_rect_.yMin() - required_top_enc->getY(),
                                   via_rect_.xMax() + required_top_enc->getX(),
                                   via_rect_.yMax() + required_top_enc->getY());
  } else {
    required_top_rect_ = enc_top_rect_;
  }

  via_center_.setX(via_rect_.xMin() + via_rect_.dx() / 2);
  via_center_.setY(via_rect_.yMin() + via_rect_.dy() / 2);

  // check if multicut can be simplified
  if (isArray() && via_centers_.size() > 1) {
    // determine number of rows and columns in via
    std::set<int> xs, ys;
    for (const auto& pt : via_centers_) {
      xs.insert(pt.x());
      ys.insert(pt.y());
    }
    const int via_rows = ys.size();
    const int via_cols = xs.size();

    // determine if pitch matches
    bool pitch_match = true;
    const int x0 = *xs.begin();
    for (const int x1 : xs) {
      if ((via_cols * std::abs(x1 - x0)) % col_pitch_ != 0) {
        pitch_match = false;
        break;
      }
    }
    if (pitch_match) {
      const int y0 = *ys.begin();
      for (const int y1 : ys) {
        if ((via_rows * std::abs(y1 - y0)) % row_pitch_ != 0) {
          pitch_match = false;
          break;
        }
      }
    }

    if (pitch_match) {
      // adjust rows and cols
      rows_ *= via_rows;
      cols_ *= via_cols;

      // adjust pitch
      row_pitch_ /= via_rows;
      col_pitch_ /= via_cols;

      // adjust via rect
      const odb::Rect org_via_rect = via_rect_;
      via_rect_ = single_via_rect_;
      via_rect_.moveTo(via_center_.x() - single_via_rect_.dx() / 2,
                       via_center_.y() - single_via_rect_.dy() / 2);

      // adjust via centers
      via_centers_.clear();
      via_centers_.insert(via_center_);

      auto adjust_enclosure
          = [this, &org_via_rect](const odb::Rect& enclosure) -> odb::Rect {
        const int x0 = org_via_rect.xMin() - enclosure.xMin();
        const int y0 = org_via_rect.yMin() - enclosure.yMin();
        const int x1 = enclosure.xMax() - org_via_rect.xMax();
        const int y1 = enclosure.yMax() - org_via_rect.yMax();

        return odb::Rect(via_rect_.xMin() - x0,
                         via_rect_.yMin() - y0,
                         via_rect_.xMax() + x1,
                         via_rect_.yMax() + y1);
      };

      required_top_rect_ = adjust_enclosure(required_top_rect_);
      required_bottom_rect_ = adjust_enclosure(required_bottom_rect_);
    }
  }
}

std::string DbTechVia::getName() const
{
  return via_->getName();
}

DbVia::ViaLayerShape DbTechVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType type,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
{
  TechLayer bottom(via_->getBottomLayer());
  TechLayer top(via_->getTopLayer());

  bool do_bottom_snap = false;
  if (ongrid.find(bottom.getLayer()) != ongrid.end()) {
    bottom.populateGrid(block);
    do_bottom_snap = true;
  }

  bool do_top_snap = false;
  if (ongrid.find(top.getLayer()) != ongrid.end()) {
    top.populateGrid(block);
    do_top_snap = true;
  }

  TechLayer* row_snap = &top;
  TechLayer* col_snap = &bottom;
  bool* do_row_snap = &do_top_snap;
  bool* do_col_snap = &do_bottom_snap;
  if (top.getLayer()->getDirection() == odb::dbTechLayerDir::VERTICAL) {
    std::swap(row_snap, col_snap);
    std::swap(do_row_snap, do_col_snap);
  }

  odb::Point new_via_center;
  ViaLayerShape via_shapes;

  auto add_via = [this, &via_shapes](odb::dbSBox* via,
                                     const odb::Point& center) {
    ViaLayerShape new_via_shapes = getLayerShapes(via);

    via_shapes.bottom.insert(new_via_shapes.bottom.begin(),
                             new_via_shapes.bottom.end());
    via_shapes.middle.insert(new_via_shapes.middle.begin(),
                             new_via_shapes.middle.end());
    via_shapes.top.insert(new_via_shapes.top.begin(), new_via_shapes.top.end());

    const odb::dbTransform xfm(center);
    odb::Rect top_shape = required_top_rect_;
    xfm.apply(top_shape);
    via_shapes.top.insert({top_shape, via});
    odb::Rect bottom_shape = required_bottom_rect_;
    xfm.apply(bottom_shape);
    via_shapes.bottom.insert({bottom_shape, via});
  };

  if (isArray()) {
    const std::string via_name = getViaName(ongrid);
    auto* bvia = block->findVia(via_name.c_str());

    new_via_center
        = odb::Point(col_snap->snapToGrid(x), row_snap->snapToGrid(y));

    if (bvia == nullptr) {
      const int cut_width = single_via_rect_.dx();
      const int cut_height = single_via_rect_.dy();

      bvia = odb::dbVia::create(block, via_name.c_str());

      odb::dbViaParams params = bvia->getViaParams();

      params.setBottomLayer(via_->getBottomLayer());
      params.setCutLayer(cut_layer_);
      params.setTopLayer(via_->getTopLayer());

      params.setXCutSize(cut_width);
      params.setYCutSize(cut_height);

      // snap to track intervals
      int x_pitch = col_pitch_;
      if (*do_col_snap) {
        x_pitch = col_snap->snapToGridInterval(block, col_pitch_);
      }
      int y_pitch = row_pitch_;
      if (*do_row_snap) {
        y_pitch = row_snap->snapToGridInterval(block, row_pitch_);
      }

      params.setXCutSpacing(x_pitch - cut_width);
      params.setYCutSpacing(y_pitch - cut_height);

      params.setXBottomEnclosure(
          std::min(via_rect_.xMin() - required_bottom_rect_.xMin(),
                   required_bottom_rect_.xMax() - via_rect_.xMax()));
      params.setYBottomEnclosure(
          std::min(via_rect_.yMin() - required_bottom_rect_.yMin(),
                   required_bottom_rect_.yMax() - via_rect_.yMax()));
      params.setXTopEnclosure(
          std::min(via_rect_.xMin() - required_top_rect_.xMin(),
                   required_top_rect_.xMax() - via_rect_.xMax()));
      params.setYTopEnclosure(
          std::min(via_rect_.yMin() - required_top_rect_.yMin(),
                   required_top_rect_.yMax() - via_rect_.yMax()));

      params.setNumCutRows(rows_);
      params.setNumCutCols(cols_);

      params.setXOrigin(via_center_.x());
      params.setYOrigin(via_center_.y());

      bvia->setViaParams(params);
    }

    for (const odb::Point& pt : via_centers_) {
      const odb::Point via_center(
          odb::Point(col_snap->snapToGrid(new_via_center.x() - pt.x()),
                     row_snap->snapToGrid(new_via_center.y() - pt.y())));

      incrementCount(rows_ * cols_);
      odb::dbSBox* via = odb::dbSBox::create(
          wire, bvia, via_center.x(), via_center.y(), type);
      add_via(via, via_center);
    }
  } else {
    incrementCount();
    new_via_center = odb::Point(col_snap->snapToGrid(x - via_center_.getX()),
                                row_snap->snapToGrid(y - via_center_.getY()));
    odb::dbSBox* via = odb::dbSBox::create(
        wire, via_, new_via_center.x(), new_via_center.y(), type);
    add_via(via, new_via_center);
  }

  return via_shapes;
}

odb::Rect DbTechVia::getViaRect(bool include_enclosure,
                                bool include_via_shape,
                                bool include_bottom,
                                bool include_top) const
{
  if (include_enclosure) {
    odb::Rect enc;
    enc.mergeInit();
    if (include_via_shape) {
      enc.merge(via_rect_);
    }
    if (include_bottom) {
      enc.merge(enc_bottom_rect_);
    }
    if (include_top) {
      enc.merge(enc_top_rect_);
    }
    return enc;
  }
  return via_rect_;
}

std::string DbTechVia::getViaName(
    const std::set<odb::dbTechLayer*>& ongrid) const
{
  const std::string seperator = "_";
  std::string name = via_->getName();
  // name after rows and columns
  name += seperator;
  name += std::to_string(rows_);
  name += seperator;
  name += std::to_string(cols_);
  // name after pitch
  name += seperator;
  name += std::to_string(row_pitch_);
  name += seperator;
  name += std::to_string(col_pitch_);
  // name on grid layers
  for (auto* layer : ongrid) {
    name += seperator;
    name += layer->getName();
  }

  return name;
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
  for (uint32_t l = 0; l < rule_->getViaLayerRuleCount(); l++) {
    auto* layer_rule = rule_->getViaLayerRule(l);
    if (layer_rule->getLayer() == cut_) {
      layer_rule->getRect(cut_rect_);
      break;
    }
  }
}

std::string DbGenerateVia::getName() const
{
  return rule_->getName();
}

std::string DbGenerateVia::getViaName() const
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

odb::Rect DbGenerateVia::getViaRect(bool include_enclosure,
                                    bool /* include_via_shape */,
                                    bool /* include_bottom */,
                                    bool /* include_top */) const
{
  const int y_enc = std::max(bottom_enclosure_y_, top_enclosure_y_);
  const int height = (rows_ - 1) * cut_pitch_y_ + cut_rect_.dy();
  const int x_enc = std::max(bottom_enclosure_x_, top_enclosure_x_);
  const int width = (columns_ - 1) * cut_pitch_x_ + cut_rect_.dx();

  const int height_enc = include_enclosure ? y_enc : 0;
  const int width_enc = include_enclosure ? x_enc : 0;

  const int height_half = height / 2;
  const int width_half = width / 2;

  return {-width_half - width_enc,
          -height_half - height_enc,
          width_half + width_enc,
          height_half + height_enc};
}

DbVia::ViaLayerShape DbGenerateVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType type,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
{
  const std::string via_name = getViaName();
  auto* via = block->findVia(via_name.c_str());

  if (via == nullptr) {
    const int cut_width = cut_rect_.dx();
    const int cut_height = cut_rect_.dy();
    via = odb::dbVia::create(block, via_name.c_str());

    via->setViaGenerateRule(rule_);

    odb::dbViaParams params = via->getViaParams();

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

  incrementCount();
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
      array_spacing_y_(array_spacing_y)
{
  if (end_of_row != nullptr) {
    rows_++;
  }
  if (end_of_column != nullptr) {
    columns_++;
  }

  const odb::Rect core_via_rect = core_via->getViaRect(false, true);

  // determine the via array offset from the center

  int total_width
      = (columns_ - 1)
        * (array_spacing_x_ + core_via_rect.dx());  // all spacing + core vias
  int x_offset = 0;
  if (end_of_column != nullptr) {
    const odb::Rect end_rect = end_of_column_->getViaRect(false, true);
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
    const odb::Rect end_rect = end_of_row_->getViaRect(false, true);
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

DbVia::ViaLayerShape DbArrayVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType type,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
{
  const odb::Rect core_via_rect = core_via_->getViaRect(false, true);
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

      auto shapes
          = via->generate(block, wire, type, array_x, array_y, ongrid, logger);
      combineLayerShapes(shapes, via_shapes);

      last_via_rect = via->getViaRect(false, true);

      array_x
          += (core_via_rect.dx() + last_via_rect.dx()) / 2 + array_spacing_x_;
    }
    array_y += (core_via_rect.dy() + last_via_rect.dy()) / 2 + array_spacing_y_;
  }
  return via_shapes;
}

ViaReport DbArrayVia::getViaReport() const
{
  ViaReport report;

  addToViaReport(core_via_.get(), report);
  addToViaReport(end_of_row_column_.get(), report);
  addToViaReport(end_of_row_.get(), report);
  addToViaReport(end_of_row_column_.get(), report);

  return report;
}

/////////////

DbSplitCutVia::DbSplitCutVia(DbBaseVia* via,
                             int rows,
                             int row_pitch,
                             int row_offset,
                             int cols,
                             int col_pitch,
                             int col_offset,
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
      row_offset_(row_offset),
      cols_(cols),
      col_pitch_(col_pitch),
      col_offset_(col_offset)
{
  if (snap_bottom) {
    bottom_->populateGrid(block);
  }
  if (snap_top) {
    top_->populateGrid(block);
  }
}

DbVia::ViaLayerShape DbSplitCutVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType type,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
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

  const int row_offset = rows_ > 1 ? row_offset_ : 0;
  const int col_offset = cols_ > 1 ? col_offset_ : 0;

  int row = via_rect.yMin() + row_offset;
  for (int r = 0; r < rows_; r++) {
    const int row_pos = horizontal->snapToGrid(row);

    int col = via_rect.xMin() + col_offset;
    for (int c = 0; c < cols_; c++) {
      const int col_pos = vertical->snapToGrid(col);

      auto shapes
          = via_->generate(block, wire, type, col_pos, row_pos, ongrid, logger);
      combineLayerShapes(shapes, via_shapes);

      col = col_pos + col_pitch_;
    }

    row = row_pos + row_pitch_;
  }

  return via_shapes;
}

ViaReport DbSplitCutVia::getViaReport() const
{
  ViaReport report;

  addToViaReport(via_.get(), report);

  return report;
}

/////////////

DbGenerateStackedVia::DbGenerateStackedVia(const std::vector<DbVia*>& vias,
                                           odb::dbTechLayer* bottom,
                                           odb::dbBlock* block)
{
  for (auto* via : vias) {
    vias_.push_back(std::unique_ptr<DbVia>(via));
  }

  int bottom_layer = bottom->getRoutingLevel();
  auto* tech = bottom->getTech();
  for (int i = 0; i < vias.size() + 1; i++) {
    auto layer
        = std::make_unique<TechLayer>(tech->findRoutingLayer(bottom_layer + i));
    layers_.push_back(std::move(layer));
  }
}

DbVia::ViaLayerShape DbGenerateStackedVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType type,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
{
  for (const auto& layer : layers_) {
    if (ongrid.find(layer->getLayer()) != ongrid.end()) {
      layer->populateGrid(block);
    }
  }

  using boost::polygon::operators::operator+=;
  using boost::polygon::operators::operator+;
  using boost::polygon::operators::operator^;
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

    auto shapes
        = via->generate(block, wire, type, layer_x, layer_y, ongrid, logger);
    if (i == 0) {
      via_shapes.bottom = shapes.bottom;
    }
    // Copy top shapes into middle
    via_shapes.middle.insert(via_shapes.top.begin(), via_shapes.top.end());
    via_shapes.top = shapes.top;

    Polygon90Set patch_shapes;
    Polygon90Set total_shape;
    odb::dbTechLayer* add_to_layer = layer_lower->getLayer();
    if (prev_via != nullptr) {
      Polygon90Set bottom_of_current;
      for (const auto& [shape, box] : shapes.bottom) {
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
      total_shape = patch_shapes + combine_layer;
    }

    if (!patch_shapes.empty()) {
      if (via->hasGenerator()) {
        Rectangle complete_shape;
        extents(complete_shape, total_shape);
        const odb::Rect patch_shape_rect(xl(complete_shape),
                                         yl(complete_shape),
                                         xh(complete_shape),
                                         yh(complete_shape));

        auto* generator = via->getGenerator();
        if (!generator->recheckConstraints(patch_shape_rect, true)) {
          // failed recheck, need to ripup entire stack
          std::set<odb::dbSBox*> shapes;
          for (const auto& [rect, box] : via_shapes.bottom) {
            shapes.insert(box);
          }
          for (const auto& [rect, box] : via_shapes.middle) {
            shapes.insert(box);
          }
          for (const auto& [rect, box] : via_shapes.top) {
            shapes.insert(box);
          }
          for (auto* box : shapes) {
            odb::dbSBox::destroy(box);
          }
          const TechLayer tech_layer(*layers_.front());
          logger->warn(
              utl::PDN,
              227,
              "Removing between {} and {} at ({:.4f} um, {:.4f} um) for {}",
              layers_.front()->getName(),
              layers_.back()->getName(),
              tech_layer.dbuToMicron(layer_x),
              tech_layer.dbuToMicron(layer_y),
              wire->getNet()->getName());

          return {};
        }
      }

      std::vector<Rectangle> patches;
      patch_shapes.get_rectangles(patches);
      for (const auto& patch : patches) {
        // add patch metal on layers between the bottom and top of the via stack
        const odb::Rect patch_rect(xl(patch), yl(patch), xh(patch), yh(patch));
        auto* patch_box = odb::dbSBox::create(wire,
                                              add_to_layer,
                                              patch_rect.xMin(),
                                              patch_rect.yMin(),
                                              patch_rect.xMax(),
                                              patch_rect.yMax(),
                                              odb::dbWireShapeType::DRCFILL);
        via_shapes.middle.insert({patch_rect, patch_box});
      }
    }

    prev_via = via.get();
    top_of_previous.clear();
    for (const auto& [shape, box] : shapes.top) {
      top_of_previous += rect_to_poly(shape);
    }
  }

  return via_shapes;
}

ViaReport DbGenerateStackedVia::getViaReport() const
{
  ViaReport report;

  for (const auto& via : vias_) {
    addToViaReport(via.get(), report);
  }

  return report;
}

/////////////

DbGenerateDummyVia::DbGenerateDummyVia(Connect* connect,
                                       const odb::Rect& shape,
                                       odb::dbTechLayer* bottom,
                                       odb::dbTechLayer* top,
                                       bool add_report,
                                       const std::string& reason)
    : connect_(connect),
      add_report_(add_report),
      shape_(shape),
      bottom_(bottom),
      top_(top),
      reason_(reason)
{
}

DbVia::ViaLayerShape DbGenerateDummyVia::generate(
    odb::dbBlock* block,
    odb::dbSWire* wire,
    odb::dbWireShapeType /* type */,
    int x,
    int y,
    const std::set<odb::dbTechLayer*>& ongrid,
    utl::Logger* logger)
{
  odb::dbTransform xfm({x, y});

  odb::Rect via_area = shape_;
  xfm.apply(via_area);
  logger->warn(utl::PDN,
               110,
               "No via inserted between {} and {} at {} on {}{}{}",
               bottom_->getName(),
               top_->getName(),
               Shape::getRectText(via_area, block->getDbUnitsPerMicron()),
               wire->getNet()->getName(),
               reason_.empty() ? "" : ": ",
               reason_);
  if (add_report_) {
    connect_->addFailedVia(failedViaReason::BUILD, via_area, wire->getNet());
  }

  return {};
}

/////////////

ViaGenerator::ViaGenerator(utl::Logger* logger,
                           const odb::Rect& lower_rect,
                           const Constraint& lower_constraint,
                           const odb::Rect& upper_rect,
                           const Constraint& upper_constraint)
    : logger_(logger),
      lower_rect_(lower_rect),
      upper_rect_(upper_rect),
      lower_constraint_(lower_constraint),
      upper_constraint_(upper_constraint),
      bottom_enclosure_(new Enclosure()),
      top_enclosure_(new Enclosure())
{
  lower_rect_.intersection(upper_rect_, intersection_rect_);
}

int ViaGenerator::getGeneratorWidth(bool bottom) const
{
  Enclosure* enc = bottom ? bottom_enclosure_.get() : top_enclosure_.get();
  const odb::Rect cut = getCut();

  const int core_width = getCutsWidth(
      core_col_, cut.dx(), getCutPitchX() - cut.dx(), enc->getX());

  if (isCutArray()) {
    const int end_width = getCutsWidth(
        end_col_, cut.dx(), getCutPitchX() - cut.dx(), enc->getX());
    return array_core_y_ * core_width + (array_core_x_ - 1) * array_spacing_x_
           + end_width;
  }
  return core_width;
}

int ViaGenerator::getGeneratorHeight(bool bottom) const
{
  Enclosure* enc = bottom ? bottom_enclosure_.get() : top_enclosure_.get();
  const odb::Rect cut = getCut();

  const int core_height = getCutsWidth(
      core_row_, cut.dy(), getCutPitchY() - cut.dy(), enc->getY());

  if (isCutArray()) {
    const int end_height = getCutsWidth(
        end_row_, cut.dy(), getCutPitchY() - cut.dy(), enc->getY());
    return array_core_y_ * core_height + (array_core_y_ - 1) * array_spacing_y_
           + end_height;
  }
  return core_height;
}

bool ViaGenerator::isPreferredOver(const ViaGenerator* other) const
{
  if (other == nullptr) {
    return true;
  }

  debugPrint(logger_,
             utl::PDN,
             "ViaPreference",
             1,
             "Comparing {} and {}",
             getName(),
             other->getName());

  if (getCutArea() > other->getCutArea()) {
    const int cut_area_diff = getCutArea() - other->getCutArea();
    debugPrint(logger_,
               utl::PDN,
               "ViaPreference",
               2,
               "Comparing {} has more area by {}",
               getName(),
               cut_area_diff);
    return true;
  }
  if (other->getCutArea() > getCutArea()) {
    return false;
  }

  const int bottom_height_diff
      = other->getGeneratorHeight(true) - getGeneratorHeight(true);
  const int bottom_width_diff
      = other->getGeneratorWidth(true) - getGeneratorWidth(true);
  const int top_height_diff
      = other->getGeneratorHeight(false) - getGeneratorHeight(false);
  const int top_width_diff
      = other->getGeneratorWidth(false) - getGeneratorWidth(false);

  const bool bottom_is_hor
      = getBottomLayer()->getDirection() == odb::dbTechLayerDir::HORIZONTAL;
  const bool top_is_hor
      = getTopLayer()->getDirection() == odb::dbTechLayerDir::HORIZONTAL;

  const int bottom_prefered
      = bottom_is_hor ? bottom_height_diff : bottom_width_diff;
  const int top_prefered = top_is_hor ? top_height_diff : top_width_diff;
  const int bottom_non_prefered
      = bottom_is_hor ? bottom_width_diff : bottom_height_diff;
  const int top_non_prefered = top_is_hor ? top_width_diff : top_height_diff;

  debugPrint(logger_,
             utl::PDN,
             "ViaPreference",
             2,
             "Bottom via diff ({}, {}, {}) and top diff ({}, {}, {})",
             bottom_width_diff,
             bottom_height_diff,
             bottom_is_hor,
             top_width_diff,
             top_height_diff,
             top_is_hor);

  if (bottom_prefered < 0) {
    return true;
  }
  if (bottom_prefered > 0) {
    return false;
  }

  if (top_prefered < 0) {
    return true;
  }
  if (top_prefered > 0) {
    return false;
  }

  if (bottom_non_prefered < 0) {
    return true;
  }
  if (bottom_non_prefered > 0) {
    return false;
  }

  if (top_non_prefered < 0) {
    return true;
  }
  if (top_non_prefered > 0) {
    return false;
  }

  return false;
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

    if ((cut_.dx() == rule_length && cut_.dy() == rule_width)
        || (cut_.dx() == rule_width && cut_.dy() == rule_length)) {
      cutclass_ = rule;
      return;
    }
  }
}

bool ViaGenerator::recheckConstraints(const odb::Rect& rect, bool bottom)
{
  odb::Rect saved_rect;
  if (bottom) {
    saved_rect = lower_rect_;
    lower_rect_ = rect;
  } else {
    saved_rect = upper_rect_;
    upper_rect_ = rect;
  }

  const bool pass = checkConstraints(true, true, false);

  if (bottom) {
    lower_rect_ = saved_rect;
  } else {
    upper_rect_ = saved_rect;
  }

  debugPrint(logger_, utl::PDN, "Via", 2, "Recheck: {}", pass);

  return pass;
}

bool ViaGenerator::checkConstraints(bool check_cuts,
                                    bool check_min_cut,
                                    bool check_enclosure) const
{
  if (check_cuts && getTotalCuts() == 0) {
    debugPrint(logger_,
               utl::PDN,
               "Via",
               2,
               "Does not generate any vias: {}",
               getTotalCuts());

    return false;
  }

  if (check_min_cut && !checkMinCuts()) {
    debugPrint(logger_,
               utl::PDN,
               "Via",
               2,
               "Violates minimum cut rules: {}",
               getTotalCuts());

    return false;
  }

  if (check_enclosure && !checkMinEnclosure()) {
    const double dbu_microns = getTech()->getLefUnits();
    debugPrint(logger_,
               utl::PDN,
               "Via",
               2,
               "Violates minimum enclosure rules: {} ({:.4f} {:.4f}) width "
               "{:.4f} - {} "
               "({:.4f} {:.4f}) width {:.4f}",
               getBottomLayer()->getName(),
               bottom_enclosure_->getX() / dbu_microns,
               bottom_enclosure_->getY() / dbu_microns,
               getLowerWidth() / dbu_microns,
               getTopLayer()->getName(),
               top_enclosure_->getX() / dbu_microns,
               top_enclosure_->getY() / dbu_microns,
               getUpperWidth() / dbu_microns);

    return false;
  }

  return true;
}

bool ViaGenerator::checkMinCuts() const
{
  const bool lower_w = checkMinCuts(getBottomLayer(), getLowerWidth());
  const bool upper_w = checkMinCuts(getTopLayer(), getUpperWidth());

  return lower_w && upper_w;
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

  debugPrint(logger_,
             utl::PDN,
             "MinCut",
             1,
             "Layer {} (below {}) of width {:.4f} has {} min cut rules.",
             layer->getName(),
             is_below,
             tech_layer.dbuToMicron(width),
             min_rules_use == nullptr ? 0 : min_rules_use->size());

  if (min_rules_use == nullptr) {
    // no rules apply so assume valid
    return true;
  }

  bool is_valid = false;
  for (const auto& min_cut_rule : *min_rules_use) {
    const bool pass = min_cut_rule.cuts <= total_cuts;

    debugPrint(logger_,
               utl::PDN,
               "MinCut",
               2,
               "Rule width {:.4f} above ({}) or below ({}) requires {} vias, "
               "has {} vias {}: {}.",
               tech_layer.dbuToMicron(min_cut_rule.width),
               min_cut_rule.above,
               min_cut_rule.below,
               min_cut_rule.cuts,
               total_cuts,
               is_below ? "below" : "above",
               pass);

    is_valid |= pass;
  }

  return is_valid;
}

bool ViaGenerator::checkMinEnclosure() const
{
  const double dbu = getTech()->getDbUnitsPerMicron();

  std::vector<Enclosure> bottom_rules;
  std::vector<Enclosure> top_rules;
  getMinimumEnclosures(bottom_rules, top_rules, true);

  const bool bottom_has_rules = !bottom_rules.empty();
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Bottom layer {} with width {:.4f} has {} rules and enclosures of "
             "{:4f} and {:4f}.",
             getBottomLayer()->getName(),
             getLowerWidth() / dbu,
             bottom_rules.size(),
             bottom_enclosure_->getX() / dbu,
             bottom_enclosure_->getY() / dbu);
  bool bottom_passed = false;
  for (const auto& rule : bottom_rules) {
    const bool pass
        = rule.check(bottom_enclosure_->getX(), bottom_enclosure_->getY());
    debugPrint(logger_,
               utl::PDN,
               "ViaEnclosure",
               2,
               "Bottom rule enclosures {:4f} and {:4f} -> {}.",
               rule.getX() / dbu,
               rule.getY() / dbu,
               pass) bottom_passed
        |= pass;
  }

  const bool top_has_rules = !top_rules.empty();
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             1,
             "Top layer {} with width {:.4f} has {} rules and enclosures of "
             "{:4f} and {:4f}.",
             getTopLayer()->getName(),
             getUpperWidth() / dbu,
             top_rules.size(),
             top_enclosure_->getX() / dbu,
             top_enclosure_->getY() / dbu);
  bool top_passed = false;
  for (const auto& rule : top_rules) {
    const bool pass
        = rule.check(top_enclosure_->getX(), top_enclosure_->getY());
    debugPrint(logger_,
               utl::PDN,
               "ViaEnclosure",
               2,
               "Top rule enclosures {:4f} and {:4f} -> {}.",
               rule.getX() / dbu,
               rule.getY() / dbu,
               pass) top_passed
        |= pass;
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

void ViaGenerator::getMinimumEnclosures(std::vector<Enclosure>& bottom,
                                        std::vector<Enclosure>& top,
                                        bool rules_only) const
{
  auto populate_enc = [this](odb::dbTechLayer* layer,
                             int width,
                             const odb::dbTechLayerDir& direction,
                             bool above,
                             std::vector<Enclosure>& encs) {
    for (auto* rule : getCutMinimumEnclosureRules(width, above)) {
      encs.emplace_back(rule, layer, getCut(), direction);
    }
  };

  populate_enc(getBottomLayer(),
               getLowerWidth(false),
               getRectDirection(lower_rect_),
               false,
               bottom);
  populate_enc(getTopLayer(),
               getUpperWidth(false),
               getRectDirection(upper_rect_),
               true,
               top);
}

odb::dbTechLayerDir ViaGenerator::getRectDirection(const odb::Rect& rect) const
{
  const int height = rect.dy();
  const int width = rect.dx();

  if (width < height) {
    return odb::dbTechLayerDir::VERTICAL;
  }
  if (height < width) {
    return odb::dbTechLayerDir::HORIZONTAL;
  }
  return odb::dbTechLayerDir::NONE;
}

bool ViaGenerator::build(bool bottom_is_internal_layer,
                         bool top_is_internal_layer)
{
  std::vector<Enclosure> bottom_enclosures_list;
  std::vector<Enclosure> top_enclosures_list;
  getMinimumEnclosures(bottom_enclosures_list, top_enclosures_list, false);

  std::set<Enclosure> bottom_enclosures(bottom_enclosures_list.begin(),
                                        bottom_enclosures_list.end());
  std::set<Enclosure> top_enclosures(top_enclosures_list.begin(),
                                     top_enclosures_list.end());

  auto make_check_rect = [](const odb::Rect& rect,
                            const int width,
                            const int height,
                            bool is_internal) -> odb::Rect {
    if (is_internal) {
      return odb::Rect(0, 0, width, height);
    }
    return rect;
  };

  int best_cuts = 0;
  const Enclosure* best_bot_enc = nullptr;
  const Enclosure* best_top_enc = nullptr;
  for (auto& bottom_enc : bottom_enclosures) {
    for (auto& top_enc : top_enclosures) {
      determineRowsAndColumns(
          bottom_is_internal_layer, top_is_internal_layer, bottom_enc, top_enc);

      odb::Rect save_lower = lower_rect_;
      odb::Rect save_upper = upper_rect_;
      lower_rect_ = make_check_rect(lower_rect_,
                                    getGeneratorWidth(true),
                                    getGeneratorHeight(true),
                                    bottom_is_internal_layer);
      upper_rect_ = make_check_rect(upper_rect_,
                                    getGeneratorWidth(false),
                                    getGeneratorHeight(false),
                                    top_is_internal_layer);
      if (checkConstraints()) {
        bool save = best_cuts == 0;

        const int cuts = getTotalCuts();
        if (best_cuts == cuts) {
          // if same cut area, pick smaller enclosure
          if (bottom_enc.isPreferredOver(best_bot_enc, getBottomLayer())) {
            save = true;
          } else if (top_enc.isPreferredOver(best_top_enc, getTopLayer())) {
            save = true;
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
      lower_rect_ = save_lower;
      upper_rect_ = save_upper;
    }
  }

  if (best_cuts == 0) {
    return false;
  }

  // rebuild best
  determineRowsAndColumns(bottom_is_internal_layer,
                          top_is_internal_layer,
                          *best_bot_enc,
                          *best_top_enc);

  return true;
}

bool ViaGenerator::updateCutSpacing(int rows, int cols)
{
  // determine max number of adjacent cuts in array
  // dims bounded by 1 -> 4
  rows = std::max(1, std::min(rows, 4));
  cols = std::max(1, std::min(cols, 4));
  const int min_dim = std::min(rows, cols);
  const int max_dim = std::max(rows, cols);
  int adj_cuts = 0;
  if (min_dim == 1) {
    // array is a 1 x N array, therefore adjacent cuts can be 1 or 2
    if (max_dim == 2) {
      adj_cuts = 1;
    } else if (max_dim >= 3) {
      adj_cuts = 2;
    }
  } else if (min_dim == 2) {
    // array is a 2 x N, therefore adjacent cuts can be 2 or 3
    if (max_dim == 2) {
      adj_cuts = 2;
    } else if (max_dim >= 3) {
      adj_cuts = 3;
    }
  } else if (min_dim == 3) {
    // array is a 3 x N, therefore any cut in the middle will have 4 adjacent
    // cuts max_dim is 3 or 4
    adj_cuts = 4;
  } else if (min_dim == 4) {
    // array is a 4 x N, therefore any cut in the middle will have 4 adjacent
    // cuts max_dim is 4
    adj_cuts = 4;
  }

  if (adj_cuts < 2) {
    // nothing to do, rules require atleast 2
    return false;
  }

  auto* layer = getCutLayer();

  bool changed = false;
  const odb::Rect cut = getCut();
  for (auto* rule : layer->getTechLayerCutSpacingRules()) {
    if (rule->getType()
        != odb::dbTechLayerCutSpacingRule::CutSpacingType::ADJACENTCUTS) {
      continue;
    }

    if (!rule->isCutClassToAll() && rule->getCutClass() != cutclass_) {
      continue;
    }

    if (rule->getAdjacentCuts() <= adj_cuts) {
      if (max_dim == rows) {
        cut_pitch_y_ = cut.dy() + rule->getCutSpacing();
        changed = true;
      } else {
        cut_pitch_x_ = cut.dx() + rule->getCutSpacing();
        changed = true;
      }
    }
  }

  if (!changed) {
    for (auto* rule : layer->getV54SpacingRules()) {
      uint32_t numcuts;
      uint32_t within;
      uint32_t spacing;
      bool except_same_pgnet;
      if (!rule->getAdjacentCuts(numcuts, within, spacing, except_same_pgnet)) {
        continue;
      }
      if (except_same_pgnet) {
        continue;
      }
      if (numcuts <= adj_cuts) {
        cut_pitch_x_ = cut.dx() + spacing;
        cut_pitch_y_ = cut.dy() + spacing;
        changed = true;
      }
    }
  }

  return changed;
}

void ViaGenerator::determineRowsAndColumns(
    bool use_bottom_min_enclosure,
    bool use_top_min_enclosure,
    const Enclosure& bottom_min_enclosure,
    const Enclosure& top_min_enclosure)
{
  const double dbu_to_microns = getTech()->getDbUnitsPerMicron();

  const odb::Rect& cut = getCut();
  const int cut_width = cut.dx();
  const int cut_height = cut.dy();

  // determine enclosure needed
  const odb::Rect& intersection = getIntersectionRect();
  const int width = intersection.dx();
  const int height = intersection.dy();

  debugPrint(
      logger_,
      utl::PDN,
      "ViaEnclosure",
      1,
      "Bottom layer {} with width {:.4f} minimum enclosures {:.4f} and {:.4f}.",
      getBottomLayer()->getName(),
      getLowerWidth(false) / dbu_to_microns,
      bottom_min_enclosure.getX() / dbu_to_microns,
      bottom_min_enclosure.getY() / dbu_to_microns);
  debugPrint(
      logger_,
      utl::PDN,
      "ViaEnclosure",
      2,
      "Bottom constraints: use_minimum {}, must_fix_x {}, must_fit_y {}.",
      use_bottom_min_enclosure,
      lower_constraint_.must_fit_x,
      lower_constraint_.must_fit_y);
  debugPrint(
      logger_,
      utl::PDN,
      "ViaEnclosure",
      1,
      "Top layer {} with width {:.4f} minimum enclosures {:.4f} and {:.4f}.",
      getTopLayer()->getName(),
      getUpperWidth(false) / dbu_to_microns,
      top_min_enclosure.getX() / dbu_to_microns,
      top_min_enclosure.getY() / dbu_to_microns);
  debugPrint(logger_,
             utl::PDN,
             "ViaEnclosure",
             2,
             "Top constraints: use_minimum {}, must_fix_x {}, must_fit_y {}.",
             use_top_min_enclosure,
             upper_constraint_.must_fit_x,
             upper_constraint_.must_fit_y);

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

    if (updateCutSpacing(rows, cols)) {
      // cut spcaing changed so need to recompute
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
  }

  debugPrint(logger_,
             utl::PDN,
             "Via",
             2,
             "Initial via setup for {} {}: Cut size {:.4f}x{:.4f} on {} with "
             "pitch x={:.4f} and y={:.4f} yielding {} rows and {} columns.",
             Shape::getRectText(intersection, dbu_to_microns),
             isSplitCutArray(),
             cut.dx() / dbu_to_microns,
             cut.dy() / dbu_to_microns,
             getCutLayer()->getName(),
             getCutPitchX() / dbu_to_microns,
             getCutPitchY() / dbu_to_microns,
             rows,
             cols);

  auto determine_enclosure = [](bool use_min,
                                bool is_x,
                                int min_enc,
                                int overlap_enc,
                                const Constraint& contraint) -> int {
    if (use_min) {
      return min_enc;
    }
    if (is_x && contraint.must_fit_x) {
      return overlap_enc;
    }
    if (!is_x && contraint.must_fit_y) {
      return overlap_enc;
    }
    return min_enc;
  };

  bool used_array = false;

  const int array_size_max = std::max(rows, cols);
  const int array_size_min = std::min(rows, cols);
  if (array_size_max >= 2) {
    // if array rules might apply
    const int array_area_x
        = width
          - 2 * std::max(bottom_min_enclosure.getX(), top_min_enclosure.getX());
    const int array_area_y
        = height
          - 2 * std::max(bottom_min_enclosure.getY(), top_min_enclosure.getY());
    int max_cut_area = 0;
    for (auto* rule : getCutLayer()->getTechLayerArraySpacingRules()) {
      if (rule->isParallelOverlap()) {
        continue;
      }

      if (!isCutClass(rule->getCutClass())) {
        continue;
      }

      if (rule->getArrayWidth() != 0 && rule->getArrayWidth() > width) {
        // this rule is ignored due to width
        continue;
      }

      for (const auto& [rule_cuts, rule_spacing] :
           rule->getCutsArraySpacing()) {
        if (rule_cuts > array_size_min + (rule->isLongArray() ? 1 : 0)) {
          // this rules does not apply because the smaller dimension of the
          // array is less than the rule
          continue;
        }

        int cut_spacing_x = getCutPitchX() - cut_width;
        int cut_spacing_y = getCutPitchY() - cut_height;
        if (rule->getCutSpacing() != 0) {
          // reset spacing based on rule
          cut_spacing_x = rule->getCutSpacing();
          cut_spacing_y = rule->getCutSpacing();
        }
        // determine new rows and columns for array segments
        int x_cuts = getCuts(width,
                             cut_width,
                             bottom_min_enclosure.getX(),
                             top_min_enclosure.getX(),
                             cut_spacing_x + cut_width,
                             getMaxColumns());
        // if long array allowed, leave x alone
        x_cuts = rule->isLongArray() ? x_cuts : std::min(x_cuts, rule_cuts);
        int y_cuts = getCuts(height,
                             cut_height,
                             bottom_min_enclosure.getY(),
                             top_min_enclosure.getY(),
                             cut_spacing_y + cut_height,
                             getMaxRows());
        y_cuts = std::min(y_cuts, rule_cuts);
        const int array_width_x
            = getCutsWidth(x_cuts, cut_width, cut_spacing_x, 0);
        const int array_width_y
            = getCutsWidth(y_cuts, cut_height, cut_spacing_y, 0);

        const int array_pitch_x = array_width_x + rule_spacing;
        const int array_pitch_y = array_width_y + rule_spacing;

        const int full_arrays_x
            = (array_area_x - array_width_x) / array_pitch_x + 1;
        const int full_arrays_y
            = (array_area_y - array_width_y) / array_pitch_y + 1;

        // determine how many via row and columns are needed in the last
        // segments of the array if any
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

          array_spacing_x_ = rule_spacing;
          array_spacing_y_ = rule_spacing;

          const int via_width_x
              = full_arrays_x
                    * getCutsWidth(x_cuts, cut_width, cut_spacing_x, 0)
                + (full_arrays_x - 1) * array_spacing_x_
                + getCutsWidth(last_cols, cut_width, cut_spacing_x, 0)
                + (last_cols > 0 ? array_spacing_x_ : 0);
          const int double_enc_x = width - via_width_x;
          const int via_width_y
              = full_arrays_y
                    * getCutsWidth(y_cuts, cut_height, cut_spacing_y, 0)
                + (full_arrays_y - 1) * array_spacing_y_
                + getCutsWidth(last_rows, cut_height, cut_spacing_y, 0)
                + (last_rows > 0 ? array_spacing_y_ : 0);
          const int double_enc_y = height - via_width_y;

          bottom_enclosure_->setX(
              determine_enclosure(use_bottom_min_enclosure,
                                  true,
                                  bottom_min_enclosure.getX(),
                                  double_enc_x / 2,
                                  lower_constraint_));
          bottom_enclosure_->setY(
              determine_enclosure(use_bottom_min_enclosure,
                                  false,
                                  bottom_min_enclosure.getY(),
                                  double_enc_y / 2,
                                  lower_constraint_));
          top_enclosure_->setX(determine_enclosure(use_top_min_enclosure,
                                                   true,
                                                   top_min_enclosure.getX(),
                                                   double_enc_x / 2,
                                                   upper_constraint_));
          top_enclosure_->setY(determine_enclosure(use_top_min_enclosure,
                                                   false,
                                                   top_min_enclosure.getY(),
                                                   double_enc_y / 2,
                                                   upper_constraint_));

          max_cut_area = total_cut_area;
        }
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

    if (isSplitCutArray()) {
      bottom_enclosure_->setX(bottom_min_enclosure.getX());
      bottom_enclosure_->setY(bottom_min_enclosure.getY());
      top_enclosure_->setX(top_min_enclosure.getX());
      top_enclosure_->setY(top_min_enclosure.getY());
    } else {
      bottom_enclosure_->setX(determine_enclosure(use_bottom_min_enclosure,
                                                  true,
                                                  bottom_min_enclosure.getX(),
                                                  double_enc_x / 2,
                                                  lower_constraint_));
      bottom_enclosure_->setY(determine_enclosure(use_bottom_min_enclosure,
                                                  false,
                                                  bottom_min_enclosure.getY(),
                                                  double_enc_y / 2,
                                                  lower_constraint_));
      top_enclosure_->setX(determine_enclosure(use_top_min_enclosure,
                                               true,
                                               top_min_enclosure.getX(),
                                               double_enc_x / 2,
                                               upper_constraint_));
      top_enclosure_->setY(determine_enclosure(use_top_min_enclosure,
                                               false,
                                               top_min_enclosure.getY(),
                                               double_enc_y / 2,
                                               upper_constraint_));
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

  bottom_enclosure_->snap(getTech());
  top_enclosure_->snap(getTech());
}

std::vector<odb::dbTechLayerCutEnclosureRule*>
ViaGenerator::getCutMinimumEnclosureRules(int width, bool above) const
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

  CutRules* rules = nullptr;
  for (auto& [min_width, width_rules] : rules_map) {
    debugPrint(
        logger_,
        utl::PDN,
        "ViaEnclosure",
        3,
        "Enclosures for minimum width {:.4f} on {} from {}: {}",
        min_width
            / static_cast<double>(getBottomLayer()->getTech()->getLefUnits()),
        getCutLayer()->getName(),
        above ? "above" : "below",
        width_rules.size());
    if (min_width <= width) {
      rules = &width_rules;
    }
  }

  if (rules == nullptr) {
    return {};
  }
  return *rules;
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

int ViaGenerator::getRectSize(const odb::Rect& rect,
                              bool min,
                              bool only_real) const
{
  if (!only_real && isSplitCutArray()) {
    return 0;
  }
  if (min) {
    return rect.minDXDY();
  }
  return rect.maxDXDY();
}

int ViaGenerator::getLowerWidth(bool only_real) const
{
  return getRectSize(lower_rect_, true, only_real);
}

int ViaGenerator::getLowerHeight(bool only_real) const
{
  return getRectSize(lower_rect_, false, only_real);
}

int ViaGenerator::getUpperWidth(bool only_real) const
{
  return getRectSize(upper_rect_, true, only_real);
}

int ViaGenerator::getUpperHeight(bool only_real) const
{
  return getRectSize(upper_rect_, false, only_real);
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
                             getCutOffsetY(),
                             getArrayCoresX(),
                             getCutPitchX(),
                             getCutOffsetX(),
                             block,
                             getBottomLayer(),
                             false,
                             getTopLayer(),
                             false);
  }
  if (!isCutArray()) {
    return makeBaseVia(
        getViaCoreRows(), getCutPitchY(), getViaCoreColumns(), getCutPitchX());
  }

  DbBaseVia* core_via = makeBaseVia(
      getViaCoreRows(), getCutPitchY(), getViaCoreColumns(), getCutPitchX());
  DbBaseVia* end_of_row = nullptr;
  DbBaseVia* end_of_column = nullptr;
  DbBaseVia* end_of_row_column = nullptr;
  const bool need_end_of_row = hasViaLastRows();
  if (need_end_of_row) {
    end_of_row = makeBaseVia(
        getViaLastRows(), getCutPitchY(), getViaCoreColumns(), getCutPitchX());
  }
  const bool need_end_of_column = hasViaLastColumns();
  if (need_end_of_column) {
    end_of_column = makeBaseVia(
        getViaCoreRows(), getCutPitchY(), getViaLastColumns(), getCutPitchX());
  }
  if (need_end_of_row || need_end_of_column) {
    const int rows = need_end_of_row ? getViaLastRows() : getViaCoreRows();
    const int cols
        = need_end_of_column ? getViaLastColumns() : getViaCoreColumns();
    end_of_row_column = makeBaseVia(rows, getCutPitchY(), cols, getCutPitchX());
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

    int max_spacing_x = 0;
    int max_spacing_y = 0;
    for (auto* rule : cut_layer->getTechLayerCutSpacingTableDefRules()) {
      if (rule->getSecondLayer() != nullptr) {
        continue;
      }

      // look for the largest / smallest spacing
      int rule_pitch_x;
      int rule_pitch_y;
      if (cut.dx() == cut.dy()) {
        rule_pitch_x = rule->getMaxSpacing(
            cut_class, cut_class, odb::dbTechLayerCutSpacingTableDefRule::MIN);
        rule_pitch_y = rule_pitch_x;
      } else {
        const bool x_is_side = cut.dx() > cut.dy();
        const bool y_is_side = !x_is_side;

        // for x pitch, need to use y_is_side since the dx is the SIDE
        rule_pitch_x
            = rule->getSpacing(cut_class, y_is_side, cut_class, y_is_side);
        // for y pitch, need to use x_is_side since the dy is the SIDE
        rule_pitch_y
            = rule->getSpacing(cut_class, x_is_side, cut_class, x_is_side);
      }
      int rule_spacing_x = rule_pitch_x;
      int rule_spacing_y = rule_pitch_y;
      if (rule->isCenterAndEdge(cut_class, cut_class)) {
        // already spacing
      } else if (rule->isCenterToCenter(cut_class, cut_class)) {
        rule_spacing_x -= cut.dx();
        rule_spacing_y -= cut.dy();
      } else {
        // already spacing
      }

      if (rule->isSameNet()) {
        // use this rule since it's the same net
        max_spacing_x = rule_spacing_x;
        max_spacing_y = rule_spacing_y;
        break;
      }

      // keep looking for spacings
      max_spacing_x = std::max(max_spacing_x, rule_spacing_x);
      max_spacing_y = std::max(max_spacing_y, rule_spacing_y);
    }
    if (max_spacing_x != 0 && max_spacing_y != 0) {
      setCutPitchX(cut.dx() + max_spacing_x);
      setCutPitchY(cut.dy() + max_spacing_y);
    }
  }
}

/////////////

GenerateViaGenerator::GenerateViaGenerator(utl::Logger* logger,
                                           odb::dbTechViaGenerateRule* rule,
                                           const odb::Rect& lower_rect,
                                           const Constraint& lower_constraint,
                                           const odb::Rect& upper_rect,
                                           const Constraint& upper_constraint)
    : ViaGenerator(logger,
                   lower_rect,
                   lower_constraint,
                   upper_rect,
                   upper_constraint),
      rule_(rule)
{
  const uint32_t layer_count = rule_->getViaLayerRuleCount();

  std::map<odb::dbTechLayer*, uint32_t> layer_map;
  std::vector<odb::dbTechLayer*> layers;
  for (uint32_t l = 0; l < layer_count; l++) {
    odb::dbTechLayer* layer = rule_->getViaLayerRule(l)->getLayer();
    layer_map[layer] = l;
    layers.push_back(layer);
  }

  std::ranges::sort(layers, [](odb::dbTechLayer* l, odb::dbTechLayer* r) {
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

std::string GenerateViaGenerator::getName() const
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

std::string GenerateViaGenerator::getRuleName() const
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
                           getBottomEnclosure()->getX(),
                           getBottomEnclosure()->getY(),
                           getTopEnclosure()->getX(),
                           getTopEnclosure()->getY(),
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

void GenerateViaGenerator::getMinimumEnclosures(std::vector<Enclosure>& bottom,
                                                std::vector<Enclosure>& top,
                                                bool rules_only) const
{
  ViaGenerator::getMinimumEnclosures(bottom, top, true);

  if (rules_only) {
    return;
  }

  bottom.emplace_back(getBottomLayerRule(), getBottomLayer());
  top.emplace_back(getTopLayerRule(), getTopLayer());
}

/////////

TechViaGenerator::TechViaGenerator(utl::Logger* logger,
                                   odb::dbTechVia* via,
                                   const odb::Rect& lower_rect,
                                   const Constraint& lower_constraint,
                                   const odb::Rect& upper_rect,
                                   const Constraint& upper_constraint)
    : ViaGenerator(logger,
                   lower_rect,
                   lower_constraint,
                   upper_rect,
                   upper_constraint),
      via_(via),
      bottom_(via_->getBottomLayer()),
      top_(via_->getTopLayer())
{
  cut_outline_.mergeInit();
  for (auto* box : via_->getBoxes()) {
    auto* layer = box->getTechLayer();
    if (layer == bottom_ || layer == top_ || layer == nullptr) {
      continue;
    }

    odb::Rect cut = box->getBox();

    if (cuts_ == 0) {
      cut_ = layer;

      setCut(cut);
    }

    cut_outline_.merge(cut);

    cuts_++;
  }

  determineCutSpacing();
}

int TechViaGenerator::getTotalCuts() const
{
  return ViaGenerator::getTotalCuts() * cuts_;
}

std::string TechViaGenerator::getName() const
{
  return via_->getName();
}

const odb::Rect& TechViaGenerator::getCut() const
{
  return cut_outline_;
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
  return new DbTechVia(via_,
                       rows,
                       row_pitch,
                       cols,
                       col_pitch,
                       getBottomEnclosure(),
                       getTopEnclosure());
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
  const auto& intersection = getIntersectionRect();
  odb::dbTransform transform(
      odb::Point(0.5 * (intersection.xMin() + intersection.xMax()),
                 0.5 * (intersection.yMin() + intersection.yMax())));

  const DbTechVia via(via_, 1, 0, 1, 0);
  odb::Rect bottom_rect = via.getViaRect(true, false, true, false);
  odb::Rect top_rect = via.getViaRect(true, false, false, true);

  transform.apply(bottom_rect);
  if (!mostlyContains(
          getLowerRect(), intersection, bottom_rect, getLowerConstraint())) {
    return false;
  }

  transform.apply(top_rect);
  if (!mostlyContains(
          getUpperRect(), intersection, top_rect, getUpperConstraint())) {
    return false;
  }

  return true;
}

// check if shape is contains on three sides
bool TechViaGenerator::mostlyContains(const odb::Rect& full_shape,
                                      const odb::Rect& intersection,
                                      const odb::Rect& small_shape,
                                      const Constraint& constraint) const
{
  const odb::Rect check_rect
      = constraint.intersection_only ? intersection : full_shape;
  const bool inside_top = check_rect.yMax() >= small_shape.yMax();
  const bool inside_right = check_rect.xMax() >= small_shape.xMax();
  const bool inside_bottom = check_rect.yMin() <= small_shape.yMin();
  const bool inside_left = check_rect.xMin() <= small_shape.xMin();

  const bool inside_x = inside_right && inside_left;
  const bool inside_y = inside_top && inside_bottom;

  if (constraint.must_fit_x && constraint.must_fit_y) {
    return inside_x && inside_y;
  }

  if (constraint.must_fit_x) {
    return inside_x;
  }

  if (constraint.must_fit_y) {
    return inside_y;
  }

  int contains = 0;
  if (full_shape.yMax() >= small_shape.yMax()) {
    contains++;
  }

  if (full_shape.xMax() >= small_shape.xMax()) {
    contains++;
  }

  if (full_shape.yMin() <= small_shape.yMin()) {
    contains++;
  }

  if (full_shape.xMin() <= small_shape.xMin()) {
    contains++;
  }

  return contains > 2;
}

void TechViaGenerator::getMinimumEnclosures(std::vector<Enclosure>& bottom,
                                            std::vector<Enclosure>& top,
                                            bool rules_only) const
{
  ViaGenerator::getMinimumEnclosures(bottom, top, true);

  if (rules_only) {
    return;
  }

  const DbTechVia via(via_, 1, 0, 1, 0);

  const odb::Rect via_rect = via.getViaRect(false, true);
  const odb::Rect enc_bottom_rect = via.getViaRect(true, false, true, false);
  const odb::Rect enc_top_rect = via.getViaRect(true, false, false, true);

  debugPrint(getLogger(),
             utl::PDN,
             "ViaEnclosure",
             3,
             "Tech via: {}, via rect {}, enc bottom rect {}, enc top rect {}",
             via_->getName(),
             Shape::getRectText(via_rect, getTech()->getLefUnits()),
             Shape::getRectText(enc_bottom_rect, getTech()->getLefUnits()),
             Shape::getRectText(enc_top_rect, getTech()->getLefUnits()));

  const bool use_via = !via_rect.isInverted() && !enc_bottom_rect.isInverted()
                       && !enc_top_rect.isInverted();

  int odx = 0;
  int ody = 0;
  if (use_via) {
    odx = std::max(via_rect.xMin() - enc_bottom_rect.xMin(),
                   enc_bottom_rect.xMax() - via_rect.xMax());
    ody = std::max(via_rect.yMin() - enc_bottom_rect.yMin(),
                   enc_bottom_rect.yMax() - via_rect.yMax());
  }
  // remove rules that do not fit the tech vias enclosures.
  std::erase_if(bottom, [&](const auto& enc) {
    return enc.getX() < odx && enc.getY() < ody;
  });
  // "fix" rules to use the tech via enclosure
  for (auto& enc : bottom) {
    if (enc.getX() < odx) {
      enc.setX(odx);
    }
    if (enc.getY() < ody) {
      enc.setY(ody);
    }
  }
  if (bottom.empty()) {
    bottom.emplace_back(odx, ody);
  }

  odx = 0;
  ody = 0;
  if (use_via) {
    odx = std::max(via_rect.xMin() - enc_top_rect.xMin(),
                   enc_top_rect.xMax() - via_rect.xMax());
    ody = std::max(via_rect.yMin() - enc_top_rect.yMin(),
                   enc_top_rect.yMax() - via_rect.yMax());
  }
  // remove rules that do not fit the tech vias enclosures.
  std::erase_if(top, [&](const auto& enc) {
    return enc.getX() < odx && enc.getY() < ody;
  });
  // "fix" rules to use the tech via enclosure
  for (auto& enc : top) {
    if (enc.getX() < odx) {
      enc.setX(odx);
    }
    if (enc.getY() < ody) {
      enc.setY(ody);
    }
  }
  if (top.empty()) {
    top.emplace_back(odx, ody);
  }
}

std::set<odb::Rect> TechViaGenerator::getViaObstructionRects(
    utl::Logger* logger,
    odb::dbTechVia* via,
    const odb::Point& pt)
{
  const TechViaGenerator generator(logger, via, {}, {}, {}, {});

  const int x_pitch = generator.getCutPitchX() - 1;
  const int y_pitch = generator.getCutPitchY() - 1;

  std::set<odb::Rect> obs;

  const odb::dbTransform xform(pt);
  for (auto* box : via->getBoxes()) {
    auto* layer = box->getTechLayer();
    if (layer->getType() != odb::dbTechLayerType::CUT) {
      continue;
    }

    odb::Rect rect = box->getBox();
    xform.apply(rect);

    const odb::Rect x_obs_rect(rect.xMax() - x_pitch,
                               rect.yMin() + 1,
                               rect.xMin() + x_pitch,
                               rect.yMax() - 1);

    const odb::Rect y_obs_rect(rect.xMin() + 1,
                               rect.yMax() - y_pitch,
                               rect.xMax() - 1,
                               rect.yMin() + y_pitch);

    odb::Rect min_space_obs;
    rect.bloat(layer->getSpacing() - 1, min_space_obs);

    obs.insert(x_obs_rect);
    obs.insert(y_obs_rect);
    obs.insert(min_space_obs);
  }

  return obs;
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

void Via::writeToDb(odb::dbSWire* wire,
                    odb::dbBlock* block,
                    const Shape::ObstructionTreeMap& obstructions)
{
  odb::dbWireShapeType type = lower_->getType();

  if (lower_->getType() != upper_->getType()) {
    // If both shapes are not the same, use stripe
    type = odb::dbWireShapeType::STRIPE;
  }

  DbVia::ViaLayerShape shapes;
  connect_->makeVia(wire, lower_, upper_, type, shapes);

  if (shapes.bottom.empty() && shapes.middle.empty() && shapes.top.empty()) {
    markFailed(failedViaReason::BUILD);
    return;
  }

  auto check_shapes
      = [this, &obstructions](
            const ShapePtr& shape,
            const std::set<DbVia::ViaLayerShape::RectBoxPair>& via_shapes)
      -> std::set<odb::dbSBox*> {
    std::set<odb::dbSBox*> ripup;

    const odb::Rect& rect = shape->getRect();
    odb::Rect new_shape = rect;
    for (const auto& via_shape : via_shapes) {
      new_shape.merge(via_shape.first);
    }
    if (new_shape != rect) {
      auto* layer = shape->getLayer();
      if (obstructions.find(layer) != obstructions.end()) {
        const auto& obs = obstructions.at(layer);
        for (const auto& [via_shape, box] : via_shapes) {
          if (obs.qbegin(bgi::intersects(via_shape)) != obs.qend()) {
            ripup.insert(box);
          }
        }
      }

      debugPrint(
          getLogger(),
          utl::PDN,
          "Via",
          3,
          "{} shape changed {}: {} -> {}",
          shape->getNet()->getName(),
          layer->getName(),
          Shape::getRectText(shape->getRect(), layer->getTech()->getLefUnits()),
          Shape::getRectText(new_shape, layer->getTech()->getLefUnits()));
      bool valid_change = shape->isModifiable();
      if (!shape->allowsNonPreferredDirectionChange()) {
        if (shape->getLayerDirection() == odb::dbTechLayerDir::HORIZONTAL) {
          if (new_shape.yMin() != rect.yMin()
              || new_shape.yMax() != rect.yMax()) {
            valid_change = false;
          }
        } else if (shape->getLayerDirection()
                   == odb::dbTechLayerDir::VERTICAL) {
          if (new_shape.xMin() != rect.xMin()
              || new_shape.xMax() != rect.xMax()) {
            valid_change = false;
          }
        }
      }

      if (valid_change && ripup.empty()) {
        shape->setRect(new_shape);
      } else {
        for (const auto& via_shape : via_shapes) {
          if (!rect.contains(via_shape.first)) {
            ripup.insert(via_shape.second);
          }
        }
      }
    }

    return ripup;
  };

  const std::set<odb::dbSBox*> ripup_shapes_bottom
      = check_shapes(lower_, shapes.bottom);
  const std::set<odb::dbSBox*> ripup_shapes_top
      = check_shapes(upper_, shapes.top);

  std::set<odb::dbSBox*> ripup_shapes;
  ripup_shapes.insert(ripup_shapes_bottom.begin(), ripup_shapes_bottom.end());
  ripup_shapes.insert(ripup_shapes_top.begin(), ripup_shapes_top.end());

  std::set<odb::dbSBox*> ripup_vias_middle;
  for (const auto& [middle_rect, box] : shapes.middle) {
    for (auto* ripup_via : ripup_shapes) {
      const odb::Rect ripup_area = ripup_via->getBox();
      if (ripup_area.overlaps(middle_rect)) {
        ripup_vias_middle.insert(box);
        break;
      }
    }
  }
  ripup_shapes.insert(ripup_vias_middle.begin(), ripup_vias_middle.end());

  if (!ripup_shapes.empty()) {
    // Check if via stack continuity will be broken

    // Collect remaining shapes
    std::set<odb::dbTechLayer*> layers;
    for (const auto& viashapes : {shapes.bottom, shapes.middle, shapes.top}) {
      for (const auto& [rect, box] : viashapes) {
        if (ripup_shapes.find(box) == ripup_shapes.end()) {
          if (box->isVia()) {
            if (auto* via = box->getBlockVia()) {
              for (auto* viabox : via->getBoxes()) {
                layers.insert(viabox->getTechLayer());
              }
            } else if (auto* via = box->getTechVia()) {
              for (auto* viabox : via->getBoxes()) {
                layers.insert(viabox->getTechLayer());
              }
            }
          } else {
            layers.insert(box->getTechLayer());
          }
        }
      }
    }

    bool broken = false;
    for (auto* layer : connect_->getAllLayers()) {
      if (layers.find(layer) == layers.end()) {
        // stack is broken
        broken = true;
      }
    }

    if (broken) {
      for (const auto& viashapes : {shapes.bottom, shapes.middle, shapes.top}) {
        for (const auto& [rect, box] : viashapes) {
          ripup_shapes.insert(box);
        }
      }
    }
  }

  if (!ripup_shapes.empty()) {
    const TechLayer tech_layer(lower_->getLayer());
    int x = 0;
    int y = 0;
    int ripup_count = 0;
    int via_ripup_count = 0;
    for (auto* shape : ripup_shapes) {
      if (shape->getBlockVia() != nullptr || shape->getTechVia() != nullptr) {
        const odb::Point pt = shape->getViaXY();
        x += pt.getX();
        y += pt.getY();
        ripup_count++;

        if (odb::dbVia* via = shape->getBlockVia()) {
          for (auto* box : via->getBoxes()) {
            if (box->getTechLayer() != nullptr
                && box->getTechLayer()->getType()
                       == odb::dbTechLayerType::CUT) {
              via_ripup_count++;
            }
          }
        } else if (odb::dbTechVia* via = shape->getTechVia()) {
          for (auto* box : via->getBoxes()) {
            if (box->getTechLayer() != nullptr
                && box->getTechLayer()->getType()
                       == odb::dbTechLayerType::CUT) {
              via_ripup_count++;
            }
          }
        }
      }

      odb::dbSBox::destroy(shape);
    }

    if (ripup_count == 0) {
      // this really should not happen but makes clang-tidy happier
      ripup_count = 1;
    }
    getLogger()->warn(
        utl::PDN,
        195,
        "Removing {} via(s) between {} and {} at ({:.4f} um, {:.4f} um) for {}",
        via_ripup_count,
        lower_->getLayer()->getName(),
        upper_->getLayer()->getName(),
        tech_layer.dbuToMicron(x / ripup_count),
        tech_layer.dbuToMicron(y / ripup_count),
        lower_->getNet()->getName());
    markFailed(failedViaReason::RIPUP);
  }
}

std::string Via::getDisplayText() const
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

void Via::markFailed(failedViaReason reason)
{
  failed_ = true;
  connect_->addFailedVia(reason, area_, net_);
}

Via::ViaTree Via::convertVectorToTree(std::vector<ViaPtr>& vec)
{
  ViaTree tree(vec.begin(), vec.end());

  vec = std::vector<ViaPtr>();

  return tree;
}

}  // namespace pdn
