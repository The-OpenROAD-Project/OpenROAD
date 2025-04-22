// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include <libgen.h>
#include <tcl.h>

#include <array>
#include <boost/stacktrace.hpp>
#include <climits>
#include <clocale>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
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

#ifdef BAZEL_CURRENT_REPOSITORY
#include "rules_cc/cc/runfiles/runfiles.h"
#endif

#include "gui/gui.h"
#include "ord/Design.h"
#include "ord/InitOpenRoad.hh"
#include "ord/OpenRoad.hh"
#include "ord/Tech.h"
#include "sta/StaMain.hh"
#include "sta/StringUtil.hh"
#include "utl/Logger.h"
#include "utl/decode.h"

using sta::findCmdLineFlag;
using sta::findCmdLineKey;
using sta::sourceTclFile;
using sta::stringEq;
using std::string;

#ifdef ENABLE_PYTHON3

#define FOREACH_TOOL_WITHOUT_OPENROAD(X) \
  X(ifp)                                 \
  X(utl)                                 \
  X(ant)                                 \
  X(grt)                                 \
  X(gpl)                                 \
  X(dpl)                                 \
  X(ppl)                                 \
  X(tap)                                 \
  X(cts)                                 \
  X(drt)                                 \
  X(dpo)                                 \
  X(fin)                                 \
  X(par)                                 \
  X(rcx)                                 \
  X(rmp)                                 \
  X(stt)                                 \
  X(psm)                                 \
  X(pdn)                                 \
  X(odb)                                 \
  X(ord)

#define FOREACH_TOOL(X) FOREACH_TOOL_WITHOUT_OPENROAD(X)

extern "C" {
#define X(name) extern PyObject* PyInit__##name##_py();
FOREACH_TOOL(X)
#undef X
}
#endif

int cmd_argc;
char** cmd_argv;
static const char* log_filename = nullptr;
static const char* metrics_filename = nullptr;
static bool no_settings = false;
static bool minimize = false;

static const char* init_filename = ".openroad";

static void showUsage(const char* prog, const char* init_filename);
static void showSplash();

#ifdef ENABLE_PYTHON3
#define X(name)                                \
  namespace name {                             \
  extern const char* name##_py_python_inits[]; \
  }
FOREACH_TOOL(X)
#undef X

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
#define X(name)                                                               \
  {                                                                           \
    std::string unencoded = utl::base64_decode(name::name##_py_python_inits); \
    PyObject* code                                                            \
        = Py_CompileString(unencoded.c_str(), #name "_py.py", Py_file_input); \
    if (code == nullptr) {                                                    \
      PyErr_Print();                                                          \
      fprintf(stderr, "Error: could not compile " #name "_py\n");             \
      exit(1);                                                                \
    }                                                                         \
    if (PyImport_ExecCodeModule(#name, code) == nullptr) {                    \
      PyErr_Print();                                                          \
      fprintf(stderr, "Error: could not add module " #name "\n");             \
      exit(1);                                                                \
    }                                                                         \
  }
  FOREACH_TOOL_WITHOUT_OPENROAD(X)
#undef X
#undef FOREACH_TOOL
#undef FOREACH_TOOL_WITHOUT_OPENROAD

  // Need to separately handle openroad here because we need both
  // the names "openroad_swig" and "openroad".
  {
    std::string unencoded = utl::base64_decode(ord::ord_py_python_inits);
    PyObject* code
        = Py_CompileString(unencoded.c_str(), "openroad.py", Py_file_input);
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
  }
}
#endif

static volatile sig_atomic_t fatal_error_in_progress = 0;

// When we enter through main() we have a single tech and design.
// Custom applications using OR as a library might define multiple.
// Such applications won't allocate or use these objects.
//
// Use a wrapper struct to ensure destruction ordering - design
// then tech (members are destroyed in reverse order).
struct TechAndDesign
{
  std::unique_ptr<ord::Tech> tech;
  std::unique_ptr<ord::Design> design;
};

static TechAndDesign the_tech_and_design;

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

#ifdef BAZEL_CURRENT_REPOSITORY

// Avoid adding any dependencies like boost.filesystem
//
// Returns path to running binary if possible, otherwise nullopt.
static std::optional<std::string> getProgramLocation()
{
#if defined(_WIN32)
  char result[MAX_PATH + 1] = {'\0'};
  auto path_len = GetModuleFileNameA(NULL, result, MAX_PATH);
#elif defined(__APPLE__)
  char result[MAXPATHLEN + 1] = {'\0'};
  uint32_t path_len = MAXPATHLEN;
  if (_NSGetExecutablePath(result, &path_len) != 0) {
    path_len = readlink("/proc/self/exe", result, MAXPATHLEN);
  }
#else
  char result[PATH_MAX + 1] = {'\0'};
  ssize_t path_len = readlink("/proc/self/exe", result, PATH_MAX);
#endif
  if (path_len > 0) {
    return result;
  }
  return std::nullopt;
}
#endif

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

#ifdef BAZEL_CURRENT_REPOSITORY
  using rules_cc::cc::runfiles::Runfiles;
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(
      getProgramLocation().value(), BAZEL_CURRENT_REPOSITORY, &error));
  if (!runfiles) {
    std::cerr << error << std::endl;
    return 1;
  }
  std::string path = runfiles->Rlocation("tk_tcl/library/");
  setenv("TCL_LIBRARY", path.c_str(), 0);
#endif

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

  no_settings = findCmdLineFlag(argc, argv, "-no_settings");
  minimize = findCmdLineFlag(argc, argv, "-minimize");

  cmd_argc = argc;
  cmd_argv = argv;

#ifdef ENABLE_PYTHON3
  if (findCmdLineFlag(cmd_argc, cmd_argv, "-python")) {
    // Setup the app with tcl
    auto* interp = Tcl_CreateInterp();
    Tcl_Init(interp);
    the_tech_and_design.tech = std::make_unique<ord::Tech>(interp);
    the_tech_and_design.design
        = std::make_unique<ord::Design>(the_tech_and_design.tech.get());
    ord::OpenRoad::setOpenRoad(the_tech_and_design.design->getOpenRoad());
    ord::initOpenRoad(interp, log_filename, metrics_filename);
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

#ifdef ENABLE_READLINE
namespace {
// A stopgap fallback from the hardcoded TCLRL_LIBRARY path for OpenROAD,
// not essential for OpenSTA
std::string findPathToTclreadlineInit(Tcl_Interp* interp)
{
  // TL;DR it is possible to run the OpenROAD binary from within the
  // official Docker image on a different distribution than the
  // distribution within the Docker image.
  //
  // In this case we have to look up
  // the location of the tclreadline scripts instead of using the hardcoded
  // path.
  //
  // It is helpful to use the official Docker image as CI infrastructure and
  // also because it is a good way to have as similar an environment as possible
  // during testing and deployment.
  //
  // See
  // https://github.com/The-OpenROAD-Project/bazel-orfs/blob/main/docker.BUILD.bazel
  // for the details on how this is done.
  //
  // Running Docker within a bazel isolated environment introduces lots of
  // problems and is not really done.
  const char* tclScript = R"(
      namespace eval temp {
        foreach dir $::auto_path {
            set folder [file join $dir]
            set path [file join $folder "tclreadline)" TCLRL_VERSION_STR
                          R"(" "tclreadlineInit.tcl"]
            if {[file exists $path]} {
                return $path
            }
        }
        error "tclreadlineInit.tcl not found in any of the directories in auto_path"
      }
    )";

  if (Tcl_Eval(interp, tclScript) == TCL_ERROR) {
    std::cerr << "Tcl_Eval failed: " << Tcl_GetStringResult(interp)
              << std::endl;
    return "";
  }

  return Tcl_GetStringResult(interp);
}
}  // namespace
#endif

// Tcl init executed inside Tcl_Main.
static int tclAppInit(int& argc,
                      char* argv[],
                      const char* init_filename,
                      Tcl_Interp* interp)
{
  bool exit_after_cmd_file = false;
  // first check if gui was requested and launch.
  // gui will call this function again as part of setup
  // ensuring the else {} will be utilized to initialize tcl and OR.
  if (findCmdLineFlag(argc, argv, "-gui")) {
    // gobble up remaining -gui flags if present, since this could result in
    // second invocation of the GUI
    while (findCmdLineFlag(argc, argv, "-gui")) {
      ;
    }

    gui::startGui(argc, argv, interp, "", true, !no_settings, minimize);
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
    exit_after_cmd_file = findCmdLineFlag(argc, argv, "-exit");
#ifdef ENABLE_READLINE
    if (!exit_after_cmd_file) {
      if (Tclreadline_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
      }
      // tclreadline is a bit of a tricky dependency because it
      // uses absolute path references below, so we don't depend on
      // tclreadline for the batch case where we exit as soon as the
      // script is done.
      Tcl_StaticPackage(
          interp, "tclreadline", Tclreadline_Init, Tclreadline_SafeInit);

      if (Tcl_EvalFile(interp, TCLRL_LIBRARY "/tclreadlineInit.tcl")
          != TCL_OK) {
        std::string path = findPathToTclreadlineInit(interp);
        if (path.empty() || Tcl_EvalFile(interp, path.c_str()) != TCL_OK) {
          printf("Failed to load tclreadline\n");
        }
      }
    }
#endif

    ord::initOpenRoad(interp, log_filename, metrics_filename);

    bool no_splash = findCmdLineFlag(argc, argv, "-no_splash");
    if (!no_splash) {
      showSplash();
    }

    const char* threads = findCmdLineKey(argc, argv, "-threads");
    if (threads) {
      ord::OpenRoad::openRoad()->setThreadCount(threads, !no_splash);
    } else {
      // set to default number of threads
      ord::OpenRoad::openRoad()->setThreadCount(
          ord::OpenRoad::openRoad()->getThreadCount(), false);
    }

    const bool gui_enabled = gui::Gui::enabled();

    const char* home = getenv("HOME");
    if (!findCmdLineFlag(argc, argv, "-no_init") && home) {
      const char* restore_state_cmd = "include -echo -verbose {{{}}}";
      std::filesystem::path init(home);
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
    }

    if (argc > 2 || (argc > 1 && argv[1][0] == '-')) {
      showUsage(argv[0], init_filename);
      exit(1);
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
  if (!gui::Gui::enabled() && !exit_after_cmd_file) {
    return tclReadlineInit(interp);
  }
#endif
  return TCL_OK;
}

int ord::tclAppInit(Tcl_Interp* interp)
{
  the_tech_and_design.tech = std::make_unique<ord::Tech>(interp);
  the_tech_and_design.design
      = std::make_unique<ord::Design>(the_tech_and_design.tech.get());
  ord::OpenRoad::setOpenRoad(the_tech_and_design.design->getOpenRoad());

  // This is to enable Design.i where a design arg can be
  // retrieved from the interpreter.  This is necessary for
  // cases with more than one interpreter (ie more than one Design).
  // This should replace the use of the singleton OpenRoad::openRoad().
  Tcl_SetAssocData(interp, "design", nullptr, the_tech_and_design.design.get());

  return ord::tclInit(interp);
}

int ord::tclInit(Tcl_Interp* interp)
{
  return tclAppInit(cmd_argc, cmd_argv, init_filename, interp);
}

static void showUsage(const char* prog, const char* init_filename)
{
  printf("Usage: %s [-help] [-version] [-no_init] [-no_splash] [-exit] ", prog);
  printf("[-gui] [-threads count|max] [-log file_name] [-metrics file_name] ");
  printf("[-no_settings] [-minimize] cmd_file\n");
  printf("  -help                 show help and exit\n");
  printf("  -version              show version and exit\n");
  printf("  -no_init              do not read %s init file\n", init_filename);
  printf("  -threads count|max    use count threads\n");
  printf("  -no_splash            do not show the license splash at startup\n");
  printf("  -exit                 exit after reading cmd_file\n");
  printf("  -gui                  start in gui mode\n");
  printf("  -minimize             start the gui minimized\n");
  printf("  -no_settings          do not load the previous gui settings\n");
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
      "Features included (+) or not (-): "
      "{}GPU {}GUI {}Python{}",
      ord::OpenRoad::getGPUCompileOption() ? "+" : "-",
      ord::OpenRoad::getGUICompileOption() ? "+" : "-",
      ord::OpenRoad::getPythonCompileOption() ? "+" : "-",
#ifdef BAZEL_CURRENT_REPOSITORY
      strcasecmp(BUILD_TYPE, "opt") == 0
#else
      strcasecmp(BUILD_TYPE, "release") == 0
#endif
          ? ""
          : fmt::format(" : {}", BUILD_TYPE));
  logger->report(
      "This program is licensed under the BSD-3 license. See the LICENSE file "
      "for details.");
  logger->report(
      "Components of this program may be licensed under more restrictive "
      "licenses which must be honored.");
}
