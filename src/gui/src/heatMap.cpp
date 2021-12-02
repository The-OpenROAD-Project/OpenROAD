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

// https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
// https://gist.github.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a
const int HeatMapDataSource::turbo_srgb_count_ = 256;
const unsigned char HeatMapDataSource::turbo_srgb_bytes_[256][3] = {{48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3}};

HeatMapSetup::HeatMapSetup(HeatMapDataSource& source,
                           const QString& title,
                           bool use_dbu,
                           int dbu,
                           QWidget* parent) :
    QDialog(parent),
    source_(source),
    use_dbu_(use_dbu),
    dbu_(dbu),
    log_scale_(new QCheckBox(this)),
    show_numbers_(new QCheckBox(this)),
    show_legend_(new QCheckBox(this)),
    grid_x_size_(nullptr),
    grid_y_size_(nullptr),
    grid_x_size_dbu_(nullptr),
    grid_y_size_dbu_(nullptr),
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

  source_.makeAdditionalSetupOptions(this, form, [this]() { source_.redraw(); });

  form->addRow(tr("Log scale"), log_scale_);

  form->addRow(tr("Show numbers"), show_numbers_);

  form->addRow(tr("Show legend"), show_legend_);

  QHBoxLayout* grid_layout = new QHBoxLayout;
  if (!use_dbu_) {
    grid_x_size_ = new QDoubleSpinBox(this);
    grid_y_size_ = new QDoubleSpinBox(this);

    const QString grid_suffix(" \u03BCm"); // micro meters
    grid_x_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
    grid_x_size_->setSuffix(grid_suffix);
    grid_y_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
    grid_y_size_->setSuffix(grid_suffix);
    grid_layout->addWidget(new QLabel("X", this));
    grid_layout->addWidget(grid_x_size_);
    grid_layout->addWidget(new QLabel("Y", this));
    grid_layout->addWidget(grid_y_size_);
    if (!source_.canAdjustGrid()) {
      grid_x_size_->setEnabled(false);
      grid_y_size_->setEnabled(false);
    }
  } else {
    grid_x_size_dbu_ = new QSpinBox(this);
    grid_y_size_dbu_ = new QSpinBox(this);

    grid_x_size_dbu_->setRange(source_.getGridSizeMinimumValue() * dbu_, source_.getGridSizeMaximumValue() * dbu_);
    grid_y_size_dbu_->setRange(source_.getGridSizeMinimumValue() * dbu_, source_.getGridSizeMaximumValue() * dbu_);
    grid_layout->addWidget(new QLabel("X", this));
    grid_layout->addWidget(grid_x_size_dbu_);
    grid_layout->addWidget(new QLabel("Y", this));
    grid_layout->addWidget(grid_y_size_dbu_);
    if (!source_.canAdjustGrid()) {
      grid_x_size_dbu_->setEnabled(false);
      grid_y_size_dbu_->setEnabled(false);
    }
  }
  form->addRow(tr("Grid"), grid_layout);

  min_range_selector_->setDecimals(3);
  QHBoxLayout* min_grid = new QHBoxLayout;
  min_grid->addWidget(min_range_selector_);
  min_grid->addWidget(new QLabel("Show values below", this));
  min_grid->addWidget(show_mins_);
  form->addRow(tr("Minimum"), min_grid);

  max_range_selector_->setDecimals(3);
  QHBoxLayout* max_grid = new QHBoxLayout;
  max_grid->addWidget(max_range_selector_);
  max_grid->addWidget(new QLabel("Show values above", this));
  max_grid->addWidget(show_maxs_);
  form->addRow(tr("Maximum"), max_grid);

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

  if (!use_dbu_) {
    connect(grid_x_size_,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(updateGridSize()));
    connect(grid_y_size_,
            SIGNAL(valueChanged(double)),
            this,
            SLOT(updateGridSize()));
  } else {
    connect(grid_x_size_dbu_,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(updateGridSize()));
    connect(grid_y_size_dbu_,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(updateGridSize()));
  }

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

  min_range_selector_->blockSignals(true);
  min_range_selector_->setRange(source_.convertPercentToValue(source_.getDisplayRangeMinimumValue()),
                                source_.convertPercentToValue(source_.getDisplayRangeMaximumValue()));
  min_range_selector_->setValue(source_.convertPercentToValue(source_.getDisplayRangeMin()));
  min_range_selector_->blockSignals(false);
  min_range_selector_->setSuffix(" " + QString::fromStdString(source_.getValueUnits()));
  min_range_selector_->setSingleStep(source_.getDisplayRangeIncrement());

  show_maxs_->setCheckState(source_.getDrawAboveRangeMax() ? Qt::Checked : Qt::Unchecked);

  max_range_selector_->blockSignals(true);
  max_range_selector_->setRange(source_.convertPercentToValue(source_.getDisplayRangeMinimumValue()),
                                source_.convertPercentToValue(source_.getDisplayRangeMaximumValue()));
  max_range_selector_->setValue(source_.convertPercentToValue(source_.getDisplayRangeMax()));
  max_range_selector_->blockSignals(false);
  max_range_selector_->setSuffix(" " + QString::fromStdString(source_.getValueUnits()));
  max_range_selector_->setSingleStep(source_.getDisplayRangeIncrement());

  if (!use_dbu_) {
    grid_x_size_->setValue(source_.getGridXSize());
    grid_y_size_->setValue(source_.getGridYSize());
  } else {
    grid_x_size_dbu_->setValue(source_.getGridXSize() * dbu_);
    grid_y_size_dbu_->setValue(source_.getGridYSize() * dbu_);
  }

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
  source_.setDisplayRange(source_.convertValueToPercent(min_range_selector_->value()),
                          source_.convertValueToPercent(max_range_selector_->value()));
  emit changed();
}

void HeatMapSetup::updateGridSize()
{
  if (!use_dbu_) {
    source_.setGridSizes(grid_x_size_->value(), grid_y_size_->value());
  } else {
    const double dbu = dbu_;
    source_.setGridSizes(grid_x_size_dbu_->value() / dbu, grid_x_size_dbu_->value() / dbu);
  }
  emit changed();
}

void HeatMapSetup::updateAlpha(int alpha)
{
  source_.setColorAlpha(alpha);
  emit changed();
}

///////////

HeatMapDataSource::HeatMapDataSource(const std::string& name,
                                     const std::string& short_name,
                                     const std::string& settings_group) :
    name_(name),
    short_name_(short_name),
    settings_group_(settings_group),
    destroy_map_(true),
    use_dbu_(false),
    populated_(false),
    colors_correct_(false),
    issue_redraw_(true),
    block_(nullptr),
    logger_(nullptr),
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
    renderer_(std::make_unique<HeatMapRenderer>(name_, *this)),
    setup_(nullptr)
{
}

void HeatMapDataSource::setLogger(utl::Logger* logger)
{
  logger_ = logger;
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
  if (setup_ == nullptr) {
    setup_ = new HeatMapSetup(*this, QString::fromStdString(name_), use_dbu_, block_->getDbUnitsPerMicron());

    QObject::connect(setup_, &QDialog::finished, &QObject::deleteLater);
    QObject::connect(setup_, &QObject::destroyed, [this]() { setup_ = nullptr; });

    setup_->show();
  } else {
    setup_->raise();
  }
}

const std::string HeatMapDataSource::formatValue(double value, bool legend) const
{
  QString text;
  text.setNum(value, 'f', 2);
  if (legend) {
    text += "%";
  }
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

    const double intersect_area = intersection.area();
    const double value_area = region.area();
    const double region_area = map_pt->rect.area();

    combineMapData(map_pt->value, value, value_area, intersect_area, region_area);

    markColorsInvalid();
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
      map_pt->color = getColor(0);

      Box bbox(Point(xMin, yMin), Point(xMax, yMax));
      map_.insert(std::make_pair(bbox, map_pt));
    }
  }
}

void HeatMapDataSource::destroyMap()
{
  destroy_map_ = true;

  redraw();
}

void HeatMapDataSource::ensureMap()
{
  if (destroy_map_) {
    map_.clear();
    destroy_map_ = false;
  }

  const bool build_map = map_.empty();
  if (build_map) {
    setupMap();
  }

  if (build_map || !isPopulated()) {
    populated_ = populateMap();

    if (isPopulated()) {
      correctMapScale(map_);
    }

    if (setup_ != nullptr) {
      // announce changes
      setIssueRedraw(false);
      setup_->changed();
      setIssueRedraw(true);
    }
  }

  if (!colors_correct_) {
    assignMapColors();
  }
}

void HeatMapDataSource::updateMapColors()
{
  // generate ranges for colors
  if (log_scale_) {
    double range = display_range_max_;
    if (display_range_min_ != 0.0) {
      range = display_range_max_ / display_range_min_;
    }

    const double step = std::pow(range, 1.0 / turbo_srgb_count_);

    for (int i = 0; i <= turbo_srgb_count_; i++) {
      double start = display_range_max_ / std::pow(step, i);
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

  markColorsInvalid();
}

void HeatMapDataSource::assignMapColors()
{
  for (auto& [bbox, map_pt] : map_) {
    map_pt->color = getColor(map_pt->value);
  }
  colors_correct_ = true;
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

void HeatMapDataSource::onShow()
{
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

HeatMapRenderer::HeatMapRenderer(const std::string& display_control, HeatMapDataSource& datasource) :
    display_control_(display_control),
    datasource_(datasource),
    first_paint_(true)
{
  addDisplayControl(display_control_,
                    false,
                    [this]() { datasource_.showSetup(); },
                    {""}); // mutually exclusive to all
}

void HeatMapRenderer::drawObjects(Painter& painter)
{
  if (!checkDisplayControl(display_control_)) {
    if (!first_paint_) {
      first_paint_ = true; // reset check
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

  for (const auto& [bbox, map_pt] : datasource_.getMap()) {
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
        const Painter::Anchor text_anchor = Painter::Anchor::CENTER;
        const double text_rect_margin = 0.8;

        const std::string text = datasource_.formatValue(map_pt->value, false);
        const odb::Rect text_bound = painter.stringBoundaries(x, y, text_anchor, text);
        bool draw = true;
        if (text_bound.dx() >= text_rect_margin * map_pt->rect.dx() ||
            text_bound.dy() >= text_rect_margin * map_pt->rect.dy()) {
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
    const double pixel_per_dbu = painter.getPixelsPerDBU();
    const int legend_offset = 20 / pixel_per_dbu; // 20 pixels
    const double box_height = 1 / pixel_per_dbu; // 1 pixels
    const int legend_width = 20 / pixel_per_dbu; // 20 pixels
    const int text_offset = 2 / pixel_per_dbu;
    const int legend_top = bounds.yMax() - legend_offset;
    const int legend_right = bounds.xMax() - legend_offset;
    const int legend_left = legend_right - legend_width;
    const Painter::Anchor key_anchor = Painter::Anchor::RIGHT_CENTER;

    odb::Rect legend_bounds(legend_left, legend_top, legend_right + text_offset, legend_top);

    const int color_count = datasource_.getColorsCount();
    const int color_incr = 2;

    std::vector<std::pair<odb::Point, std::string>> legend_key;
    for (const auto& legend_value : datasource_.getLegendValues()) {
      const int text_right = legend_left - text_offset;
      const int box_top = legend_top - ((color_count - legend_value.first) * box_height) / color_incr;

      const std::string text = datasource_.formatValue(legend_value.second, true);
      legend_key.push_back({{text_right, box_top}, text});
      const odb::Rect text_bounds = painter.stringBoundaries(text_right, box_top, key_anchor, text);

      legend_bounds.merge(text_bounds);
    }

    // draw background
    painter.setPen(Painter::dark_gray, true);
    painter.setBrush(Painter::dark_gray);
    painter.drawRect(legend_bounds, 10, 10);

    // draw color map
    double box_top = legend_top;
    for (int i = 0; i < color_count; i += color_incr) {
      const int color_idx = color_count - 1 - i;

      painter.setPen(datasource_.getColor(color_idx), true);
      painter.drawLine(odb::Point(legend_left, box_top), odb::Point(legend_right, box_top));
      box_top -= box_height;
    }

    // draw key values
    painter.setPen(Painter::black, true);
    painter.setBrush(Painter::transparent);
    for (const auto& [pt, text] : legend_key) {
      painter.drawString(pt.x(), pt.y(), key_anchor, text);
    }
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
    HeatMapDataSource("Routing Congestion", "Routing", "RoutingCongestion"),
    show_all_(true),
    show_hor_(false),
    show_ver_(false)
{
}

void RoutingCongestionDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout, const std::function<void(void)>& changed_callback)
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

void RoutingCongestionDataSource::combineMapData(double& base, const double new_data, const double data_area, const double intersection_area, const double rect_area)
{
  base += new_data * intersection_area / rect_area;
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

PlacementDensityDataSource::PlacementDensityDataSource() :
    HeatMapDataSource("Placement Density", "Placement", "PlacementDensity"),
    include_taps_(true),
    include_filler_(false),
    include_io_(false)
{
}

bool PlacementDensityDataSource::populateMap()
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

void PlacementDensityDataSource::combineMapData(double& base, const double new_data, const double data_area, const double intersection_area, const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

void PlacementDensityDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout, const std::function<void(void)>& changed_callback)
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

const Renderer::Settings PlacementDensityDataSource::getSettings() const
{
  auto settings = HeatMapDataSource::getSettings();

  settings["Taps"] = include_taps_;
  settings["Filler"] = include_filler_;
  settings["IO"] = include_io_;

  return settings;
}

void PlacementDensityDataSource::setSettings(const Renderer::Settings& settings)
{
  HeatMapDataSource::setSettings(settings);

  Renderer::setSetting<bool>(settings, "Taps", include_taps_);
  Renderer::setSetting<bool>(settings, "Filler", include_filler_);
  Renderer::setSetting<bool>(settings, "IO", include_io_);
}

void PlacementDensityDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void PlacementDensityDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void PlacementDensityDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstPlacementStatusBefore(odb::dbInst*, const odb::dbPlacementStatus&)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPreMoveInst(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

////////////

PowerDensityDataSource::PowerDensityDataSource() :
    HeatMapDataSource("Power Density", "Power", "PowerDensity"),
    sta_(nullptr),
    include_internal_(true),
    include_leakage_(true),
    include_switching_(true),
    min_power_(0.0),
    max_power_(0.0),
    units_("W")
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

void PowerDensityDataSource::combineMapData(double& base, const double new_data, const double data_area, const double intersection_area, const double rect_area)
{
  base += (new_data / data_area) * intersection_area;
}

void PowerDensityDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
  min_power_ = std::numeric_limits<double>::max();
  max_power_ = std::numeric_limits<double>::min();

  for (const auto& [bbox, map_pt] : map) {
    min_power_ = std::min(min_power_, map_pt->value);
    max_power_ = std::max(max_power_, map_pt->value);
  }

  auto round_data = [](double value, double scale) -> double {
    const double precision = 1000.0;
    double new_value = value * scale;
    return std::round(new_value * precision) / precision;
  };

  double scale;
  determineUnits(units_, scale);
  max_power_ = round_data(max_power_, scale);
  min_power_ = round_data(min_power_, scale);

  for (auto& [bbox, map_pt] : map) {
    map_pt->value = convertValueToPercent(round_data(map_pt->value, scale));
  }
}

void PowerDensityDataSource::determineUnits(std::string& text, double& scale) const
{
  if (max_power_ > 1 || max_power_ == 0.0) {
    text = "W";
    scale = 1.0;
  } else if (max_power_ > 1e-3) {
    text = "mW";
    scale = 1e3;
  } else if (max_power_ > 1e-6) {
    text = "\u03BCW"; // micro W
    scale = 1e6;
  } else if (max_power_ > 1e-9) {
    text = "nW";
    scale = 1e9;
  } else {
    text = "pW";
    scale = 1e12;
  }
}

const std::string PowerDensityDataSource::formatValue(double value, bool legend) const
{
  int digits = legend ? 3 : 2;

  QString text;
  text.setNum(convertPercentToValue(value), 'f', digits);
  if (legend) {
    text += QString::fromStdString(getValueUnits());
  }
  return text.toStdString();
}

const std::string PowerDensityDataSource::getValueUnits() const
{
  return units_;
}

double PowerDensityDataSource::getValueRange() const
{
  double range = max_power_ - min_power_;
  if (range == 0.0) {
    range = 1.0; // dummy numbers until power has been populated
  }
  return range;
}

double PowerDensityDataSource::convertValueToPercent(double value) const
{
  const double range = getValueRange();
  const double offset = min_power_;

  return 100.0 * (value - offset) / range;
}

double PowerDensityDataSource::convertPercentToValue(double percent) const
{
  const double range = getValueRange();
  const double offset = min_power_;

  return percent * range / 100.0 + offset;
}

double PowerDensityDataSource::getDisplayRangeIncrement() const
{
  return getValueRange() / 100.0;
}

void PowerDensityDataSource::makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout, const std::function<void(void)>& changed_callback)
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
                   [this, changed_callback](int value) {
                     include_internal_ = value == Qt::Checked;
                     destroyMap();
                     changed_callback();
                   });

  QObject::connect(switching,
                   &QCheckBox::stateChanged,
                   [this, changed_callback](int value) {
                     include_switching_ = value == Qt::Checked;
                     destroyMap();
                     changed_callback();
                   });

  QObject::connect(leakage,
                   &QCheckBox::stateChanged,
                   [this, changed_callback](int value) {
                     include_leakage_ = value == Qt::Checked;
                     destroyMap();
                     changed_callback();
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
