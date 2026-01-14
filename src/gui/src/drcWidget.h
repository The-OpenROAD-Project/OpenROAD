// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QString>
#include <QTreeView>
#include <QVariant>
#include <QWidget>
#include <memory>
#include <variant>

#include "gui/gui.h"
#include "inspector.h"
#include "odb/db.h"
#include "odb/dbChipCallBackObj.h"
#include "odb/geom.h"

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

class DRCWidget : public QDockWidget, public odb::dbChipCallBackObj
{
  Q_OBJECT

 public:
  DRCWidget(QWidget* parent = nullptr);

  void setLogger(utl::Logger* logger);

  // from dbBlockCallBackObj
  void inDbMarkerCategoryCreate(odb::dbMarkerCategory* category) override;
  void inDbMarkerCategoryDestroy(odb::dbMarkerCategory* category) override;
  void inDbMarkerCreate(odb::dbMarker* marker) override;
  void inDbMarkerDestroy(odb::dbMarker* marker) override;

 signals:
  void selectDRC(const Selected& selected, bool open_inspector);
  void focus(const Selected& selected);

 public slots:
  void loadReport(const QString& filename);
  void setChip(odb::dbChip* chip);
  void clicked(const QModelIndex& index);
  void doubleClicked(const QModelIndex& index);
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

  odb::dbChip* chip_;

  QComboBox* categories_;
  QPushButton* load_;

  std::unique_ptr<DRCRenderer> renderer_;

  void toggleParent(QStandardItem* child);
  bool setVisibleDRC(QStandardItem* item, bool visible, bool announce_parent);
  void populateCategory(odb::dbMarkerCategory* category, QStandardItem* model);

  void showMarker(const QModelIndex& index, bool open_inspector);
};

}  // namespace gui
