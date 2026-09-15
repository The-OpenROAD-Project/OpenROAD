// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "gui/gui.h"

namespace web {

// A gui::Chart implementation that accumulates points in memory so the
// web viewer can serialize them to the client.  Created via a factory
// installed on gui::Gui (see WebViewerHook::installChartFactory).
class WebChart : public gui::Chart
{
 public:
  struct Point
  {
    double x;
    std::vector<double> ys;
  };

  struct Marker
  {
    double x;
    gui::Painter::Color color;
  };

  WebChart(std::string name,
           std::string x_label,
           std::vector<std::string> y_labels);
  ~WebChart() override = default;

  const std::string& name() const { return name_; }
  const std::string& xLabel() const { return x_label_; }
  const std::vector<std::string>& yLabels() const { return y_labels_; }

  // Snapshot accessors — take the mutex internally and return copies so
  // the caller can serialize without racing with placer updates.
  std::string xAxisFormat() const;
  std::vector<std::string> yAxisFormats() const;
  std::vector<std::optional<double>> yAxisMins() const;
  std::vector<Point> points() const;
  std::vector<Marker> markers() const;

  // --- gui::Chart overrides ---

  void setXAxisFormat(const std::string& format) override;
  void setYAxisFormats(const std::vector<std::string>& formats) override;
  void setYAxisMin(const std::vector<std::optional<double>>& mins) override;
  void addPoint(double x, const std::vector<double>& ys) override;
  void clearPoints() override;
  void addVerticalMarker(double x, const gui::Painter::Color& color) override;

 private:
  const std::string name_;
  const std::string x_label_;
  const std::vector<std::string> y_labels_;

  mutable std::mutex mutex_;
  std::string x_axis_format_;
  std::vector<std::string> y_axis_formats_;
  std::vector<std::optional<double>> y_axis_mins_;
  std::vector<Point> points_;
  std::vector<Marker> markers_;
};

}  // namespace web
