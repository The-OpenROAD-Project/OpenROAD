///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include <memory>
#include <string>
#include <vector>

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include "db.h"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"

namespace gui {
class HeatMapRenderer;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class HeatMapDataSource
{
 public:
  struct MapColor {
    odb::Rect rect;
    double value;
    Painter::Color color;
  };
  struct ColorBand {
    double lower;
    Painter::Color lower_color;
    double upper;
    Painter::Color upper_color;
    Painter::Color center_color;
  };
  using Point = bg::model::d2::point_xy<int, bg::cs::cartesian>;
  using Box = bg::model::box<Point>;
  using Map = bgi::rtree<std::pair<Box, std::shared_ptr<MapColor>>, bgi::quadratic<16>>;

  HeatMapDataSource(const std::string& setup_title,
                    const std::string& settings_group = "");
  virtual ~HeatMapDataSource() {}

  void setBlock(odb::dbBlock* block) { block_ = block; }

  HeatMapRenderer* getRenderer() { return renderer_.get(); }

  // setup
  void showSetup();
  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) {}
  virtual const std::string formatValue(double value) const;

  // settings
  const std::string& getSettingsGroupName() const { return settings_group_; }
  virtual const Renderer::Settings getSettings() const;
  virtual void setSettings(const Renderer::Settings& settings);

  const std::vector<ColorBand>& getDisplayBands() const { return display_bands_; }
  void setDisplayBandCount(int bands);
  int getDisplayBandsCount() const { return display_bands_.size(); }
  int getDisplayBandsMinimumCount() const { return 2; }
  int getDisplayBandsMaximumCount() const { return 5; }

  void setDisplayRange(double min, double max);
  double getDisplayRangeMin() const { return display_range_min_; }
  double getDisplayRangeMax() const { return display_range_max_; }
  double getDisplayRangeMinimumValue() const { return 0.0; }
  double getDisplayRangeMaximumValue() const { return 100.0; }
  double getRealRangeMinimumValue() const;
  double getRealRangeMaximumValue() const;

  void setDrawBelowRangeMin(bool value);
  bool getDrawBelowRangeMin() const { return draw_below_min_display_range_; }
  void setDrawAboveRangeMax(bool value);
  bool getDrawAboveRangeMax() const { return draw_above_max_display_range_; }

  void setLogScale(bool scale);
  bool getLogScale() const { return log_scale_; }

  void setShowNumbers(bool numbers);
  bool getShowNumbers() const { return show_numbers_; }

  void setShowLegend(bool legend);
  bool getShowLegend() const { return show_legend_; }

  void setColorAlpha(int alpha);
  int getColorAlpha() const { return color_alpha_; }
  int getColorAlphaMinimum() const { return 0; }
  int getColorAlphaMaximum() const { return 255; }

  void setGridSizes(double x, double y);
  double getGridXSize() const { return grid_x_size_; }
  double getGridYSize() const { return grid_y_size_; }
  double getGridSizeMinimumValue() const { return 1.0; }
  double getGridSizeMaximumValue() const { return 100.0; }

  // map controls
  void ensureMap();
  void destroyMap();
  const Map& getMap() const { return map_; }

 protected:
  odb::dbBlock* getBlock() const { return block_; }

  void setupMap();
  virtual void populateMap() = 0;
  void addToMap(const odb::Rect& region, double value);
  virtual void combineMapData(double& base, const double new_data, const double region_ratio);
  virtual void correctMapScale(Map& map) {}
  void updateMapColors();

  template <typename T>
  T boundValue(T value, T min_value, T max_value)
  {
    return std::max(min_value , std::min(max_value, value));
  }

 private:
  const std::string setup_title_;
  const std::string settings_group_;

  odb::dbBlock* block_;
  double grid_x_size_;
  double grid_y_size_;
  double display_range_min_;
  double display_range_max_;
  bool draw_below_min_display_range_;
  bool draw_above_max_display_range_;
  int color_alpha_;
  bool log_scale_;
  bool show_numbers_;
  bool show_legend_;

  Map map_;

  std::vector<ColorBand> display_bands_;

  std::unique_ptr<HeatMapRenderer> renderer_;

  std::vector<Painter::Color> getBandColors(int count);

  const Painter::Color getColor(double value) const;
};

class HeatMapSetup : public QDialog
{
  Q_OBJECT
 public:
  HeatMapSetup(HeatMapDataSource& source,
               const QString& title,
               QWidget* parent = nullptr);

 signals:
  void changed();
  void apply();

 private slots:
  void updateShowNumbers(int option);
  void updateShowLegend(int option);
  void updateShowMinRange(int show);
  void updateShowMaxRange(int show);
  void updateScale(int option);
  void updateBands(int count);
  void updateAlpha(int alpha);
  void updateRange();
  void updateGridSize();
  void updateWidgets();

  void destroyMap();

 private:
  HeatMapDataSource& source_;

  QCheckBox* log_scale_;
  QCheckBox* show_numbers_;
  QCheckBox* show_legend_;

  QDoubleSpinBox* grid_x_size_;
  QDoubleSpinBox* grid_y_size_;

  QDoubleSpinBox* min_range_selector_;
  QCheckBox* show_mins_;
  QDoubleSpinBox* max_range_selector_;
  QCheckBox* show_maxs_;

  QSpinBox* alpha_selector_;

  QSpinBox* bands_selector_;

  QLabel* gradient_;
  QListWidget* colors_list_;

  QPushButton* rebuild_;
  QPushButton* redraw_;
  QPushButton* close_;

  const QColor colorToQColor(const Painter::Color& color);
  void updateGradient();
  void updateColorList();
};

class HeatMapRenderer : public Renderer
{
 public:
  HeatMapRenderer(const std::string& display_control, HeatMapDataSource& datasource);

  virtual const char* getDisplayControlGroupName() override
  {
    return "Heat Maps";
  }

  virtual void drawObjects(Painter& painter) override;

  virtual const std::string getSettingsGroupName() override;
  virtual const Settings getSettings() override;
  virtual void setSettings(const Settings& settings) override;

 private:
  std::string display_control_;
  HeatMapDataSource& datasource_;

  static constexpr char datasource_prefix_[] = "data#";
  static constexpr char groupname_prefix_[] = "HeatMap#";
};

class RoutingCongestionDataSource : public HeatMapDataSource
{
 public:
  RoutingCongestionDataSource();
  ~RoutingCongestionDataSource() {}

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

 protected:
  virtual void populateMap() override;

 private:
  bool show_all_;
  bool show_hor_;
  bool show_ver_;
};

class PlacementCongestionDataSource : public HeatMapDataSource
{
 public:
  PlacementCongestionDataSource();
  ~PlacementCongestionDataSource() {}

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

 protected:
  virtual void populateMap() override;

 private:
  bool include_taps_;
  bool include_filler_;
  bool include_io_;
};

class PowerDensityDataSource : public HeatMapDataSource
{
 public:
  PowerDensityDataSource();
  ~PowerDensityDataSource() {}

  void setSTA(sta::dbSta* sta) { sta_ = sta; }

  virtual const std::string formatValue(double value) const override;

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

 protected:
  virtual void populateMap() override;
  virtual void correctMapScale(HeatMapDataSource::Map& map) override;

 private:
  sta::dbSta* sta_;

  bool include_internal_;
  bool include_leakage_;
  bool include_switching_;

  double min_power_;
  double max_power_;
};

}  // namespace gui
