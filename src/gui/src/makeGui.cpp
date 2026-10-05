// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// The two entry points OpenRoad calls into gui with, for a build that has no
// Qt.  A Qt build compiles gui.cpp's versions of them instead and never sees
// this file, so between them every binary has exactly one of each.

#include <any>
#include <cstdio>
#include <string>

#include "gui/gui.h"
#include "odb/db.h"
#include "tcl.h"
#include "web/heatMap.h"

namespace gui {

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
  // Brings up the descriptor registry and the heat map sources for the web
  // viewer and other non-Qt consumers, and gives web::Gui the database and
  // logger its own dispatch reports through.
  web::Gui::get()->initCommon(db, sta, logger);

  // Tcl requires this to be a writable string
  std::string cmd_save_image(
      "proc save_image { args } {"
      "  utl::error GUI 4 \"Command save_image is not available as OpenROAD "
      "was not compiled with QT support.\""
      "}");
  Tcl_Eval(interp, cmd_save_image.c_str());
  std::string cmd_supported(
      "namespace eval gui {"
      "  proc supported {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, cmd_supported.c_str());
  std::string enabled_supported(
      "namespace eval gui {"
      "  proc enabled {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, enabled_supported.c_str());
  // Counterpart of gui.i's has_ui: commands shared with a non-Qt viewer
  // dispatch on it, so it has to answer in a build with no Qt at all.
  std::string cmd_has_ui(
      "namespace eval gui {"
      "  proc has_ui {} {"
      "    return 0"
      "  }"
      "}");
  Tcl_Eval(interp, cmd_has_ui.c_str());
}

}  // namespace gui
