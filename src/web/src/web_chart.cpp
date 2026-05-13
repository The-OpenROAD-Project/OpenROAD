// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "web_chart.h"

#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"

namespace web {

WebChart::WebChart(std::string name,
                   std::string x_label,
                   std::vector<std::string> y_labels)
    : name_(std::move(name)),
      x_label_(std::move(x_label)),
      y_labels_(std::move(y_labels))
{
}

std::string WebChart::xAxisFormat() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return x_axis_format_;
}

std::vector<std::string> WebChart::yAxisFormats() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return y_axis_formats_;
}

std::vector<std::optional<double>> WebChart::yAxisMins() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return y_axis_mins_;
}

std::vector<WebChart::Point> WebChart::points() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return points_;
}

std::vector<WebChart::Marker> WebChart::markers() const
{
  std::lock_guard<std::mutex> lock(mutex_);
  return markers_;
}

void WebChart::setXAxisFormat(const std::string& format)
{
  std::lock_guard<std::mutex> lock(mutex_);
  x_axis_format_ = format;
}

void WebChart::setYAxisFormats(const std::vector<std::string>& formats)
{
  std::lock_guard<std::mutex> lock(mutex_);
  y_axis_formats_ = formats;
}

void WebChart::setYAxisMin(const std::vector<std::optional<double>>& mins)
{
  std::lock_guard<std::mutex> lock(mutex_);
  y_axis_mins_ = mins;
}

void WebChart::addPoint(double x, const std::vector<double>& ys)
{
  std::lock_guard<std::mutex> lock(mutex_);
  points_.push_back({x, ys});
}

void WebChart::clearPoints()
{
  std::lock_guard<std::mutex> lock(mutex_);
  points_.clear();
  markers_.clear();
}

void WebChart::addVerticalMarker(double x, const gui::Painter::Color& color)
{
  std::lock_guard<std::mutex> lock(mutex_);
  markers_.push_back({x, color});
}

}  // namespace web
