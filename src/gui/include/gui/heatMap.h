///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <array>
#include <boost/multi_array.hpp>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <variant>
#include <vector>

#include "gui/gui.h"

namespace odb {
class dbBlock;
class Rect;
}  // namespace odb

namespace utl {
class Logger;
}  // namespace utl

namespace gui {
class HeatMapRenderer;
class HeatMapSetup;

class HeatMapDataSource
{
 public:
  struct MapSettingBoolean
  {
    std::string name;
    std::string label;
    std::function<bool(void)> getter;
    std::function<void(bool)> setter;
  };
  struct MapSettingMultiChoice
  {
    std::string name;
    std::string label;
    std::function<std::vector<std::string>(void)> choices;
    std::function<const std::string(void)> getter;
    std::function<void(const std::string&)> setter;
  };

  struct MapColor
  {
    odb::Rect rect;
    bool has_value;
    double value;
    Painter::Color color;
  };

  using Map = boost::multi_array<std::shared_ptr<MapColor>, 2>;
  using MapView = Map::array_view<2>::type;
  using MapSetting = std::variant<MapSettingBoolean, MapSettingMultiChoice>;

  HeatMapDataSource(utl::Logger* logger,
                    const std::string& name,
                    const std::string& short_name,
                    const std::string& settings_group = "");
  virtual ~HeatMapDataSource();

  void registerHeatMap();

  virtual void setBlock(odb::dbBlock* block) { block_ = block; }
  void setUseDBU(bool use_dbu) { use_dbu_ = use_dbu; }

  HeatMapRenderer* getRenderer() { return renderer_.get(); }

  const std::string& getName() const { return name_; }
  const std::string& getShortName() const { return short_name_; }

  utl::Logger* getLogger() const { return logger_; }

  void dumpToFile(const std::string& file);

  // setup
  void showSetup();
  virtual std::string formatValue(double value, bool legend) const;
  const std::vector<MapSetting>& getMapSettings() const { return settings_; }

  // settings
  const std::string& getSettingsGroupName() const { return settings_group_; }
  virtual Renderer::Settings getSettings() const;
  virtual void setSettings(const Renderer::Settings& settings);

  void setDisplayRange(double min, double max);
  double getDisplayRangeMin() const { return display_range_min_; }
  double getDisplayRangeMax() const { return display_range_max_; }
  virtual double getDisplayRangeMinimumValue() const { return 0.0; }
  virtual double getDisplayRangeMaximumValue() const
  {
    return std::numeric_limits<double>::max();
  }
  double getRealRangeMinimumValue() const;
  double getRealRangeMaximumValue() const;
  virtual double getDisplayRangeIncrement() const { return 1.0; }
  virtual std::string getValueUnits() const { return "%"; }
  virtual double convertValueToPercent(double value) const { return value; }
  virtual double convertPercentToValue(double percent) const { return percent; }

  void setDrawBelowRangeMin(bool show);
  bool getDrawBelowRangeMin() const { return draw_below_min_display_range_; }
  void setDrawAboveRangeMax(bool show);
  bool getDrawAboveRangeMax() const { return draw_above_max_display_range_; }

  void setLogScale(bool scale);
  bool getLogScale() const { return log_scale_; }
  void setReverseLogScale(bool reverse_log);
  bool getReverseLogScale() const { return reverse_log_; }

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
  virtual double getGridSizeMinimumValue() const { return 1.0; }
  virtual double getGridSizeMaximumValue() const { return 100.0; }
  // The default implementation uses the block's bounds
  virtual odb::Rect getBounds() const;
  odb::dbBlock* getBlock() const { return block_; }

  // map controls
  void update() { destroyMap(); }
  void ensureMap();
  void destroyMap();
  const Map& getMap() const { return map_; }
  MapView getMapView(const odb::Rect& bounds);
  bool isPopulated() const { return populated_; }

  bool hasData() const;

  std::vector<std::pair<int, double>> getLegendValues() const;
  Painter::Color getColor(double value) const;
  const SpectrumGenerator& getColorGenerator() const
  {
    return color_generator_;
  }

  virtual void onShow();
  virtual void onHide();

  void redraw();

 protected:
  void addBooleanSetting(const std::string& name,
                         const std::string& label,
                         const std::function<bool(void)>& getter,
                         const std::function<void(bool)>& setter);
  void addMultipleChoiceSetting(
      const std::string& name,
      const std::string& label,
      const std::function<std::vector<std::string>(void)>& choices,
      const std::function<std::string(void)>& getter,
      const std::function<void(std::string)>& setter);

  void setupMap();
  void clearMap();
  virtual bool populateMap() = 0;
  void addToMap(const odb::Rect& region, double value);
  virtual void combineMapData(bool base_has_value,
                              double& base,
                              const double new_data,
                              const double data_area,
                              const double intersection_area,
                              const double rect_area)
      = 0;
  virtual void correctMapScale(Map& map) {}
  void updateMapColors();
  void assignMapColors();
  void markColorsInvalid() { colors_correct_ = false; }

  virtual void populateXYGrid();
  void setXYMapGrid(const std::vector<int>& x_grid,
                    const std::vector<int>& y_grid);

  virtual bool destroyMapOnNotVisible() const { return false; }

  template <typename T>
  T boundValue(T value, T min_value, T max_value)
  {
    return std::max(min_value, std::min(max_value, value));
  }

  void setIssueRedraw(bool state) { issue_redraw_ = state; }

 private:
  const std::string name_;
  const std::string short_name_;
  const std::string settings_group_;
  bool destroy_map_;
  bool use_dbu_;

  bool populated_;
  bool colors_correct_;
  bool issue_redraw_;

  odb::dbBlock* block_;
  utl::Logger* logger_;
  double grid_x_size_;
  double grid_y_size_;
  double display_range_min_;
  double display_range_max_;
  bool draw_below_min_display_range_;
  bool draw_above_max_display_range_;
  int color_alpha_;
  bool log_scale_;
  bool reverse_log_;
  bool show_numbers_;
  bool show_legend_;

  Map map_;
  std::vector<int> map_x_grid_;
  std::vector<int> map_y_grid_;

  std::unique_ptr<HeatMapRenderer> renderer_;
  HeatMapSetup* setup_;

  SpectrumGenerator color_generator_;

  std::vector<double> color_lower_bounds_;

  std::vector<MapSetting> settings_;

  std::mutex ensure_mutex_;
};

class HeatMapRenderer : public Renderer
{
 public:
  HeatMapRenderer(HeatMapDataSource& datasource);

  virtual const char* getDisplayControlGroupName() override
  {
    return "Heat Maps";
  }

  virtual void drawObjects(Painter& painter) override;

  virtual std::string getSettingsGroupName() override;
  virtual Settings getSettings() override;
  virtual void setSettings(const Settings& settings) override;

 private:
  HeatMapDataSource& datasource_;
  bool first_paint_;

  static constexpr char datasource_prefix_[] = "data#";
  static constexpr char groupname_prefix_[] = "HeatMap#";
};

class RealValueHeatMapDataSource : public HeatMapDataSource
{
 public:
  RealValueHeatMapDataSource(utl::Logger* logger,
                             const std::string& unit_suffix,
                             const std::string& name,
                             const std::string& short_name,
                             const std::string& settings_group = "");
  ~RealValueHeatMapDataSource() {}

  virtual std::string formatValue(double value, bool legend) const override;
  virtual std::string getValueUnits() const override;
  virtual double convertValueToPercent(double value) const override;
  virtual double convertPercentToValue(double percent) const override;
  virtual double getDisplayRangeIncrement() const override;

  virtual double getDisplayRangeMaximumValue() const override { return 100.0; }

 protected:
  void determineUnits();

  virtual void correctMapScale(HeatMapDataSource::Map& map) override;
  virtual void determineMinMax(const HeatMapDataSource::Map& map);

  void setMinValue(double value) { min_ = value; }
  double getMinValue() const { return min_; }
  void setMaxValue(double value) { max_ = value; }
  double getMaxValue() const { return max_; }

  double roundData(double value) const;

 private:
  const std::string unit_suffix_;
  std::string units_;
  double min_;
  double max_;
  double scale_;

  double getValueRange() const;
};

class GlobalRoutingDataSource : public HeatMapDataSource
{
 public:
  GlobalRoutingDataSource(utl::Logger* logger,
                          const std::string& name,
                          const std::string& short_name,
                          const std::string& settings_group = "");
  bool canAdjustGrid() const override { return false; }
  double getGridXSize() const override;
  double getGridYSize() const override;

 protected:
  void populateXYGrid() override;

 private:
  static constexpr int default_grid_ = 10;

  std::pair<double, double> getReportableXYGrid() const;
};

}  // namespace gui
