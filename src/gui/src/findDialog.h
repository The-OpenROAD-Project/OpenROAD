// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QDialog>

#include "ui_findDlg.h"

namespace gui {
class FindObjectDialog : public QDialog, public Ui::FindObjDialog
{
  Q_OBJECT
 public:
  FindObjectDialog(QWidget* parent = nullptr);
 public slots:
  void accept() override;
  void reject() override;
  int exec() override;
};
}  // namespace gui
