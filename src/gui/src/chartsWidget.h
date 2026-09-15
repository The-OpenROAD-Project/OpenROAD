// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <qchar.h>
#include <qstackedbarseries.h>

#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtCharts>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "sta/SdcClass.hh"
#include "staGuiInterface.h"
#include "utl/histogram.h"

namespace sta {
class Pin;
class dbSta;
class Clock;
}  // namespace sta

namespace gui {

using PinBuckets = std::vector<std::vector<const sta::Pin*>>;

struct Buckets
{
  bool areEmpty() { return positive.empty() && negative.empty(); }

  PinBuckets positive;
  PinBuckets negative;
};

class HistogramView : public QChartView
{
  Q_OBJECT

 public:
  HistogramView(QWidget* parent);

  void setSTA(STAGuiInterface* sta) { sta_ = sta; }
  void setLogger(utl::Logger* logger) { logger_ = logger; }

  void clear();
  void setData(const EndPointSlackMap* data, sta::ClockSet* clocks);
  void setData(
      const std::vector<std::pair<std::string, EndPointSlackMap*>>& data_vec,
      sta::ClockSet* clocks);
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
  float computeBucketInterval(float min_slack, float max_slack);
  float computeSnapBucketInterval(float exact_interval);
  int computeNumberOfDigits(float value);

  void populateBins();
  void populateBuckets(std::unique_ptr<utl::Histogram<float>>& histogram,
                       const PinBuckets& pin_bins);
  void setVisualConfig();

  QStackedBarSeries* populateSeries();

  void setXAxisConfig(int all_bars_count);
  void setXAxisTitle();
  void setYAxisConfig();
  int computeYInterval(int largest_slack_count);
  int computeMaxYSnap(int largest_slack_count);
  int computeFirstDigit(int value, int digits);

  utl::Logger* logger_;
  STAGuiInterface* sta_{nullptr};

  QChart* chart_;
  QValueAxis* axis_x_;
  QValueAxis* axis_y_;

  QMenu* menu_;

  // Settings
  int precision_count_;  // Used to configure the x labels.

  // Data
  sta::ClockSet clocks_;
  Buckets buckets_;
  std::unique_ptr<utl::Histogram<float>> all_histogram_;

  // stacked data
  std::vector<std::unique_ptr<utl::Histogram<float>>> histograms_;
  std::vector<QString> path_groups_;
  std::vector<PinBuckets> pins_bucket_;

  static constexpr int kDefaultNumberOfBuckets = 10;
};

class ChartsWidget : public QDockWidget
{
  Q_OBJECT

 public:
  enum Mode
  {
    kSelect,
    kSetupSlack,
    kHoldSlack
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
  Chart* addChart(const std::string& name,
                  const std::string& x_label,
                  const std::vector<std::string>& y_labels);
  EndPointSlackMap* getData(const std::string& path_group,
                            sta::Clock* clock_filter);

 signals:
  void endPointsToReport(const std::set<const sta::Pin*>& report_pins,
                         const std::string& path_group_name);

 public slots:
  void reportEndPoints(const std::set<const sta::Pin*>& report_pins);

 private slots:
  void changeMode();
  void updatePathGroupMenuIndexes();
  void updateFilters(QStandardItem* item);
  void updateFilters();

 private:
  void setSlackHistogram();
  void setSlackHistogramLayout();
  void setModeMenu();
  void clearMenus();

  void setData(HistogramView* view);

  EndPointSlackMap fetchSlackHistogramData() const;

  utl::Logger* logger_;
  sta::dbSta* sta_;
  std::unique_ptr<STAGuiInterface> stagui_;

  QTabWidget* chart_tabs_;

  QComboBox* mode_menu_;
  QComboBox* path_group_menu_;
  QStandardItemModel* path_group_menu_model_;
  QStandardItem* path_group_text_;
  QStandardItem* all_endpoints_item_;
  QComboBox* clock_menu_;
  HistogramView* display_;
  QPushButton* refresh_filters_button_;

  std::vector<std::string> path_groups_names_;  // Current selected filter
  std::map<std::pair<std::string, sta::Clock*>, EndPointSlackMap>
      endpoints_cache_;
  std::map<int, sta::Clock*> clock_index_to_clock_;
  sta::ClockSet all_clocks_;
  sta::Clock* clock_filter_;

  bool resetting_menu_;

  QLabel* label_;
  bool updating_ = false;
};

}  // namespace gui