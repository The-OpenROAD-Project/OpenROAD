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

#include <QColor>
#include <QDialog>
#include <QListWidgetItem>
#include <QString>
#include <QVector>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "db.h"
#include "ord/OpenRoad.hh"
#include "ui_congestionSetup.h"

namespace gui {
class CongestionSetupDialog : public QDialog, private Ui::CongestionSetup
{
  Q_OBJECT
 public:
  CongestionSetupDialog(QWidget* parent = nullptr);

 signals:
  void congestionSetupChanged();
  void applyCongestionRequested();

 public slots:
  void accept() override;
  void reject() override;
  void colorIntervalChanged(int value);
  void colorIntervalItemDblClicked(QListWidgetItem* item);

  void congestionBandChanged(int value);

  void saveState();
  bool showHorizontalCongestion() const { return horCongDir->isChecked(); }
  bool showVerticalCongestion() const { return verCongDir->isChecked(); }
  int getMinCongestionValue() const
  {
    return startCongestionSpinBox->value();
  }
  int getMaxCongestionValue() const { return endCongestionSpinBox->value(); }

  QColor getCongestionColorForPercentage(float percent, int alpha = 100) const;

 private:
  struct CongestionBandInfo
  {
    CongestionBandInfo(const QString band_color = "",
                       const std::string style_sheet = "")
        : band_color_(band_color), color_style_sheet_(style_sheet)
    {
    }
    QColor band_color_;
    std::string color_style_sheet_;  // To be Used on Color Gradient Button
  };

  enum GCellDirection
  {
    HORIZONTAL_DIR,
    VERTICAL_DIR,

    BOTH_DIR
  };
  using ColorRangeMap
      = std::map<int, std::vector<CongestionBandInfo>>;  // Key : Number Of
                                                         //       Intervals,
                                                         // Value :
                                                         // CongestionBandInfo
                                                         // Of Each Band
  void updateCongestionButtonStyleSheet(const std::string& color_values);
  std::string getCongestionButtonStyleSheetColors();
  std::vector<QString> getColorNames(int band_index);

  // Dialog State
  int min_congestion_;
  int max_congestion_;
  QVector<QColor> colors_;
  GCellDirection cong_dir_index_;

  static ColorRangeMap _color_ranges;
};  // namespace gui
}  // namespace gui
