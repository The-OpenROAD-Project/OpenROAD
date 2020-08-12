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

#include <QAction>
#include <QDesktopWidget>
#include <QMenuBar>
#include <QSettings>
#include <QToolBar>

#include "displayControls.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "scriptWidget.h"

namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      controls_(new DisplayControls(this)),
      viewer_(new LayoutViewer(controls_)),
      scroll_(new LayoutScroll(viewer_, this)),
      script_(new ScriptWidget(this)),
      db_(nullptr)
{
  // Size and position the window
  QSize size = QDesktopWidget().availableGeometry(this).size();
  resize(size * 0.8);
  move(size.width() * 0.1, size.height() * 0.1);

  setCentralWidget(scroll_);
  addDockWidget(Qt::BottomDockWidgetArea, script_);
  addDockWidget(Qt::LeftDockWidgetArea, controls_);

  // Hook up all the signals/slots
  connect(script_, SIGNAL(commandExecuted()), viewer_, SLOT(update()));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          viewer_,
          SLOT(designLoaded(odb::dbBlock*)));
  connect(this,
          SIGNAL(designLoaded(odb::dbBlock*)),
          controls_,
          SLOT(designLoaded(odb::dbBlock*)));
  connect(controls_, SIGNAL(changed()), viewer_, SLOT(update()));

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
}

void MainWindow::createActions()
{
  exit_ = new QAction("Exit", this);

  fit_ = new QAction("Fit", this);
  fit_->setShortcut(QString("F"));

  connect(exit_, SIGNAL(triggered()), this, SIGNAL(exit()));
  connect(fit_, SIGNAL(triggered()), viewer_, SLOT(fit()));
}

void MainWindow::createMenus()
{
  fileMenu_ = menuBar()->addMenu("&File");
  fileMenu_->addAction(exit_);

  viewMenu_ = menuBar()->addMenu("&View");
  viewMenu_->addAction(fit_);

  windowsMenu_ = menuBar()->addMenu("&Windows");
  windowsMenu_->addAction(controls_->toggleViewAction());
  windowsMenu_->addAction(script_->toggleViewAction());
}

void MainWindow::createToolbars()
{
  viewToolBar_ = addToolBar("View");
  viewToolBar_->addAction(fit_);
  viewToolBar_->setObjectName("view_toolbar");  // for settings
}

void MainWindow::setDb(odb::dbDatabase* db)
{
  db_ = db;
  controls_->setDb(db);
  viewer_->setDb(db);
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

}  // namespace gui
