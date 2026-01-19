// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <QDialog>
#include <QWidget>

#include "layoutTabs.h"
#include "ui_gotoDlg.h"

namespace gui {
class GotoLocationDialog : public QDialog, public Ui::GotoLocDialog
{
  Q_OBJECT
 private:
  LayoutTabs* viewers_;

 public:
  GotoLocationDialog(QWidget* parent = nullptr, LayoutTabs* viewers = nullptr);
 public slots:
  void updateLocation();
  void showInit();
  void goTo();
  void accept() override;
};
}  // namespace gui
