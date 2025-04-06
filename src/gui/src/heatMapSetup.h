// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

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
