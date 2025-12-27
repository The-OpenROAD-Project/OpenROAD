// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractTableModel>
#include <QAction>
#include <QDockWidget>
#include <QItemDelegate>
#include <QMainWindow>
#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QShortcut>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QToolBar>
#include <QVariant>
#include <QWidget>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "ui_selectedWidget.h"

namespace gui {
class SelectionModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  SelectionModel(const SelectionSet& objs);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  const Selected* getItemAt(int idx) const { return table_data_[idx]; }

  void populateModel();

 private:
  const SelectionSet& objs_;
  std::vector<const Selected*> table_data_;
};

class HighlightModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  HighlightModel(const HighlightSet& objs);

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  const Selected* getItemAt(int idx) const { return table_data_[idx].second; }
  void populateModel();

  int highlightGroup(const QModelIndex& index) const;
  bool setData(const QModelIndex& index,
               const QVariant& value,
               int role) override;

 private:
  const HighlightSet& objs_;
  std::vector<std::pair<int, const Selected*>> table_data_;
};

class SelectHighlightWindow : public QDockWidget
{
  Q_OBJECT

 public:
  explicit SelectHighlightWindow(const SelectionSet& sel_set,
                                 const HighlightSet& hlt_set,
                                 QWidget* parent = nullptr);

 signals:
  void clearAllSelections();
  void clearAllHighlights();

  void selected(const Selected& selection);
  void clearSelectedItems(const QList<const Selected*>& items);
  void clearHighlightedItems(const QList<const Selected*>& items);
  void zoomInToItems(const QList<const Selected*>& items);
  void highlightSelectedItemsSig(const QList<const Selected*>& items);

 public slots:
  void updateSelectionModel();
  void updateHighlightModel();
  void updateModels();
  void showSelectCustomMenu(QPoint pos);
  void showHighlightCustomMenu(QPoint pos);

  void changeHighlight();

  void deselectItems();
  void highlightSelectedItems();
  void zoomInSelectedItems();
  void dehighlightItems();
  void zoomInHighlightedItems();

 private:
  Ui::SelectHighlightWidget ui_;
  SelectionModel selection_model_;
  QSortFilterProxyModel* sel_filter_proxy_;

  HighlightModel highlight_model_;
  QSortFilterProxyModel* hlt_filter_proxy_;

  QMenu* select_context_menu_;
  QMenu* highlight_context_menu_;
};

}  // namespace gui
