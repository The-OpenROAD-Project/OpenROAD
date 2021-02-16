///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, OpenROAD
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
#include "openroad/OpenRoad.hh"
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

  void designLoaded(odb::dbBlock* block);
  void colorIntervalChanged(int value);
  void colorIntervalItemDblClicked(QListWidgetItem* item);

  void congestionStartValueChanged(int value);

  void saveState();
  bool showHorizontalCongestion() const { return horCongDir->isChecked(); }
  bool showVerticalCongestion() const { return verCongDir->isChecked(); }
  int showStartCongestionValue() const
  {
    return startCongestionSpinBox->value();
  }

  int showCongestionFrom() const { return min_congestion_; }
  float getMinCongestionToShow() const
  {
    return showCongestionFrom() + showStartCongestionValue();
  }
  QColor getCongestionColorForPercentage(float percent, int alpha = 100) const;

 private:
  struct CongestionBandInfo
  {
    CongestionBandInfo(const QString band_color = "",
                       const std::string style_sheet = "",
                       int ulimit = 100)
        : band_color_(band_color),
          color_style_sheet_(style_sheet),
          band_ulimit_(ulimit)
    {
    }
    QColor band_color_;
    std::string color_style_sheet_;  // To be Used on Color Gradient Button
    int band_ulimit_;
  };

  enum GCELL_DIRECTION
  {
    GCELL_HORIZONTAL_DIR = 0,
    GCELL_VERTICAL_DIR,

    GCELL_BOTH_DIR
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
  QVector<QColor> colors_;
  GCELL_DIRECTION cong_dir_index_;

  static ColorRangeMap _color_ranges;
};  // namespace gui
}  // namespace gui