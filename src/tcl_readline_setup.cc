// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "tcl_readline_setup.h"

#include "tcl.h"

#ifdef ENABLE_READLINE
#include <iostream>
#include <string>

// If you get an error on this include be sure you have
//   the package tcl-tclreadline-devel installed
#include "tclreadline.h"
#endif

// Various way to initialize tcl readline. This might be made easier once we
// have one build system.

namespace ord {

#ifdef ENABLE_READLINE
// A stopgap fallback from the hardcoded TCLRL_LIBRARY path for OpenROAD,
// not essential for OpenSTA
// TODO: this might need to not be needed anymore after we have an all batteries
// included //zipfs:/ set-up.
static std::string findPathToTclreadlineInit(Tcl_Interp* interp)
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
  const char* tcl_script = R"(
      namespace eval temp {
        # Check standard Bazel runfiles relative path
        set runfiles_path [file join [pwd] "external/tclreadline/tclreadlineInit.tcl"]
        if {[file exists $runfiles_path]} {
            return $runfiles_path
        }

        # Check Bazel runfiles in adjacent directories for other run strategies
        set runfiles_execroot_path [file join [pwd] "../tclreadline/tclreadlineInit.tcl"]
        if {[file exists $runfiles_execroot_path]} {
            return $runfiles_execroot_path
        }

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

  if (Tcl_Eval(interp, tcl_script) == TCL_ERROR) {
    std::cerr << "Tcl_Eval failed: " << Tcl_GetStringResult(interp) << '\n';
    return "";
  }

  return Tcl_GetStringResult(interp);
}

static bool TryReadlineStaticInit(Tcl_Interp* interp)
{
  Tcl_StaticPackage(
      interp, "tclreadline", Tclreadline_Init, Tclreadline_SafeInit);

  return Tcl_EvalFile(interp, TCLRL_LIBRARY "/tclreadlineInit.tcl") == TCL_OK;
}

static bool TryTclBazelInit(Tcl_Interp* interp)
{
  // Here, ::tclreadline::library is already set up by SetupTclEnvironment()
  constexpr char kInitializeReadlineLib[] = R"(
        set ::tclreadline::setup_path [file join $::tclreadline::library "tclreadlineSetup.tcl"]
        set ::tclreadline::completer_path [file join $::tclreadline::library "tclreadlineCompleter.tcl"]
        if {[info commands history] == ""} { proc history {args} {} };
        source [file join $::tclreadline::library "tclreadlineInit.tcl"]
)";

  return Tcl_Eval(interp, kInitializeReadlineLib) == TCL_OK;
}

static bool TrySearchPathManuallyInit(Tcl_Interp* interp)
{
  const std::string path = findPathToTclreadlineInit(interp);
  return !path.empty() && Tcl_EvalFile(interp, path.c_str()) == TCL_OK;
}
#endif

int SetupTclReadlineLibrary(Tcl_Interp* interp)
{
#ifdef ENABLE_READLINE
  if (Tclreadline_Init(interp) == TCL_ERROR) {
    return TCL_ERROR;
  }

  if (!TryReadlineStaticInit(interp) &&  //
      !TryTclBazelInit(interp) &&        //
      !TrySearchPathManuallyInit(interp)) {
    return TCL_ERROR;
  }
#endif
  return TCL_OK;
}
}  // namespace ord
