//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "selectHighlightWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <string>

#include "gui/gui.h"

namespace gui {

SelectionModel::SelectionModel(const SelectionSet& objs) : objs_(objs)
{
}

void SelectionModel::populateModel()
{
  beginResetModel();
  table_data_.clear();
  table_data_.reserve(objs_.size());
  for (auto& obj : objs_) {
    table_data_.push_back(&obj);
  }
  endResetModel();
}

int SelectionModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return objs_.size();
}

int SelectionModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 3;
}

QVariant SelectionModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  unsigned int row_index = index.row();
  if (row_index > table_data_.size())
    return QVariant();
  const std::string obj_name = table_data_[row_index]->getName();
  const std::string obj_type = table_data_[row_index]->getTypeName();
  if (index.column() == 0) {
    return QString::fromStdString(obj_name);
  } else if (index.column() == 1) {
    return QString::fromStdString(obj_type);
  } else if (index.column() == 2) {
    odb::Rect bbox;
    bool valid = table_data_[row_index]->getBBox(bbox);
    return valid ? QString::fromStdString(Descriptor::Property::toString(bbox))
                 : "<none>";
  }
  return QVariant();
}

QVariant SelectionModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section == 0) {
      return QString("Object");
    } else if (section == 1) {
      return QString("Type");
    } else if (section == 2) {
      return QString("Bounds");
    }
  }
  return QVariant();
}

HighlightModel::HighlightModel(const HighlightSet& objs) : objs_(objs)
{
}

void HighlightModel::populateModel()
{
  beginResetModel();
  table_data_.clear();
  table_data_.reserve(objs_.size());
  int highlight_group = 0;
  for (auto& highlight_objs : objs_) {
    for (auto& obj : highlight_objs) {
      table_data_.push_back(std::make_pair(highlight_group, &obj));
    }
    ++highlight_group;
  }
  endResetModel();
}

int HighlightModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return table_data_.size();
}

int HighlightModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 4;
}

QVariant HighlightModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid()
      || (role != Qt::DisplayRole && role != Qt::EditRole
          && role != Qt::BackgroundRole)) {
    return QVariant();
  }
  if (role == Qt::BackgroundRole && index.column() != 3)
    return QVariant();
  else if (role == Qt::BackgroundRole && index.column() == 3) {
    auto highlight_color
        = Painter::highlightColors[table_data_[index.row()].first];
    return QColor(highlight_color.r,
                  highlight_color.g,
                  highlight_color.b,
                  highlight_color.a);
  }
  unsigned int row_index = index.row();
  if (row_index > table_data_.size())
    return QVariant();
  std::string obj_name = table_data_[row_index].second->getName();
  std::string obj_type("");
  if (obj_name.rfind("Net: ", 0) == 0) {
    obj_name = obj_name.substr(5);
    obj_type = "Net";
  } else if (obj_name.rfind("Inst: ", 0) == 0) {
    obj_name = obj_name.substr(6);
    obj_type = "Instance";
  }
  if (index.column() == 0) {
    return QString::fromStdString(obj_name);
  } else if (index.column() == 1) {
    return QString::fromStdString(obj_type);
  } else if (index.column() == 2) {
    odb::Rect bbox;
    bool valid = table_data_[row_index].second->getBBox(bbox);
    return valid ? QString::fromStdString(Descriptor::Property::toString(bbox))
                 : "<none>";
  } else if (index.column() == 3) {
    QString group_string
        = QString("Group ") + QString::number(table_data_[row_index].first + 1);
    return group_string;
  }
  return QVariant();
}

QVariant HighlightModel::headerData(int section,
                                    Qt::Orientation orientation,
                                    int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section == 0) {
      return QString("Object");
    } else if (section == 1) {
      return QString("Type");
    } else if (section == 2) {
      return QString("Bounds");
    } else if (section == 3) {
      return QString("Highlight Group");
    }
  }
  return QVariant();
}

int HighlightModel::highlightGroup(const QModelIndex& index) const
{
  int row_index = index.row();
  return table_data_[row_index].first;
}

bool HighlightModel::setData(const QModelIndex& index,
                             const QVariant& value,
                             int role)
{
  return false;
}

SelectHighlightWindow::SelectHighlightWindow(const SelectionSet& sel_set,
                                             const HighlightSet& hlt_set,
                                             QWidget* parent)
    : QDockWidget(parent),
      ui_(),
      selection_model_(sel_set),
      sel_filter_proxy_(new QSortFilterProxyModel(this)),
      highlight_model_(hlt_set),
      hlt_filter_proxy_(new QSortFilterProxyModel(this)),
      select_context_menu_(new QMenu(this)),
      highlight_context_menu_(new QMenu(this))
{
  ui_.setupUi(this);

  sel_filter_proxy_->setSourceModel(&selection_model_);
  hlt_filter_proxy_->setSourceModel(&highlight_model_);

  ui_.selTableView->setModel(sel_filter_proxy_);
  ui_.hltTableView->setModel(hlt_filter_proxy_);

  connect(ui_.findEditInSel, &QLineEdit::returnPressed, this, [this]() {
    ui_.selTableView->keyboardSearch(ui_.findEditInSel->text());
  });
  connect(ui_.findEditInHlt, &QLineEdit::returnPressed, this, [this]() {
    ui_.hltTableView->keyboardSearch(ui_.findEditInSel->text());
  });

  connect(ui_.selTableView,
          &QTableView::customContextMenuRequested,
          this,
          &SelectHighlightWindow::showSelectCustomMenu);
  connect(ui_.hltTableView,
          &QTableView::customContextMenuRequested,
          this,
          &SelectHighlightWindow::showHighlightCustomMenu);
  auto sel_header = ui_.selTableView->horizontalHeader();
  for (int i = 0; i < sel_header->count() - 1; i++) {
    sel_header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
  }
  sel_header->setSectionResizeMode(sel_header->count() - 1,
                                   QHeaderView::Stretch);
  ui_.selTableView->verticalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);
  auto hlt_header = ui_.hltTableView->horizontalHeader();
  for (int i = 0; i < hlt_header->count() - 1; i++) {
    hlt_header->setSectionResizeMode(i, QHeaderView::ResizeToContents);
  }
  hlt_header->setSectionResizeMode(hlt_header->count() - 1,
                                   QHeaderView::Stretch);
  ui_.hltTableView->verticalHeader()->setSectionResizeMode(
      QHeaderView::ResizeToContents);

  QAction* remove_sel_item_act = select_context_menu_->addAction("De-Select");
  QAction* remove_all_sel_items = select_context_menu_->addAction("Clear All");
  QAction* highlight_sel_item_act
      = select_context_menu_->addAction("Highlight");
  select_context_menu_->addSeparator();
  QAction* show_sel_item_act
      = select_context_menu_->addAction("Zoom In Layout");

  connect(remove_sel_item_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::deselectItems);
  connect(remove_all_sel_items, &QAction::triggered, [this]() {
    emit clearAllSelections();
  });
  connect(highlight_sel_item_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::highlightSelectedItems);
  connect(show_sel_item_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::zoomInSelectedItems);

  QAction* remove_hlt_item_act
      = highlight_context_menu_->addAction("De-Highlight");
  QAction* remove_all_hlt_items
      = highlight_context_menu_->addAction("Clear All");
  highlight_context_menu_->addSeparator();
  QAction* show_hlt_item_act
      = highlight_context_menu_->addAction("Zoom In Layout");
  highlight_context_menu_->addSeparator();
  QAction* change_group_act
      = highlight_context_menu_->addAction("Change group");

  connect(remove_hlt_item_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::dehighlightItems);
  connect(remove_all_hlt_items, &QAction::triggered, [this]() {
    emit clearAllHighlights();
  });
  connect(show_hlt_item_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::zoomInHighlightedItems);
  connect(change_group_act,
          &QAction::triggered,
          this,
          &SelectHighlightWindow::changeHighlight);

  connect(ui_.selTableView->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          [this](const QItemSelection& selected_items,
                 const QItemSelection& deselected_items) {
            auto indexes = selected_items.indexes();
            if (indexes.isEmpty()) {
              return;
            }
            emit selected(*selection_model_.getItemAt(
                sel_filter_proxy_->mapToSource(indexes[0]).row()));
          });
  connect(ui_.hltTableView->selectionModel(),
          &QItemSelectionModel::selectionChanged,
          [this](const QItemSelection& selected_items,
                 const QItemSelection& deselected_items) {
            auto indexes = selected_items.indexes();
            if (indexes.isEmpty()) {
              return;
            }
            emit selected(*highlight_model_.getItemAt(
                hlt_filter_proxy_->mapToSource(indexes[0]).row()));
          });

  ui_.selTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui_.hltTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

  ui_.tabWidget->setCurrentIndex(0);
}

SelectHighlightWindow::~SelectHighlightWindow()
{
}

void SelectHighlightWindow::updateSelectionModel()
{
  selection_model_.populateModel();
  ui_.tabWidget->setCurrentWidget(ui_.selTab);
}

void SelectHighlightWindow::updateHighlightModel()
{
  highlight_model_.populateModel();
  ui_.tabWidget->setCurrentWidget(ui_.hltTab);
}

void SelectHighlightWindow::updateModels()
{
  selection_model_.populateModel();
  highlight_model_.populateModel();
}

void SelectHighlightWindow::showSelectCustomMenu(QPoint pos)
{
  select_context_menu_->popup(ui_.selTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::showHighlightCustomMenu(QPoint pos)
{
  highlight_context_menu_->popup(
      ui_.hltTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::deselectItems()
{
  auto sel_indices = ui_.selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit clearSelectedItems(desel_items);
}
void SelectHighlightWindow::highlightSelectedItems()
{
  auto sel_indices = ui_.selTableView->selectionModel()->selectedRows();
  QList<const Selected*> sel_items;
  for (auto& sel_item : sel_indices) {
    sel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit highlightSelectedItemsSig(sel_items);
}

void SelectHighlightWindow::zoomInSelectedItems()
{
  auto sel_indices = ui_.selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(desel_items);
}

void SelectHighlightWindow::dehighlightItems()
{
  auto sel_indices = ui_.hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit clearHighlightedItems(dehlt_items);
}

void SelectHighlightWindow::zoomInHighlightedItems()
{
  auto sel_indices = ui_.hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(dehlt_items);
}

void SelectHighlightWindow::changeHighlight()
{
  auto sel_indices = ui_.hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> items;
  for (auto& sel_item : sel_indices) {
    items << highlight_model_.getItemAt(sel_item.row());
  }
  emit highlightSelectedItemsSig(items);
}

}  // namespace gui
