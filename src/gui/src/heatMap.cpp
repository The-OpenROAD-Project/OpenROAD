//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "gui/heatMap.h"

#include <QApplication>
#include <QThread>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "heatMapSetup.h"
#include "utl/Logger.h"

namespace gui {

HeatMapDataSource::HeatMapDataSource(utl::Logger* logger,
                                     const std::string& name,
                                     const std::string& short_name,
                                     const std::string& settings_group)
    : name_(name),
      short_name_(short_name),
      settings_group_(settings_group),
      destroy_map_(true),
      use_dbu_(false),
      populated_(false),
      colors_correct_(false),
      issue_redraw_(true),
      block_(nullptr),
      logger_(logger),
      grid_x_size_(10.0),
      grid_y_size_(10.0),
      display_range_min_(0.0),
      display_range_max_(100.0),
      draw_below_min_display_range_(false),
      draw_above_max_display_range_(true),
      color_alpha_(150),
      log_scale_(false),
      reverse_log_(false),
      show_numbers_(false),
      show_legend_(false),
      renderer_(std::make_unique<HeatMapRenderer>(*this)),
      setup_(nullptr),
      color_generator_(SpectrumGenerator(100.0))
{
  clearMap();

  // ensure color map is initialized
  updateMapColors();
}

HeatMapDataSource::~HeatMapDataSource()
{
  Gui::get()->unregisterHeatMap(this);
}

void HeatMapDataSource::registerHeatMap()
{
  Gui::get()->registerHeatMap(this);
}

void HeatMapDataSource::dumpToFile(const std::string& file)
{
  ensureMap();

  if (!isPopulated()) {
    logger_->error(utl::GUI, 72, "\"{}\" is not populated with data.", name_);
  }

  std::ofstream csv(file);
  if (!csv.is_open()) {
    logger_->error(utl::GUI, 73, "Unable to open {}", file);
  }

  const double dbu_to_micron = block_->getDbUnitsPerMicron();

  csv << "x0,y0,x1,y1,value" << std::endl;
  for (const auto& map_col : map_) {
    for (const auto& map_value : map_col) {
      if (!map_value->has_value) {
        continue;
      }
      const odb::Rect& box_rect = map_value->rect;
      const double scaled_value = convertPercentToValue(map_value->value);

      csv << std::defaultfloat << std::setprecision(4);
      csv << box_rect.xMin() / dbu_to_micron << ",";
      csv << box_rect.yMin() / dbu_to_micron << ",";
      csv << box_rect.xMax() / dbu_to_micron << ",";
      csv << box_rect.yMax() / dbu_to_micron << ",";
      csv << std::scientific << std::setprecision(6);
      csv << scaled_value << std::endl;
    }
  }

  csv.close();
}

void HeatMapDataSource::redraw()
{
  ensureMap();

  if (issue_redraw_) {
    renderer_->redraw();
  }
}

void HeatMapDataSource::setColorAlpha(int alpha)
{
  color_alpha_
      = boundValue<int>(alpha, getColorAlphaMinimum(), getColorAlphaMaximum());
  updateMapColors();

  redraw();
}

void HeatMapDataSource::setDisplayRange(double min, double max)
{
  if (max < min) {
    std::swap(min, max);
  }

  display_range_min_ = boundValue<double>(
      min, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());
  display_range_max_ = boundValue<double>(
      max, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());

  updateMapColors();

  redraw();
}

void HeatMapDataSource::setDrawBelowRangeMin(bool show)
{
  draw_below_min_display_range_ = show;

  redraw();
}

void HeatMapDataSource::setDrawAboveRangeMax(bool show)
{
  draw_above_max_display_range_ = show;

  redraw();
}

void HeatMapDataSource::setGridSizes(double x, double y)
{
  bool changed = false;
  if (grid_x_size_ != x) {
    grid_x_size_ = boundValue<double>(
        x, getGridSizeMinimumValue(), getGridSizeMaximumValue());
    changed = true;
  }
  if (grid_y_size_ != y) {
    grid_y_size_ = boundValue<double>(
        y, getGridSizeMinimumValue(), getGridSizeMaximumValue());
    changed = true;
  }

  if (changed) {
    destroyMap();
  }
}

void HeatMapDataSource::setLogScale(bool scale)
{
  log_scale_ = scale;
  updateMapColors();

  redraw();
}

void HeatMapDataSource::setReverseLogScale(bool reverse)
{
  reverse_log_ = reverse;
  updateMapColors();

  redraw();
}

void HeatMapDataSource::setShowNumbers(bool numbers)
{
  show_numbers_ = numbers;

  redraw();
}

void HeatMapDataSource::setShowLegend(bool legend)
{
  show_legend_ = legend;

  redraw();
}

Painter::Color HeatMapDataSource::getColor(double value) const
{
  auto find_val
      = std::find_if(color_lower_bounds_.begin(),
                     color_lower_bounds_.end(),
                     [value](const double other) { return other >= value; });
  const double color_index
      = std::distance(color_lower_bounds_.begin(), find_val);
  return color_generator_.getColor(
      100.0 * color_index / color_generator_.getColorCount(), color_alpha_);
}

void HeatMapDataSource::showSetup()
{
  if (setup_ == nullptr) {
    setup_ = new HeatMapSetup(*this,
                              QString::fromStdString(name_),
                              use_dbu_,
                              block_->getDbUnitsPerMicron());

    QObject::connect(setup_, &QDialog::finished, &QObject::deleteLater);
    QObject::connect(
        setup_, &QObject::destroyed, [this]() { setup_ = nullptr; });

    setup_->show();
  } else {
    setup_->raise();
  }
}

std::string HeatMapDataSource::formatValue(double value, bool legend) const
{
  QString text;
  text.setNum(value, 'f', 2);
  if (legend) {
    text += "%";
  }
  return text.toStdString();
}

void HeatMapDataSource::addBooleanSetting(
    const std::string& name,
    const std::string& label,
    const std::function<bool(void)>& getter,
    const std::function<void(bool)>& setter)
{
  settings_.emplace_back(MapSettingBoolean{name, label, getter, setter});
}

void HeatMapDataSource::addMultipleChoiceSetting(
    const std::string& name,
    const std::string& label,
    const std::function<std::vector<std::string>(void)>& choices,
    const std::function<std::string(void)>& getter,
    const std::function<void(std::string)>& setter)
{
  settings_.emplace_back(
      MapSettingMultiChoice{name, label, choices, getter, setter});
}

Renderer::Settings HeatMapDataSource::getSettings() const
{
  Renderer::Settings settings{{"DisplayMin", display_range_min_},
                              {"DisplayMax", display_range_max_},
                              {"GridX", grid_x_size_},
                              {"GridY", grid_y_size_},
                              {"Alpha", color_alpha_},
                              {"LogScale", log_scale_},
                              {"ReverseLog", reverse_log_},
                              {"ShowNumbers", show_numbers_},
                              {"ShowLegend", show_legend_}};

  for (const auto& setting : settings_) {
    if (std::holds_alternative<MapSettingBoolean>(setting)) {
      const auto& set = std::get<MapSettingBoolean>(setting);
      settings[set.name] = set.getter();
    } else if (std::holds_alternative<MapSettingMultiChoice>(setting)) {
      const auto& set = std::get<MapSettingMultiChoice>(setting);
      settings[set.name] = set.getter();
    }
  }

  return settings;
}

void HeatMapDataSource::setSettings(const Renderer::Settings& settings)
{
  Renderer::setSetting<double>(settings, "DisplayMin", display_range_min_);
  Renderer::setSetting<double>(settings, "DisplayMax", display_range_max_);
  Renderer::setSetting<double>(settings, "GridX", grid_x_size_);
  Renderer::setSetting<double>(settings, "GridY", grid_y_size_);
  Renderer::setSetting<int>(settings, "Alpha", color_alpha_);
  Renderer::setSetting<bool>(settings, "LogScale", log_scale_);
  Renderer::setSetting<bool>(settings, "ReverseLog", reverse_log_);
  Renderer::setSetting<bool>(settings, "ShowNumbers", show_numbers_);
  Renderer::setSetting<bool>(settings, "ShowLegend", show_legend_);

  for (const auto& setting : settings_) {
    if (std::holds_alternative<MapSettingBoolean>(setting)) {
      const auto& set = std::get<MapSettingBoolean>(setting);
      bool temp_value = set.getter();
      Renderer::setSetting<bool>(settings, set.name, temp_value);
      set.setter(temp_value);
    } else if (std::holds_alternative<MapSettingMultiChoice>(setting)) {
      const auto& set = std::get<MapSettingMultiChoice>(setting);
      std::string temp_value = set.getter();
      Renderer::setSetting<std::string>(settings, set.name, temp_value);
      set.setter(temp_value);
    }
  }

  // only reapply bounded value settings
  setDisplayRange(display_range_min_, display_range_max_);
  setGridSizes(grid_x_size_, grid_y_size_);
  setColorAlpha(color_alpha_);
}

HeatMapDataSource::MapView HeatMapDataSource::getMapView(
    const odb::Rect& bounds)
{
  const auto x_low_find
      = std::lower_bound(map_x_grid_.begin(), map_x_grid_.end(), bounds.xMin());
  const auto x_high_find
      = std::upper_bound(x_low_find, map_x_grid_.end(), bounds.xMax());
  const auto y_low_find
      = std::lower_bound(map_y_grid_.begin(), map_y_grid_.end(), bounds.yMin());
  const auto y_high_find
      = std::upper_bound(y_low_find, map_y_grid_.end(), bounds.yMax());

  const int shape_x = static_cast<int>(map_.shape()[0]);
  const int shape_y = static_cast<int>(map_.shape()[1]);

  const int x_low = std::max(
      static_cast<int>(std::distance(map_x_grid_.begin(), x_low_find)) - 1, 0);
  const int x_high = std::min(
      static_cast<int>(std::distance(map_x_grid_.begin(), x_high_find)),
      shape_x);
  const int y_low = std::max(
      static_cast<int>(std::distance(map_y_grid_.begin(), y_low_find)) - 1, 0);
  const int y_high = std::min(
      static_cast<int>(std::distance(map_y_grid_.begin(), y_high_find)),
      shape_y);

  return map_[boost::indices[Map::index_range(x_low, x_high)]
                            [Map::index_range(y_low, y_high)]];
}

void HeatMapDataSource::addToMap(const odb::Rect& region, double value)
{
  for (const auto& map_col : getMapView(region)) {
    for (const auto& map_pt : map_col) {
      odb::Rect intersection;
      map_pt->rect.intersection(region, intersection);

      const double intersect_area = intersection.area();
      const double value_area = region.area();
      const double region_area = map_pt->rect.area();

      combineMapData(map_pt->has_value,
                     map_pt->value,
                     value,
                     value_area,
                     intersect_area,
                     region_area);
      map_pt->has_value = true;

      markColorsInvalid();
    }
  }
}

odb::Rect HeatMapDataSource::getBounds() const
{
  return getBlock()->getDieArea();
}

void HeatMapDataSource::clearMap()
{
  map_.resize(boost::extents[1][1]);
  map_[0][0] = nullptr;
  populated_ = false;
}

void HeatMapDataSource::setupMap()
{
  if (getBlock() == nullptr) {
    return;
  }

  populateXYGrid();

  const size_t x_grid_size = map_x_grid_.size() - 1;
  const size_t y_grid_size = map_y_grid_.size() - 1;

  debugPrint(logger_,
             utl::GUI,
             "HeatMap",
             1,
             "Generating {}x{} map",
             x_grid_size,
             y_grid_size);
  map_.resize(boost::extents[x_grid_size][y_grid_size]);

  const Painter::Color default_color = getColor(0);
  for (size_t x = 0; x < x_grid_size; x++) {
    const int xMin = map_x_grid_[x];
    const int xMax = map_x_grid_[x + 1];

    for (size_t y = 0; y < y_grid_size; y++) {
      const int yMin = map_y_grid_[y];
      const int yMax = map_y_grid_[y + 1];

      auto map_pt = std::make_shared<MapColor>();
      map_pt->rect = odb::Rect(xMin, yMin, xMax, yMax);
      map_pt->has_value = false;
      map_pt->value = 0.0;
      map_pt->color = default_color;

      map_[x][y] = std::move(map_pt);
    }
  }
}

void HeatMapDataSource::populateXYGrid()
{
  const int dx = getGridXSize() * getBlock()->getDbUnitsPerMicron();
  const int dy = getGridYSize() * getBlock()->getDbUnitsPerMicron();

  const odb::Rect bounds = getBounds();

  const int x_grid = std::ceil(bounds.dx() / static_cast<double>(dx));
  const int y_grid = std::ceil(bounds.dy() / static_cast<double>(dy));

  std::vector<int> x_grid_set, y_grid_set;
  for (int x = 0; x < x_grid; x++) {
    const int xMin = bounds.xMin() + x * dx;
    const int xMax = std::min(xMin + dx, bounds.xMax());
    if (x == 0) {
      x_grid_set.push_back(xMin);
    }
    x_grid_set.push_back(xMax);
  }
  for (int y = 0; y < y_grid; y++) {
    const int yMin = bounds.yMin() + y * dy;
    const int yMax = std::min(yMin + dy, bounds.yMax());
    if (y == 0) {
      y_grid_set.push_back(yMin);
    }
    y_grid_set.push_back(yMax);
  }

  setXYMapGrid(x_grid_set, y_grid_set);
}

void HeatMapDataSource::setXYMapGrid(const std::vector<int>& x_grid,
                                     const std::vector<int>& y_grid)
{
  // ensure sorted and uniqueness
  const std::set<int> x_grid_set(x_grid.begin(), x_grid.end());
  const std::set<int> y_grid_set(y_grid.begin(), y_grid.end());

  map_x_grid_.clear();
  map_y_grid_.clear();

  map_x_grid_.insert(map_x_grid_.end(), x_grid_set.begin(), x_grid_set.end());
  map_y_grid_.insert(map_y_grid_.end(), y_grid_set.begin(), y_grid_set.end());
}

void HeatMapDataSource::destroyMap()
{
  destroy_map_ = true;

  redraw();
}

bool HeatMapDataSource::hasData() const
{
  if (!populated_) {
    return false;
  }

  for (const auto& map_col : map_) {
    for (const auto& map_pt : map_col) {
      if (map_pt->has_value) {
        return true;
      }
    }
  }

  return false;
}

void HeatMapDataSource::ensureMap()
{
  std::unique_lock<std::mutex> lock(ensure_mutex_);

  if (destroy_map_) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "Destroying map");
    clearMap();
    destroy_map_ = false;
  }

  const bool build_map = map_[0][0] == nullptr;
  if (build_map) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "Setting up map");
    setupMap();
  }

  if (build_map || !isPopulated()) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "Populating map");

    const bool update_cursor
        = gui::Gui::enabled()
          && QApplication::instance()->thread() == QThread::currentThread();

    if (update_cursor) {
      QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    populated_ = populateMap();
    if (update_cursor) {
      QApplication::restoreOverrideCursor();
    }

    if (isPopulated()) {
      debugPrint(logger_, utl::GUI, "HeatMap", 1, "Correcting map scale");
      correctMapScale(map_);
    }

    if (setup_ != nullptr) {
      // announce changes
      setIssueRedraw(false);
      setup_->changed();
      setIssueRedraw(true);
    }
  }

  if (!colors_correct_ && isPopulated()) {
    debugPrint(logger_, utl::GUI, "HeatMap", 1, "Assigning map colors");
    assignMapColors();
  }
}

void HeatMapDataSource::updateMapColors()
{
  const int color_count = color_generator_.getColorCount();
  color_lower_bounds_.clear();
  color_lower_bounds_.resize(color_count + 1);
  // generate ranges for colors
  if (log_scale_) {
    double range = display_range_max_;
    if (display_range_min_ != 0.0) {
      range = display_range_max_ / display_range_min_;
    }

    const double step = std::pow(range, 1.0 / color_count);

    for (int i = 0; i <= color_count; i++) {
      double start = display_range_max_ / std::pow(step, i);
      if (i == color_generator_.getColorCount()) {
        start = display_range_min_;
      }
      color_lower_bounds_[i] = start;
    }

    if (reverse_log_) {
      for (auto& lower_bound : color_lower_bounds_) {
        lower_bound = display_range_max_ - lower_bound + display_range_min_;
      }
    } else {
      std::reverse(color_lower_bounds_.begin(), color_lower_bounds_.end());
    }
  } else {
    const double step = (display_range_max_ - display_range_min_) / color_count;
    for (int i = 0; i <= color_count; i++) {
      color_lower_bounds_[i] = display_range_min_ + i * step;
    }
  }

  markColorsInvalid();
}

void HeatMapDataSource::assignMapColors()
{
  for (const auto& map_col : map_) {
    for (const auto& map_pt : map_col) {
      map_pt->color = getColor(map_pt->value);
    }
  }
  colors_correct_ = true;
}

double HeatMapDataSource::getRealRangeMinimumValue() const
{
  return color_lower_bounds_[0];
}

double HeatMapDataSource::getRealRangeMaximumValue() const
{
  return color_lower_bounds_[color_lower_bounds_.size() - 1];
}

std::vector<std::pair<int, double>> HeatMapDataSource::getLegendValues() const
{
  const int color_count = color_generator_.getColorCount();
  const int count = 6;
  std::vector<std::pair<int, double>> values;
  const double index_incr = static_cast<double>(color_count) / (count - 1);
  const double linear_start = getRealRangeMinimumValue();
  const double linear_step
      = (getRealRangeMaximumValue() - linear_start) / (count - 1);
  for (int i = 0; i < count; i++) {
    int idx = std::round(i * index_incr);
    if (idx > color_count) {
      idx = color_count;
    }
    double value = color_lower_bounds_[idx];
    if (!log_scale_) {
      value = linear_step * i + linear_start;
    }

    values.emplace_back(idx, value);
  }
  return values;
}

void HeatMapDataSource::onShow()
{
  if (!isPopulated()) {
    logger_->warn(utl::GUI,
                  66,
                  "Heat map \"{}\" has not been populated with data.",
                  getName());
  }
}

void HeatMapDataSource::onHide()
{
  if (destroyMapOnNotVisible()) {
    setIssueRedraw(false);
    destroyMap();
    setIssueRedraw(true);
  }
}

///////////

HeatMapRenderer::HeatMapRenderer(HeatMapDataSource& datasource)
    : datasource_(datasource), first_paint_(true)
{
  addDisplayControl(datasource_.getName(),
                    false,
                    [this]() { datasource_.showSetup(); },
                    {""});  // mutually exclusive to all
}

void HeatMapRenderer::drawObjects(Painter& painter)
{
  if (!checkDisplayControl(datasource_.getName())) {
    if (!first_paint_) {
      first_paint_ = true;  // reset check
      // first time so announce onHide
      datasource_.onHide();
    }
    return;
  }

  datasource_.ensureMap();

  if (first_paint_) {
    first_paint_ = false;
    // first time so announce onShow
    datasource_.onShow();
  }

  if (!datasource_.isPopulated()) {
    // nothing to paint
    return;
  }

  const bool show_numbers = datasource_.getShowNumbers();
  const double min_value = datasource_.getRealRangeMinimumValue();
  const double max_value = datasource_.getRealRangeMaximumValue();
  const bool show_mins = datasource_.getDrawBelowRangeMin();
  const bool show_maxs = datasource_.getDrawAboveRangeMax();

  const odb::Rect& bounds = painter.getBounds();

  // minimum box size is 2 pixels
  const double min_dbu = 2.0 / painter.getPixelsPerDBU();

  const double dbu_per_micron = datasource_.getBlock()->getDbUnitsPerMicron();
  const int x_scale
      = std::ceil(min_dbu / (datasource_.getGridXSize() * dbu_per_micron));
  const int y_scale
      = std::ceil(min_dbu / (datasource_.getGridYSize() * dbu_per_micron));

  const HeatMapDataSource::MapView map_view = datasource_.getMapView(bounds);
  const int x_size = map_view.shape()[0];
  const int y_size = map_view.shape()[1];

  for (int x = 0; x < x_size; x += x_scale) {
    for (int y = 0; y < y_size; y += y_scale) {
      HeatMapDataSource::MapColor draw_pt;
      draw_pt.rect.mergeInit();
      draw_pt.has_value = false;
      draw_pt.value = std::numeric_limits<double>::lowest();

      for (int x_sub = 0; x_sub < x_scale; x_sub++) {
        const int x_idx = x + x_sub;
        if (x_idx >= x_size) {
          continue;
        }

        for (int y_sub = 0; y_sub < y_scale; y_sub++) {
          const int y_idx = y + y_sub;
          if (y_idx >= y_size) {
            continue;
          }

          const auto& map_pt = map_view[x_idx][y_idx];
          draw_pt.rect.merge(map_pt->rect);
          if (!map_pt->has_value) {  // value not set so nothing to do
            continue;
          }

          if (draw_pt.value < map_pt->value) {
            draw_pt.has_value = true;
            draw_pt.value = map_pt->value;
            draw_pt.color = map_pt->color;
          }
        }
      }

      if (!draw_pt.has_value) {  // value not set so nothing to draw
        continue;
      }
      if (!show_mins && draw_pt.value < min_value) {
        continue;
      }
      if (!show_maxs && draw_pt.value > max_value) {
        continue;
      }

      painter.setPen(draw_pt.color, true);
      painter.setBrush(draw_pt.color);

      painter.drawRect(draw_pt.rect);

      if (show_numbers) {
        const int x = draw_pt.rect.xCenter();
        const int y = draw_pt.rect.yCenter();
        const Painter::Anchor text_anchor = Painter::Anchor::CENTER;
        const double text_rect_margin = 0.8;

        const std::string text = datasource_.formatValue(draw_pt.value, false);
        const odb::Rect text_bound
            = painter.stringBoundaries(x, y, text_anchor, text);
        bool draw = true;
        if (text_bound.dx() >= text_rect_margin * draw_pt.rect.dx()
            || text_bound.dy() >= text_rect_margin * draw_pt.rect.dy()) {
          // don't draw if text will be too small
          draw = false;
        }

        if (draw) {
          painter.setPen(Painter::white, true);
          painter.drawString(x, y, text_anchor, text);
        }
      }
    }
  }

  // legend
  if (datasource_.getShowLegend()) {
    std::vector<std::pair<int, std::string>> legend;
    for (const auto& [color_index, color_value] :
         datasource_.getLegendValues()) {
      legend.emplace_back(color_index,
                          datasource_.formatValue(color_value, true));
    }

    datasource_.getColorGenerator().drawLegend(painter, legend);
  }
}

std::string HeatMapRenderer::getSettingsGroupName()
{
  return groupname_prefix_ + datasource_.getSettingsGroupName();
}

Renderer::Settings HeatMapRenderer::getSettings()
{
  Renderer::Settings settings = Renderer::getSettings();
  for (const auto& [name, value] : datasource_.getSettings()) {
    settings[datasource_prefix_ + name] = value;
  }
  return settings;
}

void HeatMapRenderer::setSettings(const Settings& settings)
{
  Renderer::setSettings(settings);
  Renderer::Settings data_settings;
  for (const auto& [name, value] : settings) {
    if (name.find(datasource_prefix_) == 0) {
      data_settings[name.substr(strlen(datasource_prefix_))] = value;
    }
  }
  datasource_.setSettings(data_settings);
}

//////////

RealValueHeatMapDataSource::RealValueHeatMapDataSource(
    utl::Logger* logger,
    const std::string& unit_suffix,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group),
      unit_suffix_(unit_suffix),
      units_(unit_suffix_),
      min_(0.0),
      max_(0.0),
      scale_(1.0)
{
}

void RealValueHeatMapDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
  determineMinMax(map);
  determineUnits();

  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      map_pt->value = convertValueToPercent(map_pt->value);
    }
  }

  min_ = roundData(min_ * scale_);
  max_ = roundData(max_ * scale_);

  // reset since all data has been scaled by the appropriate amount
  scale_ = 1.0;
}

double RealValueHeatMapDataSource::roundData(double value) const
{
  const double precision = 1000.0;
  return std::round(value * precision) / precision;
}

void RealValueHeatMapDataSource::determineMinMax(
    const HeatMapDataSource::Map& map)
{
  min_ = std::numeric_limits<double>::max();
  max_ = std::numeric_limits<double>::lowest();

  for (const auto& map_col : map) {
    for (const auto& map_pt : map_col) {
      min_ = std::min(min_, map_pt->value);
      max_ = std::max(max_, map_pt->value);
    }
  }
}

void RealValueHeatMapDataSource::determineUnits()
{
  const double range = getValueRange();
  if (range >= 1.0 || range == 0) {
    units_ = "";
    scale_ = 1.0;
  } else if (range > 1e-3) {
    units_ = "m";
    scale_ = 1e3;
  } else if (range > 1e-6) {
    units_ = "μ";
    scale_ = 1e6;
  } else if (range > 1e-9) {
    units_ = "n";
    scale_ = 1e9;
  } else if (range > 1e-12) {
    units_ = "p";
    scale_ = 1e12;
  } else {
    units_ = "f";
    scale_ = 1e15;
  }

  units_ += unit_suffix_;
}

std::string RealValueHeatMapDataSource::formatValue(double value,
                                                    bool legend) const
{
  int digits = legend ? 3 : 2;

  QString text;
  text.setNum(convertPercentToValue(value), 'f', digits);
  if (legend) {
    text += QString::fromStdString(getValueUnits());
  }
  return text.toStdString();
}

std::string RealValueHeatMapDataSource::getValueUnits() const
{
  return units_;
}

double RealValueHeatMapDataSource::getValueRange() const
{
  double range = max_ - min_;
  if (range == 0.0) {
    range = 1.0;  // dummy numbers until drops has been populated
  }
  return range;
}

double RealValueHeatMapDataSource::convertValueToPercent(double value) const
{
  const double range = getValueRange();
  const double offset = min_;

  return roundData(100.0 * (value - offset) / range);
}

double RealValueHeatMapDataSource::convertPercentToValue(double percent) const
{
  const double range = getValueRange();
  const double offset = min_;

  return roundData(percent * range / 100.0 + offset);
}

double RealValueHeatMapDataSource::getDisplayRangeIncrement() const
{
  return getValueRange() / 100.0;
}

///////////////////////////

GlobalRoutingDataSource::GlobalRoutingDataSource(
    utl::Logger* logger,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group)
{
}

std::pair<double, double> GlobalRoutingDataSource::getReportableXYGrid() const
{
  if (getBlock() == nullptr) {
    return {default_grid_, default_grid_};
  }

  auto* gCellGrid = getBlock()->getGCellGrid();
  if (gCellGrid == nullptr) {
    return {default_grid_, default_grid_};
  }

  auto grid_mode = [gCellGrid](int num_grids,
                               void (odb::dbGCellGrid::*get_grid)(
                                   int, int&, int&, int&)) -> int {
    std::map<int, int> grid_pitch_count;
    for (int i = 0; i < num_grids; i++) {
      int origin, count, step;
      (gCellGrid->*get_grid)(i, origin, count, step);
      grid_pitch_count[step] += count;
    }

    if (grid_pitch_count.empty()) {
      return default_grid_;
    }

    auto mode = grid_pitch_count.begin();
    for (auto check_mode = grid_pitch_count.begin();
         check_mode != grid_pitch_count.end();
         check_mode++) {
      if (mode->second < check_mode->second) {
        mode = check_mode;
      }
    }
    return mode->first;
  };

  const double x_grid = grid_mode(gCellGrid->getNumGridPatternsX(),
                                  &odb::dbGCellGrid::getGridPatternX);
  const double y_grid = grid_mode(gCellGrid->getNumGridPatternsY(),
                                  &odb::dbGCellGrid::getGridPatternY);

  const double dbus = getBlock()->getDbUnitsPerMicron();
  return {x_grid / dbus, y_grid / dbus};
}

double GlobalRoutingDataSource::getGridXSize() const
{
  const auto& [x, y] = getReportableXYGrid();
  return x;
}

double GlobalRoutingDataSource::getGridYSize() const
{
  const auto& [x, y] = getReportableXYGrid();
  return y;
}

void GlobalRoutingDataSource::populateXYGrid()
{
  if (getBlock() == nullptr) {
    HeatMapDataSource::populateXYGrid();
    return;
  }

  auto* gCellGrid = getBlock()->getGCellGrid();
  if (gCellGrid == nullptr) {
    HeatMapDataSource::populateXYGrid();
    return;
  }

  std::vector<int> gcell_xgrid, gcell_ygrid;
  gCellGrid->getGridX(gcell_xgrid);
  gCellGrid->getGridY(gcell_ygrid);

  const auto die_area = getBlock()->getDieArea();
  gcell_xgrid.push_back(die_area.xMax());
  gcell_ygrid.push_back(die_area.yMax());

  setXYMapGrid(gcell_xgrid, gcell_ygrid);
}

}  // namespace gui
