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

#include <array>
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

#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "utl/Logger.h"

namespace gui {
class HeatMapRenderer;
class HeatMapSetup;

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

  HeatMapDataSource(const std::string& name,
                    const std::string& short_name,
                    const std::string& settings_group = "");
  virtual ~HeatMapDataSource() {}

  void setBlock(odb::dbBlock* block) { block_ = block; }
  void setLogger(utl::Logger* logger);

  HeatMapRenderer* getRenderer() { return renderer_.get(); }

  const std::string& getName() const { return name_; }
  const std::string& getShortName() const { return short_name_; }

  // setup
  void showSetup();
  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) {}
  virtual const std::string formatValue(double value, bool legend) const;

  // settings
  const std::string& getSettingsGroupName() const { return settings_group_; }
  virtual const Renderer::Settings getSettings() const;
  virtual void setSettings(const Renderer::Settings& settings);

  void setDisplayRange(double min, double max);
  double getDisplayRangeMin() const { return display_range_min_; }
  double getDisplayRangeMax() const { return display_range_max_; }
  double getDisplayRangeMinimumValue() const { return 0.0; }
  double getDisplayRangeMaximumValue() const { return 100.0; }
  double getRealRangeMinimumValue() const;
  double getRealRangeMaximumValue() const;
  virtual double getDisplayRangeIncrement() const { return 1.0; }
  virtual const std::string getValueUnits() const { return "%"; }
  virtual double convertValueToPercent(double value) const { return value; }
  virtual double convertPercentToValue(double percent) const { return percent; }

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

  virtual bool canAdjustGrid() const { return true; }
  void setGridSizes(double x, double y);
  virtual double getGridXSize() const { return grid_x_size_; }
  virtual double getGridYSize() const { return grid_y_size_; }
  double getGridSizeMinimumValue() const { return 1.0; }
  double getGridSizeMaximumValue() const { return 100.0; }

  // map controls
  void ensureMap();
  void destroyMap();
  const Map& getMap() const { return map_; }
  bool isPopulated() const { return populated_; }

  const std::vector<std::pair<int, double>> getLegendValues() const;
  const Painter::Color getColor(double value) const;
  const Painter::Color getColor(int idx) const;
  int getColorsCount() const { return turbo_srgb_count_; }

  virtual void onShow();
  virtual void onHide();

 protected:
  odb::dbBlock* getBlock() const { return block_; }

  void setupMap();
  virtual bool populateMap() = 0;
  void addToMap(const odb::Rect& region, double value);
  virtual void combineMapData(double& base, const double new_data, const double region_ratio);
  virtual void correctMapScale(Map& map) {}
  void updateMapColors();
  void assignMapColors();
  void markColorsInvalid() { colors_correct_ = false; }

  virtual bool destroyMapOnNotVisible() const { return false; }

  template <typename T>
  T boundValue(T value, T min_value, T max_value)
  {
    return std::max(min_value , std::min(max_value, value));
  }

  void setIssueRedraw(bool state) { issue_redraw_ = state; }
  void redraw();

 private:
  const std::string name_;
  const std::string short_name_;
  const std::string settings_group_;
  bool populated_;
  bool colors_correct_;
  bool issue_redraw_;

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

  std::unique_ptr<HeatMapRenderer> renderer_;
  std::unique_ptr<HeatMapSetup> setup_;

  static const int turbo_srgb_count_;
  static const unsigned char turbo_srgb_bytes_[256][3];

  std::array<double, 256 + 1> color_lower_bounds_;

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

 private slots:
  void updateShowNumbers(int option);
  void updateShowLegend(int option);
  void updateShowMinRange(int show);
  void updateShowMaxRange(int show);
  void updateScale(int option);
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

  QPushButton* rebuild_;
  QPushButton* close_;
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

  void setLogger(utl::Logger* logger) { logger_ = logger; }

 private:
  std::string display_control_;
  HeatMapDataSource& datasource_;
  bool check_data_loaded_;
  utl::Logger* logger_;

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

  virtual bool canAdjustGrid() const override { return false; }
  virtual double getGridXSize() const override;
  virtual double getGridYSize() const override;

 protected:
  virtual bool populateMap() override;

 private:
  bool show_all_;
  bool show_hor_;
  bool show_ver_;

  static constexpr double default_grid_ = 10.0;
};

class PlacementDensityDataSource : public HeatMapDataSource, public odb::dbBlockCallBackObj
{
 public:
  PlacementDensityDataSource();
  ~PlacementDensityDataSource() {}

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout, const std::function<void(void)>& changed_callback) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

  virtual void onShow() override;
  virtual void onHide() override;

  // from dbBlockCallBackObj API
  virtual void inDbInstCreate(odb::dbInst*) override;
  virtual void inDbInstCreate(odb::dbInst*, odb::dbRegion*) override;
  virtual void inDbInstDestroy(odb::dbInst*) override;
  virtual void inDbInstPlacementStatusBefore(odb::dbInst*, const odb::dbPlacementStatus&) override;
  virtual void inDbInstSwapMasterBefore(odb::dbInst*, odb::dbMaster*) override;
  virtual void inDbInstSwapMasterAfter(odb::dbInst*) override;
  virtual void inDbPreMoveInst(odb::dbInst*) override;
  virtual void inDbPostMoveInst(odb::dbInst*) override;

 protected:
  virtual bool populateMap() override;

  virtual bool destroyMapOnNotVisible() const override { return true; }

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

  virtual const std::string formatValue(double value, bool legend) const override;
  virtual const std::string getValueUnits() const override;
  virtual double convertValueToPercent(double value) const override;
  virtual double convertPercentToValue(double percent) const override;
  virtual double getDisplayRangeIncrement() const override;

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

 protected:
  virtual bool populateMap() override;
  virtual void correctMapScale(HeatMapDataSource::Map& map) override;

 private:
  sta::dbSta* sta_;

  bool include_internal_;
  bool include_leakage_;
  bool include_switching_;

  double min_power_;
  double max_power_;
  std::string units_;

  void determineUnits(std::string& text, double& scale) const;
  double getValueRange() const;
};

}  // namespace gui
