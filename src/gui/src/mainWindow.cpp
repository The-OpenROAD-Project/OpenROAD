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

#include "mainWindow.h"

#include <QDesktopWidget>
#include <QMenuBar>
#include <QSettings>
#include <QStatusBar>

#include "displayControls.h"
#include "layoutViewer.h"
#include "scriptWidget.h"
#include "selectHighlightWindow.h"

namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      db_(nullptr),
      controls_(new DisplayControls(this)),
      viewer_(new LayoutViewer(controls_, selected_, highlighted_)),
      selectionBrowser_(
          new SelectHighlightWindow(selected_, highlighted_, this)),
      scroll_(new LayoutScroll(viewer_, this)),
      script_(new ScriptWidget(this))
{
  // Size and position the window
  QSize size = QDesktopWidget().availableGeometry(this).size();
  resize(size * 0.8);
  move(size.width() * 0.1, size.height() * 0.1);

  findDialog_ = new FindObjectDialog(this);

  setCentralWidget(scroll_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);
  addDockWidget(Qt::BottomDockWidgetArea, selectionBrowser_);

  tabifyDockWidget(selectionBrowser_, script_);
  selectionBrowser_->hide();

  // Hook up all the signals/slots
  connect(script_, SIGNAL(commandExecuted()), viewer_, SLOT(update()));
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
          SIGNAL(selected(const Selected&)),
          this,
          SLOT(setSelected(const Selected&)));
  connect(viewer_,
          SIGNAL(addSelected(const Selected&)),
          this,
          SLOT(addSelected(const Selected&)));
  connect(this, SIGNAL(selectionChanged()), viewer_, SLOT(update()));
  connect(this, SIGNAL(highlightChanged()), viewer_, SLOT(update()));

  connect(this,
          SIGNAL(selectionChanged()),
          selectionBrowser_,
          SLOT(updateSelectionModel()));
  connect(this,
          SIGNAL(highlightChanged()),
          selectionBrowser_,
          SLOT(updateHighlightModel()));

  connect(selectionBrowser_,
          &SelectHighlightWindow::clearAllSelections,
          this,
          [this]() { this->setSelected(Selected()); });
  connect(selectionBrowser_,
          &SelectHighlightWindow::clearAllHighlights,
          this,
          [this]() { this->clearHighlighted(); });
  connect(selectionBrowser_,
          SIGNAL(clearSelectedItems(const QList<const Selected*>&)),
          this,
          SLOT(removeFromSelected(const QList<const Selected*>&)));

  connect(selectionBrowser_,
          SIGNAL(zoomInToItems(const QList<const Selected*>&)),
          this,
          SLOT(zoomInToItems(const QList<const Selected*>&)));

  connect(selectionBrowser_,
          SIGNAL(clearHighlightedItems(const QList<const Selected*>&)),
          this,
          SLOT(removeFromHighlighted(const QList<const Selected*>&)));

  connect(selectionBrowser_,
          SIGNAL(highlightSelectedItemsSig(const QList<const Selected*>&)),
          this,
          SLOT(updateHighlightedSet(const QList<const Selected*>&)));

  // Restore the settings (if none this is a no-op)
  QSettings settings("OpenRoad Project", "openroad");
  settings.beginGroup("main");
  restoreGeometry(settings.value("geometry").toByteArray());
  restoreState(settings.value("state").toByteArray());
  script_->readSettings(&settings);
  settings.endGroup();

  createActions();
  createMenus();
  createToolbars();
  createStatusBar();
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

  auto topBlock = chip->getBlock();
  return topBlock;
}

void MainWindow::createActions()
{
  exit_ = new QAction("Exit", this);

  fit_ = new QAction("Fit", this);
  fit_->setShortcut(QString("F"));

  find_ = new QAction("Find", this);
  find_->setShortcut(QString("Ctrl+F"));

  zoomIn_ = new QAction("Zoom in", this);
  zoomIn_->setShortcut(QString("Z"));

  zoomOut_ = new QAction("Zoom out", this);
  zoomOut_->setShortcut(QString("Shift+Z"));

  connect(exit_, SIGNAL(triggered()), this, SIGNAL(exit()));
  connect(fit_, SIGNAL(triggered()), viewer_, SLOT(fit()));
  connect(zoomIn_, SIGNAL(triggered()), scroll_, SLOT(zoomIn()));
  connect(zoomOut_, SIGNAL(triggered()), scroll_, SLOT(zoomOut()));
  connect(find_, SIGNAL(triggered()), this, SLOT(showFindDialog()));
}

void MainWindow::createMenus()
{
  fileMenu_ = menuBar()->addMenu("&File");
  fileMenu_->addAction(exit_);

  viewMenu_ = menuBar()->addMenu("&View");
  viewMenu_->addAction(fit_);
  viewMenu_->addAction(find_);
  viewMenu_->addAction(zoomIn_);
  viewMenu_->addAction(zoomOut_);

  windowsMenu_ = menuBar()->addMenu("&Windows");
  windowsMenu_->addAction(controls_->toggleViewAction());
  windowsMenu_->addAction(script_->toggleViewAction());
  windowsMenu_->addAction(selectionBrowser_->toggleViewAction());
  selectionBrowser_->setVisible(false);
}

void MainWindow::createToolbars()
{
  viewToolBar_ = addToolBar("View");
  viewToolBar_->addAction(fit_);
  viewToolBar_->addAction(find_);
  viewToolBar_->setObjectName("view_toolbar");  // for settings
}

void MainWindow::setDb(odb::dbDatabase* db)
{
  db_ = db;
  controls_->setDb(db);
  viewer_->setDb(db);
}

void MainWindow::setLocation(qreal x, qreal y)
{
  location_->setText(QString("%1, %2").arg(x, 0, 'f', 5).arg(y, 0, 'f', 5));
}

void MainWindow::addSelected(const Selected& selection)
{
  if (selection) {
    selected_.emplace(selection);
  }
  status(selection ? selection.getName() : "");
  emit selectionChanged();
  selectionBrowser_->show();
}

void MainWindow::addSelected(const SelectionSet& selections)
{
  selected_.insert(selections.begin(), selections.end());
  status(std::string("Added ") + std::to_string(selections.size()));
  emit selectionChanged();
  selectionBrowser_->show();
}

void MainWindow::setSelected(const Selected& selection)
{
  selected_.clear();
  addSelected(selection);
}

void MainWindow::addHighlighted(const SelectionSet& highlights)
{
  highlighted_.insert(highlights.begin(), highlights.end());
  emit highlightChanged();
}

void MainWindow::updateHighlightedSet(const QList<const Selected*>& items)
{
  for (auto item : items) {
    highlighted_.insert(*item);
  }
  emit highlightChanged();
}

void MainWindow::clearHighlighted()
{
  if (highlighted_.empty())
    return;
  highlighted_.clear();
  emit highlightChanged();
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

void MainWindow::removeFromHighlighted(const QList<const Selected*>& items)
{
  if (items.empty())
    return;
  for (auto& item : items) {
    highlighted_.erase(*item);
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
  odb::Rect itemsBBox;
  itemsBBox.mergeInit();
  for (auto& item : items) {
    odb::Rect itemBBox;
    if (item->getBBox(itemBBox))
      itemsBBox.merge(itemBBox);
  }

  zoomTo(itemsBBox);
}

void MainWindow::status(const std::string& message)
{
  statusBar()->showMessage(QString::fromStdString(message));
}

void MainWindow::showFindDialog()
{
  if (getBlock() == nullptr)
    return;
  findDialog_->exec();
}

void MainWindow::saveSettings()
{
  QSettings settings("OpenRoad Project", "openroad");
  settings.beginGroup("main");
  settings.setValue("geometry", saveGeometry());
  settings.setValue("state", saveState());
  script_->writeSettings(&settings);
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
  script_->setLogger(logger);
}

}  // namespace gui
