// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <QComboBox>
#include <QDockWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <string>

#include "gui/gui.h"

namespace gui {

class HelpWidget : public QDockWidget
{
  Q_OBJECT

 public:
  HelpWidget(QWidget* parent = nullptr);

  void init(const std::string& path);
  bool hasHelp() const { return has_help_; }
  void selectHelp(const std::string& item);

 public slots:
  void changeCategory();
  void showHelpInformation(QListWidgetItem* item);

 private:
  QComboBox* category_selector_;
  QListWidget* help_list_;
  QTextBrowser* viewer_;

  bool has_help_;
};

}  // namespace gui
