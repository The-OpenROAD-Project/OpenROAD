///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <QAbstractTableModel>
#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QShortcut>
#include <QStringList>
#include <QToolBar>
#include <QVariant>
#include <unordered_map>

#include "gui/gui.h"
#include "ui_selectedWidget.h"

namespace gui {
class SelHltModel : public QAbstractTableModel
{
  Q_OBJECT
 public:
  SelHltModel(const SelectionSet& objs);
  ~SelHltModel();

  int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex& parent
                  = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  const Selected* getItemAt(int idx) const { return tableData_[idx]; }

  void populateModel();

 private:
  const SelectionSet& objs_;
  std::vector<const Selected*> tableData_;
};

class SelectHighlightWindow : public QDockWidget
{
  Q_OBJECT

 public:
  explicit SelectHighlightWindow(const SelectionSet& selSet,
                                 const SelectionSet& hltSet,
                                 QWidget* parent = nullptr);
  ~SelectHighlightWindow();

 signals:
  void clearAllSelections();
  void clearAllHighlights();

  void clearSelectedItems(const QList<const Selected*>& items);
  void clearHighlightedItems(const QList<const Selected*>& items);
  void zoomSelectedItems(const QList<const Selected*>& items);
  void highlightSelectedItems(const QList<const Selected*>& items);
  void zoomHighlightedItems(const QList<const Selected*>& items);

 public slots:
  void updateSelectionModel();
  void updateHighlightModel();
  void showSelectCustomMenu(QPoint pos);
  void showHighlightCustomMenu(QPoint pos);

  void deselectItems();
  void highlightSelectedItems();
  void zoomInSelectedItems();
  void dehighlightItems();
  void zoomInHighlightedItems();

 private:
  Ui::SelectHighlightWidget* ui;
  SelHltModel selModel_;
  SelHltModel hltModel_;

  QMenu* selectContextMenu_;
  QMenu* highlightContextMenu_;
};

}  // namespace gui