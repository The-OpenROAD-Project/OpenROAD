// Copyright (c) 2019, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <array>
#include <stdio.h>
#include <tcl.h>
#ifdef ENABLE_READLINE
  // If you get an error on this include be sure you have
  //   the package tcl-tclreadline-devel installed
  #include <tclreadline.h>
#endif

#include "sta/StringUtil.hh"
#include "sta/StaMain.hh"
#include "openroad/Version.hh"
#include "openroad/Error.hh"
#include "openroad/InitOpenRoad.hh"

using sta::stringEq;
using sta::findCmdLineFlag;
using sta::stringPrintTmp;
using sta::sourceTclFile;

static int cmd_argc;
static char **cmd_argv;
static const char *init_filename = ".openroad";

static int
tclAppInit(Tcl_Interp *interp);
static int
tclAppInit(int argc,
	   char *argv[],
	   const char *init_filename,
	   Tcl_Interp *interp);
static void
showUsage(const char *prog,
	  const char *init_filename);

int
main(int argc,
     char *argv[])
{
  if (argc == 2 && stringEq(argv[1], "-help")) {
    showUsage(argv[0], init_filename);
    return 0;
  }
  else if (argc == 2 && stringEq(argv[1], "-version")) {
    printf("%s %s\n", OPENROAD_VERSION, OPENROAD_GIT_SHA1);
    return 0;
  }
  else {
    cmd_argc = argc;
    cmd_argv = argv;
    // Set argc to 1 so Tcl_Main doesn't source any files.
    // Tcl_Main never returns.
    Tcl_Main(1, argv, tclAppInit);
    return 0;
  }
}

#ifdef ENABLE_READLINE
static int
tclReadlineInit(Tcl_Interp *interp)
{
  std::array<const char *, 8> readline_cmds = {
    "history",
    "history event",
    "eval $auto_index(::tclreadline::ScriptCompleter)",
    "::tclreadline::readline builtincompleter true",
    "::tclreadline::readline customcompleter ::tclreadline::ScriptCompleter",
    "proc ::tclreadline::prompt1 {} { return \"openroad " OPENROAD_VERSION "> \" }",
    "proc ::tclreadline::prompt2 {} { return \"...> \" }",
    "::tclreadline::Loop"
  };

  for (auto cmd : readline_cmds) {
    if (TCL_ERROR == Tcl_Eval(interp, cmd)) {
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}
#endif

static int
tclAppInit(Tcl_Interp *interp)
{
  return tclAppInit(cmd_argc, cmd_argv, init_filename, interp);
}

// Tcl init executed inside Tcl_Main.
static int
tclAppInit(int argc,
	   char *argv[],
	   const char *init_filename,
	   Tcl_Interp *interp)
{
  // source init.tcl
  if (Tcl_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }
#ifdef ENABLE_READLINE
  if (Tclreadline_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }
  Tcl_StaticPackage(interp, "tclreadline", Tclreadline_Init, Tclreadline_SafeInit);
  if (Tcl_EvalFile(interp, TCLRL_LIBRARY "/tclreadlineInit.tcl") != TCL_OK) {
    printf("Failed to load tclreadline\n");
  }
#endif
  ord::initOpenRoad(interp);

  if (!findCmdLineFlag(argc, argv, "-no_splash"))
    Tcl_Eval(interp, "show_openroad_splash");

  bool exit_after_cmd_file = findCmdLineFlag(argc, argv, "-exit");

  if (!findCmdLineFlag(argc, argv, "-no_init")) {
    char *init_path = stringPrintTmp("[file join $env(HOME) %s]",init_filename);
    sourceTclFile(init_path, true, true, interp);
  }

  if (argc > 2 ||
      (argc > 1 && argv[1][0] == '-'))
    showUsage(argv[0], init_filename);
  else {
    if (argc == 2) {
      char *cmd_file = argv[1];
      if (cmd_file) {
	sourceTclFile(cmd_file, false, false, interp);
	if (exit_after_cmd_file)
	  exit(EXIT_SUCCESS);
      }
    }
  }
#ifdef ENABLE_READLINE
  return tclReadlineInit(interp);
#else
  return TCL_OK;
#endif
}

static void
showUsage(const char *prog,
	  const char *init_filename)
{
  printf("Usage: %s [-help] [-version] [-no_init] [-exit] cmd_file\n", prog);
  printf("  -help              show help and exit\n");
  printf("  -version           show version and exit\n");
  printf("  -no_init           do not read %s init file\n", init_filename);
  printf("  -threads count|max use count threads\n");
  printf("  -no_splash         do not show the license splash at startup\n");
  printf("  -exit              exit after reading cmd_file\n");
  printf("  cmd_file           source cmd_file\n");
}

