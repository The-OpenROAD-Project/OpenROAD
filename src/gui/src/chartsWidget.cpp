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
#include <iostream>
#include <set>
#include <vector>

#include "sta/MinMax.hh"
#include "sta/Units.hh"
#include "staGuiInterface.h"

namespace gui {

const int SELECT = 0;
const int SLACK_MODE = 1;

ChartsWidget::ChartsWidget(QWidget* parent)
    : QDockWidget("Charts", parent),
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

  connect(
      mode_menu_, SIGNAL(currentIndexChanged(int)), this, SLOT(changeMode()));
}

void ChartsWidget::changeMode()
{
  if (mode_menu_->currentIndex() == SELECT) {
    return;
  }

  clearChart();

  if (mode_menu_->currentIndex() == SLACK_MODE) {
    setSlackMode();
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
  chart_->setTitle("Endpoint Slack");

  STAGuiInterface sta_gui(sta_);

  auto time_units = sta_->units()->timeUnit();
  auto end_points = sta_gui.getEndPoints();
  std::vector<float> all_slack;
  int unconstrained_count = 0;

  for (auto pin : end_points) {
    double pin_slack = 0;
    pin_slack = sta_gui.getPinSlack(pin);
    if (pin_slack != sta::INF)
      all_slack.push_back(time_units->staToUser(pin_slack));
    else
      unconstrained_count++;
  }

  if (unconstrained_count != 0) {
    const QString label_message = "Number of unconstrained pins: ";
    QString unconstrained_number;
    unconstrained_number.setNum(unconstrained_count);
    label_->setText(label_message + unconstrained_number);
  }

  auto max_slack = std::max_element(all_slack.begin(), all_slack.end());

  auto min_slack = std::min_element(all_slack.begin(), all_slack.end());

  //+1 for values around zero
  int total_pos_buckets = *max_slack + 1;
  int total_neg_buckets = abs(*min_slack) + 1;
  int offset = abs(*min_slack);
  std::vector<float> pos_buckets[total_pos_buckets];
  std::vector<float> neg_buckets[total_neg_buckets];

  for (auto slack : all_slack) {
    if (slack < 0) {
      int bucket_index = slack;
      neg_buckets[bucket_index + offset].push_back(slack);
    } else {
      int bucket_index = slack;
      pos_buckets[bucket_index].push_back(slack);
    }
  }

  QBarSet* neg_set = new QBarSet("");
  neg_set->setBorderColor(0x8b0000);  // darkred
  neg_set->setColor(0xf08080);        // lightcoral

  QBarSet* pos_set = new QBarSet("");
  pos_set->setBorderColor(0x006400);  // darkgreen
  pos_set->setColor(0x90ee90);        // lightgreen

  QStringList time_values;

  const QString open_bracket = "[";
  const QString close_parenthesis = ")";
  const QString comma = ", ";
  int bucket_count = 0;
  int max_y = 0;

  for (int i = 0; i < total_neg_buckets; ++i) {
    bucket_count = neg_buckets[i].size();
    *neg_set << bucket_count;
    *pos_set << 0;
    QString curr_value = "";
    QString next_value = "";
    time_values << open_bracket + curr_value.setNum(i - total_neg_buckets)
                       + comma + next_value.setNum((i + 1) - total_neg_buckets)
                       + close_parenthesis;
    if (max_y < bucket_count)
      max_y = bucket_count;
  }

  for (int i = 0; i < total_pos_buckets; ++i) {
    bucket_count = pos_buckets[i].size();
    *pos_set << bucket_count;
    *neg_set << 0;
    QString curr_value = "";
    QString next_value = "";
    time_values << open_bracket + curr_value.setNum(i) + comma
                       + next_value.setNum(i + 1) + close_parenthesis;
    if (max_y < bucket_count)
      max_y = bucket_count;
  }

  const QString start_title = "Slack [";
  const QString time_suffix = time_units->suffix();
  const QString time_scale_abreviation = time_units->scaleAbbreviation();
  const QString end_title = "]";
  const QString axis_x_title
      = start_title + time_scale_abreviation + time_suffix + end_title;

  axis_x_->setTitleText(axis_x_title);
  axis_x_->append(time_values);
  axis_x_->setGridLineVisible(false);
  axis_x_->setVisible(true);

  QStackedBarSeries* series = new QStackedBarSeries(this);

  series->append(neg_set);
  series->append(pos_set);
  series->setBarWidth(1.0);
  chart_->addSeries(series);

  axis_y_->setTitleText("Number of Endpoints");
  axis_y_->setRange(0, max_y);
  axis_y_->setTickCount(15);
  axis_y_->setLabelFormat("%i");
  axis_y_->setVisible(true);

  series->attachAxis(axis_x_);
  series->attachAxis(axis_y_);

  chart_->legend()->markers(series)[0]->setVisible(false);
  chart_->legend()->markers(series)[1]->setVisible(false);
  chart_->legend()->setVisible(true);
  chart_->legend()->setAlignment(Qt::AlignBottom);
}

}  // namespace gui
