//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <string>

#include "gui/gui.h"

namespace gui {

SelectHighlightModel::SelectHighlightModel(const SelectionSet& objs)
    : objs_(objs)
{
}

void SelectHighlightModel::populateModel()
{
  beginResetModel();
  table_data_.clear();
  table_data_.reserve(objs_.size());
  for (auto& obj : objs_) {
    table_data_.push_back(&obj);
  }
  endResetModel();
}

int SelectHighlightModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return objs_.size();
}

int SelectHighlightModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 3;
}

QVariant SelectHighlightModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  int row_index = index.row();
  if (row_index > table_data_.size())
    return QVariant();
  std::string obj_name = table_data_[row_index]->getName();
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
    return QString::fromStdString(table_data_[row_index]->getLocation());
  }
  return QVariant();
}

QVariant SelectHighlightModel::headerData(int section,
                                          Qt::Orientation orientation,
                                          int role) const
{
  if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
    if (section == 0) {
      return QString("Object");
    } else if (section == 1) {
      return QString("Type");
    } else if (section == 2) {
      return QString("Loc");
    }
  }
  return QVariant();
}

SelectHighlightWindow::SelectHighlightWindow(const SelectionSet& selection_set,
                                             const SelectionSet& highlight_set,
                                             QWidget* parent)
    : QDockWidget(parent),
      ui_(new Ui::SelectHighlightWidget),
      selection_model_(selection_set),
      highlight_model_(highlight_set),
      select_context_menu_(new QMenu(this)),
      highlight_context_menu_(new QMenu(this))
{
  ui_->setupUi(this);

  QSortFilterProxyModel* sel_filter_proxy = new QSortFilterProxyModel(this);
  sel_filter_proxy->setSourceModel(&selection_model_);

  QSortFilterProxyModel* hlt_filter_proxy = new QSortFilterProxyModel(this);
  hlt_filter_proxy->setSourceModel(&highlight_model_);

  ui_->selTableView->setModel(sel_filter_proxy);
  ui_->hltTableView->setModel(hlt_filter_proxy);

  connect(ui_->findEditInSel, &QLineEdit::returnPressed, this, [this]() {
    this->ui_->selTableView->keyboardSearch(ui_->findEditInSel->text());
  });
  connect(ui_->findEditInHlt, &QLineEdit::returnPressed, this, [this]() {
    this->ui_->hltTableView->keyboardSearch(ui_->findEditInSel->text());
  });

  connect(ui_->selTableView,
          SIGNAL(customContextMenuRequested(QPoint)),
          this,
          SLOT(showSelectCustomMenu(QPoint)));
  connect(ui_->hltTableView,
          SIGNAL(customContextMenuRequested(QPoint)),
          this,
          SLOT(showHighlightCustomMenu(QPoint)));
  ui_->selTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  ui_->hltTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  QAction* remove_sel_item_act = select_context_menu_->addAction("De-Select");
  QAction* remove_all_sel_items = select_context_menu_->addAction("Clear All");
  QAction* highlight_sel_item_act
      = select_context_menu_->addAction("Highlight");
  select_context_menu_->addSeparator();
  QAction* show_sel_item_act
      = select_context_menu_->addAction("Zoom In Layout");

  connect(
      remove_sel_item_act, SIGNAL(triggered()), this, SLOT(deselectItems()));
  connect(remove_all_sel_items, &QAction::triggered, this, [this]() {
    emit clearAllSelections();
  });
  connect(highlight_sel_item_act,
          SIGNAL(triggered()),
          this,
          SLOT(highlightSelectedItems()));
  connect(show_sel_item_act,
          SIGNAL(triggered()),
          this,
          SLOT(zoomInSelectedItems()));

  QAction* remove_hlt_item_act
      = highlight_context_menu_->addAction("De-Highlight");
  QAction* remove_all_hlt_items
      = highlight_context_menu_->addAction("Clear All");
  highlight_context_menu_->addSeparator();
  QAction* show_hlt_item_act
      = highlight_context_menu_->addAction("Zoom In Layout");

  connect(
      remove_hlt_item_act, SIGNAL(triggered()), this, SLOT(dehighlightItems()));
  connect(remove_all_hlt_items, &QAction::triggered, this, [this]() {
    emit clearAllHighlights();
  });
  connect(show_hlt_item_act,
          SIGNAL(triggered()),
          this,
          SLOT(zoomInHighlightedItems()));

  ui_->selTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui_->hltTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

SelectHighlightWindow::~SelectHighlightWindow()
{
}

void SelectHighlightWindow::updateSelectionModel()
{
  selection_model_.populateModel();
}

void SelectHighlightWindow::updateHighlightModel()
{
  highlight_model_.populateModel();
}

void SelectHighlightWindow::showSelectCustomMenu(QPoint pos)
{
  select_context_menu_->popup(ui_->selTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::showHighlightCustomMenu(QPoint pos)
{
  highlight_context_menu_->popup(
      ui_->hltTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::deselectItems()
{
  auto sel_indices = ui_->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit clearSelectedItems(desel_items);
}
void SelectHighlightWindow::highlightSelectedItems()
{
  auto sel_indices = ui_->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> sel_items;
  for (auto& sel_item : sel_indices) {
    sel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit highlightSelectedItemsSig(sel_items);
}

void SelectHighlightWindow::zoomInSelectedItems()
{
  auto sel_indices = ui_->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(desel_items);
}

void SelectHighlightWindow::dehighlightItems()
{
  auto sel_indices = ui_->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit clearHighlightedItems(dehlt_items);
}

void SelectHighlightWindow::zoomInHighlightedItems()
{
  auto sel_indices = ui_->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(dehlt_items);
}

}  // namespace gui
