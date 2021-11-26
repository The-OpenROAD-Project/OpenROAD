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

#include "heatMap.h"
#include "db_sta/dbNetwork.hh"

#include <QComboBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <QDebug>

namespace gui {

HeatMapSetup::HeatMapSetup(HeatMapDataSource& source,
                           const QString& title,
                           QWidget* parent) :
    QDialog(parent),
    source_(source),
    log_scale_(new QCheckBox(this)),
    show_numbers_(new QCheckBox(this)),
    show_legend_(new QCheckBox(this)),
    grid_x_size_(new QDoubleSpinBox(this)),
    grid_y_size_(new QDoubleSpinBox(this)),
    min_range_selector_(new QDoubleSpinBox(this)),
    show_mins_(new QCheckBox(this)),
    max_range_selector_(new QDoubleSpinBox(this)),
    show_maxs_(new QCheckBox(this)),
    alpha_selector_(new QSpinBox(this)),
    rebuild_(new QPushButton("Rebuild data", this)),
    close_(new QPushButton("Close", this))
{
  setWindowTitle(title);

  QVBoxLayout* overall_layout = new QVBoxLayout;
  QFormLayout* form = new QFormLayout;
  source_.makeAdditionalSetupOptions(this, form);

  form->addRow(tr("Log scale"), log_scale_);

  form->addRow(tr("Show numbers"), show_numbers_);

  form->addRow(tr("Show legend"), show_legend_);

  QString grid_suffix(" \u03BCm"); // micro meters
  grid_x_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
  grid_x_size_->setSuffix(grid_suffix);
  grid_y_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
  grid_y_size_->setSuffix(grid_suffix);
  QHBoxLayout* grid_layout = new QHBoxLayout;
  grid_layout->addWidget(new QLabel("X", this));
  grid_layout->addWidget(grid_x_size_);
  grid_layout->addWidget(new QLabel("Y", this));
  grid_layout->addWidget(grid_y_size_);
  form->addRow(tr("Grid"), grid_layout);
  if (!source_.canAdjustGrid()) {
    grid_x_size_->setEnabled(false);
    grid_y_size_->setEnabled(false);
  }

  min_range_selector_->setRange(source_.getDisplayRangeMinimumValue(), source_.getDisplayRangeMaximumValue());
  min_range_selector_->setDecimals(3);
  QHBoxLayout* min_grid = new QHBoxLayout;
  min_grid->addWidget(min_range_selector_);
  min_grid->addWidget(new QLabel("Show values below", this));
  min_grid->addWidget(show_mins_);
  form->addRow(tr("Minimum %"), min_grid);

  max_range_selector_->setRange(source_.getDisplayRangeMinimumValue(), source_.getDisplayRangeMaximumValue());
  max_range_selector_->setDecimals(3);
  QHBoxLayout* max_grid = new QHBoxLayout;
  max_grid->addWidget(max_range_selector_);
  max_grid->addWidget(new QLabel("Show values above", this));
  max_grid->addWidget(show_maxs_);
  form->addRow(tr("Maximum %"), max_grid);

  alpha_selector_->setRange(source_.getColorAlphaMinimum(), source_.getColorAlphaMaximum());
  form->addRow(tr("Color alpha"), alpha_selector_);

  overall_layout->addLayout(form);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->addWidget(rebuild_);
  buttons->addWidget(close_);

  overall_layout->addLayout(buttons);

  setLayout(overall_layout);

  updateWidgets();

  connect(log_scale_,
          SIGNAL(stateChanged(int)),
          this,
          SLOT(updateScale(int)));

  connect(grid_x_size_,
          SIGNAL(valueChanged(double)),
          this,
          SLOT(updateGridSize()));
  connect(grid_y_size_,
          SIGNAL(valueChanged(double)),
          this,
          SLOT(updateGridSize()));

  connect(show_numbers_,
          SIGNAL(stateChanged(int)),
          this,
          SLOT(updateShowNumbers(int)));

  connect(show_legend_,
          SIGNAL(stateChanged(int)),
          this,
          SLOT(updateShowLegend(int)));

  connect(min_range_selector_,
          SIGNAL(valueChanged(double)),
          this,
          SLOT(updateRange()));
  connect(max_range_selector_,
          SIGNAL(valueChanged(double)),
          this,
          SLOT(updateRange()));
  connect(show_mins_,
          SIGNAL(stateChanged(int)),
          this,
          SLOT(updateShowMinRange(int)));
  connect(show_maxs_,
          SIGNAL(stateChanged(int)),
          this,
          SLOT(updateShowMaxRange(int)));

  connect(alpha_selector_,
          SIGNAL(valueChanged(int)),
          this,
          SLOT(updateAlpha(int)));

  connect(this,
          SIGNAL(changed()),
          this,
          SLOT(updateWidgets()));

  connect(rebuild_,
          SIGNAL(pressed()),
          this,
          SLOT(destroyMap()));

  connect(close_,
          SIGNAL(pressed()),
          this,
          SLOT(accept()));
}

void HeatMapSetup::updateWidgets()
{
  show_mins_->setCheckState(source_.getDrawBelowRangeMin() ? Qt::Checked : Qt::Unchecked);

  min_range_selector_->setValue(source_.getDisplayRangeMin());
  max_range_selector_->setValue(source_.getDisplayRangeMax());
  show_maxs_->setCheckState(source_.getDrawAboveRangeMax() ? Qt::Checked : Qt::Unchecked);

  grid_x_size_->setValue(source_.getGridXSize());
  grid_y_size_->setValue(source_.getGridYSize());

  alpha_selector_->setValue(source_.getColorAlpha());

  log_scale_->setCheckState(source_.getLogScale() ? Qt::Checked : Qt::Unchecked);
  show_numbers_->setCheckState(source_.getShowNumbers() ? Qt::Checked : Qt::Unchecked);
  show_legend_->setCheckState(source_.getShowLegend() ? Qt::Checked : Qt::Unchecked);
}

void HeatMapSetup::destroyMap()
{
  source_.destroyMap();
}

void HeatMapSetup::updateScale(int option)
{
  source_.setLogScale(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowNumbers(int option)
{
  source_.setShowNumbers(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowLegend(int option)
{
  source_.setShowLegend(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowMinRange(int option)
{
  source_.setDrawBelowRangeMin(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowMaxRange(int option)
{
  source_.setDrawAboveRangeMax(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateRange()
{
  source_.setDisplayRange(min_range_selector_->value(), max_range_selector_->value());
  emit changed();
}

void HeatMapSetup::updateGridSize()
{
  source_.setGridSizes(grid_x_size_->value(), grid_y_size_->value());
  emit changed();
}

void HeatMapSetup::updateAlpha(int alpha)
{
  source_.setColorAlpha(alpha);
  emit changed();
}

///////////

HeatMapDataSource::HeatMapDataSource(const std::string& name,
                                     const std::string& settings_group) :
    name_(name),
    settings_group_(settings_group),
    populated_(false),
    issue_redraw_(true),
    block_(nullptr),
    grid_x_size_(10.0),
    grid_y_size_(10.0),
    display_range_min_(getDisplayRangeMinimumValue()),
    display_range_max_(getDisplayRangeMaximumValue()),
    draw_below_min_display_range_(false),
    draw_above_max_display_range_(true),
    color_alpha_(150),
    log_scale_(false),
    show_numbers_(false),
    show_legend_(false),
    map_(),
    renderer_(std::make_unique<HeatMapRenderer>(name_, *this))
{
}

void HeatMapDataSource::setLogger(utl::Logger* logger)
{
  renderer_->setLogger(logger);
}

void HeatMapDataSource::redraw()
{
  if (issue_redraw_) {
    renderer_->redraw();
  }
}

void HeatMapDataSource::setColorAlpha(int alpha)
{
  color_alpha_ = boundValue<int>(alpha, getColorAlphaMinimum(), getColorAlphaMaximum());
  updateMapColors();

  redraw();
}

void HeatMapDataSource::setDisplayRange(double min, double max)
{
  if (max < min) {
    std::swap(min, max);
  }

  display_range_min_ = boundValue<double>(min, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());
  display_range_max_ = boundValue<double>(max, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());

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
    grid_x_size_ = boundValue<double>(x, getGridSizeMinimumValue(), getGridSizeMaximumValue());
    changed = true;
  }
  if (grid_y_size_ != y) {
    grid_y_size_ = boundValue<double>(y, getGridSizeMinimumValue(), getGridSizeMaximumValue());
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

const Painter::Color HeatMapDataSource::getColor(int idx) const
{
  if (idx < 0) {
    idx = 0;
  } else if (idx >= turbo_srgb_count_) {
    idx = turbo_srgb_count_ - 1;
  }

  return Painter::Color(turbo_srgb_bytes_[idx][0],
                        turbo_srgb_bytes_[idx][1],
                        turbo_srgb_bytes_[idx][2],
                        color_alpha_);
}

const Painter::Color HeatMapDataSource::getColor(double value) const
{
  if (value <= display_range_min_) {
    return getColor(0);
  } else if (value >= display_range_max_) {
    return getColor(turbo_srgb_count_ - 1);
  }

  auto color_idx_itr = std::find_if(color_lower_bounds_.rbegin(), color_lower_bounds_.rend(), [value](const double other) {
    return other <= value;
  });

  if (color_idx_itr == color_lower_bounds_.rend()) {
    return getColor(0);
  }

  const int idx = turbo_srgb_count_ - std::distance(color_lower_bounds_.rbegin(), color_idx_itr);
  if (idx >= turbo_srgb_count_) {
    return getColor(turbo_srgb_count_ - 1);
  }

  if (log_scale_) {
    if (value <= 0.0) {
      return getColor(0);
    }
  }
  return getColor(idx);
}

void HeatMapDataSource::showSetup()
{
  HeatMapSetup dlg(*this,
                   QString::fromStdString(name_));

  dlg.exec();
}

const std::string HeatMapDataSource::formatValue(double value) const
{
  QString text;
  text.setNum(value, 'f', 2);
  text += "%";
  return text.toStdString();
}

const Renderer::Settings HeatMapDataSource::getSettings() const
{
  return {{"DisplayMin", display_range_min_},
          {"DisplayMax", display_range_max_},
          {"GridX", grid_x_size_},
          {"GridY", grid_y_size_},
          {"Alpha", color_alpha_},
          {"LogScale", log_scale_},
          {"ShowNumbers", show_numbers_},
          {"ShowLegend", show_legend_}};
}

void HeatMapDataSource::setSettings(const Renderer::Settings& settings)
{
  Renderer::setSetting<double>(settings, "DisplayMin", display_range_min_);
  Renderer::setSetting<double>(settings, "DisplayMax", display_range_max_);
  Renderer::setSetting<double>(settings, "GridX", grid_x_size_);
  Renderer::setSetting<double>(settings, "GridY", grid_y_size_);
  Renderer::setSetting<int>(settings, "Alpha", color_alpha_);
  Renderer::setSetting<bool>(settings, "LogScale", log_scale_);
  Renderer::setSetting<bool>(settings, "ShowNumbers", show_numbers_);
  Renderer::setSetting<bool>(settings, "ShowLegend", show_legend_);

  // only reapply bounded value settings
  setDisplayRange(display_range_min_, display_range_max_);
  setGridSizes(grid_x_size_, grid_y_size_);
  setColorAlpha(color_alpha_);
}

void HeatMapDataSource::addToMap(const odb::Rect& region, double value)
{
  Box query(Point(region.xMin(), region.yMin()), Point(region.xMax(), region.yMax()));
  for (auto it = map_.qbegin(bgi::intersects(query)); it != map_.qend(); it++) {
    auto* map_pt = it->second.get();
    odb::Rect intersection;
    map_pt->rect.intersection(region, intersection);

    const auto insersect_area = intersection.area();
    const double ratio = static_cast<double>(insersect_area) / map_pt->rect.area();

    combineMapData(map_pt->value, value, ratio);
  }
}

void HeatMapDataSource::setupMap()
{
  if (getBlock() == nullptr) {
    return;
  }

  const int dx = getGridXSize() * getBlock()->getDbUnitsPerMicron();
  const int dy = getGridYSize() * getBlock()->getDbUnitsPerMicron();

  odb::Rect bounds;
  getBlock()->getBBox()->getBox(bounds);

  const int x_grid = std::ceil(bounds.dx() / static_cast<double>(dx));
  const int y_grid = std::ceil(bounds.dy() / static_cast<double>(dy));

  for (int x = 0; x < x_grid; x++) {
    const int xMin = bounds.xMin() + x * dx;
    const int xMax = std::min(xMin + dx, bounds.xMax());

    for (int y = 0; y < y_grid; y++) {
      const int yMin = bounds.yMin() + y * dy;
      const int yMax = std::min(yMin + dy, bounds.yMax());

      auto map_pt = std::make_shared<MapColor>();
      map_pt->rect = odb::Rect(xMin, yMin, xMax, yMax);
      map_pt->value = 0.0;
      map_pt->color = Painter::transparent;

      Box bbox(Point(xMin, yMin), Point(xMax, yMax));
      map_.insert(std::make_pair(bbox, map_pt));
    }
  }
}

void HeatMapDataSource::destroyMap()
{
  map_.clear();

  redraw();
}

void HeatMapDataSource::ensureMap()
{
  if (map_.empty()) {
    setupMap();

    populated_ = populateMap();

    correctMapScale(map_);

    updateMapColors();
  }
}

void HeatMapDataSource::updateMapColors()
{
  // generate ranges for colors
  if (log_scale_) {
    double range = display_range_max_ - display_range_min_;
    double step = std::pow(range, 1.0 / turbo_srgb_count_);
    double offset = display_range_min_;

    if (display_range_min_ != 0.0) {
      const double decade_step = std::pow(display_range_max_ / display_range_min_, 1.0 / turbo_srgb_count_);

      if ((decade_step - 1) > 1.1 * (step - 1)) {
        step = decade_step;
        offset = 0.0;
        range = display_range_max_;
      }
    }

    for (int i = 0; i <= turbo_srgb_count_; i++) {
      double start = range / std::pow(step, i) + offset;
      if (i == turbo_srgb_count_) {
        start = display_range_min_;
      }
      color_lower_bounds_[turbo_srgb_count_ - i] = start;
    }
  } else {
    const double step = (display_range_max_ - display_range_min_) / turbo_srgb_count_;
    for (int i = 0; i <= turbo_srgb_count_; i++) {
      color_lower_bounds_[i] = display_range_min_ + i * step;
    }
  }

  for (auto& [bbox, map_pt] : map_) {
    map_pt->color = getColor(map_pt->value);
  }
}

void HeatMapDataSource::combineMapData(double& base, const double new_data, const double region_ratio)
{
  base += new_data * region_ratio;
}

double HeatMapDataSource::getRealRangeMinimumValue() const
{
  return color_lower_bounds_[0];
}

double HeatMapDataSource::getRealRangeMaximumValue() const
{
  return color_lower_bounds_[turbo_srgb_count_];
}

const std::vector<std::pair<int, double>> HeatMapDataSource::getLegendValues() const
{
  const int count = 6;
  std::vector<std::pair<int, double>> values;
  const double index_incr = static_cast<double>(turbo_srgb_count_) / (count - 1);
  const double linear_start = getRealRangeMinimumValue();
  const double linear_step = (getRealRangeMaximumValue() - linear_start) / (count - 1);
  for (int i = 0; i < count; i++) {
    int idx = std::round(i * index_incr);
    if (idx > turbo_srgb_count_) {
      idx = turbo_srgb_count_;
    }
    double value = color_lower_bounds_[idx];
    if (!log_scale_) {
      value = linear_step * i + linear_start;
    }

    values.push_back({idx, value});
  }
  return values;
}

///////////

HeatMapRenderer::HeatMapRenderer(const std::string& display_control, HeatMapDataSource& datasource) :
    display_control_(display_control),
    datasource_(datasource),
    check_data_loaded_(true),
    logger_(nullptr)
{
  addDisplayControl(display_control_,
                    false,
                    [this]() { datasource_.showSetup(); });
}

void HeatMapRenderer::drawObjects(Painter& painter)
{
  if (!checkDisplayControl(display_control_)) {
    check_data_loaded_ = true; // reset check
    return;
  }

  datasource_.ensureMap();

  if (check_data_loaded_) {
    // report warning
    if (!datasource_.isPopulated()) {
      logger_->warn(utl::GUI, 57, "Heat map \"{}\" has not been populated with data.", datasource_.getName());
    }

    check_data_loaded_ = false; // only report warning once
  }

  const bool show_numbers = datasource_.getShowNumbers();
  const double min_value = datasource_.getRealRangeMinimumValue();
  const double max_value = datasource_.getRealRangeMaximumValue();
  const bool show_mins = datasource_.getDrawBelowRangeMin();
  const bool show_maxs = datasource_.getDrawAboveRangeMax();

  const odb::Rect& bounds = painter.getBounds();
  for (const auto& [bbox, map_pt] : datasource_.getMap()) {
    if (map_pt->value == 0.0 || map_pt->color == Painter::transparent) {
      continue;
    }

    if (!show_mins && map_pt->value < min_value) {
      continue;
    }
    if (!show_maxs && map_pt->value > max_value) {
      continue;
    }

    if (bounds.overlaps(map_pt->rect)) {
      painter.setPen(map_pt->color, true);
      painter.setBrush(map_pt->color);

      painter.drawRect(map_pt->rect);

      if (show_numbers) {
        const int x = 0.5 * (map_pt->rect.xMin() + map_pt->rect.xMax());
        const int y = 0.5 * (map_pt->rect.yMin() + map_pt->rect.yMax());
        painter.setPen(Painter::white, true);
        painter.drawString(x, y, Painter::Anchor::CENTER, datasource_.formatValue(map_pt->value));
      }
    }
  }

  // legend
  if (datasource_.getShowLegend()) {
    const double pixel_per_dbu = painter.getPixelsPerDBU();
    const int legend_offset = 20 / pixel_per_dbu; // 20 pixels
    const double box_height = 1 / pixel_per_dbu; // 1 pixels
    const int legend_width = 20 / pixel_per_dbu; // 20 pixels
    const int text_offset = 2 / pixel_per_dbu;
    const int legend_top = bounds.yMax() - legend_offset;
    const int legend_right = bounds.xMax() - legend_offset;
    const int legend_left = legend_right - legend_width;

    const auto legend_values = datasource_.getLegendValues();
    auto legend_values_itr = legend_values.rbegin();
    double box_top = legend_top;
    const int color_count = datasource_.getColorsCount();
    const int color_incr = 2;
    for (int i = 0; i < color_count; i += color_incr) {
      const int color_idx = color_count - 1 - i;

      const Painter::Color color = datasource_.getColor(color_idx);

      painter.setPen(color, true);
      painter.drawLine(odb::Point(legend_left, box_top), odb::Point(legend_right, box_top));


      if (legend_values_itr != legend_values.rend() && color_idx <= legend_values_itr->first) {
        const int text_right = legend_left - text_offset;
        painter.setPen(Painter::white, true);
        painter.drawString(text_right, box_top, Painter::Anchor::RIGHT_CENTER, datasource_.formatValue(legend_values_itr->second));
        legend_values_itr++;
      }
      box_top -= box_height;
    }
    if (legend_values_itr != legend_values.rend()) {
      // didn't get the last legend value
      const int text_right = legend_left - (1.0 / pixel_per_dbu);
      painter.setPen(Painter::white, true);
      painter.drawString(text_right, box_top, Painter::Anchor::RIGHT_CENTER, datasource_.formatValue(legend_values_itr->second));
    }
    painter.setPen(Painter::white, true);
    painter.setBrush(Painter::transparent);
    painter.drawRect(odb::Rect(legend_left, box_top, legend_right, legend_top));
  }
}

const std::string HeatMapRenderer::getSettingsGroupName()
{
  return groupname_prefix_ + datasource_.getSettingsGroupName();
}

const Renderer::Settings HeatMapRenderer::getSettings()
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

////////////

RoutingCongestionDataSource::RoutingCongestionDataSource() :
    HeatMapDataSource("Routing Congestion", "RoutingCongestion"),
    show_all_(true),
    show_hor_(false),
    show_ver_(false)
{
}

void RoutingCongestionDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout)
{
  QComboBox* congestion_ = new QComboBox(parent);
  congestion_->addItems({"All", "Horizontal", "Vertical"});

  if (show_all_) {
    congestion_->setCurrentIndex(0);
  } else if (show_hor_) {
    congestion_->setCurrentIndex(1);
  } else if (show_ver_) {
    congestion_->setCurrentIndex(2);
  }

  layout->addRow("Congestion Layers", congestion_);

  QObject::connect(congestion_,
                   QOverload<int>::of(&QComboBox::currentIndexChanged),
                   [this](int value) {
                     show_all_ = value == 0;
                     show_hor_ = value == 1;
                     show_ver_ = value == 2;
                     destroyMap();
                   });
}

double RoutingCongestionDataSource::getGridXSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gcellgrid = getBlock()->getGCellGrid();
  if (gcellgrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gcellgrid->getGridX(grid);

  if (grid.size() < 2) {
    return default_grid_;
  } else {
    const double delta = grid[1] - grid[0];
    return delta / getBlock()->getDbUnitsPerMicron();
  }
}

double RoutingCongestionDataSource::getGridYSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gcellgrid = getBlock()->getGCellGrid();
  if (gcellgrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gcellgrid->getGridY(grid);

  if (grid.size() < 2) {
    return default_grid_;
  } else {
    const double delta = grid[1] - grid[0];
    return delta / getBlock()->getDbUnitsPerMicron();
  }
}

bool RoutingCongestionDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  auto* grid = getBlock()->getGCellGrid();
  if (grid == nullptr) {
    return false;
  }

  auto gcell_congestion_data = grid->getCongestionMap();
  if (gcell_congestion_data.empty()) {
    return false;
  }

  std::vector<int> x_grid, y_grid;
  grid->getGridX(x_grid);
  const uint x_grid_sz = x_grid.size();
  grid->getGridY(y_grid);
  const uint y_grid_sz = y_grid.size();

  for (const auto& [key, cong_data] : gcell_congestion_data) {
    const uint x_idx = key.first;
    const uint y_idx = key.second;

    if (x_idx + 1 >= x_grid_sz || y_idx + 1 >= y_grid_sz) {
      continue;
    }

    const odb::Rect gcell_rect(
        x_grid[x_idx], y_grid[y_idx], x_grid[x_idx + 1], y_grid[y_idx + 1]);

    const auto hor_capacity = cong_data.horizontal_capacity;
    const auto hor_usage = cong_data.horizontal_usage;
    const auto ver_capacity = cong_data.vertical_capacity;
    const auto ver_usage = cong_data.vertical_usage;

    //-1 indicates capacity is not well defined...
    const double hor_congestion
        = hor_capacity != 0 ? static_cast<double>(hor_usage) / hor_capacity : -1;
    const double ver_congestion
        = ver_capacity != 0 ? static_cast<double>(ver_usage) / ver_capacity : -1;

    double congestion = 0.0;
    if (show_all_) {
      congestion = std::max(hor_congestion, ver_congestion);
    } else if (show_hor_) {
      congestion = hor_congestion;
    } else {
      congestion = ver_congestion;
    }

    if (congestion < 0) {
      continue;
    }

    addToMap(gcell_rect, 100 * congestion);
  }

  return true;
}

const Renderer::Settings RoutingCongestionDataSource::getSettings() const
{
  auto settings = HeatMapDataSource::getSettings();

  settings["ShowAll"] = show_all_;
  settings["ShowHor"] = show_hor_;
  settings["ShowVer"] = show_ver_;

  return settings;
}

void RoutingCongestionDataSource::setSettings(const Renderer::Settings& settings)
{
  HeatMapDataSource::setSettings(settings);

  Renderer::setSetting<bool>(settings, "ShowAll", show_all_);
  Renderer::setSetting<bool>(settings, "ShowHor", show_hor_);
  Renderer::setSetting<bool>(settings, "ShowVer", show_ver_);
}

////////////

PlacementCongestionDataSource::PlacementCongestionDataSource() :
    HeatMapDataSource("Placement Congestion", "PlacementCongestion"),
    include_taps_(true),
    include_filler_(false),
    include_io_(false)
{
}

bool PlacementCongestionDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }
    if (!include_filler_ && inst->getMaster()->isFiller()) {
      continue;
    }
    if (!include_taps_ && (inst->getMaster()->getType() == odb::dbMasterType::CORE_WELLTAP || inst->getMaster()->isEndCap())) {
      continue;
    }
    if (!include_io_ && (inst->getMaster()->isPad() || inst->getMaster()->isCover())) {
      continue;
    }
    odb::Rect inst_box;
    inst->getBBox()->getBox(inst_box);

    addToMap(inst_box, 100.0);
  }

  return true;
}

void PlacementCongestionDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout)
{
  QCheckBox* taps = new QCheckBox(parent);
  taps->setCheckState(include_taps_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Include taps and endcaps", taps);

  QCheckBox* filler = new QCheckBox(parent);
  filler->setCheckState(include_filler_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Include fillers", filler);

  QCheckBox* io = new QCheckBox(parent);
  io->setCheckState(include_io_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Include IO", io);

  QObject::connect(taps,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_taps_ = value == Qt::Checked;
                     destroyMap();
                   });

  QObject::connect(filler,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_filler_ = value == Qt::Checked;
                     destroyMap();
                   });

  QObject::connect(io,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_io_ = value == Qt::Checked;
                     destroyMap();
                   });
}

const Renderer::Settings PlacementCongestionDataSource::getSettings() const
{
  auto settings = HeatMapDataSource::getSettings();

  settings["Taps"] = include_taps_;
  settings["Filler"] = include_filler_;
  settings["IO"] = include_io_;

  return settings;
}

void PlacementCongestionDataSource::setSettings(const Renderer::Settings& settings)
{
  HeatMapDataSource::setSettings(settings);

  Renderer::setSetting<bool>(settings, "Taps", include_taps_);
  Renderer::setSetting<bool>(settings, "Filler", include_filler_);
  Renderer::setSetting<bool>(settings, "IO", include_io_);
}

////////////

PowerDensityDataSource::PowerDensityDataSource() :
    HeatMapDataSource("Power Density", "PowerDensity"),
    sta_(nullptr),
    include_internal_(true),
    include_leakage_(true),
    include_switching_(true),
    min_power_(0.0),
    max_power_(0.0)
{
  setIssueRedraw(false); // disable during initial setup
  setLogScale(true);
  setIssueRedraw(true);
}

bool PowerDensityDataSource::populateMap()
{
  if (getBlock() == nullptr || sta_ == nullptr) {
    return false;
  }

  if (sta_->cmdNetwork() == nullptr) {
    return false;
  }

  auto corner = sta_->findCorner("default");
  auto* network = sta_->getDbNetwork();

  const bool include_all = include_internal_ && include_leakage_ && include_switching_;
  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }

    sta::PowerResult power;
    sta_->power(network->dbToSta(inst), corner, power);

    float pwr = 0.0;
    if (include_all) {
      pwr = power.total();
    } else {
      if (include_internal_) {
        pwr += power.internal();
      }
      if (include_leakage_) {
        pwr += power.switching();
      }
      if (include_switching_) {
        pwr += power.leakage();
      }
    }

    odb::Rect inst_box;
    inst->getBBox()->getBox(inst_box);

    addToMap(inst_box, pwr);
  }

  return true;
}

void PowerDensityDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
  min_power_ = std::numeric_limits<double>::max();
  max_power_ = std::numeric_limits<double>::min();

  for (const auto& [bbox, map_pt] : map) {
    min_power_ = std::min(min_power_, map_pt->value);
    max_power_ = std::max(max_power_, map_pt->value);
  }

  const double range = max_power_ - min_power_;
  const double offset = min_power_;
  for (auto& [bbox, map_pt] : map) {
    map_pt->value = 100 * (map_pt->value - offset) / range;
  }
}

const std::string PowerDensityDataSource::formatValue(double value) const
{
  double range = max_power_ - min_power_;
  double offset = min_power_;
  if (range == 0.0) {
    range = 1.0; // dummy numbers until power has been populated
  }

  QString units;
  if (max_power_ > 1 || max_power_ == 0.0) {
    units = "W";
  } else if (max_power_ > 1e-3) {
    units = "mW";
    range *= 1e3;
    offset *= 1e3;
  } else if (max_power_ > 1e-6) {
    units = "\u03BCW"; // micro W
    range *= 1e6;
    offset *= 1e6;
  } else if (max_power_ > 1e-9) {
    units = "nW";
    range *= 1e9;
    offset *= 1e9;
  } else {
    units = "pW";
    range *= 1e12;
    offset *= 1e12;
  }

  QString text;
  text.setNum((value / 100.0) * range + offset, 'f', 3);
  text += units;
  return text.toStdString();
}

void PowerDensityDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout)
{
  QCheckBox* internal = new QCheckBox(parent);
  internal->setCheckState(include_internal_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Internal power", internal);

  QCheckBox* switching = new QCheckBox(parent);
  switching->setCheckState(include_switching_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Switching power", switching);

  QCheckBox* leakage = new QCheckBox(parent);
  leakage->setCheckState(include_leakage_ ? Qt::Checked : Qt::Unchecked);
  layout->addRow("Leakage power", leakage);

  QObject::connect(internal,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_internal_ = value == Qt::Checked;
                     destroyMap();
                   });

  QObject::connect(switching,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_switching_ = value == Qt::Checked;
                     destroyMap();
                   });

  QObject::connect(leakage,
                   &QCheckBox::stateChanged,
                   [this](int value) {
                     include_leakage_ = value == Qt::Checked;
                     destroyMap();
                   });
}

const Renderer::Settings PowerDensityDataSource::getSettings() const
{
  auto settings = HeatMapDataSource::getSettings();

  settings["Internal"] = include_internal_;
  settings["Switching"] = include_switching_;
  settings["Leakage"] = include_leakage_;

  return settings;
}

void PowerDensityDataSource::setSettings(const Renderer::Settings& settings)
{
  HeatMapDataSource::setSettings(settings);

  Renderer::setSetting<bool>(settings, "Internal", include_internal_);
  Renderer::setSetting<bool>(settings, "Switching", include_switching_);
  Renderer::setSetting<bool>(settings, "Leakage", include_leakage_);
}

}  // namespace gui
