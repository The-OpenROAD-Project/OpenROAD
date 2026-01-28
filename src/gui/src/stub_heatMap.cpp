// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include <functional>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "gui/heatMap.h"

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
      display_range_max_(0.0),
      draw_below_min_display_range_(false),
      draw_above_max_display_range_(true),
      color_alpha_(150),
      log_scale_(false),
      reverse_log_(false),
      show_numbers_(false),
      show_legend_(false),
      renderer_(nullptr),
      setup_(nullptr),
      color_generator_(SpectrumGenerator(100.0))
{
}

HeatMapDataSource::~HeatMapDataSource() = default;

void HeatMapDataSource::registerHeatMap()
{
}

void HeatMapDataSource::setLogScale(bool scale)
{
}

void HeatMapDataSource::redraw()
{
}

void HeatMapDataSource::addBooleanSetting(
    const std::string& name,
    const std::string& label,
    const std::function<bool()>& getter,
    const std::function<void(bool)>& setter)
{
}

void HeatMapDataSource::addMultipleChoiceSetting(
    const std::string& name,
    const std::string& label,
    const std::function<std::vector<std::string>()>& choices,
    const std::function<std::string()>& getter,
    const std::function<void(std::string)>& setter)
{
}

void HeatMapDataSource::addToMap(const odb::Rect& region, double value)
{
}

void HeatMapDataSource::destroyMap()
{
}

void HeatMapDataSource::onShow()
{
}

void HeatMapDataSource::onHide()
{
}

std::string HeatMapDataSource::formatValue(double value, bool legend) const
{
  return "";
}

Renderer::Settings HeatMapDataSource::getSettings() const
{
  return {};
}

void HeatMapDataSource::setSettings(const Renderer::Settings& settings)
{
}

odb::Rect HeatMapDataSource::getBounds() const
{
  return {};
}

void HeatMapDataSource::populateXYGrid()
{
}

void HeatMapDataSource::setXYMapGrid(const std::vector<int>& x_grid,
                                     const std::vector<int>& y_grid)
{
}

//////////

RealValueHeatMapDataSource::RealValueHeatMapDataSource(
    utl::Logger* logger,
    const std::string& unit_suffix,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group),
      min_(0.0),
      max_(0.0),
      scale_(0.0)
{
}

std::string RealValueHeatMapDataSource::formatValue(double value,
                                                    bool legend) const
{
  return "";
}

double RealValueHeatMapDataSource::getDisplayRangeIncrement() const
{
  return HeatMapDataSource::getDisplayRangeIncrement();
}

std::string RealValueHeatMapDataSource::getValueUnits() const
{
  return units_;
}

double RealValueHeatMapDataSource::convertValueToPercent(double value) const
{
  return value;
}

double RealValueHeatMapDataSource::convertPercentToValue(double percent) const
{
  return percent;
}

void RealValueHeatMapDataSource::correctMapScale(HeatMapDataSource::Map& map)
{
}

void RealValueHeatMapDataSource::determineMinMax(
    const HeatMapDataSource::Map& map)
{
}

//////////

GlobalRoutingDataSource::GlobalRoutingDataSource(
    utl::Logger* logger,
    const std::string& name,
    const std::string& short_name,
    const std::string& settings_group)
    : HeatMapDataSource(logger, name, short_name, settings_group)
{
}

void GlobalRoutingDataSource::populateXYGrid()
{
}

double GlobalRoutingDataSource::getGridXSize() const
{
  return HeatMapDataSource::getGridXSize();
}

double GlobalRoutingDataSource::getGridYSize() const
{
  return HeatMapDataSource::getGridYSize();
}

//////////

HeatMapRenderer::HeatMapRenderer(HeatMapDataSource& datasource)
    : datasource_(datasource), first_paint_(true)
{
}

void HeatMapRenderer::drawObjects(Painter& painter)
{
}

std::string HeatMapRenderer::getSettingsGroupName()
{
  return kGroupnamePrefix;
}

Renderer::Settings HeatMapRenderer::getSettings()
{
  return {};
}

void HeatMapRenderer::setSettings(const Renderer::Settings& settings)
{
}

//////////
PowerDensityDataSource::PowerDensityDataSource(sta::dbSta* sta,
                                               utl::Logger* logger)
    : gui::RealValueHeatMapDataSource(logger,
                                      "W",
                                      "Power Density",
                                      "Power",
                                      "PowerDensity"),
      sta_(sta)
{
}

bool PowerDensityDataSource::populateMap()
{
  return false;
}

void PowerDensityDataSource::combineMapData(bool base_has_value,
                                            double& base,
                                            const double new_data,
                                            const double data_area,
                                            const double intersection_area,
                                            const double rect_area)
{
}

sta::Scene* PowerDensityDataSource::getCorner() const
{
  return nullptr;
}

}  // namespace gui
