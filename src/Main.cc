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

#include <libgen.h>
#include <tcl.h>

#include <array>
#include <boost/stacktrace.hpp>
#include <climits>
#include <clocale>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
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

#include "gui/gui.h"
#include "ord/InitOpenRoad.hh"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "sta/StringUtil.hh"
#include "utl/Logger.h"

using sta::findCmdLineFlag;
using sta::findCmdLineKey;
using sta::is_regular_file;
using sta::sourceTclFile;
using sta::stringEq;
using std::string;

#ifdef ENABLE_PYTHON3
// par causes abseil link error at startup on apple silicon
#ifdef ENABLE_PAR
#define TOOL_PAR X(par)
#else
#define TOOL_PAR
#endif

#define FOREACH_TOOL_WITHOUT_OPENROAD(X) \
  X(ifp)                                 \
  X(utl)                                 \
  X(ant)                                 \
  X(grt)                                 \
  X(gpl)                                 \
  X(dpl)                                 \
  X(mpl)                                 \
  X(ppl)                                 \
  X(tap)                                 \
  X(cts)                                 \
  X(drt)                                 \
  X(dpo)                                 \
  X(fin)                                 \
  TOOL_PAR                               \
  X(rcx)                                 \
  X(rmp)                                 \
  X(stt)                                 \
  X(psm)                                 \
  X(pdn)                                 \
  X(odb)

#define FOREACH_TOOL(X)            \
  FOREACH_TOOL_WITHOUT_OPENROAD(X) \
  X(openroad_swig)

extern "C" {
#define X(name) extern PyObject* PyInit__##name##_py();
FOREACH_TOOL(X)
#undef X
}
#endif

int cmd_argc;
char** cmd_argv;
const char* log_filename = nullptr;
const char* metrics_filename = nullptr;

static const char* init_filename = ".openroad";

static void showUsage(const char* prog, const char* init_filename);
static void showSplash();

#ifdef ENABLE_PYTHON3
namespace sta {
#define X(name) extern const char* name##_py_python_inits[];
FOREACH_TOOL(X)
#undef X
}  // namespace sta

#if PY_VERSION_HEX >= 0x03080000
static void initPython(int argc, char* argv[])
#else
static void initPython()
#endif
{
#define X(name)                                                             \
  if (PyImport_AppendInittab("_" #name "_py", PyInit__##name##_py) == -1) { \
    fprintf(stderr, "Error: could not add module _" #name "_py\n");         \
    exit(1);                                                                \
  }
  FOREACH_TOOL(X)
#undef X
#if PY_VERSION_HEX >= 0x03080000
  bool inspect = !findCmdLineFlag(argc, argv, "-exit");
  PyConfig config;
  PyConfig_InitPythonConfig(&config);
  PyConfig_SetBytesArgv(&config, argc, argv);
  config.inspect = inspect;
  Py_InitializeFromConfig(&config);
  PyConfig_Clear(&config);
#else
  Py_Initialize();
#endif
#define X(name)                                                       \
  {                                                                   \
    char* unencoded = sta::unencode(sta::name##_py_python_inits);     \
    PyObject* code                                                    \
        = Py_CompileString(unencoded, #name "_py.py", Py_file_input); \
    if (code == nullptr) {                                            \
      PyErr_Print();                                                  \
      fprintf(stderr, "Error: could not compile " #name "_py\n");     \
      exit(1);                                                        \
    }                                                                 \
    if (PyImport_ExecCodeModule(#name, code) == nullptr) {            \
      PyErr_Print();                                                  \
      fprintf(stderr, "Error: could not add module " #name "\n");     \
      exit(1);                                                        \
    }                                                                 \
    delete[] unencoded;                                               \
  }
  FOREACH_TOOL_WITHOUT_OPENROAD(X)
#undef X
#undef FOREACH_TOOL
#undef FOREACH_TOOL_WITHOUT_OPENROAD

  // Need to separately handle openroad here because we need both
  // the names "openroad_swig" and "openroad".
  {
    char* unencoded = sta::unencode(sta::openroad_swig_py_python_inits);

    PyObject* code = Py_CompileString(unencoded, "openroad.py", Py_file_input);
    if (code == nullptr) {
      PyErr_Print();
      fprintf(stderr, "Error: could not compile openroad.py\n");
      exit(1);
    }

    if (PyImport_ExecCodeModule("openroad", code) == nullptr) {
      PyErr_Print();
      fprintf(stderr, "Error: could not add module openroad\n");
      exit(1);
    }

    delete[] unencoded;
  }
}
#endif

static volatile sig_atomic_t fatal_error_in_progress = 0;

static void handler(int sig)
{
  if (fatal_error_in_progress) {
    raise(sig);
  }
  fatal_error_in_progress = 1;

  std::cerr << "Signal " << sig << " received\n";

  std::cerr << "Stack trace:\n";
  std::cerr << boost::stacktrace::stacktrace();

  signal(sig, SIG_DFL);
  raise(sig);
}

int main(int argc, char* argv[])
{
  // This avoids problems with locale setting dependent
  // C functions like strtod (e.g. 0.5 vs 0,5).
  std::array locales = {"en_US.UTF-8", "C.UTF-8", "C"};
  for (auto locale : locales) {
    if (std::setlocale(LC_ALL, locale) != nullptr) {
      setenv("LC_ALL", locale, /* override */ 1);
      break;
    }
  }

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
    printf("%s %s\n",
           ord::OpenRoad::getVersion(),
           ord::OpenRoad::getGitDescribe());
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
  if (findCmdLineFlag(cmd_argc, cmd_argv, "-python")) {
    // Setup the app with tcl
    auto* interp = Tcl_CreateInterp();
    Tcl_Init(interp);
    ord::initOpenRoad(interp);
    if (!findCmdLineFlag(cmd_argc, cmd_argv, "-no_splash")) {
      showSplash();
    }

    utl::Logger* logger = ord::OpenRoad::openRoad()->getLogger();
    if (findCmdLineFlag(cmd_argc, cmd_argv, "-gui")) {
      logger->warn(utl::ORD, 38, "-gui is not yet supported with -python");
    }

    if (!findCmdLineFlag(cmd_argc, cmd_argv, "-no_init")) {
      logger->warn(utl::ORD, 39, ".openroad ignored with -python");
    }

    const char* threads = findCmdLineKey(cmd_argc, cmd_argv, "-threads");
    if (threads) {
      ord::OpenRoad::openRoad()->setThreadCount(threads);
    } else {
      // set to default number of threads
      ord::OpenRoad::openRoad()->setThreadCount(
          ord::OpenRoad::openRoad()->getThreadCount(), false);
    }

#if PY_VERSION_HEX >= 0x03080000
    initPython(cmd_argc, cmd_argv);
    return Py_RunMain();
#else
    initPython();
    bool exit = findCmdLineFlag(cmd_argc, cmd_argv, "-exit");
    std::vector<wchar_t*> args;
    args.push_back(Py_DecodeLocale(cmd_argv[0], nullptr));
    if (!exit) {
      args.push_back(Py_DecodeLocale("-i", nullptr));
    }
    for (int i = 1; i < cmd_argc; i++) {
      args.push_back(Py_DecodeLocale(cmd_argv[i], nullptr));
    }
    return Py_Main(args.size(), args.data());
#endif  // PY_VERSION_HEX >= 0x03080000
  }
#endif  // ENABLE_PYTHON3

  // Set argc to 1 so Tcl_Main doesn't source any files.
  // Tcl_Main never returns.
  Tcl_Main(1, argv, ord::tclAppInit);
  return 0;
}

#ifdef ENABLE_READLINE
static int tclReadlineInit(Tcl_Interp* interp)
{
  std::array<const char*, 7> readline_cmds = {
      "history event",
      "eval $auto_index(::tclreadline::ScriptCompleter)",
      "::tclreadline::readline builtincompleter true",
      "::tclreadline::readline customcompleter ::tclreadline::ScriptCompleter",
      "proc ::tclreadline::prompt1 {} { return \"openroad> \" }",
      "proc ::tclreadline::prompt2 {} { return \"...> \" }",
      "::tclreadline::Loop"};

  for (auto cmd : readline_cmds) {
    if (TCL_ERROR == Tcl_Eval(interp, cmd)) {
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}
#endif

// Tcl init executed inside Tcl_Main.
static int tclAppInit(int& argc,
                      char* argv[],
                      const char* init_filename,
                      Tcl_Interp* interp)
{
  // first check if gui was requested and launch.
  // gui will call this function again as part of setup
  // ensuring the else {} will be utilized to initialize tcl and OR.
  if (findCmdLineFlag(argc, argv, "-gui")) {
    // gobble up remaining -gui flags if present, since this could result in
    // second invocation of the GUI
    while (findCmdLineFlag(argc, argv, "-gui")) {
      ;
    }

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
    Tcl_StaticPackage(
        interp, "tclreadline", Tclreadline_Init, Tclreadline_SafeInit);
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
      ord::OpenRoad::openRoad()->setThreadCount(
          ord::OpenRoad::openRoad()->getThreadCount(), false);
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
          // need to delay loading of file until after GUI is completed
          // initialized
          gui::Gui::get()->addRestoreStateCommand(
              fmt::format(FMT_RUNTIME(restore_state_cmd), init.string()));
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
          // need to delay loading of file until after GUI is completed
          // initialized
          gui::Gui::get()->addRestoreStateCommand(
              fmt::format(FMT_RUNTIME(restore_state_cmd), init_path));
        }
      }
#endif
    }

    if (argc > 2 || (argc > 1 && argv[1][0] == '-')) {
      showUsage(argv[0], init_filename);
    } else {
      if (argc == 2) {
        char* cmd_file = argv[1];
        if (cmd_file) {
          if (!gui_enabled) {
            int result = sourceTclFile(cmd_file, false, false, interp);
            if (exit_after_cmd_file) {
              int exit_code = (result == TCL_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
              exit(exit_code);
            }
          } else {
            // need to delay loading of file until after GUI is completed
            // initialized
            gui::Gui::get()->addRestoreStateCommand(
                fmt::format("source {{{}}}", cmd_file));
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

int ord::tclAppInit(Tcl_Interp* interp)
{
  return tclAppInit(cmd_argc, cmd_argv, init_filename, interp);
}

static void showUsage(const char* prog, const char* init_filename)
{
  printf("Usage: %s [-help] [-version] [-no_init] [-exit] [-gui] ", prog);
  printf("[-threads count|max] [-log file_name] [-metrics file_name] ");
  printf("cmd_file\n");
  printf("  -help                 show help and exit\n");
  printf("  -version              show version and exit\n");
  printf("  -no_init              do not read %s init file\n", init_filename);
  printf("  -threads count|max    use count threads\n");
  printf("  -no_splash            do not show the license splash at startup\n");
  printf("  -exit                 exit after reading cmd_file\n");
  printf("  -gui                  start in gui mode\n");
#ifdef ENABLE_PYTHON3
  printf(
      "  -python               start with python interpreter [limited to db "
      "operations]\n");
#endif
  printf("  -log <file_name>      write a log in <file_name>\n");
  printf(
      "  -metrics <file_name>  write metrics in <file_name> in JSON format\n");
  printf("  cmd_file              source cmd_file\n");
}

static void showSplash()
{
  utl::Logger* logger = ord::OpenRoad::openRoad()->getLogger();
  logger->report("OpenROAD {} {}",
                 ord::OpenRoad::getVersion(),
                 ord::OpenRoad::getGitDescribe());
  logger->report(
      "This program is licensed under the BSD-3 license. See the LICENSE file "
      "for details.");
  logger->report(
      "Components of this program may be licensed under more restrictive "
      "licenses which must be honored.");
}
