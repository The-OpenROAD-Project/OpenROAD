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

#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QtCharts>
#include <memory>

#include "gui/gui.h"
#include "staGuiInterface.h"

namespace sta {
class Pin;
class dbSta;
class Clock;
}  // namespace sta

namespace gui {

struct SlackHistogramData
{
  StaPins constrained_pins;
  std::set<sta::Clock*> clocks;
};

struct Buckets
{
  bool areEmpty() { return positive.empty() && negative.empty(); }

  std::deque<std::vector<const sta::Pin*>> positive;
  std::deque<std::vector<const sta::Pin*>> negative;
};

class HistogramView : public QChartView
{
  Q_OBJECT

 public:
  HistogramView(QChart* chart, QWidget* parent);

  virtual void mousePressEvent(QMouseEvent* event) override;

 signals:
  void barIndex(int bar_index);
};

class ChartsWidget : public QDockWidget
{
  Q_OBJECT

 public:
  enum Mode
  {
    SELECT,
    SLACK_HISTOGRAM
  };

  ChartsWidget(QWidget* parent = nullptr);
  void setSTA(sta::dbSta* sta);
  void setLogger(utl::Logger* logger) { logger_ = logger; }

  void setMode(Mode mode);

 signals:
  void endPointsToReport(const std::set<const sta::Pin*>& report_pins,
                         const std::string& path_group_name);

 private slots:
  void changeMode();
  void updatePathGroupMenuIndexes();
  void changePathGroupFilter();
  void showToolTip(bool is_hovering, int bar_index);
  void emitEndPointsInBucket(int bar_index);

 private:
  void setSlackHistogram();
  void setSlackHistogramLayout();
  void setModeMenu();
  void setBucketInterval();
  void setBucketInterval(float bucket_interval)
  {
    bucket_interval_ = bucket_interval;
  }
  void setDecimalPrecision(int precision_count)
  {
    precision_count_ = precision_count;
  }
  void setClocks(const std::set<sta::Clock*>& clocks) { clocks_ = clocks; }

  SlackHistogramData fetchSlackHistogramData();
  void removeUnconstrainedPinsAndSetLimits(StaPins& end_points);
  void setLimits(const EndPointSlackMap& end_point_to_slack);

  void populateBuckets(StaPins* end_points,
                       EndPointSlackMap* end_point_to_slack);
  std::pair<QBarSet*, QBarSet*> createBarSets();
  void populateBarSets(QBarSet& neg_set, QBarSet& pos_set);

  void setVisualConfig();

  int computeSnapBucketInterval(float exact_interval);
  float computeSnapBucketDecimalInterval(float minimum_interval);
  int computeNumberofBuckets(int bucket_interval,
                             float max_slack,
                             float min_slack);

  void setXAxisConfig(int all_bars_count);
  void setXAxisTitle();
  void setYAxisConfig();
  int computeYInterval(int largest_slack_count);
  int computeMaxYSnap(int largest_slack_count);
  int computeNumberOfDigits(int value);
  int computeFirstDigit(int value, int digits);

  void clearChart();

  utl::Logger* logger_;
  sta::dbSta* sta_;
  std::unique_ptr<STAGuiInterface> stagui_;

  QComboBox* mode_menu_;
  QComboBox* filters_menu_;
  QChart* chart_;
  HistogramView* display_;
  QValueAxis* axis_x_;
  QValueAxis* axis_y_;
  QPushButton* refresh_filters_button_;

  std::string path_group_name_;  // Current selected filter
  std::set<sta::Clock*> clocks_;
  std::unique_ptr<Buckets> buckets_;
  std::map<int, std::string> filter_index_to_path_group_name_;

  int prev_filter_index_;
  bool resetting_menu_;

  const int default_number_of_buckets_;
  float max_slack_;
  float min_slack_;
  float bucket_interval_;

  int precision_count_;  // Used to configure the x labels.
  QLabel* label_;
};

}  // namespace gui
