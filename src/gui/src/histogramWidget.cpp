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

#include "histogramWidget.h"

#include <QWidget>
#include <QFrame>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <set>

#include "staGuiInterface.h"

namespace gui {

HistogramWidget::HistogramWidget(QWidget* parent)
    : QDockWidget("Histogram Plotter", parent),
      sta_(nullptr),
      settings_button_(new QPushButton("Settings", this)),
      mode_menu_(new QComboBox(this)),
      bar_set_(new QBarSet("Slack [ns]")),
      series_(new QBarSeries(this)),
      chart_(new QChart),
      values_x_(new QBarCategoryAxis(this)),
      values_y_(new QValueAxis(this))    
{
  setObjectName("histogram_widget"); // for settings

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  QFrame* controls_frame = new QFrame(this);
  QChartView *display = new QChartView(chart_);
  QHBoxLayout* controls_layout = new QHBoxLayout;

  mode_menu_->addItem("Select a Category");
  mode_menu_->addItem("End-Point Slack");
  mode_menu_->addItem("Congestion");

  controls_layout->addWidget(mode_menu_);
  controls_layout->addWidget(settings_button_);
  controls_layout->insertStretch(1);

  controls_frame->setLayout(controls_layout);
  controls_frame->setFrameShape(QFrame::StyledPanel);
  controls_frame->setFrameShadow(QFrame::Raised);  

  layout->addWidget(controls_frame);
  layout->addWidget(display);
  container->setLayout(layout);
  setWidget(container);

  connect(mode_menu_,
          SIGNAL(currentIndexChanged(int)),
          this,
          SLOT(populateChart()));

  connect(settings_button_,
          SIGNAL(clicked()),
          this,
          SLOT());  

}

void HistogramWidget::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
}

void HistogramWidget::populateChart()
{
  if(mode_menu_->currentIndex() == 1) {

    STAGuiInterface sta_gui(sta_);

    int unused_count = 0;
    float pin_slack = 0;
    auto end_points = sta_gui.getEndPoints();
    auto pin_iterator = end_points.begin();
    std::vector<float> pos_slack;
    std::vector<float> neg_slack;   

    // Converts slack time values to (ns)
    for(int i = 0; i < end_points.size(); i++) {
      pin_slack = sta_gui.getPinSlack(*pin_iterator);
      if(pin_slack > 0 && pin_slack < 1) {
        pos_slack.push_back(pin_slack*1e9);
      } else if (pin_slack < 0) {
        neg_slack.push_back(pin_slack*1e9);
      } else {
        unused_count++;
      }
      pin_iterator++;
    }

    std::cout << "-----------------------------------------" << std::endl;
    std::cout << "Number of end-points with POSITIVE slack: "
              << pos_slack.size()
              << std::endl;
    std::cout << "Number of end-points with NEGATIVE slack: "
              << neg_slack.size()
              << std::endl;
    std::cout << std::endl;
    std::cout << "Number of valid end-points: " << pos_slack.size() + neg_slack.size() << std::endl;
    std::cout << "Number of invalid end-points: " << unused_count << std::endl;
    std::cout << std::endl;
    std::cout << "Total end-points: " << end_points.size() << std::endl;
    std::cout << "-----------------------------------------";

    auto max_pos_slack = std::max_element(pos_slack.begin(), pos_slack.end());
    auto min_neg_slack = std::min_element(neg_slack.begin(), neg_slack.end());

    // +1 for values from '-1ns to 0' and '0 to 1ns'
    int buckets_num = ((abs(*min_neg_slack))+1)+((*max_pos_slack)+1);

    std::cout << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "     Handling slack values    " << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << std::endl;
    std::cout << "Necessary amount of buckets: " << buckets_num << std::endl;

    std::vector<float> buckets[buckets_num];
    int offset = abs(*min_neg_slack);

    //Fills buckets with negative values
    for(int i = 0; i < neg_slack.size(); ++i) {
      int bucket_index = neg_slack[i];
      buckets[bucket_index+offset].push_back(neg_slack[i]);
    }

    //..then the positives respectively
    for(int i = 0; i < pos_slack.size(); ++i) {
      int bucket_index = pos_slack[i];
      buckets[bucket_index+(offset+1)].push_back(pos_slack[i]);
    }

    int count = 0;
    int max_count = 0;

    for(int i = 0; i < buckets_num; ++i) {
      for(int j = 0; j < buckets[i].size(); ++j) {
          count++;
      }
      if(count > max_count)
        max_count = count;
      std::cout << std::endl;
      std::cout << "Bucket[" << i << "] = "; 
      std::cout << count << std::endl;
      *bar_set_ << count;
      count = 0;
    }

    chart_->setTitle("End-Point Slack");
    bar_set_->setColor(0x008000);

    series_->append(bar_set_);
    chart_->addSeries(series_);

    QStringList time_values;
    time_values << "    -1" 
                << "     0"
                << "     1" 
                << "     2" 
                << "     3" 
                << "     4";

    values_x_->append(time_values);

    //Essa escala vai depender da leitura da quantidade do número de pinos
    values_y_->setRange(0,max_count);
    values_y_->setTickCount(10);
    values_y_->setLabelFormat("%i");

    chart_->addAxis(values_x_, Qt::AlignBottom);
    series_->attachAxis(values_x_);
    chart_->addAxis(values_y_, Qt::AlignLeft);
    series_->attachAxis(values_y_);

    chart_->legend()->setVisible(true);
    chart_->legend()->setAlignment(Qt::AlignBottom);

  }
  
}

}
