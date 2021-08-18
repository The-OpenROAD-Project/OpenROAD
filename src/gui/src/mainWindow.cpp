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

#include <QDesktopWidget>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>
#include <QToolButton>
#include <QWidgetAction>
#include <map>
#include <vector>

#include "displayControls.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "ord/OpenRoad.hh"
#include "scriptWidget.h"
#include "selectHighlightWindow.h"
#include "staGui.h"
#include "utl/Logger.h"
#include "timingWidget.h"

namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      db_(nullptr),
      logger_(nullptr),
      controls_(new DisplayControls(this)),
      inspector_(new Inspector(selected_, this)),
      viewer_(new LayoutViewer(
          controls_,
          selected_,
          highlighted_,
          rulers_,
          [this](const std::any& object) { return makeSelected(object); },
          this)),
      selection_browser_(
          new SelectHighlightWindow(selected_, highlighted_, this)),
      scroll_(new LayoutScroll(viewer_, this)),
      script_(new ScriptWidget(this)),
      timing_dialog_(new TimingWidget(this))
{
  // Size and position the window
  QSize size = QDesktopWidget().availableGeometry(this).size();
  resize(size * 0.8);
  move(size.width() * 0.1, size.height() * 0.1);

  find_dialog_ = new FindObjectDialog(this);

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  script_->setFont(font);

  setCentralWidget(scroll_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);
  addDockWidget(Qt::RightDockWidgetArea, inspector_);
  addDockWidget(Qt::BottomDockWidgetArea, selection_browser_);
  addDockWidget(Qt::RightDockWidgetArea, timing_dialog_);

  tabifyDockWidget(selection_browser_, script_);
  selection_browser_->hide();

  tabifyDockWidget(inspector_, timing_dialog_);

  // Hook up all the signals/slots
  connect(script_, SIGNAL(tclExiting()), this, SIGNAL(exit()));
  connect(script_, SIGNAL(commandExecuted(int)), viewer_, SLOT(update()));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          viewer_,
          SLOT(designLoaded(odb::dbBlock*)));
  connect(this, SIGNAL(redraw()), viewer_, SLOT(repaint()));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          controls_,
          SLOT(designLoaded(odb::dbBlock*)));

  connect(this, SIGNAL(pause()), script_, SLOT(pause()));
  connect(controls_, SIGNAL(changed()), viewer_, SLOT(update()));
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
          SIGNAL(addRuler(int, int, int, int)),
          this,
          SLOT(addRuler(int, int, int, int)));

  connect(this, SIGNAL(selectionChanged()), viewer_, SLOT(update()));
  connect(this, SIGNAL(highlightChanged()), viewer_, SLOT(update()));
  connect(this, SIGNAL(rulersChanged()), viewer_, SLOT(update()));

  connect(inspector_,
          SIGNAL(selected(const Selected&, bool)),
          this,
          SLOT(setSelected(const Selected&, bool)));
  connect(this, SIGNAL(selectionChanged()), inspector_, SLOT(update()));
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

  connect(timing_dialog_,
          SIGNAL(highlightTimingPath(TimingPath*)),
          viewer_,
          SLOT(update()));

  createActions();
  createToolbars();
  createMenus();
  createStatusBar();

  // Restore the settings (if none this is a no-op)
  QSettings settings("OpenRoad Project", "openroad");
  settings.beginGroup("main");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("state").toByteArray());
  script_->readSettings(&settings);
  controls_->readSettings(&settings);
  timing_dialog_->readSettings(&settings);
  settings.endGroup();
}

void MainWindow::createStatusBar()
{
  location_ = new QLabel();
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

  timing_debug_ = new QAction("Timing ...", this);
  timing_debug_->setShortcut(QString("Ctrl+T"));

  congestion_setup_ = new QAction("Congestion Setup...", this);

  connect(congestion_setup_,
          SIGNAL(triggered()),
          controls_,
          SLOT(showCongestionSetup()));

  connect(exit_, SIGNAL(triggered()), this, SIGNAL(exit()));
  connect(fit_, SIGNAL(triggered()), viewer_, SLOT(fit()));
  connect(zoom_in_, SIGNAL(triggered()), viewer_, SLOT(zoomIn()));
  connect(zoom_out_, SIGNAL(triggered()), viewer_, SLOT(zoomOut()));
  connect(find_, SIGNAL(triggered()), this, SLOT(showFindDialog()));
  connect(inspect_, SIGNAL(triggered()), inspector_, SLOT(show()));
}

void MainWindow::createMenus()
{
  file_menu_ = menuBar()->addMenu("&File");
  file_menu_->addAction(exit_);

  view_menu_ = menuBar()->addMenu("&View");
  view_menu_->addAction(fit_);
  view_menu_->addAction(find_);
  view_menu_->addAction(zoom_in_);
  view_menu_->addAction(zoom_out_);

  tools_menu_ = menuBar()->addMenu("&Tools");
  tools_menu_->addAction(congestion_setup_);

  windows_menu_ = menuBar()->addMenu("&Windows");
  windows_menu_->addAction(controls_->toggleViewAction());
  windows_menu_->addAction(inspector_->toggleViewAction());
  windows_menu_->addAction(script_->toggleViewAction());
  windows_menu_->addAction(selection_browser_->toggleViewAction());
  windows_menu_->addAction(view_tool_bar_->toggleViewAction());
  windows_menu_->addAction(timing_dialog_->toggleViewAction());
  selection_browser_->setVisible(false);
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

void MainWindow::setDb(odb::dbDatabase* db)
{
  db_ = db;
  controls_->setDb(db);
  viewer_->setDb(db);
  selection_browser_->setDb(db);
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
  }
  emit updateSelectedStatus(selection);
  emit selectionChanged();
}

void MainWindow::addSelected(const SelectionSet& selections)
{
  selected_.insert(selections.begin(), selections.end());
  status(std::string("Added ") + std::to_string(selections.size()));
  emit selectionChanged();
}

void MainWindow::setSelected(const Selected& selection, bool show_connectivity)
{
  selected_.clear();
  addSelected(selection);
  if (show_connectivity)
    selectHighlightConnectedNets(true, true, true, false);
}

void MainWindow::addHighlighted(const SelectionSet& highlights,
                                int highlight_group)
{
  if (highlight_group >= 7)
    return;
  highlighted_[highlight_group].insert(highlights.begin(), highlights.end());
  emit highlightChanged();
}

void MainWindow::addRuler(int x0, int y0, int x1, int y1)
{
  QLine ruler(QPoint(x0, y0), QPoint(x1, y1));
  rulers_.push_back(ruler);
  emit rulersChanged();
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
        connected_insts.insert(makeSelected(inst_term));
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
          connected_nets.insert(makeSelected(inst_term->getNet()));
        if (input
            && (inst_term_dir == odb::dbIoType::INPUT
                || inst_term_dir == odb::dbIoType::INOUT))
          connected_nets.insert(makeSelected(inst_term->getNet(), inst_term));
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
  script_->writeSettings(&settings);
  controls_->writeSettings(&settings);
  timing_dialog_->writeSettings(&settings);
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
}

void MainWindow::fit()
{
  fit_->trigger();
}

Selected MainWindow::makeSelected(std::any object, void* additional_data)
{
  if (!object.has_value()) {
    return Selected();
  }

  auto it = descriptors_.find(object.type());
  if (it != descriptors_.end()) {
    return it->second->makeSelected(object, additional_data);
  } else {
    return Selected();  // FIXME: null descriptor
  }
}

void MainWindow::registerDescriptor(const std::type_info& type,
                                    const Descriptor* descriptor)
{
  descriptors_[type] = descriptor;
}

}  // namespace gui
