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

#include "mainWindow.h"

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>
#include <QWidgetAction>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "browserWidget.h"
#include "clockWidget.h"
#include "dbDescriptors.h"
#include "displayControls.h"
#include "drcWidget.h"
#include "globalConnectDialog.h"
#include "gui/heatMap.h"
#include "highlightGroupDialog.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "scriptWidget.h"
#include "selectHighlightWindow.h"
#include "staGui.h"
#include "timingWidget.h"
#include "utl/Logger.h"

// must be loaded in global namespace
static void loadQTResources()
{
  Q_INIT_RESOURCE(resource);
}

namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      db_(nullptr),
      logger_(nullptr),
      controls_(new DisplayControls(this)),
      inspector_(new Inspector(selected_, highlighted_, this)),
      script_(new ScriptWidget(this)),
      viewer_(new LayoutViewer(
          controls_,
          script_,
          selected_,
          highlighted_,
          rulers_,
          Gui::get(),
          [this]() -> bool { return show_dbu_->isChecked(); },
          [this]() -> bool { return default_ruler_style_->isChecked(); },
          this)),
      selection_browser_(
          new SelectHighlightWindow(selected_, highlighted_, this)),
      scroll_(new LayoutScroll(viewer_, this)),
      timing_widget_(new TimingWidget(this)),
      drc_viewer_(new DRCWidget(this)),
      clock_viewer_(new ClockWidget(this)),
      hierarchy_widget_(
          new BrowserWidget(viewer_->getModuleSettings(), controls_, this)),
      find_dialog_(new FindObjectDialog(this))
{
  // Size and position the window
  QSize size = QDesktopWidget().availableGeometry(this).size();
  resize(size * 0.8);
  move(size.width() * 0.1, size.height() * 0.1);

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  script_->setWidgetFont(font);

  setCentralWidget(scroll_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::BottomDockWidgetArea, selection_browser_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);
  addDockWidget(Qt::RightDockWidgetArea, inspector_);
  addDockWidget(Qt::RightDockWidgetArea, hierarchy_widget_);
  addDockWidget(Qt::RightDockWidgetArea, timing_widget_);
  addDockWidget(Qt::RightDockWidgetArea, drc_viewer_);
  addDockWidget(Qt::RightDockWidgetArea, clock_viewer_);

  tabifyDockWidget(selection_browser_, script_);
  selection_browser_->hide();

  tabifyDockWidget(inspector_, hierarchy_widget_);
  tabifyDockWidget(inspector_, timing_widget_);
  tabifyDockWidget(inspector_, drc_viewer_);
  tabifyDockWidget(inspector_, clock_viewer_);
  drc_viewer_->hide();
  clock_viewer_->hide();

  // Hook up all the signals/slots
  connect(script_, SIGNAL(exiting()), this, SIGNAL(exit()));
  connect(script_, SIGNAL(commandExecuted(bool)), viewer_, SLOT(update()));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          viewer_,
          SLOT(designLoaded(odb::dbBlock*)));
  connect(this, SIGNAL(redraw()), viewer_, SLOT(fullRepaint()));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          controls_,
          SLOT(designLoaded(odb::dbBlock*)));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          timing_widget_,
          SLOT(setBlock(odb::dbBlock*)));

  connect(this, SIGNAL(pause(int)), script_, SLOT(pause(int)));
  connect(controls_, SIGNAL(changed()), viewer_, SLOT(fullRepaint()));
  connect(controls_,
          SIGNAL(changed()),
          hierarchy_widget_,
          SLOT(displayControlsUpdated()));
  connect(
      viewer_, SIGNAL(location(int, int)), this, SLOT(setLocation(int, int)));
  connect(viewer_,
          SIGNAL(selected(const Selected&, bool)),
          this,
          SLOT(setSelected(const Selected&, bool)));
  connect(viewer_,
          SIGNAL(addSelected(const Selected&)),
          this,
          SLOT(addSelected(const Selected&)));
  connect(viewer_,
          SIGNAL(addSelected(const SelectionSet&)),
          this,
          SLOT(addSelected(const SelectionSet&)));

  connect(
      viewer_, &LayoutViewer::addRuler, [this](int x0, int y0, int x1, int y1) {
        addRuler(x0, y0, x1, y1, "", "", default_ruler_style_->isChecked());
      });

  connect(this, SIGNAL(selectionChanged()), viewer_, SLOT(update()));
  connect(this, SIGNAL(highlightChanged()), viewer_, SLOT(update()));
  connect(this, SIGNAL(rulersChanged()), viewer_, SLOT(update()));

  connect(controls_,
          SIGNAL(selected(const Selected&)),
          this,
          SLOT(setSelected(const Selected&)));

  connect(inspector_,
          SIGNAL(selected(const Selected&, bool)),
          this,
          SLOT(setSelected(const Selected&, bool)));
  connect(inspector_,
          SIGNAL(addSelected(const Selected&)),
          this,
          SLOT(addSelected(const Selected&)));
  connect(inspector_,
          SIGNAL(removeSelected(const Selected&)),
          this,
          SLOT(removeSelected(const Selected&)));
  connect(this,
          SIGNAL(selectionChanged(const Selected&)),
          inspector_,
          SLOT(update(const Selected&)));
  connect(inspector_,
          SIGNAL(selectedItemChanged(const Selected&)),
          selection_browser_,
          SLOT(updateModels()));
  connect(inspector_,
          SIGNAL(selectedItemChanged(const Selected&)),
          viewer_,
          SLOT(update()));
  connect(inspector_,
          SIGNAL(selectedItemChanged(const Selected&)),
          this,
          SLOT(updateSelectedStatus(const Selected&)));
  connect(inspector_,
          SIGNAL(selection(const Selected&)),
          viewer_,
          SLOT(selection(const Selected&)));
  connect(inspector_,
          SIGNAL(focus(const Selected&)),
          viewer_,
          SLOT(selectionFocus(const Selected&)));
  connect(drc_viewer_,
          SIGNAL(focus(const Selected&)),
          viewer_,
          SLOT(selectionFocus(const Selected&)));
  connect(
      this, SIGNAL(highlightChanged()), inspector_, SLOT(highlightChanged()));
  connect(viewer_,
          SIGNAL(focusNetsChanged()),
          inspector_,
          SLOT(focusNetsChanged()));
  connect(inspector_,
          SIGNAL(removeHighlight(const QList<const Selected*>&)),
          this,
          SLOT(removeFromHighlighted(const QList<const Selected*>&)));
  connect(inspector_,
          SIGNAL(addHighlight(const SelectionSet&)),
          this,
          SLOT(addHighlighted(const SelectionSet&)));

  connect(hierarchy_widget_,
          SIGNAL(select(const SelectionSet&)),
          this,
          SLOT(setSelected(const SelectionSet&)));
  connect(hierarchy_widget_,
          SIGNAL(removeSelect(const Selected&)),
          this,
          SLOT(removeSelected(const Selected&)));
  connect(hierarchy_widget_,
          SIGNAL(highlight(const SelectionSet&)),
          this,
          SLOT(addHighlighted(const SelectionSet&)));
  connect(hierarchy_widget_,
          SIGNAL(removeHighlight(const Selected&)),
          this,
          SLOT(removeHighlighted(const Selected&)));
  connect(hierarchy_widget_,
          SIGNAL(updateModuleVisibility(odb::dbModule*, bool)),
          viewer_,
          SLOT(updateModuleVisibility(odb::dbModule*, bool)));
  connect(hierarchy_widget_,
          SIGNAL(updateModuleColor(odb::dbModule*, const QColor&, bool)),
          viewer_,
          SLOT(updateModuleColor(odb::dbModule*, const QColor&, bool)));

  connect(
      timing_widget_, &TimingWidget::inspect, [this](const Selected& selected) {
        inspector_->inspect(selected);
        inspector_->raise();
      });
  connect(selection_browser_,
          SIGNAL(selected(const Selected&)),
          inspector_,
          SLOT(inspect(const Selected&)));
  connect(this,
          SIGNAL(selectionChanged()),
          selection_browser_,
          SLOT(updateSelectionModel()));
  connect(this,
          SIGNAL(highlightChanged()),
          selection_browser_,
          SLOT(updateHighlightModel()));
  connect(clock_viewer_,
          SIGNAL(selected(const Selected&)),
          this,
          SLOT(addSelected(const Selected&)));

  connect(selection_browser_,
          &SelectHighlightWindow::clearAllSelections,
          this,
          [this]() { this->setSelected(Selected(), false); });
  connect(selection_browser_,
          &SelectHighlightWindow::clearAllHighlights,
          this,
          [this]() { this->clearHighlighted(); });
  connect(selection_browser_,
          SIGNAL(clearSelectedItems(const QList<const Selected*>&)),
          this,
          SLOT(removeFromSelected(const QList<const Selected*>&)));

  connect(selection_browser_,
          SIGNAL(zoomInToItems(const QList<const Selected*>&)),
          this,
          SLOT(zoomInToItems(const QList<const Selected*>&)));

  connect(selection_browser_,
          SIGNAL(clearHighlightedItems(const QList<const Selected*>&)),
          this,
          SLOT(removeFromHighlighted(const QList<const Selected*>&)));

  connect(selection_browser_,
          SIGNAL(highlightSelectedItemsSig(const QList<const Selected*>&)),
          this,
          SLOT(updateHighlightedSet(const QList<const Selected*>&)));

  connect(timing_widget_,
          SIGNAL(highlightTimingPath(TimingPath*)),
          viewer_,
          SLOT(update()));

  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          this,
          SLOT(setBlock(odb::dbBlock*)));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          drc_viewer_,
          SLOT(setBlock(odb::dbBlock*)));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          clock_viewer_,
          SLOT(setBlock(odb::dbBlock*)));
  connect(drc_viewer_, &DRCWidget::selectDRC, [this](const Selected& selected) {
    setSelected(selected, false);
    odb::Rect bbox;
    selected.getBBox(bbox);

    auto* block = getBlock();
    int zoomout_dist = std::numeric_limits<int>::max();
    if (block != nullptr) {
      // 10 microns
      zoomout_dist = 10 * block->getDbUnitsPerMicron();
    }
    // twice the largest dimension of bounding box
    const int zoomout_box = 2 * std::max(bbox.dx(), bbox.dy());
    // pick smallest
    const int zoomout_margin = std::min(zoomout_dist, zoomout_box);
    bbox.set_xlo(bbox.xMin() - zoomout_margin);
    bbox.set_ylo(bbox.yMin() - zoomout_margin);
    bbox.set_xhi(bbox.xMax() + zoomout_margin);
    bbox.set_yhi(bbox.yMax() + zoomout_margin);
    zoomTo(bbox);
  });
  connect(this, &MainWindow::selectionChanged, [this]() {
    if (!selected_.empty()) {
      drc_viewer_->updateSelection(*selected_.begin());
    }
  });

  createActions();
  createToolbars();
  createMenus();
  createStatusBar();

  // Restore the settings (if none this is a no-op)
  QSettings settings("OpenRoad Project", "openroad");
  settings.beginGroup("main");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("state").toByteArray());
  QApplication::setFont(
      settings.value("font", QApplication::font()).value<QFont>());
  hide_option_->setChecked(
      settings.value("check_exit", hide_option_->isChecked()).toBool());
  show_dbu_->setChecked(
      settings.value("use_dbu", show_dbu_->isChecked()).toBool());
  default_ruler_style_->setChecked(
      settings.value("ruler_style", default_ruler_style_->isChecked())
          .toBool());
  script_->readSettings(&settings);
  controls_->readSettings(&settings);
  timing_widget_->readSettings(&settings);
  hierarchy_widget_->readSettings(&settings);
  settings.endGroup();

  // load resources and set window icon and title
  loadQTResources();
  setWindowIcon(QIcon(":/icon.png"));
  setWindowTitle("OpenROAD");

  Descriptor::Property::convert_dbu
      = [this](int value, bool add_units) -> std::string {
    return convertDBUToString(value, add_units);
  };
  Descriptor::Property::convert_string
      = [this](const std::string& value, bool* ok) -> int {
    return convertStringToDBU(value, ok);
  };
}

MainWindow::~MainWindow()
{
  auto* gui = Gui::get();
  // unregister descriptors with GUI dependencies
  gui->unregisterDescriptor<Ruler*>();
  gui->unregisterDescriptor<odb::dbNet*>();
  gui->unregisterDescriptor<DbNetDescriptor::NetWithSink>();
}

void MainWindow::setDatabase(odb::dbDatabase* db)
{
  // set database and pass along
  db_ = db;
  controls_->setDb(db_);

  auto* chip = db->getChip();
  if (chip != nullptr) {
    viewer_->designLoaded(chip->getBlock());
  }
}

void MainWindow::setBlock(odb::dbBlock* block)
{
  for (auto* heat_map : Gui::get()->getHeatMaps()) {
    heat_map->setBlock(block);
  }
  hierarchy_widget_->setBlock(block);
}

void MainWindow::init(sta::dbSta* sta)
{
  // Setup widgets
  timing_widget_->init(sta);
  controls_->setSTA(sta);
  hierarchy_widget_->setSTA(sta);
  clock_viewer_->setSTA(sta);
  // register descriptors
  auto* gui = Gui::get();
  auto* inst_descriptor = new DbInstDescriptor(db_, sta);
  gui->registerDescriptor<odb::dbInst*>(inst_descriptor);
  gui->registerDescriptor<odb::dbMaster*>(new DbMasterDescriptor(db_, sta));
  gui->registerDescriptor<odb::dbNet*>(new DbNetDescriptor(
      db_, sta, viewer_->getFocusNets(), viewer_->getRouteGuides()));
  gui->registerDescriptor<DbNetDescriptor::NetWithSink>(new DbNetDescriptor(
      db_, sta, viewer_->getFocusNets(), viewer_->getRouteGuides()));
  gui->registerDescriptor<odb::dbITerm*>(new DbITermDescriptor(db_));
  gui->registerDescriptor<odb::dbBTerm*>(new DbBTermDescriptor(db_));
  gui->registerDescriptor<odb::dbBlockage*>(new DbBlockageDescriptor(db_));
  gui->registerDescriptor<odb::dbObstruction*>(
      new DbObstructionDescriptor(db_));
  gui->registerDescriptor<odb::dbTechLayer*>(new DbTechLayerDescriptor(db_));
  gui->registerDescriptor<DbItermAccessPoint>(
      new DbItermAccessPointDescriptor(db_));
  gui->registerDescriptor<odb::dbGroup*>(new DbGroupDescriptor(db_));
  gui->registerDescriptor<odb::dbRegion*>(new DbRegionDescriptor(db_));
  gui->registerDescriptor<odb::dbModule*>(new DbModuleDescriptor(db_));
  gui->registerDescriptor<odb::dbTechVia*>(new DbTechViaDescriptor(db_));
  gui->registerDescriptor<odb::dbTechViaGenerateRule*>(
      new DbGenerateViaDescriptor(db_));
  gui->registerDescriptor<odb::dbTechNonDefaultRule*>(
      new DbNonDefaultRuleDescriptor(db_));
  gui->registerDescriptor<odb::dbTechLayerRule*>(
      new DbTechLayerRuleDescriptor(db_));
  gui->registerDescriptor<odb::dbTechSameNetRule*>(
      new DbTechSameNetRuleDescriptor(db_));
  gui->registerDescriptor<odb::dbSite*>(new DbSiteDescriptor(db_));
  gui->registerDescriptor<DbSiteDescriptor::SpecificSite>(
      new DbSiteDescriptor(db_));
  gui->registerDescriptor<odb::dbRow*>(new DbRowDescriptor(db_));
  gui->registerDescriptor<Ruler*>(new RulerDescriptor(rulers_, db_));

  controls_->setDBInstDescriptor(inst_descriptor);
  hierarchy_widget_->setDBInstDescriptor(inst_descriptor);
}

void MainWindow::createStatusBar()
{
  location_ = new QLabel(this);
  statusBar()->addPermanentWidget(location_);
}

odb::dbBlock* MainWindow::getBlock() const
{
  if (!db_) {
    return nullptr;
  }

  auto chip = db_->getChip();
  if (!chip) {
    return nullptr;
  }

  auto top_block = chip->getBlock();
  return top_block;
}

void MainWindow::createActions()
{
  hide_ = new QAction("Hide GUI", this);
  hide_option_ = new QAction("Check on exit", this);
  hide_option_->setCheckable(true);
  hide_option_->setChecked(true);
  exit_ = new QAction("Exit", this);

  open_ = new QAction("Open DB", this);

  fit_ = new QAction("Fit", this);
  fit_->setShortcut(QString("F"));

  find_ = new QAction("Find", this);
  find_->setShortcut(QString("Ctrl+F"));
  zoom_in_ = new QAction("Zoom in", this);
  zoom_in_->setShortcut(QString("Z"));

  zoom_out_ = new QAction("Zoom out", this);
  zoom_out_->setShortcut(QString("Shift+Z"));

  inspect_ = new QAction("Inspect", this);
  inspect_->setShortcut(QString("q"));

  timing_debug_ = new QAction("Timing...", this);
  timing_debug_->setShortcut(QString("Ctrl+T"));

  help_ = new QAction("Help", this);
  help_->setShortcut(QString("Ctrl+H"));

  build_ruler_ = new QAction("Ruler", this);
  build_ruler_->setShortcut(QString("k"));

  show_dbu_ = new QAction("Show DBU", this);
  show_dbu_->setCheckable(true);
  show_dbu_->setChecked(false);

  default_ruler_style_ = new QAction("Make euclidian rulers", this);
  default_ruler_style_->setCheckable(true);
  default_ruler_style_->setChecked(true);

  font_ = new QAction("Application font", this);

  global_connect_ = new QAction("Global connect", this);
  global_connect_->setShortcut(QString("Ctrl+G"));

  connect(open_, SIGNAL(triggered()), this, SLOT(openDesign()));
  connect(
      this, &MainWindow::designLoaded, [this]() { open_->setEnabled(false); });
  connect(hide_, SIGNAL(triggered()), this, SIGNAL(hide()));
  connect(exit_, SIGNAL(triggered()), this, SIGNAL(exit()));
  connect(fit_, SIGNAL(triggered()), viewer_, SLOT(fit()));
  connect(zoom_in_, SIGNAL(triggered()), viewer_, SLOT(zoomIn()));
  connect(zoom_out_, SIGNAL(triggered()), viewer_, SLOT(zoomOut()));
  connect(find_, SIGNAL(triggered()), this, SLOT(showFindDialog()));
  connect(inspect_, SIGNAL(triggered()), inspector_, SLOT(show()));
  connect(timing_debug_, SIGNAL(triggered()), timing_widget_, SLOT(show()));
  connect(help_, SIGNAL(triggered()), this, SLOT(showHelp()));

  connect(build_ruler_, SIGNAL(triggered()), viewer_, SLOT(startRulerBuild()));

  connect(show_dbu_, SIGNAL(toggled(bool)), viewer_, SLOT(fullRepaint()));
  connect(show_dbu_, SIGNAL(toggled(bool)), inspector_, SLOT(reload()));
  connect(show_dbu_,
          SIGNAL(toggled(bool)),
          selection_browser_,
          SLOT(updateModels()));
  connect(show_dbu_, SIGNAL(toggled(bool)), this, SLOT(setUseDBU(bool)));
  connect(show_dbu_, SIGNAL(toggled(bool)), this, SLOT(setClearLocation()));

  connect(font_, SIGNAL(triggered()), this, SLOT(showApplicationFont()));

  connect(
      global_connect_, SIGNAL(triggered()), this, SLOT(showGlobalConnect()));
}

void MainWindow::setUseDBU(bool use_dbu)
{
  for (auto* heat_map : Gui::get()->getHeatMaps()) {
    heat_map->setUseDBU(use_dbu);
  }
}

void MainWindow::showApplicationFont()
{
  bool okay = false;
  QFont font = QFontDialog::getFont(
      &okay, QApplication::font(), this, "Application font");

  if (okay) {
    QApplication::setFont(font);
    update();
  }
}

void MainWindow::createMenus()
{
  file_menu_ = menuBar()->addMenu("&File");
  file_menu_->addAction(open_);
  file_menu_->addAction(hide_);
  file_menu_->addAction(exit_);

  view_menu_ = menuBar()->addMenu("&View");
  view_menu_->addAction(fit_);
  view_menu_->addAction(find_);
  view_menu_->addAction(zoom_in_);
  view_menu_->addAction(zoom_out_);

  tools_menu_ = menuBar()->addMenu("&Tools");
  tools_menu_->addAction(build_ruler_);
  auto heat_maps = tools_menu_->addMenu("&Heat maps");
  heat_maps->setObjectName("HeatMaps");
  for (auto* heat_map : Gui::get()->getHeatMaps()) {
    registerHeatMap(heat_map);
  }
  tools_menu_->addAction(global_connect_);

  windows_menu_ = menuBar()->addMenu("&Windows");
  windows_menu_->addAction(controls_->toggleViewAction());
  windows_menu_->addAction(inspector_->toggleViewAction());
  windows_menu_->addAction(script_->toggleViewAction());
  windows_menu_->addAction(selection_browser_->toggleViewAction());
  windows_menu_->addAction(view_tool_bar_->toggleViewAction());
  windows_menu_->addAction(timing_widget_->toggleViewAction());
  windows_menu_->addAction(drc_viewer_->toggleViewAction());
  windows_menu_->addAction(clock_viewer_->toggleViewAction());
  windows_menu_->addAction(hierarchy_widget_->toggleViewAction());

  auto option_menu = menuBar()->addMenu("&Options");
  option_menu->addAction(hide_option_);
  option_menu->addAction(show_dbu_);
  option_menu->addAction(default_ruler_style_);
  option_menu->addAction(font_);

  menuBar()->addAction(help_);
}

void MainWindow::createToolbars()
{
  view_tool_bar_ = addToolBar("Toolbar");
  view_tool_bar_->addAction(fit_);
  view_tool_bar_->addAction(find_);
  view_tool_bar_->addAction(inspect_);
  view_tool_bar_->addAction(timing_debug_);

  view_tool_bar_->setObjectName("view_toolbar");  // for settings
}

const std::string MainWindow::addToolbarButton(const std::string& name,
                                               const QString& text,
                                               const QString& script,
                                               bool echo)
{
  // ensure key is unique
  std::string key;
  if (name.empty()) {
    int key_idx = 0;
    do {
      // default to "buttonX" naming
      key = "button" + std::to_string(key_idx);
      key_idx++;
    } while (buttons_.count(key) != 0);
  } else {
    if (buttons_.count(name) != 0) {
      logger_->error(utl::GUI, 22, "Button {} already defined.", name);
    }
    key = name;
  }

  auto action = view_tool_bar_->addAction(text);
  // save the command so it can be restored later
  QString cmd = "gui::create_toolbar_button ";
  cmd += "{" + QString::fromStdString(name) + "} {" + text + "} ";
  cmd += "{" + script + "} ";
  cmd += echo ? "true" : "false";
  action->setData(cmd);

  connect(action, &QAction::triggered, [script, echo, this]() {
    script_->executeCommand(script, echo);
  });

  buttons_[key] = std::unique_ptr<QAction>(action);

  return key;
}

void MainWindow::removeToolbarButton(const std::string& name)
{
  if (buttons_.count(name) == 0) {
    return;
  }

  view_tool_bar_->removeAction(buttons_[name].get());
  buttons_.erase(name);
}

QMenu* MainWindow::findMenu(QStringList& path, QMenu* parent)
{
  if (path.isEmpty()) {
    return parent;
  }

  auto cleanupText = [](const QString& text) -> QString {
    QString text_cpy = text;
    text_cpy.replace(QRegExp("&(?!&)"), "");  // remove single &, but keep &&
    return text_cpy;
  };

  const QString top_name = path[0];
  const QString compare_name = cleanupText(top_name);
  path.pop_front();

  QList<QAction*> actions;
  if (parent == nullptr) {
    actions = menuBar()->actions();
  } else {
    actions = parent->actions();
  }

  QMenu* menu = nullptr;
  for (auto* action : actions) {
    if (cleanupText(action->text()) == compare_name) {
      menu = action->menu();
    }
  }

  if (menu == nullptr) {
    if (parent == nullptr) {
      menu = menuBar()->addMenu(top_name);
    } else {
      menu = parent->addMenu(top_name);
    }
  }

  return findMenu(path, menu);
}

const std::string MainWindow::addMenuItem(const std::string& name,
                                          const QString& path,
                                          const QString& text,
                                          const QString& script,
                                          const QString& shortcut,
                                          bool echo)
{
  // ensure key is unique
  std::string key;
  if (name.empty()) {
    int key_idx = 0;
    do {
      // default to "actionX" naming
      key = "action" + std::to_string(key_idx);
      key_idx++;
    } while (menu_actions_.count(key) != 0);
  } else {
    if (menu_actions_.count(name) != 0) {
      logger_->error(utl::GUI, 25, "Menu action {} already defined.", name);
    }
    key = name;
  }

  QStringList path_parts;
  for (const auto& part : path.split("/")) {
    const QString path_part = part.trimmed();
    if (!path_part.isEmpty()) {
      path_parts.append(path_part);
    }
  }
  if (path_parts.isEmpty()) {
    path_parts.append("&Custom Scripts");
  }
  QMenu* menu = findMenu(path_parts);

  auto action = menu->addAction(text);
  if (!shortcut.isEmpty()) {
    action->setShortcut(shortcut);
  }
  // save the command so it can be restored later
  QString cmd = "gui::create_menu_item ";
  cmd += "{" + QString::fromStdString(name) + "} ";
  cmd += "{" + path_parts.join("/") + "} ";
  cmd += "{" + text + "} ";
  cmd += "{" + script + "} ";
  cmd += "{" + shortcut + "} ";
  cmd += echo ? "true" : "false";
  action->setData(cmd);

  connect(action, &QAction::triggered, [script, echo, this]() {
    script_->executeCommand(script, echo);
  });

  menu_actions_[key] = std::unique_ptr<QAction>(action);

  return key;
}

void MainWindow::removeMenu(QMenu* menu)
{
  if (!menu->isEmpty()) {
    return;
  }

  auto* parent = menu->parent();
  if (parent != menuBar()) {
    QMenu* parent_menu = qobject_cast<QMenu*>(parent);
    parent_menu->removeAction(menu->menuAction());
    removeMenu(parent_menu);
  } else {
    menuBar()->removeAction(menu->menuAction());
  }
}

void MainWindow::removeMenuItem(const std::string& name)
{
  if (menu_actions_.count(name) == 0) {
    return;
  }

  auto* action = menu_actions_[name].get();
  QMenu* menu = qobject_cast<QMenu*>(action->parent());
  menu->removeAction(action);

  removeMenu(menu);

  menu_actions_.erase(name);
}

const std::string MainWindow::requestUserInput(const QString& title,
                                               const QString& question)
{
  QString text = QInputDialog::getText(this, title, question);
  return text.toStdString();
}

void MainWindow::setLocation(int x, int y)
{
  QString location;
  location += QString::fromStdString(convertDBUToString(x, false));
  location += ", ";
  location += QString::fromStdString(convertDBUToString(y, false));
  location_->setText(location);
}

void MainWindow::setClearLocation()
{
  location_->setText("");
}

void MainWindow::updateSelectedStatus(const Selected& selection)
{
  status(selection ? selection.getName() : "");
}

void MainWindow::addSelected(const Selected& selection)
{
  if (selection) {
    selected_.emplace(selection);
    emit selectionChanged(selection);
  }
  emit updateSelectedStatus(selection);
}

void MainWindow::removeSelected(const Selected& selection)
{
  auto itr = std::find(selected_.begin(), selected_.end(), selection);
  if (itr != selected_.end()) {
    selected_.erase(itr);
    emit selectionChanged();
  }
}

void MainWindow::removeHighlighted(const Selected& selection)
{
  for (auto& group : highlighted_) {
    auto itr = std::find(group.begin(), group.end(), selection);
    if (itr != group.end()) {
      group.erase(itr);
      emit highlightChanged();
    }
  }
}

void MainWindow::removeSelectedByType(const std::string& type)
{
  bool changed = false;
  for (auto itr = selected_.begin(); itr != selected_.end();) {
    const auto& selection = *itr;

    if (selection.getTypeName() == type) {
      itr = selected_.erase(itr);
      changed = true;
    } else {
      itr++;
    }
  }

  if (changed) {
    emit selectionChanged();
  }
}

void MainWindow::addSelected(const SelectionSet& selections)
{
  int prev_selected_size = selected_.size();
  for (const auto& selection : selections) {
    if (selection) {
      selected_.insert(selection);
    }
  }
  status(std::string("Added ")
         + std::to_string(selected_.size() - prev_selected_size));
  emit selectionChanged();
}

void MainWindow::setSelected(const SelectionSet& selections)
{
  selected_.clear();
  addSelected(selections);
}

void MainWindow::setSelected(const Selected& selection, bool show_connectivity)
{
  selected_.clear();
  addSelected(selection);
  if (show_connectivity)
    selectHighlightConnectedNets(true, true, true, false);

  emit selectionChanged();
}

void MainWindow::addHighlighted(const SelectionSet& highlights,
                                int highlight_group)
{
  if (highlight_group < 0) {
    highlight_group = requestHighlightGroup();
    if (highlight_group < 0) {
      return;
    }
  }

  if (highlight_group >= highlighted_.size()) {
    return;
  }

  auto& group = highlighted_[highlight_group];
  for (const auto& highlight : highlights) {
    if (highlight) {
      group.insert(highlight);
    }
  }
  emit highlightChanged();
}

std::string MainWindow::addRuler(int x0,
                                 int y0,
                                 int x1,
                                 int y1,
                                 const std::string& label,
                                 const std::string& name,
                                 bool euclidian)
{
  auto new_ruler = std::make_unique<Ruler>(
      odb::Point(x0, y0), odb::Point(x1, y1), name, label);
  new_ruler->setEuclidian(euclidian);
  std::string new_name = new_ruler->getName();

  // check if ruler name is unique
  for (const auto& ruler : rulers_) {
    if (new_name == ruler->getName()) {
      logger_->warn(
          utl::GUI, 24, "Ruler with name \"{}\" already exists", new_name);
      return "";
    }
  }

  rulers_.push_back(std::move(new_ruler));
  emit rulersChanged();
  return new_name;
}

void MainWindow::deleteRuler(const std::string& name)
{
  auto ruler_find
      = std::find_if(rulers_.begin(), rulers_.end(), [name](const auto& l) {
          return l->getName() == name;
        });
  if (ruler_find != rulers_.end()) {
    // remove from selected set
    auto remove_selected = Gui::get()->makeSelected(ruler_find->get());
    if (selected_.find(remove_selected) != selected_.end()) {
      selected_.erase(remove_selected);
      emit selectionChanged();
    }
    rulers_.erase(ruler_find);
    emit rulersChanged();
  }
}

int MainWindow::requestHighlightGroup()
{
  HighlightGroupDialog dlg;
  if (dlg.exec() == QDialog::Rejected) {
    return -1;
  }
  return dlg.getSelectedHighlightGroup();
}

void MainWindow::updateHighlightedSet(const QList<const Selected*>& items,
                                      int highlight_group)
{
  if (highlight_group < 0) {
    highlight_group = requestHighlightGroup();
    if (highlight_group < 0) {
      return;
    }
  }

  if (highlight_group >= highlighted_.size()) {
    return;
  }

  // Hold on to selected items as the pointers will be invalid
  QList<Selected> items_storage;
  for (auto item : items) {
    items_storage.push_back(*item);
  }
  // Remove any items that might already be selected
  removeFromHighlighted(items);

  highlighted_[highlight_group].insert(items_storage.begin(),
                                       items_storage.end());
  emit highlightChanged();
}

void MainWindow::clearHighlighted(int highlight_group)
{
  if (highlighted_.empty())
    return;
  int num_items_cleared = 0;
  if (highlight_group < 0) {
    for (auto& highlighted_set : highlighted_) {
      num_items_cleared += highlighted_set.size();
      highlighted_set.clear();
    }
  } else if (highlight_group < highlighted_.size()) {
    num_items_cleared += highlighted_[highlight_group].size();
    highlighted_[highlight_group].clear();
  }
  if (num_items_cleared > 0)
    emit highlightChanged();
}

void MainWindow::clearRulers()
{
  if (rulers_.empty()) {
    return;
  }
  Gui::get()->removeSelected<Ruler*>();
  rulers_.clear();
  emit rulersChanged();
}

void MainWindow::removeFromSelected(const QList<const Selected*>& items)
{
  if (items.empty())
    return;
  for (auto& item : items) {
    selected_.erase(*item);
  }
  emit selectionChanged();
}

void MainWindow::removeFromHighlighted(const QList<const Selected*>& items,
                                       int highlight_group)
{
  if (items.empty())
    return;
  if (highlight_group < 0) {
    for (auto& item : items) {
      for (auto& highlighted_set : highlighted_)
        highlighted_set.erase(*item);
    }
  } else if (highlight_group < highlighted_.size()) {
    for (auto& item : items) {
      highlighted_[highlight_group].erase(*item);
    }
  }

  emit highlightChanged();
}

void MainWindow::zoomTo(const odb::Rect& rect_dbu)
{
  viewer_->zoomTo(rect_dbu);
}

void MainWindow::zoomInToItems(const QList<const Selected*>& items)
{
  if (items.empty())
    return;
  odb::Rect items_bbox;
  items_bbox.mergeInit();
  int merge_cnt = 0;
  for (auto& item : items) {
    odb::Rect item_bbox;
    if (item->getBBox(item_bbox)) {
      merge_cnt++;
      items_bbox.merge(item_bbox);
    }
  }
  if (merge_cnt == 0)
    return;
  zoomTo(items_bbox);
}

void MainWindow::status(const std::string& message)
{
  statusBar()->showMessage(QString::fromStdString(message));
}

void MainWindow::showFindDialog()
{
  if (getBlock() == nullptr)
    return;
  find_dialog_->exec();
}

void MainWindow::showHelp()
{
  const QUrl help_url("https://openroad.readthedocs.io/en/latest/");
  if (!QDesktopServices::openUrl(help_url)) {
    // failed to open
    logger_->warn(utl::GUI,
                  23,
                  "Failed to open help automatically, navigate to: {}",
                  help_url.toString().toStdString());
  }
}

bool MainWindow::anyObjectInSet(bool selection_set, odb::dbObjectType obj_type)
{
  if (selection_set) {
    for (auto& selected_obj : selected_) {
      if ((selected_obj.isInst() && obj_type == odb::dbInstObj)
          || (selected_obj.isNet() && obj_type == odb::dbNetObj))
        return true;
    }
    return false;
  } else {
    for (auto& highlight_set : highlighted_) {
      for (auto& selected_obj : highlight_set) {
        if (selected_obj.isInst() && obj_type == odb::dbInstObj)
          return true;
        if (selected_obj.isNet() && obj_type == odb::dbNetObj)
          return true;
      }
    }
  }
  return false;
}

void MainWindow::selectHighlightConnectedInsts(bool select_flag,
                                               int highlight_group)
{
  SelectionSet connected_insts;
  for (auto& sel_obj : selected_) {
    if (sel_obj.isNet()) {
      auto net_obj = std::any_cast<odb::dbNet*>(sel_obj.getObject());
      for (auto inst_term : net_obj->getITerms()) {
        connected_insts.insert(Gui::get()->makeSelected(inst_term));
      }
    }
  }
  if (connected_insts.empty())
    return;
  if (select_flag)
    addSelected(connected_insts);
  else
    addHighlighted(connected_insts, highlight_group);
}

void MainWindow::selectHighlightConnectedNets(bool select_flag,
                                              bool output,
                                              bool input,
                                              int highlight_group)
{
  SelectionSet connected_nets;
  for (auto& sel_obj : selected_) {
    if (sel_obj.isInst()) {
      auto inst_obj = std::any_cast<odb::dbInst*>(sel_obj.getObject());
      for (auto inst_term : inst_obj->getITerms()) {
        if (inst_term->getNet() == nullptr
            || inst_term->getNet()->getSigType() != odb::dbSigType::SIGNAL)
          continue;
        auto inst_term_dir = inst_term->getIoType();

        if (output
            && (inst_term_dir == odb::dbIoType::OUTPUT
                || inst_term_dir == odb::dbIoType::INOUT))
          connected_nets.insert(Gui::get()->makeSelected(inst_term->getNet()));
        if (input
            && (inst_term_dir == odb::dbIoType::INPUT
                || inst_term_dir == odb::dbIoType::INOUT))
          connected_nets.insert(Gui::get()->makeSelected(
              DbNetDescriptor::NetWithSink{inst_term->getNet(), inst_term}));
      }
    }
  }
  if (connected_nets.empty())
    return;
  if (select_flag)
    addSelected(connected_nets);
  else
    addHighlighted(connected_nets, highlight_group);
}

void MainWindow::saveSettings()
{
  QSettings settings("OpenRoad Project", "openroad");
  settings.beginGroup("main");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("state", saveState());
  settings.setValue("font", QApplication::font());
  settings.setValue("check_exit", hide_option_->isChecked());
  settings.setValue("use_dbu", show_dbu_->isChecked());
  settings.setValue("ruler_style", default_ruler_style_->isChecked());
  script_->writeSettings(&settings);
  controls_->writeSettings(&settings);
  timing_widget_->writeSettings(&settings);
  hierarchy_widget_->writeSettings(&settings);
  settings.endGroup();
}

void MainWindow::postReadLef(odb::dbTech* tech, odb::dbLib* library)
{
  // We don't process this until we have a design to show
}

void MainWindow::postReadDef(odb::dbBlock* block)
{
  emit designLoaded(block);
}

void MainWindow::postReadDb(odb::dbDatabase* db)
{
  auto chip = db->getChip();
  if (chip == nullptr) {
    return;
  }
  auto block = chip->getBlock();
  if (block == nullptr) {
    return;
  }

  emit designLoaded(block);
}

void MainWindow::setLogger(utl::Logger* logger)
{
  logger_ = logger;
  controls_->setLogger(logger);
  script_->setLogger(logger);
  viewer_->setLogger(logger);
  drc_viewer_->setLogger(logger);
  clock_viewer_->setLogger(logger);
}

void MainWindow::fit()
{
  fit_->trigger();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape) {
    // Esc stop building ruler
    viewer_->cancelRulerBuild();
  } else if (event->key() == Qt::Key_K
             && event->modifiers() & Qt::ShiftModifier) {
    // Shift + K, remove all rulers
    clearRulers();
  }
  QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (!hide_option_->isChecked()) {
    // no check required, just go ahead and exit
    event->accept();
    return;
  }

  // Ask if user wants to exit or return to command line.
  QMessageBox exit_check;
  exit_check.setIcon(QMessageBox::Question);
  exit_check.setWindowTitle(windowTitle());
  exit_check.setText("Are you sure you want to exit?");
  QPushButton* exit_button
      = exit_check.addButton("Exit", QMessageBox::AcceptRole);
  QPushButton* cancel_button = exit_check.addButton(QMessageBox::Cancel);
  QPushButton* hide_button
      = exit_check.addButton("Hide GUI", QMessageBox::ActionRole);

  // Colorize exit and hide buttons
  exit_button->setStyleSheet("background-color: darkred; color: white;");
  hide_button->setStyleSheet("background-color: darkgreen; color: white;");

  // default option is to cancel
  exit_check.setDefaultButton(cancel_button);

  exit_check.exec();

  if (exit_check.clickedButton() == exit_button) {
    // exit selected so go ahead and close
    event->accept();
  } else if (exit_check.clickedButton() == hide_button) {
    // hide selected so process as hide
    emit hide();
    event->accept();
  } else {
    // cancel selected so ignore event
    event->ignore();
  }
}

const std::vector<std::string> MainWindow::getRestoreTclCommands()
{
  std::vector<std::string> cmds;
  // Save rulers
  for (const auto& ruler : rulers_) {
    cmds.push_back(ruler->getTclCommand(
        db_->getChip()->getBlock()->getDbUnitsPerMicron()));
  }
  // Save buttons
  for (const auto& action : view_tool_bar_->actions()) {
    // iterate over toolbar actions to get the correct order
    QVariant cmd = action->data();
    if (cmd.isValid()) {
      cmds.push_back(cmd.toString().toStdString());
    }
  }
  // Save menu actions
  for (const auto& [name, action] : menu_actions_) {
    cmds.push_back(action->data().toString().toStdString());
  }
  // save display settings
  controls_->restoreTclCommands(cmds);

  // save layout view
  viewer_->restoreTclCommands(cmds);

  return cmds;
}

std::string MainWindow::convertDBUToString(int value, bool add_units) const
{
  if (show_dbu_->isChecked()) {
    return std::to_string(value);
  } else {
    auto* block = getBlock();
    if (block == nullptr) {
      return std::to_string(value);
    } else {
      const double dbu_per_micron = block->getDbUnitsPerMicron();

      std::stringstream ss;
      const int precision = std::ceil(std::log10(dbu_per_micron));
      const double micron_value = value / dbu_per_micron;

      ss << std::fixed << std::setprecision(precision) << micron_value;

      if (add_units) {
        ss << " \u03BCm";  // micro meter
      }

      return ss.str();
    }
  }
}

int MainWindow::convertStringToDBU(const std::string& value, bool* ok) const
{
  QString new_value = QString::fromStdString(value).simplified();

  if (new_value.contains(" ")) {
    new_value = new_value.left(new_value.indexOf(" "));
  } else if (new_value.contains("u")) {
    new_value = new_value.left(new_value.indexOf("u"));
  } else if (new_value.contains("\u03BC")) {
    new_value = new_value.left(new_value.indexOf("\u03BC"));
  }

  if (show_dbu_->isChecked()) {
    return new_value.toInt(ok);
  } else {
    auto* block = getBlock();
    if (block == nullptr) {
      return new_value.toInt(ok);
    } else {
      const int dbu_per_micron = block->getDbUnitsPerMicron();

      return new_value.toDouble(ok) * dbu_per_micron;
    }
  }
}

void MainWindow::timingCone(Gui::odbTerm term, bool fanin, bool fanout)
{
  auto* renderer = timing_widget_->getConeRenderer();

  if (std::holds_alternative<odb::dbITerm*>(term)) {
    renderer->setITerm(std::get<odb::dbITerm*>(term), fanin, fanout);
  } else {
    renderer->setBTerm(std::get<odb::dbBTerm*>(term), fanin, fanout);
  }
}

void MainWindow::timingPathsThrough(const std::set<Gui::odbTerm>& terms)
{
  auto* settings = timing_widget_->getSettings();
  settings->setFromPin({});
  std::set<sta::Pin*> pins;
  for (const auto& term : terms) {
    pins.insert(settings->convertTerm(term));
  }
  settings->setThruPin({pins});
  settings->setToPin({});

  timing_widget_->updatePaths();
  timing_widget_->show();
  timing_widget_->raise();
}

void MainWindow::registerHeatMap(HeatMapDataSource* heatmap)
{
  auto* heat_maps = menuBar()->findChild<QMenu*>("HeatMaps");
  auto* action
      = heat_maps->addAction(QString::fromStdString(heatmap->getName()));
  heatmap_actions_[heatmap] = action;
  connect(action, &QAction::triggered, [heatmap]() { heatmap->showSetup(); });
}

void MainWindow::unregisterHeatMap(HeatMapDataSource* heatmap)
{
  auto* heat_maps = menuBar()->findChild<QMenu*>("HeatMaps");
  heat_maps->removeAction(heatmap_actions_[heatmap]);
  heatmap_actions_.erase(heatmap);
}

void MainWindow::showGlobalConnect()
{
  odb::dbBlock* block = getBlock();
  if (block == nullptr) {
    return;
  }

  GlobalConnectDialog dialog(block, this);
  dialog.exec();
}

void MainWindow::openDesign()
{
  const QString filefilter = "OpenDB (*.odb *.ODB)";
  const QString file = QFileDialog::getOpenFileName(
      this, "Open Design", QString(), filefilter);

  if (file.isEmpty()) {
    return;
  }

  try {
    if (file.endsWith(".odb", Qt::CaseInsensitive)) {
      ord::OpenRoad::openRoad()->readDb(file.toStdString().c_str());
      logger_->warn(utl::GUI,
                    77,
                    "Timing data is not stored in {} and must be loaded "
                    "separately, if needed.",
                    file.toStdString());
    } else {
      logger_->error(utl::GUI, 76, "Unknown filetype: {}", file.toStdString());
    }
  } catch (const std::exception&) {
    // do nothing
  }
}

}  // namespace gui
