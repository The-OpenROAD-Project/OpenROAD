// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QCheckBox>
#include <QColor>
#include <QDockWidget>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTreeView>
#include <QWidget>
#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "gui/gui.h"
#include "layoutViewer.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace sta {
class dbSta;
}

namespace gui {
class DbInstDescriptor;
class DisplayControls;

class BrowserWidget : public QDockWidget,
                      public odb::dbBlockCallBackObj,
                      public sta::dbNetworkObserver
{
  Q_OBJECT

 public:
  BrowserWidget(const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>&
                    modulesettings,
                DisplayControls* controls,
                QWidget* parent = nullptr);

  void setSTA(sta::dbSta* sta);
  void setDBInstDescriptor(DbInstDescriptor* desciptor);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  bool eventFilter(QObject* obj, QEvent* event) override;

  // dbBlockCallBackObj
  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstDestroy(odb::dbInst*) override;
  void inDbInstSwapMasterAfter(odb::dbInst*) override;

  // API from dbNetworkObserver
  void postReadLiberty() override;
  void postReadDb() override;

  // from QT
  void paintEvent(QPaintEvent* event) override;

 signals:
  void select(const SelectionSet& selected);
  void removeSelect(const Selected& selected);
  void highlight(const SelectionSet& selected);
  void removeHighlight(const Selected& selected);

  void updateModuleVisibility(odb::dbModule* module, bool visible);
  void updateModuleColor(odb::dbModule* module,
                         const QColor& color,
                         bool user_selected);

 public slots:
  void setBlock(odb::dbBlock* block);
  void clicked(const QModelIndex& index);
  void selectionChanged(const QItemSelection& selected,
                        const QItemSelection& deselected);
  void displayControlsUpdated();

 protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

 private slots:
  void itemContextMenu(const QPoint& point);
  void itemChanged(QStandardItem* item);

  void itemCollapsed(const QModelIndex& index);
  void itemExpanded(const QModelIndex& index);
  void updateModuleColorIcon(odb::dbModule* module, const QColor& color);
  void enableModuleView();
  void markModelModified();

 private:
  void updateModel();
  void clearModel();

  void makeMenu();

  Selected getSelectedFromIndex(const QModelIndex& index);

  void toggleParent(QStandardItem* item);

  odb::dbBlock* block_;
  sta::dbSta* sta_;
  DbInstDescriptor* inst_descriptor_;
  DisplayControls* display_controls_;
  QPushButton* display_controls_warning_;
  QCheckBox* include_physical_cells_;

  const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>& modulesettings_;

  QTreeView* view_;
  QStandardItemModel* model_;
  bool model_modified_;
  bool initial_load_;

  bool ignore_selection_;

  QMenu* menu_;
  Selected menu_item_;
  static const int kSortRole;

  std::set<odb::dbModule*> getChildren(odb::dbModule* parent);
  std::set<odb::dbModule*> getAllChildren(odb::dbModule* parent);
  SelectionSet getMenuItemChildren();

  void updateChildren(odb::dbModule* module, const QColor& color);
  void resetChildren(odb::dbModule* module);

  std::map<odb::dbModule*, QStandardItem*> modulesmap_;

  struct ModuleStats;

  ModuleStats populateModule(odb::dbModule* module, QStandardItem* parent);

  ModuleStats addInstanceItem(odb::dbInst* inst,
                              QStandardItem* parent,
                              bool create_row);
  ModuleStats addInstanceItems(const std::vector<odb::dbInst*>& insts,
                               const std::string& title,
                               QStandardItem* parent);
  ModuleStats addModuleItem(odb::dbModule* module,
                            QStandardItem* parent,
                            bool expand);

  QIcon makeModuleIcon(const QColor& color);

  void makeRowItems(QStandardItem* item,
                    const std::string& master,
                    const ModuleStats& stats,
                    QStandardItem* parent,
                    bool is_leaf) const;

  enum Columns
  {
    kInstance,
    kMaster,
    kInstances,
    kMacros,
    kModules,
    kArea
  };

  // Limit number of visible physical instances
  static constexpr int kMaxVisibleLeafs = 1000;
};

}  // namespace gui
