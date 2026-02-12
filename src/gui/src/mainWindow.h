// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QShortcut>
#include <QString>
#include <QToolBar>
#include <QWidget>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "findDialog.h"
#include "gotoDialog.h"
#include "gui/gui.h"
#include "label.h"
#include "odb/dbDatabaseObserver.h"
#include "odb/dbObject.h"
#include "ord/OpenRoad.hh"
#include "ruler.h"
#include "utl/Progress.h"

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
class Progress;
}  // namespace utl

namespace sta {
class Pin;
}

namespace gui {

class LayoutViewer;
class LayoutTabs;
class SelectHighlightWindow;
class LayoutScroll;
class ScriptWidget;
class DisplayControls;
class Inspector;
class TimingWidget;
class DRCWidget;
class ClockWidget;
class BrowserWidget;
class ChartsWidget;
class Chiplet3DWidget;
class HelpWidget;

// This is the main window for the GUI.  Currently we use a single
// instance of this class.
class MainWindow : public QMainWindow, public odb::dbDatabaseObserver
{
  Q_OBJECT

 public:
  MainWindow(bool load_settings = true, QWidget* parent = nullptr);
  ~MainWindow() override;

  void setDatabase(odb::dbDatabase* db);
  void init(sta::dbSta* sta, const std::string& help_path);

  odb::dbDatabase* getDb() const { return db_; }

  // From ord::OpenRoad::Observer
  void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  void postReadDef(odb::dbBlock* block) override;
  void postReadDb(odb::dbDatabase* db) override;
  void postRead3Dbx(odb::dbChip* chip) override;

  // Capture logger messages into the script widget output
  void setLogger(utl::Logger* logger);

  // Fit design in window
  void fit();

  DisplayControls* getControls() const { return controls_; }
  LayoutViewer* getLayoutViewer() const;
  LayoutTabs* getLayoutTabs() const { return viewers_; }
  DRCWidget* getDRCViewer() const { return drc_viewer_; }
  ClockWidget* getClockViewer() const { return clock_viewer_; }
  ScriptWidget* getScriptWidget() const { return script_; }
  Inspector* getInspector() const { return inspector_; }
  HelpWidget* getHelpViewer() const { return help_widget_; }
  ChartsWidget* getChartsWidget() const { return charts_widget_; }
  Chiplet3DWidget* getChiplet3DViewer() const { return chiplet_viewer_; }
  TimingWidget* getTimingWidget() const { return timing_widget_; }

  std::vector<std::string> getRestoreTclCommands();

  void setTitle(const std::string& title);

  // Return the selected set
  const SelectionSet& selection();

 signals:
  // Signaled when we get a postRead callback to tell the sub-widgets
  // to update
  void blockLoaded(odb::dbBlock* block);

  // Signaled when we get a postRead3Dbx callback to tell the sub-widgets
  // to update
  void chipLoaded(odb::dbChip* chip);

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

  // Label Requested on the Layout
  void labelsChanged();

  void displayUnitsChanged(int dbu_per_micron, bool use_dbu);

  // Find selection in the CTS Viewer
  void findInCts(const Selected& selection);

  // Find selections in the CTS Viewer
  void findInCts(const SelectionSet& selection);

 public slots:
  // Save the current state into settings for the next session.
  void saveSettings();

  // Set the location to display in the status bar
  void setLocation(int x, int y);

  // Update selected name in status bar
  void updateSelectedStatus(const Selected& selection);

  // Add to the selection
  void addSelected(const Selected& selection, bool find_in_cts = false);

  // Add the selections to the current selections
  void addSelected(const SelectionSet& selections, bool find_in_cts = false);

  // Sets and replaces the current selections
  void setSelected(const SelectionSet& selections);

  // Remove a selection from the set of selections
  void removeSelected(const Selected& selection);

  // Remove a selection type from the set of selections
  void removeSelectedByType(const std::string& type);

  // Displays the selection in the status bar
  void setSelected(const Selected& selection, bool show_connectivity = false);

  // Add the selections to highlight set
  void addHighlighted(const SelectionSet& highlights, int highlight_group = -1);

  // Remove a selection from the set of highlights
  void removeHighlighted(const Selected& selection);

  // Add Label to Layout View
  std::string addLabel(int x,
                       int y,
                       const std::string& text,
                       std::optional<Painter::Color> color = {},
                       std::optional<int> size = {},
                       std::optional<Painter::Anchor> anchor = {},
                       std::optional<std::string> name = {});

  // Delete Label from Layout View
  void deleteLabel(const std::string& name);

  // Clear Labels
  void clearLabels();

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

  // zoom to the specified point
  void zoomTo(const odb::Point& focus, int diameter);

  // Zoom In To Items such that its bbox is in visible Area
  void zoomInToItems(const QList<const Selected*>& items);

  // Show a message in the status bar
  void status(const std::string& message);

  // Show Find Dialog Box
  void showFindDialog();

  // Show Goto Dialog Box
  void showGotoDialog();

  // Show help in browser
  void showHelp();

  // add/remove toolbar button
  std::string addToolbarButton(const std::string& name,
                               const QString& text,
                               const QString& script,
                               bool echo);
  void removeToolbarButton(const std::string& name);

  // add/remove menu actions
  std::string addMenuItem(const std::string& name,
                          const QString& path,
                          const QString& text,
                          const QString& script,
                          const QString& shortcut,
                          bool echo);
  void removeMenuItem(const std::string& name);

  // request for user input
  std::string requestUserInput(const QString& title, const QString& question);

  bool anyObjectInSet(bool selection_set, odb::dbObjectType obj_type);
  void selectHighlightConnectedInsts(bool select_flag, int highlight_group = 0);
  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group = 0);
  void selectHighlightConnectedBufferTrees(bool select_flag,
                                           int highlight_group = 0);

  void timingCone(Gui::Term term, bool fanin, bool fanout);
  void timingPathsThrough(const std::set<Gui::Term>& terms);

  void registerHeatMap(HeatMapDataSource* heatmap);
  void unregisterHeatMap(HeatMapDataSource* heatmap);

 private slots:
  void setUseDBU(bool use_dbu);
  void setClearLocation();
  void showApplicationFont();
  void showArrowKeysScrollStep();
  void showGlobalConnect();
  void openDesign();
  void saveDesign();
  void reportSlackHistogramPaths(const std::set<const sta::Pin*>& report_pins,
                                 const std::string& path_group_name);
  void enableDeveloper();

 protected:
  // used to check if user intends to close Openroad or just the GUI.
  void closeEvent(QCloseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void showEvent(QShowEvent* event) override;

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

  void updateTitle();

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  SelectionSet selected_;
  HighlightSet highlighted_;
  Rulers rulers_;
  Labels labels_;

  int arrow_keys_scroll_step_;

  bool first_show_{true};
  std::optional<QByteArray> saved_geometry_;
  std::optional<QByteArray> saved_state_;

  // All but viewer_ are owned by this widget.  Qt will
  // handle destroying the children.
  DisplayControls* controls_;
  Inspector* inspector_;
  ScriptWidget* script_;
  LayoutTabs* viewers_;
  SelectHighlightWindow* selection_browser_;
  TimingWidget* timing_widget_;
  DRCWidget* drc_viewer_;
  ClockWidget* clock_viewer_;
  BrowserWidget* hierarchy_widget_;
  ChartsWidget* charts_widget_;
  Chiplet3DWidget* chiplet_viewer_;
  QDockWidget* chiplet_dock_;
  HelpWidget* help_widget_;

  FindObjectDialog* find_dialog_;
  GotoLocationDialog* goto_dialog_;

  std::string window_title_;

  QMenu* file_menu_;
  QMenu* view_menu_;
  QMenu* tools_menu_;
  QMenu* windows_menu_;

  QToolBar* view_tool_bar_;

  QAction* open_;
  QAction* save_;
  QAction* exit_;
  QAction* hide_option_;
  QAction* hide_;
  QAction* fit_;
  QAction* find_;
  QAction* inspect_;
  QAction* timing_debug_;
  QAction* zoom_in_;
  QAction* zoom_out_;
  QAction* goto_position_;
  QAction* help_;
  QAction* build_ruler_;
  QAction* show_dbu_;
  QAction* show_poly_decomp_view_;
  QAction* default_ruler_style_;
  QAction* default_mouse_wheel_zoom_;
  QAction* arrow_keys_scroll_step_dialog_;
  QAction* font_;
  QAction* global_connect_;

  QShortcut* enable_developer_mode_;

  QLabel* location_;

  // created button actions
  std::map<const std::string, std::unique_ptr<QAction>> buttons_;

  // created menu actions
  std::map<const std::string, std::unique_ptr<QAction>> menu_actions_;

  // heat map actions
  std::map<HeatMapDataSource*, QAction*> heatmap_actions_;

  std::unique_ptr<utl::Progress> cli_progress_ = nullptr;

  std::unique_ptr<QTimer> selection_timer_;
  std::unique_ptr<QTimer> highlight_timer_;
};

}  // namespace gui
