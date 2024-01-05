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

#include <QColor>
#include <QFrame>
#include <QString>
#include <QWidget>
#include <QtCharts>
#include <algorithm>
#include <cmath>
#include <vector>

#include "sta/MinMax.hh"
#include "sta/Units.hh"
#include "staGuiInterface.h"

namespace gui {

ChartsWidget::ChartsWidget(QWidget* parent)
    : QDockWidget("Charts", parent),
      logger_(nullptr),
      sta_(nullptr),
      label_(new QLabel(this)),
      mode_menu_(new QComboBox(this)),
      chart_(new QChart),
      display_(new QChartView(chart_, this)),
      axis_x_(new QBarCategoryAxis(this)),
      axis_y_(new QValueAxis(this))
{
  setObjectName("charts_widget");  // for settings

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  QFrame* controls_frame = new QFrame;
  QHBoxLayout* controls_layout = new QHBoxLayout;

  mode_menu_->addItem("Select Mode");
  mode_menu_->addItem("Endpoint Slack");

  controls_layout->addWidget(mode_menu_);
  controls_layout->addWidget(label_);
  controls_layout->insertStretch(1);

  controls_frame->setLayout(controls_layout);
  controls_frame->setFrameShape(QFrame::StyledPanel);
  controls_frame->setFrameShadow(QFrame::Raised);

  layout->addWidget(controls_frame);
  layout->addWidget(display_);
  container->setLayout(layout);
  setWidget(container);

  chart_->addAxis(axis_y_, Qt::AlignLeft);
  chart_->addAxis(axis_x_, Qt::AlignBottom);

  connect(mode_menu_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this,
          &ChartsWidget::changeMode);
}

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
        = QString("Number of Endpoints: %1\n").arg(bar_index);

    QString time_unit = sta_->units()->timeUnit()->scaleAbbreviation();
    time_unit.append(sta_->units()->timeUnit()->suffix());

    QString time_info
        = QString("Interval: %1 ").arg(axis_x_->categories()[bar_index])
          + time_unit;

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
  axis_x_->clear();
  axis_x_->hide();

  axis_y_->setTitleText("");
  axis_y_->hide();
}

void ChartsWidget::setSlackMode()
{
  std::vector<float> all_slack;
  getSlackForAllEndpoints(all_slack);

  if (all_slack.size() == 0) {
    logger_->warn(utl::GUI,
                  97,
                  "All pins are unconstrained. Cannot plot histogram. Check if "
                  "timing data is loaded!");
    return;
  }

  std::deque<int> neg_buckets, pos_buckets;

  populateBuckets(all_slack, pos_buckets, neg_buckets);

  QBarSet* neg_set = new QBarSet("");
  neg_set->setBorderColor(0x8b0000);  // darkred
  neg_set->setColor(0xf08080);        // lightcoral

  QBarSet* pos_set = new QBarSet("");
  pos_set->setBorderColor(0x006400);  // darkgreen
  pos_set->setColor(0x90ee90);        // lightgreen

  connect(neg_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);
  connect(pos_set, &QBarSet::hovered, this, &ChartsWidget::showToolTip);

  QStringList time_values;

  const QString open_bracket = "[";
  const QString close_parenthesis = ")";
  const QString comma = ", ";

  for (int i = 0; i < neg_buckets.size(); ++i) {
    *neg_set << neg_buckets[i];
    *pos_set << 0;
    float lower = (i - static_cast<int>(neg_buckets.size())) * bucket_interval_;
    float upper = lower + bucket_interval_;

    time_values << open_bracket + QString("").setNum(lower) + comma
                       + QString("").setNum(upper) + close_parenthesis;
  }

  for (int i = 0; i < pos_buckets.size(); ++i) {
    *neg_set << 0;
    *pos_set << pos_buckets[i];
    float lower = i * bucket_interval_;
    float upper = lower + bucket_interval_;

    time_values << open_bracket + QString().setNum(lower) + comma
                       + QString().setNum(upper) + close_parenthesis;
  }

  setXAxisConfig(time_values);
  setYAxisConfig();

  QStackedBarSeries* series = new QStackedBarSeries(this);
  chart_->addSeries(series);

  series->append(neg_set);
  series->append(pos_set);
  series->setBarWidth(1.0);
  series->attachAxis(axis_x_);
  series->attachAxis(axis_y_);

  chart_->legend()->markers(series)[0]->setVisible(false);
  chart_->legend()->markers(series)[1]->setVisible(false);
  chart_->legend()->setVisible(true);
  chart_->legend()->setAlignment(Qt::AlignBottom);
  chart_->setTitle("Endpoint Slack");
}

void ChartsWidget::getSlackForAllEndpoints(std::vector<float>& all_slack) const
{
  STAGuiInterface sta_gui(sta_);

  auto time_units = sta_->units()->timeUnit();
  auto end_points = sta_gui.getEndPoints();
  int unconstrained_count = 0;

  for (const auto& pin : end_points) {
    double pin_slack = 0;
    pin_slack = sta_gui.getPinSlack(pin);
    if (pin_slack != sta::INF)
      all_slack.push_back(time_units->staToUser(pin_slack));
    else
      unconstrained_count++;
  }

  if (unconstrained_count != 0 && unconstrained_count != end_points.size()) {
    const QString label_message = "Number of unconstrained pins: ";
    QString unconstrained_number;
    unconstrained_number.setNum(unconstrained_count);
    label_->setText(label_message + unconstrained_number);
  }
}

// The define the slack interval as being inclusive in its lower
// boundary and exclusive in upper: [lower upper)
void ChartsWidget::populateBuckets(const std::vector<float>& all_slack,
                                   std::deque<int>& pos_buckets,
                                   std::deque<int>& neg_buckets)
{
  auto max_slack = std::max_element(all_slack.begin(), all_slack.end());
  auto min_slack = std::min_element(all_slack.begin(), all_slack.end());

  float bucket_interval = (*max_slack - *min_slack) / number_of_buckets_;
  bucket_interval = std::ceil(bucket_interval * 10) / 10;

  setBucketInterval(bucket_interval);

  float positive_lower = 0.0f, positive_upper = 0.0f, negative_lower = 0.0f,
        negative_upper = 0.0f;

  int bucket_index = 0, pos_slack_count = 0, neg_slack_count = 0;

  do {
    pos_slack_count = 0;
    neg_slack_count = 0;

    positive_lower = bucket_interval * bucket_index;
    positive_upper = bucket_interval * (bucket_index + 1);
    negative_lower = -positive_upper;
    negative_upper = -positive_lower;

    for (const auto& slack : all_slack) {
      if (negative_lower <= slack && slack < negative_upper) {
        ++neg_slack_count;
      } else if (positive_lower <= slack && slack < positive_upper) {
        ++pos_slack_count;
      }
    }

    // Push zeros - meaning no slack values in the current range - only in
    // situations where the bucket is in a valid position of the queue.
    if (*min_slack < negative_upper) {
      neg_buckets.push_front(neg_slack_count);

      if (largest_slack_count_ < neg_slack_count) {
        largest_slack_count_ = neg_slack_count;
      }
    }

    if (*max_slack >= positive_lower) {
      pos_buckets.push_back(pos_slack_count);

      if (largest_slack_count_ < pos_slack_count) {
        largest_slack_count_ = pos_slack_count;
      }
    }

    ++bucket_index;
  } while (*min_slack < negative_upper || *max_slack >= positive_upper);
}

void ChartsWidget::setBucketInterval(float bucket_interval)
{
  bucket_interval_ = bucket_interval;
}

void ChartsWidget::setXAxisConfig(const QStringList& time_values)
{
  const QString start_title = "Slack [";
  const QString time_scale_abreviation
      = sta_->units()->timeUnit()->scaleAbbreviation();
  QString time_suffix = sta_->units()->timeUnit()->suffix();

  const QString end_title = "]";
  const QString axis_x_title
      = start_title + time_scale_abreviation + time_suffix + end_title;

  axis_x_->setTitleText(axis_x_title);
  axis_x_->append(time_values);
  axis_x_->setLabelsAngle(-90);
  axis_x_->setGridLineVisible(false);
  axis_x_->setVisible(true);
}

void ChartsWidget::setYAxisConfig()
{
  axis_y_->setTitleText("Number of Endpoints");
  axis_y_->setRange(0, largest_slack_count_);
  axis_y_->setTickCount(default_tick_count_);
  axis_y_->setLabelFormat("%i");
  axis_y_->setVisible(true);
}

void ChartsWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

}  // namespace gui
