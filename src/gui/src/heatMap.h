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

#include "db.h"
#include "db_sta/dbSta.hh"
#include "gui/gui.h"
#include "utl/Logger.h"

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

 protected:
  odb::dbBlock* getBlock() const { return block_; }

  void setupMap();
  virtual bool populateMap() = 0;
  void addToMap(const odb::Rect& region, double value);
  virtual void combineMapData(double& base, const double new_data, const double region_ratio);
  virtual void correctMapScale(Map& map) {}
  void updateMapColors();

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

  // https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
  // https://gist.github.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a
  constexpr static int turbo_srgb_count_ = 256;
  constexpr static unsigned char turbo_srgb_bytes_[256][3] = {{48,18,59},{50,21,67},{51,24,74},{52,27,81},{53,30,88},{54,33,95},{55,36,102},{56,39,109},{57,42,115},{58,45,121},{59,47,128},{60,50,134},{61,53,139},{62,56,145},{63,59,151},{63,62,156},{64,64,162},{65,67,167},{65,70,172},{66,73,177},{66,75,181},{67,78,186},{68,81,191},{68,84,195},{68,86,199},{69,89,203},{69,92,207},{69,94,211},{70,97,214},{70,100,218},{70,102,221},{70,105,224},{70,107,227},{71,110,230},{71,113,233},{71,115,235},{71,118,238},{71,120,240},{71,123,242},{70,125,244},{70,128,246},{70,130,248},{70,133,250},{70,135,251},{69,138,252},{69,140,253},{68,143,254},{67,145,254},{66,148,255},{65,150,255},{64,153,255},{62,155,254},{61,158,254},{59,160,253},{58,163,252},{56,165,251},{55,168,250},{53,171,248},{51,173,247},{49,175,245},{47,178,244},{46,180,242},{44,183,240},{42,185,238},{40,188,235},{39,190,233},{37,192,231},{35,195,228},{34,197,226},{32,199,223},{31,201,221},{30,203,218},{28,205,216},{27,208,213},{26,210,210},{26,212,208},{25,213,205},{24,215,202},{24,217,200},{24,219,197},{24,221,194},{24,222,192},{24,224,189},{25,226,187},{25,227,185},{26,228,182},{28,230,180},{29,231,178},{31,233,175},{32,234,172},{34,235,170},{37,236,167},{39,238,164},{42,239,161},{44,240,158},{47,241,155},{50,242,152},{53,243,148},{56,244,145},{60,245,142},{63,246,138},{67,247,135},{70,248,132},{74,248,128},{78,249,125},{82,250,122},{85,250,118},{89,251,115},{93,252,111},{97,252,108},{101,253,105},{105,253,102},{109,254,98},{113,254,95},{117,254,92},{121,254,89},{125,255,86},{128,255,83},{132,255,81},{136,255,78},{139,255,75},{143,255,73},{146,255,71},{150,254,68},{153,254,66},{156,254,64},{159,253,63},{161,253,61},{164,252,60},{167,252,58},{169,251,57},{172,251,56},{175,250,55},{177,249,54},{180,248,54},{183,247,53},{185,246,53},{188,245,52},{190,244,52},{193,243,52},{195,241,52},{198,240,52},{200,239,52},{203,237,52},{205,236,52},{208,234,52},{210,233,53},{212,231,53},{215,229,53},{217,228,54},{219,226,54},{221,224,55},{223,223,55},{225,221,55},{227,219,56},{229,217,56},{231,215,57},{233,213,57},{235,211,57},{236,209,58},{238,207,58},{239,205,58},{241,203,58},{242,201,58},{244,199,58},{245,197,58},{246,195,58},{247,193,58},{248,190,57},{249,188,57},{250,186,57},{251,184,56},{251,182,55},{252,179,54},{252,177,54},{253,174,53},{253,172,52},{254,169,51},{254,167,50},{254,164,49},{254,161,48},{254,158,47},{254,155,45},{254,153,44},{254,150,43},{254,147,42},{254,144,41},{253,141,39},{253,138,38},{252,135,37},{252,132,35},{251,129,34},{251,126,33},{250,123,31},{249,120,30},{249,117,29},{248,114,28},{247,111,26},{246,108,25},{245,105,24},{244,102,23},{243,99,21},{242,96,20},{241,93,19},{240,91,18},{239,88,17},{237,85,16},{236,83,15},{235,80,14},{234,78,13},{232,75,12},{231,73,12},{229,71,11},{228,69,10},{226,67,10},{225,65,9},{223,63,8},{221,61,8},{220,59,7},{218,57,7},{216,55,6},{214,53,6},{212,51,5},{210,49,5},{208,47,5},{206,45,4},{204,43,4},{202,42,4},{200,40,3},{197,38,3},{195,37,3},{193,35,2},{190,33,2},{188,32,2},{185,30,2},{183,29,2},{180,27,1},{178,26,1},{175,24,1},{172,23,1},{169,22,1},{167,20,1},{164,19,1},{161,18,1},{158,16,1},{155,15,1},{152,14,1},{149,13,1},{146,11,1},{142,10,1},{139,9,2},{136,8,2},{133,7,2},{129,6,2},{126,5,2},{122,4,3}};

  std::array<double, turbo_srgb_count_ + 1> color_lower_bounds_;
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

class PlacementCongestionDataSource : public HeatMapDataSource
{
 public:
  PlacementCongestionDataSource();
  ~PlacementCongestionDataSource() {}

  virtual void makeAdditionalSetupOptions(QWidget* parent, QFormLayout* layout) override;

  virtual const Renderer::Settings getSettings() const override;
  virtual void setSettings(const Renderer::Settings& settings) override;

 protected:
  virtual bool populateMap() override;

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
};

}  // namespace gui
