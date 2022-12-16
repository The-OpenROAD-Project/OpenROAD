///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QToolBar>
#include <memory>

#include "findDialog.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "ruler.h"

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
class Inspector;
class TimingWidget;
class DRCWidget;
class ClockWidget;
class BrowserWidget;

// This is the main window for the GUI.  Currently we use a single
// instance of this class.
class MainWindow : public QMainWindow, public ord::OpenRoad::Observer
{
  Q_OBJECT

 public:
  MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

  void setDatabase(odb::dbDatabase* db);
  void init(sta::dbSta* sta);

  odb::dbDatabase* getDb() const { return db_; }

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

  // Capture logger messages into the script widget output
  void setLogger(utl::Logger* logger);

  // Fit design in window
  void fit();

  DisplayControls* getControls() const { return controls_; }
  LayoutViewer* getLayoutViewer() const { return viewer_; }
  DRCWidget* getDRCViewer() const { return drc_viewer_; }
  ClockWidget* getClockViewer() const { return clock_viewer_; }
  ScriptWidget* getScriptWidget() const { return script_; }
  Inspector* getInspector() const { return inspector_; }

  const std::vector<std::string> getRestoreTclCommands();

 signals:
  // Signaled when we get a postRead callback to tell the sub-widgets
  // to update
  void designLoaded(odb::dbBlock* block);

  // The user chose the exit action; notify the app
  void exit();

  // The user chose to hide the gui
  void hide();

  // Trigger a redraw (used by Renderers)
  void redraw();

  // Waits for the user to click continue before returning
  // Draw events are processed while paused.
  void pause(int timeout);

  // The selected set of objects has changed
  void selectionChanged(const Selected& selection = Selected());

  // The highlight set of objects has changed
  void highlightChanged();

  // Ruler Requested on the Layout
  void rulersChanged();

 public slots:
  // Save the current state into settings for the next session.
  void saveSettings();

  // Set the location to display in the status bar
  void setLocation(int x, int y);

  // Update selected name in status bar
  void updateSelectedStatus(const Selected& selection);

  // Add to the selection
  void addSelected(const Selected& selection);

  // Add the selections to the current selections
  void addSelected(const SelectionSet& selections);

  // Sets and replaces the current selections
  void setSelected(const SelectionSet& selections);

  // Remove a selection from the set of selections
  void removeSelected(const Selected& selection);

  // Remove a selection type from the set of selections
  void removeSelectedByType(const std::string& type);

  // Displays the selection in the status bar
  void setSelected(const Selected& selection, bool show_connectivity = false);

  // Add the selections to highlight set
  void addHighlighted(const SelectionSet& selection, int highlight_group = -1);

  // Remove a selection from the set of highlights
  void removeHighlighted(const Selected& selection);

  // Add Ruler to Layout View
  std::string addRuler(int x0,
                       int y0,
                       int x1,
                       int y1,
                       const std::string& label = "",
                       const std::string& name = "",
                       bool euclidian = true);

  // Delete Ruler to Layout View
  void deleteRuler(const std::string& name);

  // Add the selections(List) to highlight set
  void updateHighlightedSet(const QList<const Selected*>& items_to_highlight,
                            int highlight_group = -1);

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

  // Show help in browser
  void showHelp();

  // add/remove toolbar button
  const std::string addToolbarButton(const std::string& name,
                                     const QString& text,
                                     const QString& script,
                                     bool echo);
  void removeToolbarButton(const std::string& name);

  // add/remove menu actions
  const std::string addMenuItem(const std::string& name,
                                const QString& path,
                                const QString& text,
                                const QString& script,
                                const QString& shortcut,
                                bool echo);
  void removeMenuItem(const std::string& name);

  // request for user input
  const std::string requestUserInput(const QString& title,
                                     const QString& question);

  bool anyObjectInSet(bool selection_set, odb::dbObjectType obj_type);
  void selectHighlightConnectedInsts(bool select_flag, int highlight_group = 0);
  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group = 0);

  void timingCone(Gui::odbTerm term, bool fanin, bool fanout);
  void timingPathsThrough(const std::set<Gui::odbTerm>& terms);

  void registerHeatMap(HeatMapDataSource* heatmap);
  void unregisterHeatMap(HeatMapDataSource* heatmap);

 private slots:
  void setUseDBU(bool use_dbu);
  void setClearLocation();
  void showApplicationFont();
  void showGlobalConnect();
  void openDesign();

 protected:
  // used to check if user intends to close Openroad or just the GUI.
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private slots:
  void setBlock(odb::dbBlock* block);

 private:
  void createMenus();
  void createActions();
  void createToolbars();
  void createStatusBar();

  QMenu* findMenu(QStringList& path, QMenu* parent = nullptr);
  void removeMenu(QMenu* menu);

  int requestHighlightGroup();

  odb::dbBlock* getBlock() const;

  std::string convertDBUToString(int value, bool add_units) const;
  int convertStringToDBU(const std::string& value, bool* ok) const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  SelectionSet selected_;
  HighlightSet highlighted_;
  std::vector<std::unique_ptr<Ruler>> rulers_;

  // All but viewer_ are owned by this widget.  Qt will
  // handle destroying the children.
  DisplayControls* controls_;
  Inspector* inspector_;
  ScriptWidget* script_;
  LayoutViewer* viewer_;  // owned by scroll_
  SelectHighlightWindow* selection_browser_;
  LayoutScroll* scroll_;
  TimingWidget* timing_widget_;
  DRCWidget* drc_viewer_;
  ClockWidget* clock_viewer_;
  BrowserWidget* hierarchy_widget_;

  FindObjectDialog* find_dialog_;

  QMenu* file_menu_;
  QMenu* view_menu_;
  QMenu* tools_menu_;
  QMenu* windows_menu_;

  QToolBar* view_tool_bar_;

  QAction* open_;
  QAction* exit_;
  QAction* hide_option_;
  QAction* hide_;
  QAction* fit_;
  QAction* find_;
  QAction* inspect_;
  QAction* timing_debug_;
  QAction* zoom_in_;
  QAction* zoom_out_;
  QAction* help_;
  QAction* build_ruler_;
  QAction* show_dbu_;
  QAction* default_ruler_style_;
  QAction* font_;
  QAction* global_connect_;

  QLabel* location_;

  // created button actions
  std::map<const std::string, std::unique_ptr<QAction>> buttons_;

  // created menu actions
  std::map<const std::string, std::unique_ptr<QAction>> menu_actions_;

  // heat map actions
  std::map<HeatMapDataSource*, QAction*> heatmap_actions_;
};

}  // namespace gui
