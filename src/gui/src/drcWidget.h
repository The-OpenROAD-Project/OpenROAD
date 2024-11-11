/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2020, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTreeView>
#include <memory>
#include <variant>

#include "gui/gui.h"
#include "inspector.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace utl {
class Logger;
}

namespace gui {

class DRCItemModel : public QStandardItemModel
{
 public:
  DRCItemModel(QWidget* parent = nullptr) : QStandardItemModel(parent) {}
  QVariant data(const QModelIndex& index, int role) const override;
};

class DRCRenderer : public Renderer
{
 public:
  DRCRenderer();

  void setCategory(odb::dbMarkerCategory* category);

  // Renderer
  void drawObjects(Painter& painter) override;
  SelectionSet select(odb::dbTechLayer* layer,
                      const odb::Rect& region) override;

 private:
  odb::dbMarkerCategory* category_;
};

class DRCWidget : public QDockWidget, public odb::dbBlockCallBackObj
{
  Q_OBJECT

 public:
  DRCWidget(QWidget* parent = nullptr);
  ~DRCWidget() {}

  void setLogger(utl::Logger* logger);

  // from dbBlockCallBackObj
  void inDbMarkerCategoryCreate(odb::dbMarkerCategory* category) override;
  void inDbMarkerCategoryDestroy(odb::dbMarkerCategory* category) override;
  void inDbMarkerCreate(odb::dbMarker* marker) override;
  void inDbMarkerDestroy(odb::dbMarker* marker) override;

 signals:
  void selectDRC(const Selected& selected);
  void focus(const Selected& selected);

 public slots:
  void loadReport(const QString& filename);
  void setBlock(odb::dbBlock* block);
  void clicked(const QModelIndex& index);
  void selectReport();
  void toggleRenderer(bool visible);
  void updateSelection(const Selected& selection);
  void selectCategory(odb::dbMarkerCategory* category);

  void selectionChanged(const QItemSelection& selected,
                        const QItemSelection& deselected);

 private slots:
  void focusIndex(const QModelIndex& index);
  void defocus();
  void updateMarkerGroups();
  void updateModel();

 protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private:
  odb::dbMarkerCategory* loadTRReport(const QString& filename);
  odb::dbMarkerCategory* loadJSONReport(const QString& filename);
  void updateMarkerGroupsWithIgnore(odb::dbMarkerCategory* ignore);

  utl::Logger* logger_;

  ObjectTree* view_;
  DRCItemModel* model_;

  odb::dbBlock* block_;

  QComboBox* categories_;
  QPushButton* load_;

  std::unique_ptr<DRCRenderer> renderer_;

  void toggleParent(QStandardItem* child);
  bool setVisibleDRC(QStandardItem* item, bool visible, bool announce_parent);
  void populateCategory(odb::dbMarkerCategory* category, QStandardItem* model);
};

}  // namespace gui
