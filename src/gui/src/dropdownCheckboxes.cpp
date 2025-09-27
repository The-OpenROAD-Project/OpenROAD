// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "dropdownCheckboxes.h"

#include <qnamespace.h>

#include <QComboBox>
#include <QString>
#include <QWidget>
#include <string>
#include <vector>

namespace gui {

DropdownCheckboxes::DropdownCheckboxes(const std::string& name,
                                       const std::string& all_text,
                                       QWidget* parent)
    : QComboBox(parent),
      name_item_(new QStandardItem(QString::fromStdString(name))),
      all_item_(new QStandardItem(QString::fromStdString(all_text))),
      model_(new QStandardItemModel(this)),
      updating_check_boxes_(false)
{
  setup();
}

DropdownCheckboxes::DropdownCheckboxes(const QString& name,
                                       const QString& all_text,
                                       QWidget* parent)
    : QComboBox(parent),
      name_item_(new QStandardItem(name)),
      all_item_(new QStandardItem(all_text)),
      model_(new QStandardItemModel(this)),
      updating_check_boxes_(false)
{
  setup();
}

void DropdownCheckboxes::setup()
{
  name_item_->setFlags(Qt::NoItemFlags);
  all_item_->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled
                      | Qt::ItemIsAutoTristate);
  all_item_->setData(Qt::Checked, Qt::CheckStateRole);
  model_->appendRow(name_item_);
  model_->appendRow(all_item_);

  setModel(model_);
  setCurrentIndex(0);

  connect(model_,
          &QStandardItemModel::itemChanged,
          this,
          &DropdownCheckboxes::itemChanged);
}

void DropdownCheckboxes::clear()
{
  model_->removeRows(2, model_->rowCount() - 2);
}

void DropdownCheckboxes::itemChanged(QStandardItem* item)
{
  if (updating_check_boxes_) {
    return;
  }

  updating_check_boxes_ = true;
  if (item == all_item_) {  // set all check boxes to all_item_ state
    Qt::CheckState state = all_item_->checkState();
    if (state == Qt::PartiallyChecked) {
      return;
    }
    for (auto row = 2; row < model_->rowCount(); row++) {
      QStandardItem* item_iter = model_->item(row);
      if (item_iter->checkState() != state) {
        item_iter->setCheckState(state);
      }
    }
  } else {  // updates all_item_ state
    bool all_checked = true;
    bool all_unchecked = true;
    for (auto row = 2; row < model_->rowCount(); row++) {
      QStandardItem* item_iter = model_->item(row);
      if (item_iter->checkState() == Qt::Unchecked) {
        all_checked = false;
      } else {
        all_unchecked = false;
      }
    }
    if (all_checked) {
      all_item_->setCheckState(Qt::Checked);
    } else if (all_unchecked) {
      all_item_->setCheckState(Qt::Unchecked);
    } else {
      all_item_->setCheckState(Qt::PartiallyChecked);
    }
  }
  updating_check_boxes_ = false;
}

std::vector<QString> DropdownCheckboxes::selectedItems()
{
  std::vector<QString> selected;
  for (auto row = 2; row < model_->rowCount(); row++) {
    QStandardItem* item_iter = model_->item(row);
    if (item_iter->checkState() == Qt::Checked) {
      selected.push_back(item_iter->text());
    }
  }
  return selected;
}

}  // namespace gui