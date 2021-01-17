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

#include <QComboBox>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>
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
  int objIdx = 0;
  tableData_.clear();
  tableData_.reserve(objs_.size());
  for (auto& obj : objs_) {
    tableData_.push_back(&obj);
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
  int rowIndex = index.row();
  if (rowIndex > tableData_.size())
    return QVariant();
  std::string objName = tableData_[rowIndex]->getName();
  std::string objType("");
  if (objName.rfind("Net: ", 0) == 0) {
    objName = objName.substr(5);
    objType = "Net";
  } else if (objName.rfind("Inst: ", 0) == 0) {
    objName = objName.substr(6);
    objType = "Instance";
  }
  if (index.column() == 0) {
    return QString::fromStdString(objName);
  } else if (index.column() == 1) {
    return QString::fromStdString(objType);
  } else if (index.column() == 2) {
    return QString::fromStdString(tableData_[rowIndex]->getLocation());
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
      return QString("Loc");
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
  int objIdx = 0;
  tableData_.clear();
  tableData_.reserve(objs_.size());
  int highlightGroup = 0;
  for (auto& highlightObjs : objs_) {
    for (auto& obj : highlightObjs) {
      tableData_.push_back(std::make_pair(highlightGroup, &obj));
    }
    ++highlightGroup;
  }
  endResetModel();
}

int HighlightModel::rowCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return tableData_.size();
  // return objs_.size();
}

int HighlightModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED(parent);
  return 4;
}

QVariant HighlightModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || role != Qt::DisplayRole) {
    return QVariant();
  }
  int rowIndex = index.row();
  if (rowIndex > tableData_.size())
    return QVariant();
  std::string objName = tableData_[rowIndex].second->getName();
  std::string objType("");
  if (objName.rfind("Net: ", 0) == 0) {
    objName = objName.substr(5);
    objType = "Net";
  } else if (objName.rfind("Inst: ", 0) == 0) {
    objName = objName.substr(6);
    objType = "Instance";
  }
  if (index.column() == 0) {
    return QString::fromStdString(objName);
  } else if (index.column() == 1) {
    return QString::fromStdString(objType);
  } else if (index.column() == 2) {
    return QString::fromStdString(tableData_[rowIndex].second->getLocation());
  } else if (index.column() == 3) {
    return QString::number(tableData_[rowIndex].first);
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
      return QString("Loc");
    } else if (section == 3) {
      return QString("Highlight Group");
    }
  }
  return QVariant();
}

bool HighlightModel::setData(const QModelIndex& index,
                             const QVariant& value,
                             int role)
{
  int rowIndex = index.row();
  return true;
}

HighlightGroupDelegate::HighlightGroupDelegate(QObject* parent)
    : QItemDelegate(parent)
{
  Items.push_back("Group 0");
  Items.push_back("Group 1");
  Items.push_back("Group 2");
  Items.push_back("Group 3");
  Items.push_back("Group 4");
  Items.push_back("Group 5");
  Items.push_back("Group 6");
}

QWidget* HighlightGroupDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& /* option */,
    const QModelIndex& /* index */) const
{
  QComboBox* editor = new QComboBox(parent);
  for (unsigned int i = 0; i < Items.size(); ++i) {
    editor->addItem(Items[i].c_str());
  }
  editor->setFrame(false);
  return editor;
}

void HighlightGroupDelegate::setEditorData(QWidget* editor,
                                           const QModelIndex& index) const
{
  QComboBox* comboBox = static_cast<QComboBox*>(editor);
  int value = 0;
  comboBox->setCurrentIndex(value);
}

void HighlightGroupDelegate::setModelData(QWidget* editor,
                                          QAbstractItemModel* model,
                                          const QModelIndex& index) const
{
  QComboBox* comboBox = static_cast<QComboBox*>(editor);
  model->setData(index, comboBox->currentIndex(), Qt::EditRole);
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
  QStyleOptionViewItemV4 myOption = option;
  QString text = Items[index.row()].c_str();

  myOption.text = text;

  QApplication::style()->drawControl(
      QStyle::CE_ItemViewItem, &myOption, painter);
}

SelectHighlightWindow::SelectHighlightWindow(const SelectionSet& selSet,
                                             const HighlightSet& hltSet,
                                             QWidget* parent)
    : QDockWidget(parent),
      ui(new Ui::SelectHighlightWidget),
      selectionModel_(selSet),
      highlightModel_(hltSet),
      selectContextMenu_(new QMenu(this)),
      highlightContextMenu_(new QMenu(this))
{
  ui->setupUi(this);

  QSortFilterProxyModel* selFilterProxy = new QSortFilterProxyModel(this);
  selFilterProxy->setSourceModel(&selectionModel_);

  QSortFilterProxyModel* hltFilterProxy = new QSortFilterProxyModel(this);
  hltFilterProxy->setSourceModel(&highlightModel_);

  ui->selTableView->setModel(selFilterProxy);
  ui->hltTableView->setModel(hltFilterProxy);

  HighlightGroupDelegate* delegate = new HighlightGroupDelegate(this);
  // tableView.setItemDelegate(&delegate);
  ui->hltTableView->setItemDelegateForColumn(3, delegate);

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

  QAction* removeSelItemAct = selectContextMenu_->addAction("De-Select");
  QAction* removeAllSelItems = selectContextMenu_->addAction("Clear All");
  QAction* highlightSelItemAct = selectContextMenu_->addAction("Highlight");
  selectContextMenu_->addSeparator();
  QAction* showSelItemAct = selectContextMenu_->addAction("Zoom In Layout");

  connect(removeSelItemAct, SIGNAL(triggered()), this, SLOT(deselectItems()));
  connect(removeAllSelItems, &QAction::triggered, this, [this]() {
    emit clearAllSelections();
  });
  connect(highlightSelItemAct,
          SIGNAL(triggered()),
          this,
          SLOT(highlightSelectedItems()));
  connect(
      showSelItemAct, SIGNAL(triggered()), this, SLOT(zoomInSelectedItems()));

  QAction* removeHltItemAct = highlightContextMenu_->addAction("De-Highlight");
  QAction* removeAllHltItems = highlightContextMenu_->addAction("Clear All");
  highlightContextMenu_->addSeparator();
  QAction* showHltItemAct = highlightContextMenu_->addAction("Zoom In Layout");

  connect(
      removeHltItemAct, SIGNAL(triggered()), this, SLOT(dehighlightItems()));
  connect(removeAllHltItems, &QAction::triggered, this, [this]() {
    emit clearAllHighlights();
  });
  connect(showHltItemAct,
          SIGNAL(triggered()),
          this,
          SLOT(zoomInHighlightedItems()));

  ui->selTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->hltTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
}

SelectHighlightWindow::~SelectHighlightWindow()
{
}

void SelectHighlightWindow::updateSelectionModel()
{
  selectionModel_.populateModel();
}

void SelectHighlightWindow::updateHighlightModel()
{
  highlightModel_.populateModel();
}

void SelectHighlightWindow::showSelectCustomMenu(QPoint pos)
{
  selectContextMenu_->popup(ui->selTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::showHighlightCustomMenu(QPoint pos)
{
  highlightContextMenu_->popup(ui->hltTableView->viewport()->mapToGlobal(pos));
}

void SelectHighlightWindow::deselectItems()
{
  auto selIndices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> deselItems;
  for (auto& selItem : selIndices) {
    deselItems << selectionModel_.getItemAt(selItem.row());
  }
  emit clearSelectedItems(deselItems);
}
void SelectHighlightWindow::highlightSelectedItems()
{
  auto selIndices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> selItems;
  for (auto& selItem : selIndices) {
    selItems << selectionModel_.getItemAt(selItem.row());
  }
  emit highlightSelectedItemsSig(selItems);
}

void SelectHighlightWindow::zoomInSelectedItems()
{
  auto selIndices = ui->selTableView->selectionModel()->selectedRows();
  QList<const Selected*> deselItems;
  for (auto& selItem : selIndices) {
    deselItems << selectionModel_.getItemAt(selItem.row());
  }
  emit zoomInToItems(deselItems);
}

void SelectHighlightWindow::dehighlightItems()
{
  auto selIndices = ui->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehltItems;
  for (auto& selItem : selIndices) {
    dehltItems << highlightModel_.getItemAt(selItem.row());
  }
  emit clearHighlightedItems(dehltItems);
}

void SelectHighlightWindow::zoomInHighlightedItems()
{
  auto selIndices = ui->hltTableView->selectionModel()->selectedRows();
  QList<const Selected*> dehltItems;
  for (auto& selItem : selIndices) {
    dehltItems << highlightModel_.getItemAt(selItem.row());
  }
  emit zoomInToItems(dehltItems);
}

}  // namespace gui