// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "boost/multi_array.hpp"
#include "gui/gui.h"
#include "odb/db.h"

namespace odb {
class dbBlock;
class dbChip;
class Rect;
}  // namespace odb

namespace sta {
class dbSta;
class Scene;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace gui {
class HeatMapSourceRegistration;

class HeatMapDataSource
{
 public:
  struct MapSettingBoolean
  {
    std::string name;
    std::string label;
    std::function<bool()> getter;
    std::function<void(bool)> setter;
  };
  struct MapSettingMultiChoice
  {
    std::string name;
    std::string label;
    std::function<std::vector<std::string>()> choices;
    std::function<const std::string()> getter;
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

  bool useSelectedOnly() const { return use_selected_only_; }
  void setUseSelectedOnly(bool value)
  {
    if (use_selected_only_ != value) {
      use_selected_only_ = value;
      destroyMap();
      redraw();
    }
  }

  // Returns the label for the "use selected only" checkbox in the setup dialog.
  // An empty string means this heatmap does not support selection filtering
  // and the checkbox will not be shown.
  virtual std::string getSelectionFilterLabel() const { return ""; }

  void registerHeatMap();

  virtual void setChip(odb::dbChip* chip) { chip_ = chip; }
  void setUseDBU(bool use_dbu) { use_dbu_ = use_dbu; }
  bool getUseDBU() const { return use_dbu_; }

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
  odb::dbChip* getChip() const { return chip_; }
  odb::dbBlock* getBlock() const;
  double getDbuPerMicron() const;
  // map controls
  void update() { destroyMap(); }
  void ensureMap();
  void destroyMap();
  const Map& getMap() const { return map_; }
  MapView getMapView(const odb::Rect& bounds);
  std::vector<MapColor> getVisibleMap(const odb::Rect& bounds,
                                      double pixels_per_dbu,
                                      double min_pixels_per_bin = 2.0);
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
  void setRedrawCallback(std::function<void()> callback);
  void setSetupCallback(std::function<void()> callback);
  void setUnregisterCallback(std::function<void(HeatMapDataSource*)> callback);

 protected:
  void addBooleanSetting(const std::string& name,
                         const std::string& label,
                         const std::function<bool()>& getter,
                         const std::function<void(bool)>& setter);
  void addMultipleChoiceSetting(
      const std::string& name,
      const std::string& label,
      const std::function<std::vector<std::string>()>& choices,
      const std::function<std::string()>& getter,
      const std::function<void(std::string)>& setter);

  bool setupMap();
  void clearMap();
  virtual bool populateMap() = 0;
  void addToMap(const odb::Rect& region, double value);
  virtual void combineMapData(bool base_has_value,
                              double& base,
                              double new_data,
                              double data_area,
                              double intersection_area,
                              double rect_area)
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

  // Returns the set of selected dbInst* objects when use_selected_only_ is
  // enabled, or an empty set (meaning no filtering) when it is disabled.
  std::set<odb::dbInst*> getSelectedInsts() const;

 private:
  const std::string name_;
  const std::string short_name_;
  const std::string settings_group_;
  bool destroy_map_;
  bool use_dbu_;

  bool populated_;
  bool colors_correct_;
  bool issue_redraw_;

  odb::dbChip* chip_;
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
  bool use_selected_only_;

  Map map_;
  std::vector<int> map_x_grid_;
  std::vector<int> map_y_grid_;

  SpectrumGenerator color_generator_;

  std::vector<double> color_lower_bounds_;

  std::vector<MapSetting> settings_;

  std::function<void()> redraw_callback_;
  std::function<void()> setup_callback_;
  std::function<void(HeatMapDataSource*)> unregister_callback_;

  absl::Mutex ensure_mutex_;
};

class RealValueHeatMapDataSource : public HeatMapDataSource
{
 public:
  RealValueHeatMapDataSource(utl::Logger* logger,
                             const std::string& unit_suffix,
                             const std::string& name,
                             const std::string& short_name,
                             const std::string& settings_group = "");

  std::string formatValue(double value, bool legend) const override;
  std::string getValueUnits() const override;
  double convertValueToPercent(double value) const override;
  double convertPercentToValue(double percent) const override;
  double getDisplayRangeIncrement() const override;

  double getDisplayRangeMaximumValue() const override { return 100.0; }

 protected:
  void determineUnits();

  void correctMapScale(HeatMapDataSource::Map& map) override;
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
  static constexpr int kDefaultGrid = 10;

  std::pair<double, double> getReportableXYGrid() const;
};

class PowerDensityDataSource : public RealValueHeatMapDataSource
{
 public:
  PowerDensityDataSource(sta::dbSta* sta, utl::Logger* logger);

  odb::Rect getBounds() const override { return getBlock()->getCoreArea(); }

  std::string getSelectionFilterLabel() const override
  {
    return "Only use selected instances";
  }

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;

 private:
  sta::dbSta* sta_;

  bool include_internal_ = true;
  bool include_leakage_ = true;
  bool include_switching_ = true;

  std::string scene_;

  sta::Scene* getScene() const;
};

class HeatMapSourceRegistration
{
 public:
  using Factory = std::function<std::shared_ptr<HeatMapDataSource>()>;

  HeatMapSourceRegistration(std::string name,
                            std::string short_name,
                            std::string settings_group,
                            Factory factory);

  const std::string& getName() const { return name_; }
  const std::string& getShortName() const { return short_name_; }
  const std::string& getSettingsGroupName() const { return settings_group_; }

  std::shared_ptr<HeatMapDataSource> createInstance() const;
  void invalidateInstances() const;

 private:
  std::string name_;
  std::string short_name_;
  std::string settings_group_;
  Factory factory_;
  mutable std::vector<std::weak_ptr<HeatMapDataSource>> instances_;
  mutable std::mutex instances_mutex_;
};

using HeatMapSourceHandle = std::shared_ptr<HeatMapSourceRegistration>;

HeatMapSourceHandle registerHeatMapSource(
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group,
    const HeatMapSourceRegistration::Factory& factory);
const std::vector<HeatMapSourceHandle>& getRegisteredHeatMapSources();
HeatMapSourceHandle findRegisteredHeatMapSource(const std::string& short_name);
void registerBuiltinHeatMapSources(sta::dbSta* sta, utl::Logger* logger);

}  // namespace gui
