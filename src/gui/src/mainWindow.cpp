// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#include "mainWindow.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QPushButton>
#include <QSize>
#include <QWidget>
#include <algorithm>
#include <any>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#endif
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
#include <cmath>
#include <string>
#include <utility>

#include "GUIProgress.h"
#include "browserWidget.h"
#include "bufferTreeDescriptor.h"
#include "chartsWidget.h"
#include "chiplet3DWidget.h"
#include "clockWidget.h"
#include "dbDescriptors.h"
#include "displayControls.h"
#include "drcWidget.h"
#include "findDialog.h"
#include "globalConnectDialog.h"
#include "gotoDialog.h"
#include "gui/gui.h"
#include "gui/heatMap.h"
#include "helpWidget.h"
#include "highlightGroupDialog.h"
#include "inspector.h"
#include "label.h"
#include "layoutTabs.h"
#include "layoutViewer.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "ruler.h"
#include "scriptWidget.h"
#include "selectHighlightWindow.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "staDescriptors.h"
#include "staGui.h"
#include "timingWidget.h"
#include "utl/Logger.h"
#include "utl/Progress.h"
#include "utl/algorithms.h"

// must be loaded in global namespace
static void loadQTResources()
{
  Q_INIT_RESOURCE(resource);
}

namespace gui {

MainWindow::MainWindow(bool load_settings, QWidget* parent)
    : QMainWindow(parent),
      db_(nullptr),
      logger_(nullptr),
      controls_(new DisplayControls(this)),
      inspector_(new Inspector(selected_, highlighted_, this)),
      script_(new ScriptWidget(this)),
      viewers_(new LayoutTabs(
          controls_,
          script_,
          selected_,
          highlighted_,
          rulers_,
          labels_,
          Gui::get(),
          [this]() -> bool { return show_dbu_->isChecked(); },
          [this]() -> bool { return show_poly_decomp_view_->isChecked(); },
          [this]() -> bool { return default_ruler_style_->isChecked(); },
          [this]() -> bool { return default_mouse_wheel_zoom_->isChecked(); },
          [this]() -> int { return arrow_keys_scroll_step_; },
          this)),
      selection_browser_(
          new SelectHighlightWindow(selected_, highlighted_, this)),
      timing_widget_(new TimingWidget(this)),
      drc_viewer_(new DRCWidget(this)),
      clock_viewer_(new ClockWidget(this)),
      hierarchy_widget_(
          new BrowserWidget(viewers_->getModuleSettings(), controls_, this)),
      charts_widget_(new ChartsWidget(this)),
      chiplet_viewer_(new Chiplet3DWidget(this)),
      chiplet_dock_(new QDockWidget("3D Viewer", this)),
      help_widget_(new HelpWidget(this)),
      find_dialog_(new FindObjectDialog(this)),
      goto_dialog_(new GotoLocationDialog(this, viewers_)),
      selection_timer_(std::make_unique<QTimer>()),
      highlight_timer_(std::make_unique<QTimer>())
{
  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  script_->setWidgetFont(font);

  setCentralWidget(viewers_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::BottomDockWidgetArea, selection_browser_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);
  addDockWidget(Qt::RightDockWidgetArea, inspector_);
  addDockWidget(Qt::RightDockWidgetArea, hierarchy_widget_);
  addDockWidget(Qt::RightDockWidgetArea, timing_widget_);
  addDockWidget(Qt::RightDockWidgetArea, drc_viewer_);
  addDockWidget(Qt::RightDockWidgetArea, clock_viewer_);
  addDockWidget(Qt::RightDockWidgetArea, charts_widget_);
  chiplet_dock_->setObjectName("chiplet_viewer_dock");
  chiplet_dock_->setWidget(chiplet_viewer_);
  addDockWidget(Qt::RightDockWidgetArea, chiplet_dock_);
  addDockWidget(Qt::RightDockWidgetArea, help_widget_);

  tabifyDockWidget(selection_browser_, script_);
  selection_browser_->hide();

  tabifyDockWidget(inspector_, hierarchy_widget_);
  tabifyDockWidget(inspector_, timing_widget_);
  tabifyDockWidget(inspector_, drc_viewer_);
  tabifyDockWidget(inspector_, clock_viewer_);
  tabifyDockWidget(inspector_, charts_widget_);
  tabifyDockWidget(inspector_, chiplet_dock_);
  tabifyDockWidget(inspector_, help_widget_);

  drc_viewer_->hide();
  clock_viewer_->hide();
  chiplet_dock_->hide();

  // Hook up all the signals/slots
  connect(viewers_,
          &LayoutTabs::setCurrentChip,
          controls_,
          &DisplayControls::setCurrentChip);
  connect(script_, &ScriptWidget::exiting, this, &MainWindow::exit);
  connect(script_,
          &ScriptWidget::commandExecuted,
          viewers_,
          &LayoutTabs::commandFinishedExecuting);
  connect(script_,
          &ScriptWidget::commandAboutToExecute,
          viewers_,
          &LayoutTabs::commandAboutToExecute);
  connect(this, &MainWindow::chipLoaded, viewers_, &LayoutTabs::chipLoaded);
  connect(this,
          &MainWindow::chipLoaded,
          chiplet_viewer_,
          &Chiplet3DWidget::setChip);
  connect(this, &MainWindow::redraw, viewers_, &LayoutTabs::fullRepaint);
  connect(
      this, &MainWindow::blockLoaded, controls_, &DisplayControls::blockLoaded);
  connect(
      this, &MainWindow::blockLoaded, timing_widget_, &TimingWidget::setBlock);

  connect(this, &MainWindow::pause, script_, &ScriptWidget::pause);
  connect(script_,
          &ScriptWidget::executionPaused,
          viewers_,
          &LayoutTabs::executionPaused);
  connect(
      controls_, &DisplayControls::changed, viewers_, &LayoutTabs::fullRepaint);
  connect(controls_,
          &DisplayControls::colorChanged,
          viewers_,
          &LayoutTabs::updateBackgroundColors);
  connect(controls_,
          &DisplayControls::changed,
          hierarchy_widget_,
          &BrowserWidget::displayControlsUpdated);
  connect(viewers_, &LayoutTabs::location, this, &MainWindow::setLocation);
  connect(viewers_,
          &LayoutTabs::selected,
          this,
          qOverload<const Selected&, bool>(&MainWindow::setSelected));
  connect(viewers_,
          qOverload<const Selected&>(&LayoutTabs::addSelected),
          [this](const Selected& selection) { addSelected(selection); });
  connect(viewers_,
          qOverload<const SelectionSet&>(&LayoutTabs::addSelected),
          [this](const SelectionSet& selections) { addSelected(selections); });

  connect(
      viewers_, &LayoutTabs::addRuler, [this](int x0, int y0, int x1, int y1) {
        addRuler(x0, y0, x1, y1, "", "", default_ruler_style_->isChecked());
      });

  connect(viewers_,
          &LayoutTabs::focusNetsChanged,
          inspector_,
          &Inspector::loadActions);
  connect(viewers_,
          &LayoutTabs::routeGuidesChanged,
          inspector_,
          &Inspector::loadActions);
  connect(viewers_,
          &LayoutTabs::netTracksChanged,
          inspector_,
          &Inspector::loadActions);

  connect(
      this, &MainWindow::selectionChanged, viewers_, &LayoutTabs::fullRepaint);
  connect(
      this, &MainWindow::highlightChanged, viewers_, &LayoutTabs::fullRepaint);
  connect(this, &MainWindow::rulersChanged, viewers_, &LayoutTabs::fullRepaint);
  connect(this, &MainWindow::labelsChanged, viewers_, &LayoutTabs::fullRepaint);

  connect(controls_,
          &DisplayControls::selected,
          [this](const Selected& selected) { setSelected(selected); });

  connect(inspector_,
          &Inspector::selected,
          this,
          qOverload<const Selected&, bool>(&MainWindow::setSelected));
  connect(inspector_,
          &Inspector::addSelected,
          [this](const Selected& selection) { addSelected(selection); });
  connect(inspector_,
          &Inspector::removeSelected,
          this,
          &MainWindow::removeSelected);
  connect(this, &MainWindow::selectionChanged, inspector_, &Inspector::update);
  connect(inspector_,
          &Inspector::selectedItemChanged,
          selection_browser_,
          &SelectHighlightWindow::updateModels);
  connect(inspector_,
          &Inspector::selectedItemChanged,
          viewers_,
          &LayoutTabs::fullRepaint);
  connect(inspector_,
          &Inspector::selectedItemChanged,
          this,
          &MainWindow::updateSelectedStatus);
  connect(inspector_, &Inspector::selection, viewers_, &LayoutTabs::selection);
  connect(inspector_, &Inspector::focus, viewers_, &LayoutTabs::selectionFocus);
  connect(
      drc_viewer_, &DRCWidget::focus, viewers_, &LayoutTabs::selectionFocus);
  connect(
      this, &MainWindow::highlightChanged, inspector_, &Inspector::loadActions);
  connect(viewers_,
          &LayoutTabs::focusNetsChanged,
          inspector_,
          &Inspector::loadActions);
  connect(inspector_,
          &Inspector::removeHighlight,
          [this](const QList<const Selected*>& selected) {
            removeFromHighlighted(selected);
          });
  connect(inspector_,
          &Inspector::addHighlight,
          [this](const SelectionSet& selected) { addHighlighted(selected); });
  connect(
      inspector_, &Inspector::setCommand, script_, &ScriptWidget::setCommand);
  connect(script_,
          &ScriptWidget::commandAboutToExecute,
          inspector_,
          &Inspector::setReadOnly);
  connect(script_,
          &ScriptWidget::commandExecuted,
          inspector_,
          &Inspector::unsetReadOnly);

  connect(hierarchy_widget_,
          &BrowserWidget::select,
          [this](const SelectionSet& selected) { setSelected(selected); });
  connect(hierarchy_widget_,
          &BrowserWidget::removeSelect,
          this,
          &MainWindow::removeSelected);
  connect(hierarchy_widget_,
          &BrowserWidget::highlight,
          [this](const SelectionSet& selected) { addHighlighted(selected); });
  connect(hierarchy_widget_,
          &BrowserWidget::removeHighlight,
          this,
          &MainWindow::removeHighlighted);
  connect(hierarchy_widget_,
          &BrowserWidget::updateModuleVisibility,
          viewers_,
          &LayoutTabs::updateModuleVisibility);
  connect(hierarchy_widget_,
          &BrowserWidget::updateModuleColor,
          viewers_,
          &LayoutTabs::updateModuleColor);

  connect(
      timing_widget_, &TimingWidget::inspect, [this](const Selected& selected) {
        inspector_->inspect(selected);
        inspector_->raise();
      });
  connect(selection_browser_,
          &SelectHighlightWindow::selected,
          inspector_,
          &Inspector::inspect);
  connect(this,
          &MainWindow::selectionChanged,
          selection_browser_,
          &SelectHighlightWindow::updateSelectionModel);
  connect(this,
          &MainWindow::highlightChanged,
          selection_browser_,
          &SelectHighlightWindow::updateHighlightModel);
  connect(clock_viewer_,
          &ClockWidget::selected,
          [this](const Selected& selection) { addSelected(selection); });
  connect(this,
          qOverload<const Selected&>(&MainWindow::findInCts),
          clock_viewer_,
          qOverload<const Selected&>(&ClockWidget::findInCts));
  connect(this,
          qOverload<const SelectionSet&>(&MainWindow::findInCts),
          clock_viewer_,
          qOverload<const SelectionSet&>(&ClockWidget::findInCts));

  connect(selection_browser_,
          &SelectHighlightWindow::clearAllSelections,
          [this] { this->setSelected(Selected(), false); });
  connect(selection_browser_,
          &SelectHighlightWindow::clearAllHighlights,
          [this] { this->clearHighlighted(); });
  connect(selection_browser_,
          &SelectHighlightWindow::clearSelectedItems,
          this,
          &MainWindow::removeFromSelected);

  connect(selection_browser_,
          &SelectHighlightWindow::zoomInToItems,
          this,
          &MainWindow::zoomInToItems);

  connect(selection_browser_,
          &SelectHighlightWindow::clearHighlightedItems,
          [this](const QList<const Selected*>& selected) {
            removeFromHighlighted(selected);
          });

  connect(selection_browser_,
          &SelectHighlightWindow::highlightSelectedItemsSig,
          [this](const QList<const Selected*>& items) {
            updateHighlightedSet(items);
          });

  connect(timing_widget_,
          &TimingWidget::highlightTimingPath,
          viewers_,
          qOverload<>(&LayoutTabs::update));

  connect(timing_widget_,
          &TimingWidget::setCommand,
          script_,
          &ScriptWidget::setCommand);
  connect(charts_widget_,
          &ChartsWidget::endPointsToReport,
          this,
          &MainWindow::reportSlackHistogramPaths);

  connect(this, &MainWindow::blockLoaded, this, &MainWindow::setBlock);
  connect(this, &MainWindow::chipLoaded, drc_viewer_, &DRCWidget::setChip);
  connect(
      this, &MainWindow::blockLoaded, clock_viewer_, &ClockWidget::setBlock);
  connect(drc_viewer_,
          &DRCWidget::selectDRC,
          [this](const Selected& selected, const bool open_inspector) {
            if (open_inspector) {
              setSelected(selected, false);
            }
            odb::Rect bbox;
            selected.getBBox(bbox);

            int zoomout_dist = std::numeric_limits<int>::max();
            if (db_ != nullptr) {
              // 10 microns
              zoomout_dist = 10 * db_->getDbuPerMicron();
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
  connect(viewers_,
          &LayoutTabs::viewUpdated,
          goto_dialog_,
          &GotoLocationDialog::updateLocation);
  connect(selection_timer_.get(), &QTimer::timeout, [this]() {
    emit selectionChanged();
  });
  connect(highlight_timer_.get(),
          &QTimer::timeout,
          this,
          &MainWindow::highlightChanged);

  createActions();
  createToolbars();
  createMenus();
  createStatusBar();

  if (load_settings) {
    // Restore the settings (if none this is a no-op)
    QSettings settings("OpenRoad Project", "openroad");
    settings.beginGroup("main");
    // Save these for the showEvent as the window manager may not respect them
    // if restore here.
    saved_geometry_ = settings.value("geometry").toByteArray();
    saved_state_ = settings.value("state").toByteArray();
    QApplication::setFont(
        settings.value("font", QApplication::font()).value<QFont>());
    hide_option_->setChecked(
        settings.value("check_exit", hide_option_->isChecked()).toBool());
    show_dbu_->setChecked(
        settings.value("use_dbu", show_dbu_->isChecked()).toBool());
    default_ruler_style_->setChecked(
        settings.value("ruler_style", default_ruler_style_->isChecked())
            .toBool());
    default_mouse_wheel_zoom_->setChecked(
        settings
            .value("mouse_wheel_zoom", default_mouse_wheel_zoom_->isChecked())
            .toBool());
    arrow_keys_scroll_step_
        = settings.value("arrow_keys_scroll_step", arrow_keys_scroll_step_)
              .toInt();
    script_->readSettings(&settings);
    controls_->readSettings(&settings);
    timing_widget_->readSettings(&settings);
    hierarchy_widget_->readSettings(&settings);
    settings.endGroup();
  }

  // load resources and set window icon
  loadQTResources();
  setWindowIcon(QIcon(":/icon.png"));

  Descriptor::Property::convert_dbu
      = [this](int value, bool add_units) -> std::string {
    return convertDBUToString(value, add_units);
  };
  Descriptor::Property::convert_string
      = [this](const std::string& value, bool* ok) -> int {
    return convertStringToDBU(value, ok);
  };

  selection_timer_->setSingleShot(true);
  highlight_timer_->setSingleShot(true);

  selection_timer_->setInterval(100 /* ms */);
  highlight_timer_->setInterval(100 /* ms */);
}

void MainWindow::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);

  if (!first_show_) {
    return;
  }

  if (saved_geometry_.has_value() && saved_state_.has_value()) {
    restoreGeometry(saved_geometry_.value());
    restoreState(saved_state_.value());
  } else {
    // Default size and position the window
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QSize size = screen()->availableGeometry().size();
#else
    QSize size = QDesktopWidget().availableGeometry(this).size();
#endif

    resize(size * 0.8);
    move(size.width() * 0.1, size.height() * 0.1);
  }
  first_show_ = false;
}

MainWindow::~MainWindow()
{
  auto* gui = Gui::get();
  // unregister descriptors with GUI dependencies
  gui->unregisterDescriptor<Ruler*>();
  gui->unregisterDescriptor<Label*>();
  gui->unregisterDescriptor<odb::dbNet*>();
  gui->unregisterDescriptor<DbNetDescriptor::NetWithSink>();
  gui->unregisterDescriptor<BufferTree>();
  gui->unregisterDescriptor<odb::dbITerm*>();
  gui->unregisterDescriptor<odb::dbMTerm*>();

  if (cli_progress_ != nullptr) {
    logger_->swapProgress(cli_progress_.release());
  }
}

void MainWindow::setDatabase(odb::dbDatabase* db)
{
  db_ = db;
}

void MainWindow::setTitle(const std::string& title)
{
  window_title_ = title;
  updateTitle();
}

void MainWindow::updateTitle()
{
  if (!window_title_.empty()) {
    odb::dbBlock* block = getBlock();
    if (block != nullptr) {
      const std::string title
          = fmt::format("{} - {}", window_title_, block->getName());
      setWindowTitle(QString::fromStdString(title));
    } else {
      setWindowTitle(QString::fromStdString(window_title_));
    }
  }
}

const SelectionSet& MainWindow::selection()
{
  return selected_;
}

void MainWindow::setBlock(odb::dbBlock* block)
{
  updateTitle();
  if (block != nullptr) {
    save_->setEnabled(true);
  }
  for (auto* heat_map : Gui::get()->getHeatMaps()) {
    heat_map->setBlock(block);
  }
  hierarchy_widget_->setBlock(block);
}

void MainWindow::init(sta::dbSta* sta, const std::string& help_path)
{
  // Setup widgets
  timing_widget_->init(sta);
  controls_->setSTA(sta);
  hierarchy_widget_->setSTA(sta);
  clock_viewer_->setSTA(sta);
  charts_widget_->setSTA(sta);
  help_widget_->init(help_path);

  // register descriptors
  auto* gui = Gui::get();
  auto* inst_descriptor = new DbInstDescriptor(db_, sta);
  gui->registerDescriptor<odb::dbInst*>(inst_descriptor);
  gui->registerDescriptor<odb::dbMaster*>(new DbMasterDescriptor(db_, sta));
  gui->registerDescriptor<odb::dbNet*>(
      new DbNetDescriptor(db_,
                          sta,
                          viewers_->getFocusNets(),
                          viewers_->getRouteGuides(),
                          viewers_->getNetTracks()));
  gui->registerDescriptor<DbNetDescriptor::NetWithSink>(
      new DbNetDescriptor(db_,
                          sta,
                          viewers_->getFocusNets(),
                          viewers_->getRouteGuides(),
                          viewers_->getNetTracks()));
  gui->registerDescriptor<odb::dbWire*>(new DbWireDescriptor(db_));
  gui->registerDescriptor<odb::dbSWire*>(new DbSWireDescriptor(db_));
  gui->registerDescriptor<odb::dbITerm*>(new DbITermDescriptor(
      db_, [this]() -> bool { return show_poly_decomp_view_->isChecked(); }));
  gui->registerDescriptor<odb::dbMTerm*>(new DbMTermDescriptor(
      db_, [this]() -> bool { return show_poly_decomp_view_->isChecked(); }));
  gui->registerDescriptor<odb::dbBTerm*>(new DbBTermDescriptor(db_));
  gui->registerDescriptor<odb::dbBPin*>(new DbBPinDescriptor(db_));
  gui->registerDescriptor<odb::dbVia*>(new DbViaDescriptor(db_));
  gui->registerDescriptor<odb::dbBlockage*>(new DbBlockageDescriptor(db_));
  gui->registerDescriptor<odb::dbObstruction*>(
      new DbObstructionDescriptor(db_));
  gui->registerDescriptor<odb::dbTechLayer*>(new DbTechLayerDescriptor(db_));
  gui->registerDescriptor<DbTermAccessPoint>(
      new DbTermAccessPointDescriptor(db_));
  gui->registerDescriptor<odb::dbGroup*>(new DbGroupDescriptor(db_));
  gui->registerDescriptor<odb::dbRegion*>(new DbRegionDescriptor(db_));
  gui->registerDescriptor<odb::dbModule*>(new DbModuleDescriptor(db_));
  gui->registerDescriptor<odb::dbModBTerm*>(new DbModBTermDescriptor(db_));
  gui->registerDescriptor<odb::dbModITerm*>(new DbModITermDescriptor(db_));
  gui->registerDescriptor<odb::dbModInst*>(new DbModInstDescriptor(db_));
  gui->registerDescriptor<odb::dbModNet*>(new DbModNetDescriptor(db_));
  gui->registerDescriptor<odb::dbTechVia*>(new DbTechViaDescriptor(db_));
  gui->registerDescriptor<odb::dbTechViaRule*>(
      new DbTechViaRuleDescriptor(db_));
  gui->registerDescriptor<odb::dbTechViaLayerRule*>(
      new DbTechViaLayerRuleDescriptor(db_));
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
  gui->registerDescriptor<Label*>(new LabelDescriptor(labels_, db_, logger_));
  gui->registerDescriptor<odb::dbBlock*>(new DbBlockDescriptor(db_));
  gui->registerDescriptor<odb::dbTech*>(new DbTechDescriptor(db_));
  gui->registerDescriptor<odb::dbMetalWidthViaMap*>(
      new DbMetalWidthViaMapDescriptor(db_));
  gui->registerDescriptor<odb::dbMarkerCategory*>(
      new DbMarkerCategoryDescriptor(db_));
  gui->registerDescriptor<odb::dbMarker*>(new DbMarkerDescriptor(db_));
  gui->registerDescriptor<odb::dbScanInst*>(new DbScanInstDescriptor(db_));
  gui->registerDescriptor<odb::dbScanList*>(new DbScanListDescriptor(db_));
  gui->registerDescriptor<odb::dbScanPartition*>(
      new DbScanPartitionDescriptor(db_));
  gui->registerDescriptor<odb::dbScanChain*>(new DbScanChainDescriptor(db_));
  gui->registerDescriptor<odb::dbBox*>(new DbBoxDescriptor(db_));
  gui->registerDescriptor<odb::dbSBox*>(new DbSBoxDescriptor(db_));
  gui->registerDescriptor<DbBoxDescriptor::BoxWithTransform>(
      new DbBoxDescriptor(db_));
  gui->registerDescriptor<odb::dbMasterEdgeType*>(
      new DbMasterEdgeTypeDescriptor(db_));
  gui->registerDescriptor<odb::dbCellEdgeSpacing*>(
      new DbCellEdgeSpacingDescriptor(db_));

  gui->registerDescriptor<sta::Scene*>(new SceneDescriptor(sta));
  gui->registerDescriptor<sta::LibertyLibrary*>(
      new LibertyLibraryDescriptor(sta));
  gui->registerDescriptor<sta::LibertyCell*>(new LibertyCellDescriptor(sta));
  gui->registerDescriptor<sta::LibertyPort*>(new LibertyPortDescriptor(sta));
  gui->registerDescriptor<sta::Instance*>(new StaInstanceDescriptor(sta));
  gui->registerDescriptor<sta::Clock*>(new ClockDescriptor(sta));

  gui->registerDescriptor<BufferTree>(
      new BufferTreeDescriptor(db_,
                               sta,
                               viewers_->getFocusNets(),
                               viewers_->getRouteGuides(),
                               viewers_->getNetTracks()));

  controls_->setDBInstDescriptor(inst_descriptor);
  hierarchy_widget_->setDBInstDescriptor(inst_descriptor);
}

void MainWindow::createStatusBar()
{
  location_ = new QLabel(this);
  statusBar()->addPermanentWidget(location_);
}

LayoutViewer* MainWindow::getLayoutViewer() const
{
  return viewers_->getCurrent();
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
  save_ = new QAction("Save DB", this);
  save_->setEnabled(false);

  fit_ = new QAction("Fit", this);
  fit_->setShortcut(QString("F"));

  find_ = new QAction("Find", this);
  find_->setShortcut(QString("Ctrl+F"));

  goto_position_ = new QAction("Go to position", this);
  goto_position_->setShortcut(QString("Shift+G"));

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

  enable_developer_mode_ = new QShortcut(QKeySequence("Ctrl+="), this);

  show_poly_decomp_view_ = new QAction("Show polygon decomposition", this);
  show_poly_decomp_view_->setCheckable(true);
  show_poly_decomp_view_->setChecked(false);
  show_poly_decomp_view_->setVisible(false);

  default_ruler_style_ = new QAction("Make euclidian rulers", this);
  default_ruler_style_->setCheckable(true);
  default_ruler_style_->setChecked(true);

  default_mouse_wheel_zoom_
      = new QAction("Mouse wheel mapped to zoom by default", this);
  default_mouse_wheel_zoom_->setCheckable(true);
  default_mouse_wheel_zoom_->setChecked(false);

  arrow_keys_scroll_step_dialog_ = new QAction("Arrow keys scroll step", this);
  arrow_keys_scroll_step_ = 20;

  font_ = new QAction("Application font", this);

  global_connect_ = new QAction("Global connect", this);
  global_connect_->setShortcut(QString("Ctrl+G"));

  connect(open_, &QAction::triggered, this, &MainWindow::openDesign);
  connect(save_, &QAction::triggered, this, &MainWindow::saveDesign);
  connect(
      this, &MainWindow::blockLoaded, [this]() { open_->setEnabled(false); });
  connect(hide_, &QAction::triggered, this, &MainWindow::hide);
  connect(exit_, &QAction::triggered, this, &MainWindow::exit);
  connect(this, &MainWindow::exit, viewers_, &LayoutTabs::exit);
  connect(fit_, &QAction::triggered, viewers_, &LayoutTabs::fit);
  connect(zoom_in_,
          &QAction::triggered,
          viewers_,
          qOverload<>(&LayoutTabs::zoomIn));
  connect(zoom_out_,
          &QAction::triggered,
          viewers_,
          qOverload<>(&LayoutTabs::zoomOut));
  connect(find_, &QAction::triggered, this, &MainWindow::showFindDialog);
  connect(
      goto_position_, &QAction::triggered, this, &MainWindow::showGotoDialog);
  connect(inspect_, &QAction::triggered, inspector_, &Inspector::show);
  connect(
      timing_debug_, &QAction::triggered, timing_widget_, &TimingWidget::show);
  connect(help_, &QAction::triggered, this, &MainWindow::showHelp);

  connect(build_ruler_,
          &QAction::triggered,
          viewers_,
          &LayoutTabs::startRulerBuild);

  connect(show_dbu_, &QAction::toggled, viewers_, &LayoutTabs::fullRepaint);
  connect(show_dbu_, &QAction::toggled, inspector_, &Inspector::reload);
  connect(show_dbu_,
          &QAction::toggled,
          selection_browser_,
          &SelectHighlightWindow::updateModels);
  connect(show_dbu_, &QAction::toggled, this, &MainWindow::setUseDBU);
  connect(show_dbu_, &QAction::toggled, this, &MainWindow::setClearLocation);

  connect(enable_developer_mode_,
          &QShortcut::activated,
          this,
          &MainWindow::enableDeveloper);

  connect(show_poly_decomp_view_,
          &QAction::toggled,
          viewers_,
          &LayoutTabs::resetCache);

  connect(arrow_keys_scroll_step_dialog_,
          &QAction::triggered,
          this,
          &MainWindow::showArrowKeysScrollStep);

  connect(font_, &QAction::triggered, this, &MainWindow::showApplicationFont);

  connect(global_connect_,
          &QAction::triggered,
          this,
          &MainWindow::showGlobalConnect);
}

void MainWindow::setUseDBU(bool use_dbu)
{
  for (auto* heat_map : Gui::get()->getHeatMaps()) {
    heat_map->setUseDBU(use_dbu);
  }
  if (db_) {
    emit displayUnitsChanged(db_->getDbuPerMicron(), use_dbu);
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

void MainWindow::showArrowKeysScrollStep()
{
  bool okay = false;
  int arrow_keys_scroll_step
      = QInputDialog::getInt(this,
                             tr("Configure arrow keys"),
                             tr("Arrow keys scrool step value"),
                             arrow_keys_scroll_step_,
                             10,
                             1000,
                             1,
                             &okay);

  if (okay) {
    arrow_keys_scroll_step_ = arrow_keys_scroll_step;
    update();
  }
}

void MainWindow::createMenus()
{
  file_menu_ = menuBar()->addMenu("&File");
  file_menu_->addAction(open_);
  file_menu_->addAction(save_);
  file_menu_->addAction(hide_);
  file_menu_->addAction(exit_);

  view_menu_ = menuBar()->addMenu("&View");
  view_menu_->addAction(fit_);
  view_menu_->addAction(find_);
  view_menu_->addAction(goto_position_);
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
  windows_menu_->addAction(charts_widget_->toggleViewAction());
  windows_menu_->addAction(chiplet_dock_->toggleViewAction());
  windows_menu_->addAction(help_widget_->toggleViewAction());

  auto option_menu = menuBar()->addMenu("&Options");
  option_menu->addAction(hide_option_);
  option_menu->addAction(show_dbu_);
  option_menu->addAction(default_ruler_style_);
  option_menu->addAction(default_mouse_wheel_zoom_);
  option_menu->addAction(arrow_keys_scroll_step_dialog_);
  option_menu->addAction(font_);
  option_menu->addAction(show_poly_decomp_view_);

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

std::string MainWindow::addToolbarButton(const std::string& name,
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
    } while (buttons_.contains(key));
  } else {
    if (buttons_.contains(name)) {
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
  if (!buttons_.contains(name)) {
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

  auto cleanup_text = [](const QString& text) -> QString {
    QString text_cpy = text;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    text_cpy.replace(QRegularExpression("&(?!&)"),
                     "");  // remove single &, but keep &&
#else
    text_cpy.replace(QRegExp("&(?!&)"), "");  // remove single &, but keep &&
#endif
    return text_cpy;
  };

  const QString top_name = path[0];
  const QString compare_name = cleanup_text(top_name);
  path.pop_front();

  QList<QAction*> actions;
  if (parent == nullptr) {
    actions = menuBar()->actions();
  } else {
    actions = parent->actions();
  }

  QMenu* menu = nullptr;
  for (auto* action : actions) {
    if (cleanup_text(action->text()) == compare_name) {
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

std::string MainWindow::addMenuItem(const std::string& name,
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
    } while (menu_actions_.contains(key));
  } else {
    if (menu_actions_.contains(name)) {
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
  if (!menu_actions_.contains(name)) {
    return;
  }

  auto* action = menu_actions_[name].get();
  QMenu* menu = qobject_cast<QMenu*>(action->parent());
  menu->removeAction(action);

  removeMenu(menu);

  menu_actions_.erase(name);
}

std::string MainWindow::requestUserInput(const QString& title,
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

void MainWindow::addSelected(const Selected& selection, bool find_in_cts)
{
  if (selection) {
    selected_.emplace(selection);
    emit selectionChanged(selection);
    if (find_in_cts) {
      emit findInCts(selection);
    }
  }
  emit updateSelectedStatus(selection);
}

void MainWindow::removeSelected(const Selected& selection)
{
  auto itr = std::ranges::find(selected_, selection);
  if (itr != selected_.end()) {
    selected_.erase(itr);
    emit selectionChanged();
  }
}

void MainWindow::removeHighlighted(const Selected& selection)
{
  for (auto& group : highlighted_) {
    auto itr = std::ranges::find(group, selection);
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

void MainWindow::addSelected(const SelectionSet& selections, bool find_in_cts)
{
  int prev_selected_size = selected_.size();
  for (const auto& selection : selections) {
    if (selection) {
      selected_.insert(selection);
    }
  }
  status(std::string("Added ")
         + std::to_string(selected_.size() - prev_selected_size));
  selection_timer_->start();

  if (find_in_cts) {
    emit findInCts(selections);
  }
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
  if (show_connectivity) {
    selectHighlightConnectedNets(true, true, true, false);
  }

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
  highlight_timer_->start();
}

std::string MainWindow::addLabel(int x,
                                 int y,
                                 const std::string& text,
                                 std::optional<Painter::Color> color,
                                 std::optional<int> size,
                                 std::optional<Painter::Anchor> anchor,
                                 std::optional<std::string> name)
{
  auto new_label
      = std::make_unique<Label>(odb::Point(x, y),
                                text,
                                anchor.value_or(Painter::Anchor::kCenter),
                                color.value_or(gui::Painter::kWhite),
                                size,
                                std::move(name));
  std::string new_name = new_label->getName();

  // check if ruler name is unique
  for (const auto& label : labels_) {
    if (new_name == label->getName()) {
      logger_->warn(
          utl::GUI, 44, "Label with name \"{}\" already exists", new_name);
      return "";
    }
  }

  labels_.push_back(std::move(new_label));
  emit labelsChanged();
  return new_name;
}

void MainWindow::deleteLabel(const std::string& name)
{
  auto label_find = std::ranges::find_if(
      labels_, [name](const auto& l) { return l->getName() == name; });
  if (label_find != labels_.end()) {
    // remove from selected set
    auto remove_selected = Gui::get()->makeSelected(label_find->get());
    if (selected_.find(remove_selected) != selected_.end()) {
      selected_.erase(remove_selected);
      emit selectionChanged();
    }
    labels_.erase(label_find);
    emit labelsChanged();
  }
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
  auto ruler_find = std::ranges::find_if(
      rulers_, [name](const auto& l) { return l->getName() == name; });
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
  if (highlighted_.empty()) {
    return;
  }
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
  if (num_items_cleared > 0) {
    emit highlightChanged();
  }
}

void MainWindow::clearLabels()
{
  if (labels_.empty()) {
    return;
  }
  Gui::get()->removeSelected<Label*>();
  labels_.clear();
  emit labelsChanged();
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
  if (items.empty()) {
    return;
  }
  for (auto& item : items) {
    selected_.erase(*item);
  }
  emit selectionChanged();
}

void MainWindow::removeFromHighlighted(const QList<const Selected*>& items,
                                       int highlight_group)
{
  if (items.empty()) {
    return;
  }
  if (highlight_group < 0) {
    for (auto& item : items) {
      for (auto& highlighted_set : highlighted_) {
        highlighted_set.erase(*item);
      }
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
  viewers_->zoomTo(rect_dbu);
}

void MainWindow::zoomTo(const odb::Point& focus, int diameter)
{
  viewers_->zoomTo(focus, diameter);
}

void MainWindow::zoomInToItems(const QList<const Selected*>& items)
{
  if (items.empty()) {
    return;
  }
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
  if (merge_cnt == 0) {
    return;
  }
  zoomTo(items_bbox);
}

void MainWindow::status(const std::string& message)
{
  statusBar()->showMessage(QString::fromStdString(message));
}

void MainWindow::showFindDialog()
{
  if (getBlock() == nullptr) {
    return;
  }
  find_dialog_->exec();
}

void MainWindow::showGotoDialog()
{
  if (getBlock() == nullptr) {
    return;
  }

  goto_dialog_->showInit();
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
          || (selected_obj.isNet() && obj_type == odb::dbNetObj)) {
        return true;
      }
    }
    return false;
  }
  for (auto& highlight_set : highlighted_) {
    for (auto& selected_obj : highlight_set) {
      if (selected_obj.isInst() && obj_type == odb::dbInstObj) {
        return true;
      }
      if (selected_obj.isNet() && obj_type == odb::dbNetObj) {
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
  if (connected_insts.empty()) {
    return;
  }
  if (select_flag) {
    addSelected(connected_insts);
  } else {
    addHighlighted(connected_insts, highlight_group);
  }
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
            || inst_term->getNet()->getSigType() != odb::dbSigType::SIGNAL) {
          continue;
        }
        auto inst_term_dir = inst_term->getIoType();

        if (output
            && (inst_term_dir == odb::dbIoType::OUTPUT
                || inst_term_dir == odb::dbIoType::INOUT)) {
          connected_nets.insert(Gui::get()->makeSelected(inst_term->getNet()));
        }
        if (input
            && (inst_term_dir == odb::dbIoType::INPUT
                || inst_term_dir == odb::dbIoType::INOUT)) {
          connected_nets.insert(Gui::get()->makeSelected(
              DbNetDescriptor::NetWithSink{inst_term->getNet(), inst_term}));
        }
      }
    }
  }
  if (connected_nets.empty()) {
    return;
  }
  if (select_flag) {
    addSelected(connected_nets);
  } else {
    addHighlighted(connected_nets, highlight_group);
  }
}

void MainWindow::selectHighlightConnectedBufferTrees(bool select_flag,
                                                     int highlight_group)
{
  SelectionSet connected_objects;
  for (auto& sel_obj : selected_) {
    if (sel_obj.isInst()) {
      auto inst_obj = std::any_cast<odb::dbInst*>(sel_obj.getObject());
      for (auto inst_term : inst_obj->getITerms()) {
        auto inst_term_dir = inst_term->getIoType();
        if (!inst_term->getSigType().isSupply()
            && (inst_term_dir == odb::dbIoType::INPUT
                || inst_term_dir == odb::dbIoType::OUTPUT
                || inst_term_dir == odb::dbIoType::INOUT)) {
          auto net_obj = inst_term->getNet();
          if (net_obj == nullptr
              || net_obj->getSigType() != odb::dbSigType::SIGNAL) {
            continue;
          }
          connected_objects.insert(
              Gui::get()->makeSelected(gui::BufferTree(net_obj)));
        }
      }
    }
  }

  if (connected_objects.empty()) {
    return;
  }
  if (select_flag) {
    addSelected(connected_objects);
  } else {
    addHighlighted(connected_objects, highlight_group);
  }
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
  settings.setValue("mouse_wheel_zoom", default_mouse_wheel_zoom_->isChecked());
  settings.setValue("arrow_keys_scroll_step", arrow_keys_scroll_step_);
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
  emit chipLoaded(block->getChip());
  emit blockLoaded(block);
}

void MainWindow::postRead3Dbx(odb::dbChip* chip)
{
  emit chipLoaded(chip);
}

void MainWindow::postReadDb(odb::dbDatabase* db)
{
  auto chip = db->getChip();
  if (chip == nullptr) {
    return;
  }
  auto block = chip->getBlock();
  if (block == nullptr) {
    // It is a top chip
    emit chipLoaded(chip);
    return;
  }

  // Only create a tab for the top block
  emit chipLoaded(block->getChip());
  emit blockLoaded(block);
}

void MainWindow::setLogger(utl::Logger* logger)
{
  logger_ = logger;

  auto progress = std::make_unique<GUIProgress>(logger_, this);
  cli_progress_ = logger_->swapProgress(progress.release());

  controls_->setLogger(logger);
  script_->setLogger(logger);
  viewers_->setLogger(logger);
  drc_viewer_->setLogger(logger);
  clock_viewer_->setLogger(logger);
  charts_widget_->setLogger(logger);
  timing_widget_->setLogger(logger);
  chiplet_viewer_->setLogger(logger);
}

void MainWindow::fit()
{
  fit_->trigger();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape) {
    // Esc stop building ruler
    viewers_->cancelRulerBuild();
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
    // close all dialogs
    for (QWidget* w : QApplication::topLevelWidgets()) {
      if (w != this) {
        w->close();
      }
    }
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

std::vector<std::string> MainWindow::getRestoreTclCommands()
{
  std::vector<std::string> cmds;
  // Save rulers
  if (db_) {
    for (const auto& ruler : rulers_) {
      cmds.push_back(ruler->getTclCommand(db_->getDbuPerMicron()));
    }
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
  viewers_->restoreTclCommands(cmds);

  return cmds;
}

std::string MainWindow::convertDBUToString(int value, bool add_units) const
{
  if (show_dbu_->isChecked()) {
    return std::to_string(value);
  }
  if (db_ == nullptr) {
    return std::to_string(value);
  }
  const double dbu_per_micron = db_->getDbuPerMicron();

  const int precision = std::ceil(std::log10(dbu_per_micron));
  const double micron_value = value / dbu_per_micron;

  auto str = utl::to_numeric_string(micron_value, precision);

  if (add_units) {
    str += " μm";
  }

  return str;
}

int MainWindow::convertStringToDBU(const std::string& value, bool* ok) const
{
  QString new_value = QString::fromStdString(value).simplified();

  if (new_value.contains(" ")) {
    new_value = new_value.left(new_value.indexOf(" "));
  } else if (new_value.contains("u")) {
    new_value = new_value.left(new_value.indexOf("u"));
  } else if (new_value.contains("μ")) {
    new_value = new_value.left(new_value.indexOf("μ"));
  }

  if (show_dbu_->isChecked()) {
    return new_value.toInt(ok);
  }
  if (db_ == nullptr) {
    return new_value.toInt(ok);
  }
  const int dbu_per_micron = db_->getDbuPerMicron();

  return new_value.toDouble(ok) * dbu_per_micron;
}

void MainWindow::timingCone(Gui::Term term, bool fanin, bool fanout)
{
  auto* renderer = timing_widget_->getConeRenderer();

  if (std::holds_alternative<odb::dbITerm*>(term)) {
    renderer->setITerm(std::get<odb::dbITerm*>(term), fanin, fanout);
  } else {
    renderer->setBTerm(std::get<odb::dbBTerm*>(term), fanin, fanout);
  }
}

void MainWindow::timingPathsThrough(const std::set<Gui::Term>& terms)
{
  auto* settings = timing_widget_->getSettings();
  settings->setFromPin({});
  std::set<const sta::Pin*> pins;
  for (const auto& term : terms) {
    pins.insert(settings->convertTerm(term));
  }
  settings->setThruPin({std::move(pins)});
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
  const std::vector<QString> exts{".odb", ".odb.gz", ".db", ".db.gz"};

  QString filefilter = "OpenDB (";
  for (const auto& ext : exts) {
    if (filefilter.length() != 8) {
      filefilter += " ";
    }
    filefilter += "*" + ext;
    filefilter += " *" + ext.toUpper();
  }
  filefilter += ")";
  const QString file = QFileDialog::getOpenFileName(
      this, "Open Design", QString(), filefilter);

  if (file.isEmpty()) {
    return;
  }

  try {
    bool found = false;
    for (const auto& ext : exts) {
      if (file.endsWith(ext, Qt::CaseInsensitive)) {
        open_->setEnabled(false);
        ord::OpenRoad::openRoad()->readDb(file.toStdString().c_str());
        logger_->warn(utl::GUI,
                      109,
                      "Timing data is not stored in {} and must be loaded "
                      "separately, if needed.",
                      file.toStdString());
        found = true;
        break;
      }
    }
    if (!found) {
      logger_->error(utl::GUI, 108, "Unknown filetype: {}", file.toStdString());
    }
  } catch (const std::exception&) {
    // restore option
    open_->setEnabled(true);
  }
}

void MainWindow::saveDesign()
{
  const std::vector<QString> exts{".odb", ".odb.gz"};

  QString filefilter = "OpenDB (";
  for (const auto& ext : exts) {
    if (filefilter.length() != 8) {
      filefilter += " ";
    }
    filefilter += "*" + ext;
    filefilter += " *" + ext.toUpper();
  }
  filefilter += ")";
  const QString file = QFileDialog::getSaveFileName(
      this, "Save Design", QString(), filefilter);

  if (file.isEmpty()) {
    return;
  }

  try {
    ord::OpenRoad::openRoad()->writeDb(file.toStdString().c_str());
  } catch (const std::exception& e) {
    QMessageBox::warning(
        this, "Save Error", QString("Db save failed: %1").arg(e.what()));
  }
}

void MainWindow::enableDeveloper()
{
  show_poly_decomp_view_->setVisible(true);
}

void MainWindow::reportSlackHistogramPaths(
    const std::set<const sta::Pin*>& report_pins,
    const std::string& path_group_name)
{
  if (!timing_widget_->isVisible()) {
    timing_widget_->show();
  }

  // In Qt, an enabled tabified widget is visible, so
  // we need to make it the active tab.
  if (timing_widget_->visibleRegion().isEmpty()) {
    timing_widget_->raise();
  }

  timing_widget_->reportSlackHistogramPaths(report_pins, path_group_name);
}
}  // namespace gui
