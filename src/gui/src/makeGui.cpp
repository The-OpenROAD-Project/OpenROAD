// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// gui's entry points for a build with no Qt.  A Qt build compiles gui.cpp's
// versions instead, so every binary has exactly one of each.

#include "gui/MakeGui.h"

#include <cstdio>
#include <string>

#include "gui/gui.h"
#include "odb/db.h"
#include "tcl.h"
#include "utl/Logger.h"
#include "utl/decode.h"
#include "web/core.h"

namespace gui {

// Tcl files encoded into strings.
extern const char* gui_tcl_inits[];

extern "C" {
extern int Gui_Init(Tcl_Interp* interp);
}

// using namespace odb;
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive,
             bool load_settings,
             bool minimize)
{
  printf(
      "[ERROR] This code was compiled with the GUI disabled.  Please recompile "
      "with Qt5 if you want the GUI.\n");

  return 1;  // return unix err
}

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
{
  // The same gui:: commands as a Qt build, so a script runs on either; with
  // no window they answer as a Qt build does before its window opens.
  Gui_Init(interp);
  utl::evalTclInit(interp, gui::gui_tcl_inits);

  web::Gui::get()->initCommon(db, sta, logger);
}

}  // namespace gui

namespace web {

// Gui methods only the Qt main window has; gui.i's check_gui() keeps all but
// gui::show and set_title from them.  The NOLINTs: gui.cpp's signatures.

// NOLINTNEXTLINE(modernize-use-string-view)
std::string Gui::addToolbarButton(const std::string& /* name */,
                                  const std::string& /* text */,
                                  const std::string& /* script */,
                                  bool /* echo */)
{
  return {};
}

void Gui::removeToolbarButton(const std::string& /* name */)
{
}

// NOLINTNEXTLINE(modernize-use-string-view)
std::string Gui::addMenuItem(const std::string& /* name */,
                             const std::string& /* path */,
                             const std::string& /* text */,
                             const std::string& /* script */,
                             const std::string& /* shortcut */,
                             bool /* echo */)
{
  return {};
}

void Gui::removeMenuItem(const std::string& /* name */)
{
}

// NOLINTNEXTLINE(modernize-use-string-view)
std::string Gui::requestUserInput(const std::string& /* title */,
                                  const std::string& /* question */)
{
  return {};
}

void Gui::hideGui()
{
}

void Gui::minimize()
{
}

void Gui::unminimize()
{
}

void Gui::selectHelp(const std::string& /* item */)
{
}

void Gui::selectChart(const std::string& /* name */)
{
}

void Gui::updateTimingReport()
{
}

void Gui::showGui(const std::string& /* cmds */,
                  bool /* interactive */,
                  bool /* load_settings */)
{
  logger_->error(utl::GUI,
                 112,
                 "Command gui::show is not available as OpenROAD was not "
                 "compiled with QT support.");
}

void Gui::setMainWindowTitle(const std::string& title)
{
  main_window_title_ = title;
}

std::string Gui::getMainWindowTitle()
{
  return main_window_title_;
}

}  // namespace web
