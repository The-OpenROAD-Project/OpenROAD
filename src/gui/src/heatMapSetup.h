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

#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>

#include "gui/heatMap.h"

namespace gui {

class HeatMapSetup : public QDialog
{
  Q_OBJECT
 public:
  HeatMapSetup(HeatMapDataSource& source,
               const QString& title,
               bool use_dbu,
               int dbu,
               QWidget* parent = nullptr);

 signals:
  void changed();

 private slots:
  void updateShowNumbers(int option);
  void updateShowLegend(int option);
  void updateShowMinRange(int option);
  void updateShowMaxRange(int option);
  void updateScale(int option);
  void updateReverseScale(int option);
  void updateAlpha(int alpha);
  void updateRange();
  void updateGridSize();
  void updateWidgets();

  void destroyMap();

 private:
  HeatMapDataSource& source_;
  bool use_dbu_;
  int dbu_;

  QCheckBox* log_scale_;
  QCheckBox* reverse_log_scale_;
  QCheckBox* show_numbers_;
  QCheckBox* show_legend_;

  QDoubleSpinBox* grid_x_size_;
  QDoubleSpinBox* grid_y_size_;
  QSpinBox* grid_x_size_dbu_;
  QSpinBox* grid_y_size_dbu_;

  QDoubleSpinBox* min_range_selector_;
  QCheckBox* show_mins_;
  QDoubleSpinBox* max_range_selector_;
  QCheckBox* show_maxs_;

  QSpinBox* alpha_selector_;

  QPushButton* rebuild_;
  QPushButton* close_;

  void addBooleanOption(QFormLayout* layout,
                        const HeatMapDataSource::MapSettingBoolean& option);
  void addMultiChoiceOption(
      QFormLayout* layout,
      const HeatMapDataSource::MapSettingMultiChoice& option);
};

}  // namespace gui
