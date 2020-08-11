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

#include <QMainWindow>
#include <memory>

#include "openroad/OpenRoad.hh"

namespace odb {
class dbDatabase;
}

namespace gui {

class LayoutViewer;
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

  void setDb(odb::dbDatabase* db);

  // From ord::OpenRoad::Observer
  virtual void postReadLef(odb::dbTech* tech, odb::dbLib* library) override;
  virtual void postReadDef(odb::dbBlock* block) override;
  virtual void postReadDb(odb::dbDatabase* db) override;

 signals:
  // Signaled when we get a postRead callback to tell the sub-widgets
  // to update
  void designLoaded(odb::dbBlock* block);

  // The user chose the exit action; notify the app
  void exit();

 public slots:
  // Save the current state into settings for the next session.
  void saveSettings();
  
 private:
  void         createMenus();
  void         createActions();
  void         createToolbars();

  // All but viewer_ are owned by this widget.  Qt will
  // handle destroying the children.
  DisplayControls* controls_;
  LayoutViewer*    viewer_;  // owned by scroll_
  LayoutScroll*    scroll_;
  ScriptWidget*    script_;
  odb::dbDatabase* db_;

  QMenu* fileMenu_;
  QMenu* viewMenu_;
  QMenu* windowsMenu_;

  QToolBar* viewToolBar_;

  QAction* exit_;
  QAction* fit_;
};

}  // namespace gui
