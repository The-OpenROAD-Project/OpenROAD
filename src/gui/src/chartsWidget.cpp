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
#include "staGuiInterface.h"
#endif

namespace gui {

ChartsWidget::ChartsWidget(QWidget* parent)
    : QDockWidget("Charts", parent),
#ifdef ENABLE_CHARTS
      logger_(nullptr),
      sta_(nullptr),
      stagui_(nullptr),
      mode_menu_(new QComboBox(this)),
      chart_(new QChart),
      display_(new QChartView(chart_, this)),
      axis_x_(new QValueAxis(this)),
      axis_y_(new QValueAxis(this)),
      buckets_(std::make_unique<Buckets>()),
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

  mode_menu_->addItem("Select Mode");
  mode_menu_->addItem("Endpoint Slack");

  controls_layout->insertWidget(0, mode_menu_);
  controls_layout->insertStretch(1);

  controls_frame->setLayout(controls_layout);
  controls_frame->setFrameShape(QFrame::StyledPanel);
  controls_frame->setFrameShadow(QFrame::Raised);

  layout->addWidget(controls_frame);
  layout->addWidget(display_);

  container->setLayout(layout);

  chart_->addAxis(axis_y_, Qt::AlignLeft);
  chart_->addAxis(axis_x_, Qt::AlignBottom);

  connect(mode_menu_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &ChartsWidget::changeMode);
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
    setSlackMode();
  }
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
  chart_->setTitle("");
  chart_->removeAllSeries();

  axis_x_->setTitleText("");
  axis_x_->hide();

  axis_y_->setTitleText("");
  axis_y_->hide();
}

void ChartsWidget::setSlackMode()
{
  SlackHistogramData data = fetchSlackHistogramData();

  if (data.constrained_pins.size() == 0) {
    logger_->warn(utl::GUI,
                  97,
                  "All pins are unconstrained. Cannot plot histogram. Check if "
                  "timing data is loaded!");
    return;
  }

  setBucketInterval(data.max_slack, data.min_slack);
  populateBuckets(data);
  setNegativeCountOffset(static_cast<int>(buckets_->negative.size()));

  QBarSet* neg_set = new QBarSet("");
  neg_set->setBorderColor(0x8b0000);  // darkred
  neg_set->setColor(0xf08080);        // lightcoral
  QBarSet* pos_set = new QBarSet("");
  pos_set->setBorderColor(0x006400);  // darkgreen
  pos_set->setColor(0x90ee90);        // lightgreen

  connect(neg_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);
  connect(pos_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);
  connect(
      neg_set, &QBarSet::clicked, this, &ChartsWidget::emitEndPointsInBucket);
  connect(
      pos_set, &QBarSet::clicked, this, &ChartsWidget::emitEndPointsInBucket);

  for (int i = 0; i < buckets_->negative.size(); ++i) {
    *neg_set << buckets_->negative[i].size();
    *pos_set << 0;
  }
  for (int i = 0; i < buckets_->positive.size(); ++i) {
    *neg_set << 0;
    *pos_set << buckets_->positive[i].size();
  }

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

SlackHistogramData ChartsWidget::fetchSlackHistogramData() const
{
  SlackHistogramData data;

  sta::Unit* time_unit = sta_->units()->timeUnit();
  gui::StaPins end_points = stagui_->getEndPoints();
  int unconstrained_count = 0;

  data.min_slack = std::numeric_limits<float>::max();

  for (const sta::Pin* pin : end_points) {
    float slack = stagui_->getPinSlack(pin);

    if (slack != sta::INF) {
      slack = time_unit->staToUser(slack);
      data.constrained_pins.push_back(pin);

      if (slack < data.min_slack) {
        data.min_slack = slack;
      }

      if (slack > data.max_slack) {
        data.max_slack = slack;
      }
    } else {
      unconstrained_count++;
    }
  }

  if (unconstrained_count != 0 && unconstrained_count != end_points.size()) {
    const QString label_message = "Number of unconstrained pins: ";
    QString unconstrained_number;
    unconstrained_number.setNum(unconstrained_count);
    label_->setText(label_message + unconstrained_number);
  }

  for (std::unique_ptr<gui::ClockTree>& clk_tree : stagui_->getClockTrees()) {
    data.clocks.insert(clk_tree.get()->getClock());
  }

  return data;
}

// We define the slack interval as being inclusive in its lower
// boundary and exclusive in upper: [lower upper)
void ChartsWidget::populateBuckets(const SlackHistogramData& data)
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

    for (const auto& pin : data.constrained_pins) {
      const float slack = time_unit->staToUser(stagui_->getPinSlack(pin));

      if (negative_lower <= slack && slack < negative_upper) {
        neg_bucket.push_back(pin);
      } else if (positive_lower <= slack && slack < positive_upper) {
        pos_bucket.push_back(pin);
      }
    }

    // Push zeros - meaning no slack values in the current range - only in
    // situations where the bucket is in a valid position of the queue.
    if (data.min_slack < negative_upper) {
      buckets_->negative.push_front(neg_bucket);

      if (largest_slack_count_ < neg_bucket.size()) {
        largest_slack_count_ = neg_bucket.size();
      }
    }

    if (data.max_slack >= positive_lower) {
      buckets_->positive.push_back(pos_bucket);

      if (largest_slack_count_ < pos_bucket.size()) {
        largest_slack_count_ = pos_bucket.size();
      }
    }

    ++bucket_index;
  } while (data.min_slack < negative_upper || data.max_slack >= positive_upper);
}

void ChartsWidget::emitEndPointsInBucket(const int bar_index)
{
  std::vector<const sta::Pin*> pins;

  if (buckets_->negative.empty()) {
    pins = buckets_->positive[bar_index];
  } else {
    int num_of_neg_buckets = static_cast<int>(buckets_->negative.size());

    if (bar_index >= num_of_neg_buckets) {
      pins = buckets_->positive[bar_index - num_of_neg_buckets];
    } else {
      pins = buckets_->negative[bar_index];
    }
  }

  auto compareSlack = [=](const sta::Pin* a, const sta::Pin* b) {
    return stagui_->getPinSlack(a) < stagui_->getPinSlack(b);
  };
  std::sort(pins.begin(), pins.end(), compareSlack);

  sta::dbNetwork* network = sta_->getDbNetwork();
  std::vector<odb::dbITerm*> iterms;
  std::vector<odb::dbBTerm*> bterms;

  // Depeding on the size of the bucket, the report can become rather slow
  // to generate so we define this limit.
  int max_number_of_pins = 50;
  int pin_count = 1;
  for (const sta::Pin* pin : pins) {
    if (pin_count >= max_number_of_pins) {
      break;
    }

    odb::dbITerm* iterm = nullptr;
    odb::dbBTerm* bterm = nullptr;

    network->staToDb(pin, iterm, bterm);
    if (iterm) {
      iterms.push_back(iterm);
    } else if (bterm) {
      bterms.push_back(bterm);
    }
    ++pin_count;
  }

  emit endPointsToReport(iterms, bterms);
}

void ChartsWidget::setBucketInterval(const float max_slack,
                                     const float min_slack)
{
  const float exact_interval
      = (max_slack - min_slack) / default_number_of_buckets_;

  int snap_interval = computeSnapBucketInterval(exact_interval);

  // We compute a new number of buckets based on the snap interval.
  const int new_number_of_buckets
      = computeNumberofBuckets(snap_interval, max_slack, min_slack);
  const int minimum_number_of_buckets = 8;

  if (new_number_of_buckets < minimum_number_of_buckets) {
    const float minimum_interval
        = (max_slack - min_slack) / minimum_number_of_buckets;

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

#endif
}  // namespace gui
