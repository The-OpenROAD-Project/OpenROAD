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

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>
#include <QWidgetAction>
#include <map>
#include <vector>
#include <QDebug>

#include "dbDescriptors.h"
#include "displayControls.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "scriptWidget.h"
#include "selectHighlightWindow.h"
#include "staGui.h"
#include "utl/Logger.h"
#include "timingWidget.h"
#include "drcWidget.h"

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
      inspector_(new Inspector(selected_, this)),
      script_(new ScriptWidget(this)),
      viewer_(new LayoutViewer(
          controls_,
          script_,
          selected_,
          highlighted_,
          rulers_,
          [](const std::any& object) { return Gui::get()->makeSelected(object); },
          this)),
      selection_browser_(
          new SelectHighlightWindow(selected_, highlighted_, this)),
      scroll_(new LayoutScroll(viewer_, this)),
      timing_widget_(new TimingWidget(this)),
      drc_viewer_(new DRCWidget(this)),
      find_dialog_(new FindObjectDialog(this))
{
  // Size and position the window
  QSize size = QDesktopWidget().availableGeometry(this).size();
  resize(size * 0.8);
  move(size.width() * 0.1, size.height() * 0.1);

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  script_->setFont(font);

  setCentralWidget(scroll_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::BottomDockWidgetArea, selection_browser_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);
  addDockWidget(Qt::RightDockWidgetArea, inspector_);
  addDockWidget(Qt::RightDockWidgetArea, timing_widget_);
  addDockWidget(Qt::RightDockWidgetArea, drc_viewer_);

  tabifyDockWidget(selection_browser_, script_);
  selection_browser_->hide();

  tabifyDockWidget(inspector_, timing_widget_);
  tabifyDockWidget(inspector_, drc_viewer_);
  drc_viewer_->hide();

  // Hook up all the signals/slots
  connect(script_, SIGNAL(tclExiting()), this, SIGNAL(exit()));
  connect(script_, SIGNAL(commandExecuted(int)), viewer_, SLOT(update()));
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
  connect(viewer_,
          SIGNAL(location(qreal, qreal)),
          this,
          SLOT(setLocation(qreal, qreal)));
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

  connect(viewer_,
          SIGNAL(addRuler(int, int, int, int)),
          this,
          SLOT(addRuler(int, int, int, int)));

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
  connect(this, SIGNAL(selectionChanged(const Selected&)), inspector_, SLOT(update(const Selected&)));
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
          SIGNAL(highlightSelectedItemsSig(const QList<const Selected*>&, int)),
          this,
          SLOT(updateHighlightedSet(const QList<const Selected*>&, int)));

  connect(timing_widget_,
          SIGNAL(highlightTimingPath(TimingPath*)),
          viewer_,
          SLOT(update()));

  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          drc_viewer_,
          SLOT(setBlock(odb::dbBlock*)));
  connect(drc_viewer_,
          &DRCWidget::selectDRC,
          [this](const Selected& selected) {
            setSelected(selected, false);
            odb::Rect bbox;
            selected.getBBox(bbox);
            // 10 microns
            const int zoomout_dist = 10 * getBlock()->getDbUnitsPerMicron();
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
  connect(this,
          &MainWindow::selectionChanged,
          [this]() {
            if (!selected_.empty()) {
              drc_viewer_->updateSelection(*selected_.begin());
            }
          });

  connect(this,
          &MainWindow::designLoaded,
          [](odb::dbBlock* block) {
            if (block != nullptr) {
              Descriptor::Property::dbu = block->getDbUnitsPerMicron();
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
  hide_option_->setChecked(settings.value("check_exit", hide_option_->isChecked()).toBool());
  script_->readSettings(&settings);
  controls_->readSettings(&settings);
  timing_widget_->readSettings(&settings);
  settings.endGroup();

  // load resources and set window icon and title
  loadQTResources();
  setWindowIcon(QIcon(":/icon.png"));
  setWindowTitle("OpenROAD");
}

MainWindow::~MainWindow()
{
  // unregister descriptors
  Gui::get()->unregisterDescriptor<Ruler*>();
}

void MainWindow::setDatabase(odb::dbDatabase* db)
{
  // set database and pass along
  db_ = db;
  controls_->setDb(db_);
  viewer_->setDb(db_);
}

void MainWindow::init(sta::dbSta* sta)
{
  // Setup timing widget
  timing_widget_->init(sta);

  // register descriptors
  auto* gui = Gui::get();
  gui->registerDescriptor<odb::dbInst*>(new DbInstDescriptor(db_, sta));
  gui->registerDescriptor<odb::dbMaster*>(new DbMasterDescriptor(db_, sta));
  gui->registerDescriptor<odb::dbNet*>(new DbNetDescriptor(db_));
  gui->registerDescriptor<odb::dbITerm*>(new DbITermDescriptor(db_));
  gui->registerDescriptor<odb::dbBTerm*>(new DbBTermDescriptor(db_));
  gui->registerDescriptor<odb::dbBlockage*>(new DbBlockageDescriptor(db_));
  gui->registerDescriptor<odb::dbObstruction*>(new DbObstructionDescriptor(db_));
  gui->registerDescriptor<odb::dbTechLayer*>(new DbTechLayerDescriptor(db_));
  gui->registerDescriptor<Ruler*>(new RulerDescriptor(rulers_, db_));
}

void MainWindow::createStatusBar()
{
  location_ = new QLabel(this);
  statusBar()->addPermanentWidget(location_);
}

odb::dbBlock* MainWindow::getBlock()
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

  congestion_setup_ = new QAction("Congestion Setup...", this);

  help_ = new QAction("Help", this);
  help_->setShortcut(QString("Ctrl+H"));

  build_ruler_ = new QAction("Ruler", this);
  build_ruler_->setShortcut(QString("k"));

  connect(congestion_setup_,
          SIGNAL(triggered()),
          controls_,
          SLOT(showCongestionSetup()));

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
}

void MainWindow::createMenus()
{
  file_menu_ = menuBar()->addMenu("&File");
  file_menu_->addAction(hide_);
  file_menu_->addAction(exit_);

  view_menu_ = menuBar()->addMenu("&View");
  view_menu_->addAction(fit_);
  view_menu_->addAction(find_);
  view_menu_->addAction(zoom_in_);
  view_menu_->addAction(zoom_out_);

  tools_menu_ = menuBar()->addMenu("&Tools");
  tools_menu_->addAction(congestion_setup_);
  tools_menu_->addAction(build_ruler_);

  windows_menu_ = menuBar()->addMenu("&Windows");
  windows_menu_->addAction(controls_->toggleViewAction());
  windows_menu_->addAction(inspector_->toggleViewAction());
  windows_menu_->addAction(script_->toggleViewAction());
  windows_menu_->addAction(selection_browser_->toggleViewAction());
  windows_menu_->addAction(view_tool_bar_->toggleViewAction());
  windows_menu_->addAction(timing_widget_->toggleViewAction());
  windows_menu_->addAction(drc_viewer_->toggleViewAction());

  auto option_menu = menuBar()->addMenu("&Options");
  option_menu->addAction(hide_option_);

  menuBar()->addAction(help_);
}

void MainWindow::createToolbars()
{
  view_tool_bar_ = addToolBar("Toolbar");
  view_tool_bar_->addAction(fit_);
  view_tool_bar_->addAction(find_);
  view_tool_bar_->addAction(congestion_setup_);
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

const std::string MainWindow::requestUserInput(const QString& title, const QString& question)
{
  QString text = QInputDialog::getText(this,
                                       title,
                                       question);
  return text.toStdString();
}

void MainWindow::setLocation(qreal x, qreal y)
{
  location_->setText(QString("%1, %2").arg(x, 0, 'f', 5).arg(y, 0, 'f', 5));
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

void MainWindow::removeSelectedByType(const std::string& type)
{
  bool changed = false;
  for (auto itr = selected_.begin(); itr != selected_.end(); ) {
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
  status(std::string("Added ") + std::to_string(selected_.size() - prev_selected_size));
  emit selectionChanged();
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
  if (highlight_group >= 7) {
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

std::string MainWindow::addRuler(int x0, int y0, int x1, int y1, const std::string& label, const std::string& name)
{
  auto new_ruler = std::make_unique<Ruler>(odb::Point(x0, y0), odb::Point(x1, y1), name, label);
  std::string new_name = new_ruler->getName();

  // check if ruler name is unique
  for (const auto& ruler : rulers_) {
    if (new_name == ruler->getName()) {
      logger_->warn(utl::GUI, 24, "Ruler with name \"{}\" already exists", new_name);
      return "";
    }
  }

  rulers_.push_back(std::move(new_ruler));
  emit rulersChanged();
  return new_name;
}

void MainWindow::deleteRuler(const std::string& name)
{
  auto ruler_find = std::find_if(rulers_.begin(), rulers_.end(), [name](const auto& l) {
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

void MainWindow::updateHighlightedSet(const QList<const Selected*>& items,
                                      int highlight_group)
{
  if (highlight_group >= 7)
    return;
  for (auto item : items) {
    highlighted_[highlight_group].insert(*item);
  }
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
  } else if (highlight_group < 7) {
    num_items_cleared += highlighted_[highlight_group].size();
    highlighted_[highlight_group].clear();
  }
  if (num_items_cleared > 0)
    emit highlightChanged();
}

void MainWindow::clearRulers()
{
  if (rulers_.empty())
    return;
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
  } else if (highlight_group < 7) {
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
  for (auto sel_obj : selected_) {
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
          connected_nets.insert(Gui::get()->makeSelected(inst_term->getNet(), inst_term));
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
  settings.setValue("check_exit", hide_option_->isChecked());
  script_->writeSettings(&settings);
  controls_->writeSettings(&settings);
  timing_widget_->writeSettings(&settings);
  settings.endGroup();
}

void MainWindow::postReadLef(odb::dbTech* tech, odb::dbLib* library)
{
  // We don't process this until we have a design to show
}

void MainWindow::postReadDef(odb::dbBlock* block)
{
  congestion_setup_->setEnabled(true);
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

  congestion_setup_->setEnabled(true);
  emit designLoaded(block);
}

void MainWindow::setLogger(utl::Logger* logger)
{
  logger_ = logger;
  controls_->setLogger(logger);
  script_->setLogger(logger);
  viewer_->setLogger(logger);
  drc_viewer_->setLogger(logger);
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
  } else if (event->key() == Qt::Key_K && event->modifiers() & Qt::ShiftModifier) {
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
  QPushButton* exit_button = exit_check.addButton("Exit", QMessageBox::AcceptRole);
  QPushButton* cancel_button = exit_check.addButton(QMessageBox::Cancel);
  QPushButton* hide_button = exit_check.addButton("Hide GUI", QMessageBox::ActionRole);

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
    cmds.push_back(ruler->getTclCommand(db_->getChip()->getBlock()->getDbUnitsPerMicron()));
  }
  // Save buttons
  for (const auto& action : view_tool_bar_->actions()) {
    // iterate over toolbar actions to get the correct order
    QVariant cmd = action->data();
    if (cmd.isValid()) {
      cmds.push_back(cmd.toString().toStdString());
    }
  }
  // save display settings
  controls_->restoreTclCommands(cmds);

  // save layout view
  viewer_->restoreTclCommands(cmds);

  return cmds;
}

}  // namespace gui
