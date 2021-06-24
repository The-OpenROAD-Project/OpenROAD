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

#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <iomanip>
#include <sstream>
#include <string>

#include "gui/gui.h"
#include "highlightGroupDialog.h"
#include "selectHighlightWindow.h"

namespace gui {

static QString getBBoxString(odb::Rect& bbox, odb::dbDatabase* db)
{
  double to_microns = db->getChip()->getBlock()->getDbUnitsPerMicron();
  std::stringstream ss;
  ss << std::fixed << std::setprecision(5) << "(";
  ss << bbox.xMin() / to_microns << ",";
  ss << bbox.yMin() / to_microns << "), (";
  ss << bbox.xMax() / to_microns << ",";
  ss << bbox.yMax() / to_microns << ")";
  return QString::fromStdString(ss.str());
}

SelectionModel::SelectionModel(const SelectionSet& objs)
    : db_(nullptr), objs_(objs)
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
    return valid ? getBBoxString(bbox, db_) : "<none>";
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

HighlightModel::HighlightModel(const HighlightSet& objs)
    : db_(nullptr), objs_(objs)
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
    return QColor(highlight_color.r, highlight_color.g, highlight_color.b, 100);
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
    return valid ? getBBoxString(bbox, db_) : "<none>";
  } else if (index.column() == 3) {
    return QString("Group ")
           + QString::number(table_data_[row_index].first + 1);
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

HighlightGroupDelegate::HighlightGroupDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
  items_.push_back("Group 1");
  items_.push_back("Group 2");
  items_.push_back("Group 3");
  items_.push_back("Group 4");
  items_.push_back("Group 5");
  items_.push_back("Group 6");
  items_.push_back("Group 7");
  items_.push_back("Group 8");
}

QWidget* HighlightGroupDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& index) const
{
  QComboBox* editor = new QComboBox(parent);
  for (unsigned int i = 0; i < items_.size(); ++i) {
    editor->addItem(items_[i].c_str());
  }
  editor->setFrame(true);
  editor->setEditable(true);
  return editor;
}

void HighlightGroupDelegate::setEditorData(QWidget* editor,
                                           const QModelIndex& index) const
{
  QString value = index.model()->data(index, Qt::EditRole).toString();
  QComboBox* combo_box = static_cast<QComboBox*>(editor);
  combo_box->setCurrentText(value);
}

void HighlightGroupDelegate::setModelData(QWidget* editor,
                                          QAbstractItemModel* model,
                                          const QModelIndex& index) const
{
  QComboBox* combo_box = static_cast<QComboBox*>(editor);
  model->setData(index, combo_box->currentIndex(), Qt::EditRole);
}

void HighlightGroupDelegate::updateEditorGeometry(
    QWidget* editor,
    const QStyleOptionViewItem& option,
    const QModelIndex& /* index */) const
{
  editor->setGeometry(option.rect);
}

void HighlightGroupDelegate::paint(QPainter* painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const
{
  QStyleOptionViewItem my_option = option;

  QString text = index.model()->data(index, Qt::EditRole).toString();
  my_option.text = text;
  QApplication::style()->drawControl(
      QStyle::CE_ItemViewItem, &my_option, painter);
}

SelectHighlightWindow::SelectHighlightWindow(const SelectionSet& sel_set,
                                             const HighlightSet& hlt_set,
                                             QWidget* parent)
    : QDockWidget(parent),
      ui(new Ui::SelectHighlightWidget),
      selection_model_(sel_set),
      highlight_model_(hlt_set),
      select_context_menu_(new QMenu(this)),
      highlight_context_menu_(new QMenu(this))
{
  ui->setupUi(this);

  QSortFilterProxyModel* sel_filter_proxy = new QSortFilterProxyModel(this);
  sel_filter_proxy->setSourceModel(&selection_model_);

  QSortFilterProxyModel* hlt_filter_proxy = new QSortFilterProxyModel(this);
  hlt_filter_proxy->setSourceModel(&highlight_model_);

  ui->selTableView->setModel(sel_filter_proxy);
  ui->hltTableView->setModel(hlt_filter_proxy);

  connect(ui->findEditInSel, &QLineEdit::returnPressed, this, [this]() {
    this->ui->selTableView->keyboardSearch(ui->findEditInSel->text());
  });
  connect(ui->findEditInHlt, &QLineEdit::returnPressed, this, [this]() {
    this->ui->hltTableView->keyboardSearch(ui->findEditInSel->text());
  });

  connect(ui->selTableView,
          SIGNAL(customContextMenuRequested(QPoint)),
          this,
          SLOT(showSelectCustomMenu(QPoint)));
  connect(ui->hltTableView,
          SIGNAL(customContextMenuRequested(QPoint)),
          this,
          SLOT(showHighlightCustomMenu(QPoint)));
  ui->selTableView->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  ui->hltTableView->horizontalHeader()->setSectionResizeMode(
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

  ui->selTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->hltTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

  ui->tabWidget->setCurrentIndex(0);
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
  select_context_menu_->popup(ui->selTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::showHighlightCustomMenu(QPoint pos)
{
  highlight_context_menu_->popup(
      ui->hltTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::deselectItems()
{
  auto sel_indices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit clearSelectedItems(desel_items);
}
void SelectHighlightWindow::highlightSelectedItems()
{
  HighlightGroupDialog dlg;
  dlg.exec();
  auto sel_indices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> sel_items;
  for (auto& sel_item : sel_indices) {
    sel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit highlightSelectedItemsSig(sel_items, dlg.getSelectedHighlightGroup());
}

void SelectHighlightWindow::zoomInSelectedItems()
{
  auto sel_indices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> desel_items;
  for (auto& sel_item : sel_indices) {
    desel_items << selection_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(desel_items);
}

void SelectHighlightWindow::dehighlightItems()
{
  auto sel_indices = ui->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit clearHighlightedItems(dehlt_items);
}

void SelectHighlightWindow::zoomInHighlightedItems()
{
  auto sel_indices = ui->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehlt_items;
  for (auto& sel_item : sel_indices) {
    dehlt_items << highlight_model_.getItemAt(sel_item.row());
  }
  emit zoomInToItems(dehlt_items);
}

void SelectHighlightWindow::setDb(odb::dbDatabase* db)
{
  selection_model_.setDb(db);
  highlight_model_.setDb(db);
}

}  // namespace gui
