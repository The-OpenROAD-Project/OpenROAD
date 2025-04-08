// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

#include "odb/db.h"

namespace gui {

class GlobalConnectDialog : public QDialog
{
  Q_OBJECT
 public:
  GlobalConnectDialog(odb::dbBlock* block, QWidget* parent = nullptr);

 signals:
  void connectionsMade(int connections);

 private slots:
  void runRules();
  void clearRules();
  void deleteRule(odb::dbGlobalConnect* gconnect);
  void makeRule();
  void announceConnections(int connections);
  void addRegexTextChanged(const QString& text);

 private:
  odb::dbBlock* block_;

  QGridLayout* layout_;

  QPushButton* add_;
  QPushButton* clear_;
  QPushButton* run_;

  struct GlobalConnectWidgets
  {
    QLineEdit* inst_pattern;
    QLineEdit* pin_pattern;
    QLineEdit* net;
    QLineEdit* region;

    QPushButton* run;
    QPushButton* remove;
  };
  std::map<odb::dbGlobalConnect*, GlobalConnectWidgets> rules_;

  QLineEdit* inst_pattern_;
  QLineEdit* pin_pattern_;
  QComboBox* net_;
  QComboBox* region_;

  QLabel* connections_;

  void addRule(odb::dbGlobalConnect* gc);
  void runRule(odb::dbGlobalConnect* gc);
};

}  // namespace gui
