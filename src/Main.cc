/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <stdio.h>
#include <tcl.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <boost/stacktrace.hpp>
#include <iostream>
#include <string>
#include <libgen.h>
// We have had too many problems with this std::filesytem on various platforms
// so it is disabled but kept for future reference
#ifdef USE_STD_FILESYSTEM
#include <filesystem>
#endif
#ifdef ENABLE_READLINE
  // If you get an error on this include be sure you have
  //   the package tcl-tclreadline-devel installed
  #include <tclreadline.h>
#endif
#ifdef ENABLE_PYTHON3
  #define PY_SSIZE_T_CLEAN
  #include "Python.h"
#endif

#ifdef ENABLE_TCLX
  #include <tclExtend.h>
#endif

#include "sta/StringUtil.hh"
#include "sta/StaMain.hh"
#include "ord/Version.hh"
#include "ord/InitOpenRoad.hh"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "gui/gui.h"

using std::string;
using sta::stringEq;
using sta::findCmdLineFlag;
using sta::findCmdLineKey;
using sta::sourceTclFile;
using sta::is_regular_file;

#ifdef ENABLE_PYTHON3
extern "C"
{
    extern PyObject* PyInit__openroad_swig_py();
    extern PyObject* PyInit__odbpy();
}
#endif

int cmd_argc;
char **cmd_argv;
const char* log_filename = nullptr;
const char* metrics_filename = nullptr;

static const char *init_filename = ".openroad";

static void
showUsage(const char *prog, const char *init_filename);
static void
showSplash();

#ifdef ENABLE_PYTHON3
namespace sta {
extern const char *odbpy_python_inits[];
extern const char *openroad_swig_py_python_inits[];
}

static void
initPython()
{
  if (PyImport_AppendInittab("_odbpy", PyInit__odbpy) == -1) {
    fprintf(stderr, "Error: could not add module odbpy\n");
    exit(1);
  }

  if (PyImport_AppendInittab("_openroad_swig_py", PyInit__openroad_swig_py) == -1) {
    fprintf(stderr, "Error: could not add module openroadpy\n");
    exit(1);
  }

  Py_Initialize();

  char *unencoded = sta::unencode(sta::odbpy_python_inits);

  PyObject* odb_code = Py_CompileString(unencoded, "odbpy.py", Py_file_input);
  if (odb_code == nullptr) {
    PyErr_Print();
    fprintf(stderr, "Error: could not compile odbpy\n");
    exit(1);
  }

  if (PyImport_ExecCodeModule("odb", odb_code) == nullptr) {
    PyErr_Print();
    fprintf(stderr, "Error: could not add module odb\n");
    exit(1);
  }

  delete [] unencoded;

  unencoded = sta::unencode(sta::openroad_swig_py_python_inits);

  PyObject* ord_code = Py_CompileString(unencoded, "openroad.py", Py_file_input);
  if (ord_code == nullptr) {
    PyErr_Print();
    fprintf(stderr, "Error: could not compile openroad.py\n");
    exit(1);
  }

  if (PyImport_ExecCodeModule("openroad", ord_code) == nullptr) {
    PyErr_Print();
    fprintf(stderr, "Error: could not add module openroad\n");
    exit(1);
  }

  delete [] unencoded;
}
#endif

static void handler(int) {
  std::cerr << "Stack trace:\n";
  std::cerr << boost::stacktrace::stacktrace();
  exit(1);
}

int
main(int argc,
     char *argv[])
{
  // This avoids problems with locale setting dependent
  // C functions like strtod (e.g. 0.5 vs 0,5).
  setenv("LC_ALL", "en_US.UTF-8", /* override */ 1);
  setenv("LANG", "en_US.UTF-8", /* override */ 1);

  // Generate a stacktrace on crash
  signal(SIGABRT, handler);
  signal(SIGBUS, handler);
  signal(SIGFPE, handler);
  signal(SIGILL, handler);
  signal(SIGSEGV, handler);

  if (argc == 2 && stringEq(argv[1], "-help")) {
    showUsage(argv[0], init_filename);
    return 0;
  }
  if (argc == 2 && stringEq(argv[1], "-version")) {
    printf("%s %s\n", OPENROAD_VERSION, OPENROAD_GIT_DESCRIBE);
    return 0;
  }

  log_filename = findCmdLineKey(argc, argv, "-log");
  if (log_filename) {
    remove(log_filename);
  }

  metrics_filename = findCmdLineKey(argc, argv, "-metrics");
  if (metrics_filename) {
    remove(metrics_filename);
  }

  cmd_argc = argc;
  cmd_argv = argv;
#ifdef ENABLE_PYTHON3
  // Capture the current SIGINT handler before python changes it.
  struct sigaction orig_sigint_handler;
  sigaction(SIGINT, NULL, &orig_sigint_handler);

  initPython();

  if (findCmdLineFlag(cmd_argc, cmd_argv, "-python")) {
    // Setup the app with tcl
    auto* interp = Tcl_CreateInterp();
    Tcl_Init(interp);
    ord::initOpenRoad(interp);
    if (!findCmdLineFlag(cmd_argc, cmd_argv, "-no_splash")) {
      showSplash();
    }

    std::vector<wchar_t*> args;
    for(int i = 0; i < cmd_argc; i++) {
      size_t sz = strlen(cmd_argv[i]);
      args.push_back(new wchar_t[sz+1]);
      args[i][sz] = '\0';
      for(size_t j = 0;j < sz; j++) {
        args[i][j] = (wchar_t) cmd_argv[i][j];
      }
    }

    return Py_Main(cmd_argc, args.data());
  } else {
    // Python wants to install its own SIGINT handler to print KeyboardInterrupt
    // on ctrl-C. We don't want that if python is not the main interpreter.
    // We restore the handler from before initPython.
    sigaction(SIGINT, &orig_sigint_handler, NULL);
  }
#endif

  // Set argc to 1 so Tcl_Main doesn't source any files.
  // Tcl_Main never returns.
  Tcl_Main(1, argv, ord::tclAppInit);
  return 0;
}

#ifdef ENABLE_READLINE
static int
tclReadlineInit(Tcl_Interp *interp)
{
  std::array<const char *, 7> readline_cmds = {
    "history event",
    "eval $auto_index(::tclreadline::ScriptCompleter)",
    "::tclreadline::readline builtincompleter true",
    "::tclreadline::readline customcompleter ::tclreadline::ScriptCompleter",
    "proc ::tclreadline::prompt1 {} { return \"openroad> \" }",
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

// Tcl init executed inside Tcl_Main.
static int
tclAppInit(int& argc,
           char *argv[],
           const char *init_filename,
           Tcl_Interp *interp)
{
  // first check if gui was requested and launch.
  // gui will call this function again as part of setup
  // ensuring the else {} will be utilized to initialize tcl and OR.
  if (findCmdLineFlag(argc, argv, "-gui")) {
    gui::startGui(argc, argv, interp);
  } else {
    // init tcl
    if (Tcl_Init(interp) == TCL_ERROR) {
      return TCL_ERROR;
    }
#ifdef ENABLE_TCLX
    if (Tclx_Init(interp) == TCL_ERROR) {
      return TCL_ERROR;
    }
#endif
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

    if (!findCmdLineFlag(argc, argv, "-no_splash")) {
      showSplash();
    }

    const char* threads = findCmdLineKey(argc, argv, "-threads");
    if (threads) {
      ord::OpenRoad::openRoad()->setThreadCount(threads);
    } else {
      // set to default number of threads
      ord::OpenRoad::openRoad()->setThreadCount(ord::OpenRoad::openRoad()->getThreadCount(), false);
    }

    bool exit_after_cmd_file = findCmdLineFlag(argc, argv, "-exit");

    const bool gui_enabled = gui::Gui::enabled();

    if (!findCmdLineFlag(argc, argv, "-no_init")) {
      const char* restore_state_cmd = "source -echo -verbose {{{}}}";
#ifdef USE_STD_FILESYSTEM
      std::filesystem::path init(getenv("HOME"));
      init /= init_filename;
      if (std::filesystem::is_regular_file(init)) {
        if (!gui_enabled) {
          sourceTclFile(init.c_str(), true, true, interp);
        } else {
          // need to delay loading of file until after GUI is completed initialized
          gui::Gui::get()->addRestoreStateCommand(fmt::format(restore_state_cmd, init.string()));
        }
      }
#else
      string init_path = getenv("HOME");
      init_path += "/";
      init_path += init_filename;
      if (is_regular_file(init_path.c_str())) {
        if (!gui_enabled) {
          sourceTclFile(init_path.c_str(), true, true, interp);
        } else {
          // need to delay loading of file until after GUI is completed initialized
          gui::Gui::get()->addRestoreStateCommand(fmt::format(restore_state_cmd, init_path));
        }
      }
#endif
    }

    if (argc > 2 || (argc > 1 && argv[1][0] == '-')) {
      showUsage(argv[0], init_filename);
    }
    else {
      if (argc == 2) {
        char *cmd_file = argv[1];
        if (cmd_file) {
          if (!gui_enabled) {
            int result = sourceTclFile(cmd_file, false, false, interp);
            if (exit_after_cmd_file) {
              int exit_code = (result == TCL_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
              exit(exit_code);
            }
          } else {
            // need to delay loading of file until after GUI is completed initialized
            const char* restore_state_cmd = "source {{{}}}";
            gui::Gui::get()->addRestoreStateCommand(fmt::format(restore_state_cmd, cmd_file));
            if (exit_after_cmd_file) {
              gui::Gui::get()->addRestoreStateCommand("exit");
            }
          }
        }
      }
    }
  }
#ifdef ENABLE_READLINE
  if (!gui::Gui::enabled()) {
    return tclReadlineInit(interp);
  }
#endif
  return TCL_OK;
}

int
ord::tclAppInit(Tcl_Interp *interp)
{
  return tclAppInit(cmd_argc, cmd_argv, init_filename, interp);
}


static void
showUsage(const char *prog, const char *init_filename)
{
  printf("Usage: %s [-help] [-version] [-no_init] [-exit] [-gui] [-threads count|max] [-log file_name] cmd_file\n", prog);
  printf("  -help              show help and exit\n");
  printf("  -version           show version and exit\n");
  printf("  -no_init           do not read %s init file\n", init_filename);
  printf("  -threads count|max use count threads\n");
  printf("  -no_splash         do not show the license splash at startup\n");
  printf("  -exit              exit after reading cmd_file\n");
  printf("  -gui               start in gui mode\n");
#ifdef ENABLE_PYTHON3
  printf("  -python            start with python interpreter [limited to db operations]\n");
#endif
  printf("  -log <file_name>   write a log in <file_name>\n");
  printf("  cmd_file           source cmd_file\n");
}

static void
showSplash()
{
  utl::Logger *logger = ord::OpenRoad::openRoad()->getLogger();
  string sha = OPENROAD_GIT_DESCRIBE;
  logger->report("OpenROAD {} {}",
                 OPENROAD_VERSION,
                 sha.c_str());
  logger->report("This program is licensed under the BSD-3 license. See the LICENSE file for details.");
  logger->report("Components of this program may be licensed under more restrictive licenses which must be honored.");
}
