// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <qchar.h>
#include <qnamespace.h>
#include <qobjectdefs.h>

#include <QComboBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QWidget>
#include <string>
#include <vector>

namespace sta {
class dbSta;
class Clock;
}  // namespace sta

namespace gui {
class DropdownCheckboxes : public QComboBox
{
  Q_OBJECT
 public:
  DropdownCheckboxes(const std::string& name,
                     const std::string& all_text = "All",
                     QWidget* parent = nullptr);
  DropdownCheckboxes(const QString& name,
                     const QString& all_text = "All",
                     QWidget* parent = nullptr);
  void clear();

  void setName(const std::string& name)
  {
    name_item_->setText(QString::fromStdString(name));
  };
  void setName(const QString& name) { name_item_->setText(name); };
  QString getName() { return name_item_->text(); };
  QStandardItem* getNameItem() { return name_item_; };

  void setAllText(const std::string& text)
  {
    all_item_->setText(QString::fromStdString(text));
  };
  void setAllText(const QString& text) { all_item_->setText(text); };
  QString getAllText() { return all_item_->text(); };
  QStandardItem* getAllItem() { return all_item_; };

  QStandardItemModel* model() { return model_; };
  bool isAllSelected() { return all_item_->checkState() == Qt::Checked; };
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
