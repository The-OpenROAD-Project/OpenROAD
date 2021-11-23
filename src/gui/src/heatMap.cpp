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

static Painter::Color combineColors(const Painter::Color& c0,
                                    const Painter::Color& c1,
                                    double ratio0 = 0.5)
{
  Painter::Color color;
  const double ratio1 = 1 - ratio0;
  color.r = ratio0 * c0.r + ratio1 * c1.r;
  color.g = ratio0 * c0.g + ratio1 * c1.g;
  color.b = ratio0 * c0.b + ratio1 * c1.b;
  color.a = ratio0 * c0.a + ratio1 * c1.a;
  return color;
}


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
    max_range_selector_(new QDoubleSpinBox(this)),
    alpha_selector_(new QSpinBox(this)),
    bands_selector_(new QSpinBox(this)),
    gradient_(new QLabel(this)),
    colors_list_(new QListWidget(this)),
    rebuild_(new QPushButton("Rebuild", this)),
    redraw_(new QPushButton("Repaint", this)),
    close_(new QPushButton("Close", this))
{
  setWindowTitle(title);

  QVBoxLayout* overall_layout = new QVBoxLayout;
  QFormLayout* form = new QFormLayout;
  source_.makeAdditionalSetupOptions(this, form);

  form->addRow(tr("Log scale"), log_scale_);

  form->addRow(tr("Show numbers"), show_numbers_);

  form->addRow(tr("Show legend"), show_legend_);

  grid_x_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
  grid_y_size_->setRange(source_.getGridSizeMinimumValue(), source_.getGridSizeMaximumValue());
  QGridLayout* grid_layout = new QGridLayout;
  grid_layout->addWidget(new QLabel("X", this), 0, 0);
  grid_layout->addWidget(grid_x_size_, 0, 1);
  grid_layout->addWidget(new QLabel("Y", this), 0, 2);
  grid_layout->addWidget(grid_y_size_, 0, 3);
  form->addRow(tr("Grid"), grid_layout);

  min_range_selector_->setRange(source_.getDisplayRangeMinimumValue(), source_.getDisplayRangeMaximumValue());
  min_range_selector_->setDecimals(3);
  max_range_selector_->setRange(source_.getDisplayRangeMinimumValue(), source_.getDisplayRangeMaximumValue());
  max_range_selector_->setDecimals(3);
  form->addRow(tr("Minimum %"), min_range_selector_);
  form->addRow(tr("Maximum %"), max_range_selector_);

  alpha_selector_->setRange(source_.getColorAlphaMinimum(), source_.getColorAlphaMaximum());
  form->addRow(tr("Color alpha"), alpha_selector_);

  bands_selector_->setRange(source_.getDisplayBandsMinimumCount(), source_.getDisplayBandsMaximumCount());
  form->addRow(tr("Color bins"), bands_selector_);

  gradient_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

  overall_layout->addLayout(form);
  overall_layout->addWidget(gradient_);
  overall_layout->addWidget(colors_list_);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->addWidget(rebuild_);
  buttons->addWidget(redraw_);
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

  connect(bands_selector_,
          SIGNAL(valueChanged(int)),
          this,
          SLOT(updateBands(int)));

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
  connect(redraw_,
          SIGNAL(pressed()),
          this,
          SIGNAL(apply()));
  connect(close_,
          SIGNAL(pressed()),
          this,
          SIGNAL(apply()));

  connect(close_,
          SIGNAL(pressed()),
          this,
          SLOT(accept()));
}

void HeatMapSetup::updateWidgets()
{
  min_range_selector_->setMaximum(source_.getDisplayRangeMax());
  max_range_selector_->setMinimum(source_.getDisplayRangeMin());

  min_range_selector_->setValue(source_.getDisplayRangeMin());
  max_range_selector_->setValue(source_.getDisplayRangeMax());

  grid_x_size_->setValue(source_.getGridXSize());
  grid_y_size_->setValue(source_.getGridYSize());

  alpha_selector_->setValue(source_.getColorAlpha());
  bands_selector_->setValue(source_.getDisplayBandsCount());

  log_scale_->setCheckState(source_.getLogScale() ? Qt::Checked : Qt::Unchecked);
  show_numbers_->setCheckState(source_.getShowNumbers() ? Qt::Checked : Qt::Unchecked);
  show_legend_->setCheckState(source_.getShowLegend() ? Qt::Checked : Qt::Unchecked);

  updateGradient();
  updateColorList();
}

void HeatMapSetup::destroyMap()
{
  source_.destroyMap();
  emit apply();
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
  emit apply();
}

void HeatMapSetup::updateShowLegend(int option)
{
  source_.setShowLegend(option == Qt::Checked);
  emit changed();
  emit apply();
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

void HeatMapSetup::updateBands(int count)
{
  source_.setDisplayBandCount(count);
  emit changed();
}

void HeatMapSetup::updateAlpha(int alpha)
{
  source_.setColorAlpha(alpha);
  emit changed();
}

void HeatMapSetup::updateGradient()
{
  QString gradient_style_sheet =
      "background-color: qlineargradient(x1:0 y1:0, x2:1 y2:0, ";

  std::vector<Painter::Color> colors;
  const auto& bands = source_.getDisplayBands();
  for (const auto& color_band : bands) {
    colors.push_back(color_band.lower_color);
  }
  colors.push_back(bands[bands.size() - 1].upper_color);

  const double step = 1.0 / (colors.size() - 1);
  double stop = 0.0;
  for (const auto& color : colors) {
    gradient_style_sheet += "stop:" + QString::number(stop) + " ";
    gradient_style_sheet += colorToQColor(color).name(QColor::HexArgb) + ", ";
    stop += step;
  }
  gradient_style_sheet.chop(2); // remove last ", "
  gradient_style_sheet += ");";
  gradient_->setStyleSheet(gradient_style_sheet);
}

void HeatMapSetup::updateColorList()
{
  colors_list_->clear();

  for (const auto& color_band : source_.getDisplayBands()) {
    std::string label = source_.formatValue(color_band.lower) +
                + " <= Usage < "
                + source_.formatValue(color_band.upper);
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(label));
    item->setBackground(QBrush(colorToQColor(color_band.center_color)));
    colors_list_->addItem(item);
  }
}

const QColor HeatMapSetup::colorToQColor(const Painter::Color& color)
{
  return QColor(color.r, color.g, color.b, color.a);
}

///////////

HeatMapDataSource::HeatMapDataSource(const std::string& setup_title,
                                     const std::string& settings_group) :
    setup_title_(setup_title),
    settings_group_(settings_group),
    block_(nullptr),
    grid_x_size_(10.0),
    grid_y_size_(10.0),
    display_range_min_(getDisplayRangeMinimumValue()),
    display_range_max_(getDisplayRangeMaximumValue()),
    color_alpha_(100),
    log_scale_(false),
    show_numbers_(false),
    show_legend_(false),
    map_(),
    display_bands_(),
    renderer_(std::make_unique<HeatMapRenderer>(setup_title, *this))
{
  setDisplayBandCount(getDisplayBandsMaximumCount());
}

void HeatMapDataSource::setColorAlpha(int alpha)
{
  color_alpha_ = boundValue<int>(alpha, getColorAlphaMinimum(), getColorAlphaMaximum());

  for (auto& color_band : display_bands_) {
    color_band.lower_color.a = alpha;
    color_band.upper_color.a = alpha;
    color_band.center_color.a = alpha;
  }

  updateMapColors();
}

void HeatMapDataSource::setDisplayRange(double min, double max)
{
  if (max < min) {
    std::swap(min, max);
  }

  display_range_min_ = boundValue<double>(min, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());
  display_range_max_ = boundValue<double>(max, getDisplayRangeMinimumValue(), getDisplayRangeMaximumValue());

  setDisplayBandCount(getDisplayBandsCount());
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

  setDisplayBandCount(getDisplayBandsCount());
}

void HeatMapDataSource::setShowNumbers(bool numbers)
{
  show_numbers_ = numbers;
}

void HeatMapDataSource::setShowLegend(bool legend)
{
  show_legend_ = legend;
}

std::vector<Painter::Color> HeatMapDataSource::getBandColors(int count)
{
  std::vector<Painter::Color> colors;

  colors.push_back(Painter::blue);
  if (count % 2 == 1) {
    colors.push_back(Painter::green);
  }
  if (count > 3) {
    colors.insert(colors.begin() + 1, Painter::cyan);
    colors.push_back(Painter::yellow);
  }
  colors.push_back(Painter::red);

  return colors;
}

void HeatMapDataSource::setDisplayBandCount(int bands)
{
  display_bands_.clear();

  std::vector<Painter::Color> colors_base = getBandColors(bands);
  bands = colors_base.size();

  // determine correct start and end points colors
  std::vector<std::tuple<Painter::Color, Painter::Color, Painter::Color>> colors;
  for (int i = 0; i < bands; i++) {
    Painter::Color start;
    if (i == 0) {
      start = colors_base[0];
    } else {
      start = std::get<2>(colors[i - 1]);
    }
    Painter::Color stop;
    if (i == bands - 1) {
      stop = colors_base[i];
    } else {
      stop = combineColors(colors_base[i], colors_base[i + 1]);
    }
    colors.push_back({start, colors_base[i], stop});
  }

  // generate ranges for colors
  if (log_scale_) {
    double range = display_range_max_ - display_range_min_;
    double step = std::pow(range, 1.0 / bands);
    double offset = display_range_min_;

    if (display_range_min_ != 0.0) {
      double decade_step = std::pow(display_range_max_ / display_range_min_, 1.0 / bands);

      if (decade_step > 1.1 * step) {
        step = decade_step;
        offset = 0.0;
        range = display_range_max_;
      }
    }

    std::vector<std::pair<double, double>> ranges;
    for (int i = 0; i < bands; i++) {
      double stop = range / std::pow(step, i) + offset;
      double start = range / std::pow(step, i + 1) + offset;
      if (i == bands - 1) {
        start = display_range_min_;
      }

      ranges.push_back({start, stop});
    }

    int idx = bands - 1;
    for (const auto& [lower_color, center_color, upper_color] : colors) {
      auto& [start, stop] = ranges[idx];
      display_bands_.push_back({start, lower_color, stop, upper_color, center_color});
      idx--;
    }
  } else {
    double start = display_range_min_;

    const double step = (display_range_max_ - start) / bands;

    double stop = start + step;
    for (const auto& [lower_color, center_color, upper_color] : colors) {
      display_bands_.push_back({start, lower_color, stop, upper_color, center_color});
      start = stop;
      stop += step;
    }
  }

  setColorAlpha(color_alpha_);

  updateMapColors();
}

const Painter::Color HeatMapDataSource::getColor(double value) const
{
  if (value <= display_range_min_) {
    return display_bands_[0].lower_color;
  } else if (value >= display_range_max_) {
    return display_bands_[display_bands_.size() - 1].upper_color;
  }

  int band_idx = 0;
  for (; band_idx < display_bands_.size(); band_idx++) {
    const auto& band = display_bands_[band_idx];
    if (value >= band.lower &&
        value < band.upper) {
      break;
    }
  }

  const ColorBand& band = display_bands_[band_idx];

  double band_percentage = 0.0;

  if (log_scale_) {
    if (value > 0.0) {
      // if lower is 0
      double lower_val = band.lower;
      if (lower_val == 0.0) {
        // this is always going to be the first band, so get ratio from set above
        const ColorBand& next_band = display_bands_[band_idx + 1];
        const double ratio = next_band.upper / next_band.lower;
        lower_val = band.upper / ratio;
      }

      if (lower_val <= value) {
        lower_val = std::log(lower_val);
        const double val = std::log(value);
        const double upper_val = std::log(band.upper);
        band_percentage = (val - lower_val) / (upper_val - lower_val);
      }
    }
  } else {
    band_percentage = (value - band.lower) / (band.upper - band.lower);
  }

  const Painter::Color& ll_color = band.lower_color;
  const Painter::Color& ul_color = band.upper_color;

  Painter::Color new_color = ll_color;
  new_color.r += std::round(band_percentage * (ul_color.r - ll_color.r));
  new_color.g += std::round(band_percentage * (ul_color.g - ll_color.g));
  new_color.b += std::round(band_percentage * (ul_color.b - ll_color.b));

  // limit rgb to 0 .. 255
  new_color.r = std::min(255, std::max(new_color.r, 0));
  new_color.g = std::min(255, std::max(new_color.g, 0));
  new_color.b = std::min(255, std::max(new_color.b, 0));

  return new_color;
}

void HeatMapDataSource::showSetup()
{
  HeatMapSetup dlg(*this,
                   QString::fromStdString(setup_title_));

  QObject::connect(&dlg, &HeatMapSetup::apply, [this]() { renderer_->redraw(); });

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
          {"ShowLegend", show_legend_},
          {"Bins", static_cast<int>(display_bands_.size())}};
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

  int bins = display_bands_.size();
  if (bins == 0) {
    bins = getDisplayBandsMaximumCount(); // default to max
  }
  Renderer::setSetting<int>(settings, "Bins", bins);
  setDisplayBandCount(bins);

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

  const int dx = grid_x_size_ * getBlock()->getDbUnitsPerMicron();
  const int dy = grid_y_size_ * getBlock()->getDbUnitsPerMicron();

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
}

void HeatMapDataSource::ensureMap()
{
  if (map_.empty()) {
    setupMap();

    populateMap();

    correctMapScale(map_);

    updateMapColors();
  }
}

void HeatMapDataSource::updateMapColors()
{
  for (auto& [bbox, map_pt] : map_) {
    map_pt->color = getColor(map_pt->value);
  }
}

void HeatMapDataSource::combineMapData(double& base, const double new_data, const double region_ratio)
{
  base += new_data * region_ratio;
}

///////////

HeatMapRenderer::HeatMapRenderer(const std::string& display_control, HeatMapDataSource& datasource) :
    display_control_(display_control),
    datasource_(datasource)
{
  addDisplayControl(display_control_,
                    false,
                    [this]() { datasource_.showSetup(); });
}

void HeatMapRenderer::drawObjects(Painter& painter)
{
  if (!checkDisplayControl(display_control_)) {
    return;
  }

  datasource_.ensureMap();

  const bool show_numbers = datasource_.getShowNumbers();

  const odb::Rect& bounds = painter.getBounds();
  for (const auto& [bbox, map_pt] : datasource_.getMap()) {
    if (map_pt->value == 0.0) {
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
    const int box_height = 20 / pixel_per_dbu; // 20 pixels
    const int legend_width = 20 / pixel_per_dbu; // 20 pixels
    const int legend_top = bounds.yMax() - legend_offset;
    const int legend_right = bounds.xMax() - legend_offset;
    const int legend_left = legend_right - legend_width;

    const auto& bands = datasource_.getDisplayBands();
    int box_top = legend_top;
    for (auto itr = bands.rbegin(); itr != bands.rend(); itr++) {
      const int box_bottom = box_top - box_height;
      const odb::Rect box(legend_left, box_bottom, legend_right, box_top);

      painter.setPen(itr->center_color, true);
      painter.setBrush(itr->center_color);
      painter.drawRect(box);

      painter.setPen(Painter::white, true);
      painter.drawString(legend_left, box_top, Painter::Anchor::RIGHT_CENTER, datasource_.formatValue(itr->upper));

      box_top = box_bottom;
   }

    painter.setPen(Painter::white, true);
    painter.drawString(legend_left, box_top, Painter::Anchor::RIGHT_CENTER, datasource_.formatValue(bands[0].lower));
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

}  // namespace gui
