// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QDialog>
#include <QWidget>

#include "gui/gui.h"
#include "ui_highlightGroupDlg.h"

namespace gui {
class HighlightGroupDialog : public QDialog, private Ui::HighlightGroupDlg
{
  Q_OBJECT
 public:
  HighlightGroupDialog(QWidget* parent = nullptr);
  int getSelectedHighlightGroup() const;

 public slots:
  void accept() override;

 private:
  void setButtonBackground(QRadioButton* button, Painter::Color color);
};  // namespace gui
}  // namespace gui
