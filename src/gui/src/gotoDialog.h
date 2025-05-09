// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <QDialog>

#include "layoutTabs.h"
#include "ui_gotoDlg.h"

namespace gui {
class GotoLocationDialog : public QDialog, public Ui::GotoLocDialog
{
  Q_OBJECT
 private:
  LayoutTabs* viewers_;

 public:
  GotoLocationDialog(QWidget* parent = nullptr, LayoutTabs* viewers_ = nullptr);
 public slots:
  void updateLocation(QLineEdit* xEdit, QLineEdit* yEdit);
  void updateUnits(int dbu_per_micron, bool useDBU);
  void show_init();
  void accept() override;
};
}  // namespace gui
