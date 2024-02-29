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
      map_(),
      renderer_(nullptr),
      setup_(nullptr),
      color_generator_(SpectrumGenerator(100.0))
{
}

HeatMapDataSource::~HeatMapDataSource()
{
}

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
    const std::function<bool(void)>& getter,
    const std::function<void(bool)>& setter)
{
}

void HeatMapDataSource::addMultipleChoiceSetting(
    const std::string& name,
    const std::string& label,
    const std::function<std::vector<std::string>(void)>& choices,
    const std::function<std::string(void)>& getter,
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
      unit_suffix_(),
      units_(),
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

}  // namespace gui
