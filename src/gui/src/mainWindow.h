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

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QToolBar>
#include <memory>

#include "findDialog.h"
#include "gui/gui.h"
#include "openroad/OpenRoad.hh"

#include "timingDebugDialog.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace gui {

class LayoutViewer;
class SelectHighlightWindow;
class LayoutScroll;
class ScriptWidget;
class DisplayControls;

// This is the main window for the GUI.  Currently we use a single
// instance of this class.
class MainWindow : public QMainWindow, public ord::OpenRoad::Observer
{
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);

  odb::dbDatabase* getDb() const { return db_; }
  void setDb(odb::dbDatabase* db);

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

  // Capture logger messages into the script widget output
  void setLogger(utl::Logger* logger);

 signals:
  // Signaled when we get a postRead callback to tell the sub-widgets
  // to update
  void designLoaded(odb::dbBlock* block);

  // The user chose the exit action; notify the app
  void exit();

  // Trigger a redraw (used by Renderers)
  void redraw();

  // Waits for the user to click continue before returning
  // Draw events are processed while paused.
  void pause();

  // The selected set of objects has changed
  void selectionChanged();

  // The highlight set of objects has changed
  void highlightChanged();

  // Ruler Requested on the Layout
  void rulersChanged();

 public slots:
  // Save the current state into settings for the next session.
  void saveSettings();

  // Set the location to display in the status bar
  void setLocation(qreal x, qreal y);

  // Add to the selection
  void addSelected(const Selected& selection);

  // Add the selections to the current selections
  void addSelected(const SelectionSet& selections);

  // Displays the selection in the status bar
  void setSelected(const Selected& selection, bool show_connectivity = false);

  // Add the selections to highlight set
  void addHighlighted(const SelectionSet& selection, int highlight_group = 0);

  // Add Ruler to Layout View
  void addRuler(int x0, int y0, int x1, int y1);

  // Add the selections(List) to highlight set
  void updateHighlightedSet(const QList<const Selected*>& items_to_highlight,
                            int highlight_group = 0);

  // Higlight set will be cleared with this explicit call
  void clearHighlighted(int highlight_group = -1 /* -1 : clear all Groups */);

  // Clear Rulers
  void clearRulers();

  // Remove items from the Selected Set
  void removeFromSelected(const QList<const Selected*>& items);

  // Remove items from the Highlighted Set
  void removeFromHighlighted(const QList<const Selected*>& items,
                             int highlight_group
                             = -1 /* Search and remove...*/);

  // Zoom to the given rectangle
  void zoomTo(const odb::Rect& rect_dbu);

  // Zoom In To Items such that its bbox is in visible Area
  void zoomInToItems(const QList<const Selected*>& items);

  // Show a message in the status bar
  void status(const std::string& message);

  // Show Find Dialog Box
  void showFindDialog();

  // show Timing Dialog Box
  void showTimingDialog();

  DisplayControls* getControls() const{
      return controls_;
  }


  bool anyObjectInSet(bool selection_set, odb::dbObjectType obj_type);
  void selectHighlightConnectedInsts(bool select_flag, int highlight_group = 0);
  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group = 0);
 private:
  void createMenus();
  void createActions();
  void createToolbars();
  void createStatusBar();

  odb::dbBlock* getBlock();

  odb::dbDatabase* db_;
  SelectionSet selected_;
  HighlightSet highlighted_;
  std::vector<QLine> rulers_;

  // All but viewer_ are owned by this widget.  Qt will
  // handle destroying the children.
  DisplayControls* controls_;
  LayoutViewer* viewer_;  // owned by scroll_
  SelectHighlightWindow* selection_browser_;
  LayoutScroll* scroll_;
  ScriptWidget* script_;

  QMenu* file_menu_;
  QMenu* view_menu_;
  QMenu* windows_menu_;

  QToolBar* view_tool_bar_;

  QAction* exit_;
  QAction* fit_;
  QAction* find_;
  QAction* timing_debug_;
  QAction* zoom_in_;
  QAction* zoom_out_;

  QAction* congestion_setup_;

  QLabel* location_;

  FindObjectDialog* find_dialog_;
  TimingDebugDialog* timing_dialog_;
};

}  // namespace gui
