// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <qchar.h>
#include <qobjectdefs.h>

#include <QComboBox>
#include <QStandardItem>
#include <QStandardItemModel>

namespace sta {
class dbSta;
class Clock;
}  // namespace sta

namespace gui {
class qComboCheckBoxes : public QComboBox
{
  Q_OBJECT
 public:
  qComboCheckBoxes(const char* name,
                   const char* all_text = "All",
                   QWidget* parent = nullptr);
  qComboCheckBoxes(const QString& name,
                   const char* all_text = "All",
                   QWidget* parent = nullptr);
  void clear();

  void setName(const char* name) { name_item_->setText(QString(name)); };
  void setName(const QString& name) { name_item_->setText(name); };
  QString getName() { return name_item_->text(); };
  QStandardItem* getNameItem() { return name_item_; };

  void setAllText(const char* text) { all_item_->setText(QString(text)); };
  void setAllText(const QString& text) { all_item_->setText(text); };
  QString getAllText() { return all_item_->text(); };
  QStandardItem* getAllItem() { return all_item_; };

  QStandardItemModel* model() { return model_; };
  std::vector<QString> selectedItems();

 public slots:
  void itemChanged(QStandardItem* item);

 private:
  QStandardItem* name_item_;
  QStandardItem* all_item_;
  QStandardItemModel* model_;
  bool updating_check_boxes_;

  void setup();
  void updateName();
};

}  // namespace gui
