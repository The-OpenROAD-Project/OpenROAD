///////////////////////////////////////////////////////////////////////////////
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

#include "chartsWidget.h"

#include <QHBoxLayout>

#ifdef ENABLE_CHARTS
#include <QColor>
#include <QFrame>
#include <QString>
#include <QWidget>
#include <QtCharts>
#include <algorithm>
#include <cmath>
#include <vector>

#include "sta/Clock.hh"
#include "sta/MinMax.hh"
#include "sta/Units.hh"
#endif

namespace gui {

ChartsWidget::ChartsWidget(QWidget* parent)
    : QDockWidget("Charts", parent),
#ifdef ENABLE_CHARTS
      logger_(nullptr),
      sta_(nullptr),
      stagui_(nullptr),
      mode_menu_(new QComboBox(this)),
      filters_menu_(new QComboBox(this)),
      chart_(new QChart),
      display_(new HistogramView(chart_, this)),
      axis_x_(new QValueAxis(this)),
      axis_y_(new QValueAxis(this)),
      buckets_(std::make_unique<Buckets>()),
      prev_filter_index_(0),  // start with no filter
      max_slack_(0.0f),
      min_slack_(0.0f),
#endif
      label_(new QLabel(this))
{
  setObjectName("charts_widget");  // for settings

  QWidget* container = new QWidget(this);
  QHBoxLayout* controls_layout = new QHBoxLayout;
  controls_layout->addWidget(label_);

#ifdef ENABLE_CHARTS
  QVBoxLayout* layout = new QVBoxLayout;
  QFrame* controls_frame = new QFrame;

  controls_layout->insertWidget(0, mode_menu_);
  setModeMenu();

  controls_layout->insertWidget(1, filters_menu_);
  setStartEndFiltersMenu();
  filters_menu_->hide();

  controls_layout->insertStretch(2);

  controls_frame->setLayout(controls_layout);
  controls_frame->setFrameShape(QFrame::StyledPanel);
  controls_frame->setFrameShadow(QFrame::Raised);

  layout->addWidget(controls_frame);
  layout->addWidget(display_);

  container->setLayout(layout);

  chart_->addAxis(axis_y_, Qt::AlignLeft);
  chart_->addAxis(axis_x_, Qt::AlignBottom);

  connect(display_,
          &HistogramView::barIndex,
          this,
          &ChartsWidget::emitEndPointsInBucket);
#else
  label_->setText("QtCharts is not installed.");
  label_->setAlignment(Qt::AlignCenter);
  // We need this layout in order to centralize the label.
  container->setLayout(controls_layout);
#endif
  setWidget(container);
}

#ifdef ENABLE_CHARTS
void ChartsWidget::changeMode()
{
  if (mode_menu_->currentIndex() == SELECT) {
    return;
  }

  clearChart();

  if (mode_menu_->currentIndex() == SLACK_HISTOGRAM) {
    filters_menu_->show();
    setSlackHistogram();
  }
}

void ChartsWidget::setModeMenu()
{
  mode_menu_->addItem("Select Mode");
  mode_menu_->addItem("Endpoint Slack");

  connect(mode_menu_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &ChartsWidget::changeMode);
}

void ChartsWidget::setStartEndFiltersMenu()
{
  filters_menu_->addItem("No Filter");  // Index 0

  filters_menu_->addItem(QString::fromStdString(toString(RegisterToRegister)));
  filters_menu_->addItem(QString::fromStdString(toString(RegisterToIO)));
  filters_menu_->addItem(QString::fromStdString(toString(IOToRegister)));
  filters_menu_->addItem(QString::fromStdString(toString(IOToIO)));

  connect(filters_menu_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &ChartsWidget::changeStartEndFilter);
}

void ChartsWidget::showToolTip(bool is_hovering, int bar_index)
{
  if (is_hovering) {
    const QString number_of_pins
        = QString("Number of Endpoints: %1\n")
              .arg(static_cast<QBarSet*>(sender())->at(bar_index));

    QString scaled_suffix = sta_->units()->timeUnit()->scaledSuffix();

    const float lower = (bar_index - neg_count_offset_) * bucket_interval_;
    const float upper = lower + bucket_interval_;

    QString time_info
        = QString("Interval: [%1, %2) ").arg(lower).arg(upper) + scaled_suffix;

    const QString tool_tip = number_of_pins + time_info;

    QToolTip::showText(QCursor::pos(), tool_tip, this);
  } else {
    QToolTip::hideText();
  }
}

void ChartsWidget::clearChart()
{
  if (filters_menu_->isVisible()) {
    filters_menu_->hide();
  }

  buckets_->positive.clear();
  buckets_->negative.clear();

  chart_->setTitle("");
  chart_->removeAllSeries();

  axis_x_->setTitleText("");
  axis_x_->hide();

  axis_y_->setTitleText("");
  axis_y_->hide();
}

void ChartsWidget::setSlackHistogram()
{
  SlackHistogramData data = fetchSlackHistogramData();

  if (data.constrained_pins.size() == 0) {
    logger_->warn(utl::GUI,
                  97,
                  "All pins are unconstrained. Cannot plot histogram. Check if "
                  "timing data is loaded!");
    return;
  }

  setBucketInterval();
  populateBuckets(data.constrained_pins);

  setNegativeCountOffset(static_cast<int>(buckets_->negative.size()));

  QBarSet* neg_set = new QBarSet("");
  neg_set->setBorderColor(0x8b0000);  // darkred
  neg_set->setColor(0xf08080);        // lightcoral
  QBarSet* pos_set = new QBarSet("");
  pos_set->setBorderColor(0x006400);  // darkgreen
  pos_set->setColor(0x90ee90);        // lightgreen

  connect(neg_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);
  connect(pos_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);

  populateBarSets(*neg_set, *pos_set);

  QStackedBarSeries* series = new QStackedBarSeries(this);
  series->append(neg_set);
  series->append(pos_set);
  series->setBarWidth(1.0);
  chart_->addSeries(series);

  setXAxisConfig(pos_set->count(), data.clocks);
  setYAxisConfig();
  series->attachAxis(axis_y_);

  chart_->legend()->markers(series)[0]->setVisible(false);
  chart_->legend()->markers(series)[1]->setVisible(false);
  chart_->legend()->setVisible(true);
  chart_->legend()->setAlignment(Qt::AlignBottom);
  chart_->setTitle("Endpoint Slack");
}

SlackHistogramData ChartsWidget::fetchSlackHistogramData()
{
  SlackHistogramData data;

  StaPins endpoints = stagui_->getEndPoints();
  fetchConstrainedPins(endpoints, true);

  data.constrained_pins = endpoints;

  for (std::unique_ptr<gui::ClockTree>& clk_tree : stagui_->getClockTrees()) {
    data.clocks.insert(clk_tree.get()->getClock());
  }

  return data;
}

void ChartsWidget::fetchConstrainedPins(StaPins& endpoints,
                                        const bool set_mix_max)
{
  const int all_endpoints_count = endpoints.size();

  int unconstrained_count = 0;
  sta::Unit* time_unit = sta_->units()->timeUnit();

  for (const sta::Pin* pin : endpoints) {
    float slack = stagui_->getPinSlack(pin);

    if (slack != sta::INF) {
      slack = time_unit->staToUser(slack);

      if (set_mix_max) {
        if (slack < min_slack_) {
          min_slack_ = slack;
        }

        if (slack > max_slack_) {
          max_slack_ = slack;
        }
      }
    } else {
      endpoints.erase(pin);
      unconstrained_count++;
    }
  }

  if (unconstrained_count != 0 && unconstrained_count != all_endpoints_count) {
    const QString label_message = "Number of unconstrained pins: ";
    QString unconstrained_number;
    unconstrained_number.setNum(unconstrained_count);
    label_->setText(label_message + unconstrained_number);
  }
}

// We define the slack interval as being inclusive in its lower
// boundary and exclusive in upper: [lower upper)
void ChartsWidget::populateBuckets(const StaPins& endpoints)
{
  sta::Unit* time_unit = sta_->units()->timeUnit();

  float positive_lower = 0.0f, positive_upper = 0.0f, negative_lower = 0.0f,
        negative_upper = 0.0f;

  int bucket_index = 0;

  do {
    positive_lower = bucket_interval_ * bucket_index;
    positive_upper = bucket_interval_ * (bucket_index + 1);
    negative_lower = -positive_upper;
    negative_upper = -positive_lower;

    std::vector<const sta::Pin*> pos_bucket, neg_bucket;

    for (const sta::Pin* pin : endpoints) {
      const float slack = time_unit->staToUser(stagui_->getPinSlack(pin));

      if (negative_lower <= slack && slack < negative_upper) {
        neg_bucket.push_back(pin);
      } else if (positive_lower <= slack && slack < positive_upper) {
        pos_bucket.push_back(pin);
      }
    }

    // Push zeros - meaning no slack values in the current range - only in
    // situations where the bucket is in a valid position of the queue.
    if (min_slack_ < negative_upper) {
      buckets_->negative.push_front(neg_bucket);

      if (largest_slack_count_ < neg_bucket.size()) {
        largest_slack_count_ = neg_bucket.size();
      }
    }

    if (max_slack_ >= positive_lower) {
      buckets_->positive.push_back(pos_bucket);

      if (largest_slack_count_ < pos_bucket.size()) {
        largest_slack_count_ = pos_bucket.size();
      }
    }

    ++bucket_index;
  } while (min_slack_ < negative_upper || max_slack_ >= positive_upper);
}

void ChartsWidget::emitEndPointsInBucket(const int bar_index)
{
  std::vector<const sta::Pin*> endpoints;

  if (buckets_->negative.empty()) {
    endpoints = buckets_->positive[bar_index];
  } else {
    const int num_of_neg_buckets = static_cast<int>(buckets_->negative.size());

    if (bar_index >= num_of_neg_buckets) {
      endpoints = buckets_->positive[bar_index - num_of_neg_buckets];
    } else {
      endpoints = buckets_->negative[bar_index];
    }
  }

  auto compareSlack = [=](const sta::Pin*& a, const sta::Pin*& b) {
    return stagui_->getPinSlack(a) < stagui_->getPinSlack(b);
  };
  std::sort(endpoints.begin(), endpoints.end(), compareSlack);

  // Depeding on the size of the bucket, the report can become rather slow
  // to generate so we define this limit.
  const int max_number_of_pins = 50;
  int pin_count = 0;

  std::set<const sta::Pin*> report_pins;
  for (const sta::Pin* pin : endpoints) {
    if (pin_count == max_number_of_pins) {
      break;
    }

    report_pins.insert(pin);
    ++pin_count;
  }

  emit endPointsToReport(report_pins);
}

void ChartsWidget::setBucketInterval()
{
  const float exact_interval
      = (max_slack_ - min_slack_) / default_number_of_buckets_;

  int snap_interval = computeSnapBucketInterval(exact_interval);

  // We compute a new number of buckets based on the snap interval.
  const int new_number_of_buckets
      = computeNumberofBuckets(snap_interval, max_slack_, min_slack_);
  const int minimum_number_of_buckets = 8;

  if (new_number_of_buckets < minimum_number_of_buckets) {
    const float minimum_interval
        = (max_slack_ - min_slack_) / minimum_number_of_buckets;

    float decimal_snap_interval
        = computeSnapBucketDecimalInterval(minimum_interval);

    setBucketInterval(decimal_snap_interval);
  } else {
    setBucketInterval(snap_interval);
  }
}

int ChartsWidget::computeNumberofBuckets(const int bucket_interval,
                                         const float max_slack,
                                         const float min_slack)
{
  int bucket_count = 1;
  float current_value = min_slack;

  while (current_value < max_slack) {
    current_value += bucket_interval;
    ++bucket_count;
  }

  return bucket_count;
}

float ChartsWidget::computeSnapBucketDecimalInterval(float minimum_interval)
{
  float integer_part = minimum_interval;
  int power_count = 0;

  while (static_cast<int>(integer_part) == 0) {
    integer_part *= 10;
    ++power_count;
  }

  setDecimalPrecision(power_count);

  return std::ceil(integer_part) / std::pow(10, power_count);
}

int ChartsWidget::computeSnapBucketInterval(float exact_interval)
{
  if (exact_interval < 10) {
    return std::ceil(exact_interval);
  }

  int snap_interval = 0;
  int digits = computeNumberOfDigits(static_cast<int>(exact_interval));

  while (snap_interval < exact_interval) {
    snap_interval += 5 * std::pow(10, digits - 2);
  }

  return snap_interval;
}

void ChartsWidget::setXAxisConfig(int all_bars_count,
                                  const std::set<sta::Clock*>& clocks)
{
  QString axis_x_title = createXAxisTitle(clocks);
  axis_x_->setTitleText(axis_x_title);

  const QString format = "%." + QString::number(precision_count_) + "f";
  axis_x_->setLabelFormat(format);

  const int pos_bars_count = all_bars_count - neg_count_offset_;
  const float min = -(static_cast<float>(neg_count_offset_)) * bucket_interval_;
  const float max = static_cast<float>(pos_bars_count) * bucket_interval_;
  axis_x_->setRange(min, max);

  axis_x_->setTickCount(all_bars_count + 1);
  axis_x_->setGridLineVisible(false);
  axis_x_->setVisible(true);
}

QString ChartsWidget::createXAxisTitle(const std::set<sta::Clock*>& clocks)
{
  const QString start_title = "<center>Slack [";

  sta::Unit* time_units = sta_->units()->timeUnit();

  const QString scaled_suffix = time_units->scaledSuffix();
  const QString end_title = "], Clocks: ";

  QString axis_x_title = start_title + scaled_suffix + end_title;

  int clock_count = 1;

  for (sta::Clock* clock : clocks) {
    float period = time_units->staToUser(clock->period());

    if (period < 1) {
      const float decimal_digits = 3;
      const float places = std::pow(10, decimal_digits);

      period = std::round(period * places) / places;
    }

    // Adjust strings: two clocks in the first row and three per row afterwards
    if (clock->name() != (*(clocks.begin()))->name()) {
      axis_x_title += ", ";

      if (clock_count % 3 == 0) {
        axis_x_title += "<br>";
      }
    }

    axis_x_title
        += QString::fromStdString(fmt::format(" {} {}", clock->name(), period));

    ++clock_count;
  }

  return axis_x_title;
}

void ChartsWidget::setYAxisConfig()
{
  int y_interval = computeYInterval();
  int max_y = 0;
  int tick_count = 1;

  // Do this instead of just using the return value of computeMaxYSnap()
  // so we don't get an empty range at the end of the axis.
  while (max_y < largest_slack_count_) {
    max_y += y_interval;
    ++tick_count;
  }

  axis_y_->setRange(0, max_y);
  axis_y_->setTickCount(tick_count);
  axis_y_->setTitleText("Number of Endpoints");
  axis_y_->setLabelFormat("%i");
  axis_y_->setVisible(true);
}

int ChartsWidget::computeYInterval()
{
  int snap_max = computeMaxYSnap();
  int digits = computeNumberOfDigits(snap_max);
  int total = std::pow(10, digits);

  if (computeFirstDigit(snap_max, digits) < 5) {
    total /= 2;
  }

  // We ceil to deal with the cases in which we have less than
  // 10 endpoints in the largest bucket.
  return static_cast<int>(std::ceil(static_cast<float>(total) / 10));
}

// Snap to an upper value based on the first digit
int ChartsWidget::computeMaxYSnap()
{
  if (largest_slack_count_ <= 10) {
    return largest_slack_count_;
  }

  int digits = computeNumberOfDigits(largest_slack_count_);
  int first_digit = computeFirstDigit(largest_slack_count_, digits);

  return (first_digit + 1) * std::pow(10, digits - 1);
}

int ChartsWidget::computeNumberOfDigits(int value)
{
  return static_cast<int>(std::log10(value)) + 1;
}

int ChartsWidget::computeFirstDigit(int value, int digits)
{
  return static_cast<int>(value / std::pow(10, digits - 1));
}

void ChartsWidget::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
  stagui_ = std::make_unique<STAGuiInterface>(sta_);
}

void ChartsWidget::changeStartEndFilter()
{
  buckets_->negative.clear();
  buckets_->positive.clear();

  clearBarSets();

  QStackedBarSeries* series
      = static_cast<QStackedBarSeries*>(chart_->series().front());

  // The negative bar set is the one we appended first.
  QBarSet* neg_set = series->barSets().front();
  QBarSet* pos_set = series->barSets().back();

  const int filter_index = filters_menu_->currentIndex();

  StaPins endpoints = stagui_->getEndPoints();
  fetchConstrainedPins(endpoints, false);

  if (filter_index > 0) {
    const int no_filter_index_offset = 1;
    const StartEndPathType path_type
        = static_cast<StartEndPathType>(filter_index - no_filter_index_offset);

    TimingPathList paths = fetchPathsBasedOnStartEnd(endpoints, path_type);
    endpoints = getEndPoints(paths);
  }

  populateBuckets(endpoints);
  populateBarSets(*neg_set, *pos_set);

  prev_filter_index_ = filter_index;
}

TimingPathList ChartsWidget::fetchPathsBasedOnStartEnd(
    const StaPins& endpoints,
    const StartEndPathType path_type)
{
  gui::StaPins iterms;
  gui::StaPins bterms;

  for (const sta::Pin* pin : endpoints) {
    const std::unique_ptr<TimingPathNode>& node = stagui_->getTimingNode(pin);

    if (!node) {
      continue;
    }

    if (node->isPinBTerm()) {
      bterms.insert(pin);
    }

    if (node->isPinITerm()) {
      iterms.insert(pin);
    }
  }

  const int initial_max_path_count = stagui_->getMaxPathCount();
  const bool one_path_per_end_point = stagui_->isOnePathPerEndpoint();

  // Set max path count based on the total number of registers.
  stagui_->setMaxPathCount(iterms.size());
  stagui_->setOnePathPerEndpoint(true);

  TimingPathList paths;

  // TO DO: This query is not working

  switch (path_type) {
    case RegisterToRegister: {
      paths = stagui_->getTimingPaths(iterms, {}, iterms);
      break;
    }
    case RegisterToIO: {
      paths = stagui_->getTimingPaths(iterms, {}, bterms);
      break;
    }
    case IOToRegister: {
      paths = stagui_->getTimingPaths(bterms, {}, iterms);
      break;
    }
    case IOToIO: {
      paths = stagui_->getTimingPaths(bterms, {}, bterms);
      break;
    }
  }

  stagui_->setMaxPathCount(initial_max_path_count);
  stagui_->setOnePathPerEndpoint(one_path_per_end_point);

  return paths;
}

StaPins ChartsWidget::getEndPoints(const TimingPathList& paths)
{
  StaPins endpoints;

  for (const std::unique_ptr<TimingPath>& path : paths) {
    endpoints.insert(path->getEndStageNode()->getPinAsSTA());
  }

  return endpoints;
}

void ChartsWidget::populateBarSets(QBarSet& neg_set, QBarSet& pos_set)
{
  for (int i = 0; i < buckets_->negative.size(); ++i) {
    neg_set << buckets_->negative[i].size();
    pos_set << 0;
  }
  for (int i = 0; i < buckets_->positive.size(); ++i) {
    neg_set << 0;
    pos_set << buckets_->positive[i].size();
  }
}

void ChartsWidget::clearBarSets()
{
  const auto abstract_series = chart_->series();
  if (abstract_series.isEmpty()) {
    return;
  }

  QStackedBarSeries* series
      = static_cast<QStackedBarSeries*>(abstract_series.front());

  // Negative set first
  for (QBarSet* bar_set : series->barSets()) {
    bar_set->remove(0, bar_set->count());
  }
}

std::string ChartsWidget::toString(StartEndPathType path_type)
{
  switch (path_type) {
    case RegisterToRegister:
      return "Register to Register";
    case RegisterToIO:
      return "Register to IO";
    case IOToRegister:
      return "IO to Register";
    case IOToIO:
      return "IO to IO";
  }

  return "";
}

////// HistogramView ///////
HistogramView::HistogramView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent)
{
}

void HistogramView::mousePressEvent(QMouseEvent* event)
{
  const auto abstract_series = chart()->series();
  if (abstract_series.isEmpty()) {
    return;
  }

  // There's only one series for the slack histogram mode.
  QStackedBarSeries* series
      = static_cast<QStackedBarSeries*>(abstract_series.front());

  const QPointF series_value = chart()->mapToValue(event->pos(), series);

  // Using QValueAxis for the x axis results in an offset of half unit between
  // the chart's origin (not the widget's origin, the actual x,y origin from
  // the chart) and the first bar.
  const float attachment_offset = 0.5;
  const float index_mapped_x
      = static_cast<float>(series_value.x()) + attachment_offset;
  const float index_mapped_y = static_cast<float>(series_value.y());

  bool valid_horizontal_range
      = index_mapped_x >= 0
        && index_mapped_x <= series->barSets().front()->count();

  QValueAxis* y_axis
      = static_cast<QValueAxis*>(chart()->axes(Qt::Vertical).first());

  bool valid_vertical_range
      = index_mapped_y >= 0
        && index_mapped_y <= static_cast<float>(y_axis->max());

  if (valid_horizontal_range && valid_vertical_range) {
    emit barIndex(static_cast<int>(index_mapped_x));
  }
}

#endif
}  // namespace gui
