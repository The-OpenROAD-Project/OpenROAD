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

#include <QDockWidget>
#include <QLabel>

#ifdef ENABLE_CHARTS
#include <QComboBox>
#include <QPushButton>
#include <QString>
#include <QtCharts>
#include <memory>

#include "gui/gui.h"

namespace sta {
class dbSta;
}
#endif

namespace gui {

class ChartsWidget : public QDockWidget
{
  Q_OBJECT

 public:
  ChartsWidget(QWidget* parent = nullptr);
#ifdef ENABLE_CHARTS
  enum Mode
  {
    SELECT,
    SLACK_HISTOGRAM
  };
  void setSTA(sta::dbSta* sta)
  {
    sta_ = sta;
  };
  void setLogger(utl::Logger* logger);

 private slots:
  void changeMode();
  void showToolTip(bool is_hovering, int bar_index);

 private:
  void setSlackMode();
  void clearChart();
  void getSlackForAllEndpoints(std::vector<float>& all_slack) const;
  void populateBuckets(const std::vector<float>& all_slack,
                       std::deque<int>& neg_buckets,
                       std::deque<int>& pos_buckets);
  int computeSnapBucketInterval(float exact_interval);
  void setBucketInterval(float bucket_interval);
  void setNegativeCountOffset(int neg_count_offset);
  void setXAxisConfig(int all_bars_count);
  void setYAxisConfig();
  int computeMaxYSnap();
  int computeNumberOfDigits(int value);
  int computeFirstDigit(int value, int digits);
  int computeYInterval();

  utl::Logger* logger_;
  sta::dbSta* sta_;

  QComboBox* mode_menu_;
  QChart* chart_;
  QChartView* display_;
  QValueAxis* axis_x_;
  QValueAxis* axis_y_;

  const int number_of_buckets_ = 15;
  int largest_slack_count_ = 0;  // Used to configure the y axis.

  float bucket_interval_ = 0;
  int neg_count_offset_ = 0;
#endif
  QLabel* label_;
};

}  // namespace gui
