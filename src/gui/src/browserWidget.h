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

#include "gui/gui.h"
#include "layoutViewer.h"

#include <QDockWidget>
#include <QMenu>
#include <QTreeView>
#include <QSettings>
#include <QStandardItemModel>

#include <array>
#include <memory>

#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"

namespace gui {

class BrowserWidget : public QDockWidget, public odb::dbBlockCallBackObj
{
  Q_OBJECT

  public:
    BrowserWidget(const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>& modulesettings,
                  QWidget* parent = nullptr);

    void readSettings(QSettings* settings);
    void writeSettings(QSettings* settings);

    bool eventFilter(QObject* obj, QEvent* event) override;

    // dbBlockCallBackObj
    virtual void inDbInstCreate(odb::dbInst*);
    virtual void inDbInstCreate(odb::dbInst*, odb::dbRegion*);
    virtual void inDbInstDestroy(odb::dbInst*);
    virtual void inDbInstSwapMasterAfter(odb::dbInst*);

  signals:
    void select(const SelectionSet& selected);
    void removeSelect(const Selected& selected);
    void highlight(const SelectionSet& selected);
    void removeHighlight(const Selected& selected);

    void updateModuleVisibility(odb::dbModule* module, bool visible);
    void updateModuleColor(odb::dbModule* module, const QColor& color, bool user_selected);

  public slots:
    void setBlock(odb::dbBlock* block);
    void clicked(const QModelIndex& index);
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

  protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

  private slots:
    void itemContextMenu(const QPoint &point);
    void itemChanged(QStandardItem* item);

    void itemCollapsed(const QModelIndex& index);
    void itemExpanded(const QModelIndex& index);
    void updateModuleColorIcon(odb::dbModule* module, const QColor& color);

  private:
    void updateModel();
    void clearModel();

    void makeMenu();

    Selected getSelectedFromIndex(const QModelIndex& index);

    void toggleParent(QStandardItem* item);

    odb::dbBlock* block_;

    const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>& modulesettings_;

    QTreeView* view_;
    QStandardItemModel* model_;

    bool ignore_selection_;

    QMenu* menu_;
    Selected menu_item_;
    std::set<odb::dbModule*> getChildren(odb::dbModule* parent);
    std::set<odb::dbModule*> getAllChildren(odb::dbModule* parent);
    SelectionSet getMenuItemChildren();

    void updateChildren(odb::dbModule* module, const QColor& color);
    void resetChildren(odb::dbModule* module);

    std::map<odb::dbModule*, QStandardItem*> modulesmap_;

    struct ModuleStats {
      int64_t area;
      int macros;
      int insts;
      int modules;

      int hier_macros;
      int hier_insts;
      int hier_modules;

      ModuleStats()
      {
        area = 0;
        macros = 0;
        insts = 0;
        modules = 0;

        resetInstances();
        resetMacros();
        resetModules();
      }

      void incrementInstances()
      {
        insts++;
        hier_insts++;
      }

      void resetInstances()
      {
        hier_insts = 0;
      }

      void incrementMacros()
      {
        macros++;
        hier_macros++;
      }

      void resetMacros()
      {
        hier_macros = 0;
      }

      void incrementModules()
      {
        modules++;
        hier_modules++;
      }

      void resetModules()
      {
        hier_modules = 0;
      }

      ModuleStats& operator+=(const ModuleStats& other)
      {
        area += other.area;
        macros += other.macros;
        insts += other.insts;
        modules += other.modules;

        hier_macros += other.hier_macros;
        hier_insts += other.hier_insts;
        hier_modules += other.hier_modules;

        return *this;
      }

      friend ModuleStats operator+(ModuleStats lhs, const ModuleStats& other)
      {
        lhs += other;

        return lhs;
      }
    };

    ModuleStats populateModule(odb::dbModule* module, QStandardItem* parent);

    ModuleStats addInstanceItem(odb::dbInst* inst, QStandardItem* parent);
    ModuleStats addModuleItem(odb::dbModule* module, QStandardItem* parent, bool expand);

    const QIcon makeModuleIcon(const QColor& color);

    void makeRowItems(QStandardItem* item,
                      const std::string& master,
                      const ModuleStats& stats,
                      QStandardItem* parent,
                      bool is_leaf) const;

    enum Columns {
      Instance,
      Master,
      Instances,
      Macros,
      Modules,
      Area
    };
};

}  // namespace gui
