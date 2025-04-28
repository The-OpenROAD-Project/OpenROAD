// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QString>
#include <QtCharts>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
  float min = std::numeric_limits<float>::max();
  float max = std::numeric_limits<float>::lowest();
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
  HistogramView(QWidget* parent);

  void setSTA(STAGuiInterface* sta) { sta_ = sta; }
  void setLogger(utl::Logger* logger) { logger_ = logger; }

  void clear();
  void setData(const SlackHistogramData& data);
  void setData(const EndPointSlackMap& data);

  void save(const QString& path);

 public slots:
  void saveImage();

 signals:
  void endPointsToReport(const std::set<const sta::Pin*>& report_pins);

 private slots:
  void showToolTip(bool is_hovering, int bar_index);
  void emitEndPointsInBucket(int bar_index);

 protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void setBucketInterval();
  float computeSnapBucketInterval(float exact_interval);
  float computeSnapBucketDecimalInterval(float minimum_interval);
  int computeNumberofBuckets(float bucket_interval,
                             float max_slack,
                             float min_slack);
  int computeNumberOfDigits(float value);

  void setVisualConfig();

  void populateBuckets(const StaPins* end_points,
                       const EndPointSlackMap* end_point_to_slack);
  std::pair<QBarSet*, QBarSet*> createBarSets();
  void populateBarSets(QBarSet& neg_set, QBarSet& pos_set);

  void setXAxisConfig(int all_bars_count);
  void setXAxisTitle();
  void setYAxisConfig();
  int computeYInterval(int largest_slack_count);
  int computeMaxYSnap(int largest_slack_count);
  int computeFirstDigit(int value, int digits);

  void setLimits(const EndPointSlackMap& end_point_to_slack);

  utl::Logger* logger_;
  STAGuiInterface* sta_{nullptr};

  QChart* chart_;
  QValueAxis* axis_x_;
  QValueAxis* axis_y_;

  QMenu* menu_;

  // Settings
  float max_slack_;
  float min_slack_;
  float bucket_interval_;
  int precision_count_;  // Used to configure the x labels.

  // Data
  std::set<sta::Clock*> clocks_;
  Buckets buckets_;

  static constexpr int default_number_of_buckets_ = 15;
};

class ChartsWidget : public QDockWidget
{
  Q_OBJECT

 public:
  enum Mode
  {
    SELECT,
    SETUP_SLACK,
    HOLD_SLACK
  };

  ChartsWidget(QWidget* parent = nullptr);
  void setSTA(sta::dbSta* sta);
  void setLogger(utl::Logger* logger);

  void setMode(Mode mode);

  void saveImage(const std::string& path,
                 Mode mode,
                 const std::optional<int>& width_px,
                 const std::optional<int>& height_px);

  Mode modeFromString(const std::string& mode) const;

 signals:
  void endPointsToReport(const std::set<const sta::Pin*>& report_pins,
                         const std::string& path_group_name);

 public slots:
  void reportEndPoints(const std::set<const sta::Pin*>& report_pins);

 private slots:
  void changeMode();
  void updatePathGroupMenuIndexes();
  void changePathGroupFilter();

 private:
  void setSlackHistogram();
  void setSlackHistogramLayout();
  void setModeMenu();

  void setData(HistogramView* view, const std::string& path_group) const;

  SlackHistogramData fetchSlackHistogramData() const;
  void removeUnconstrainedPinsAndSetLimits(SlackHistogramData& data) const;

  utl::Logger* logger_;
  sta::dbSta* sta_;
  std::unique_ptr<STAGuiInterface> stagui_;

  QComboBox* mode_menu_;
  QComboBox* filters_menu_;
  HistogramView* display_;
  QPushButton* refresh_filters_button_;

  std::string path_group_name_;  // Current selected filter
  std::map<int, std::string> filter_index_to_path_group_name_;

  int prev_filter_index_;
  bool resetting_menu_;

  QLabel* label_;
};

}  // namespace gui
