// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "bazel/tcl_library_init.h"

#include <iostream>
#include <optional>
#include <string>

#include "bazel/tcl_resources_zip_data.h"
#include "tcl.h"

namespace in_bazel {

static std::optional<std::string> TclLibraryMountPoint(Tcl_Interp* interp)
{
  // In tcl9, we can mount an encoded zipfile as //zipfs:/ and read
  // libraries from there.
  if (TclZipfs_MountBuffer(
          interp, kTclResourceZip, sizeof(kTclResourceZip), "/app", 0)
      != TCL_OK) {
    std::cerr << "[Warning] Failed to mount Tcl zipfs.\n";
    return std::nullopt;
  }
  return Tcl_GetStringResult(interp);
}

int SetupTclEnvironment(Tcl_Interp* interp)
{
  const auto mount_point = TclLibraryMountPoint(interp);
  if (!mount_point) {
    return TCL_ERROR;
  }

  // Main tcl library, init.tcl and friends.
  const std::string tcl_lib = *mount_point + "/library";
  if (!Tcl_SetVar(interp, "::tcl_library", tcl_lib.c_str(), TCL_GLOBAL_ONLY)) {
    std::cerr << "[Warning] could not set ::tcl_library\n";
    return TCL_ERROR;
  }

  // Readline library location
  const std::string rd_lib = *mount_point + "/tclreadline";
  Tcl_Eval(interp, "namespace eval ::tclreadline {}");
  if (!Tcl_SetVar(
          interp, "::tclreadline::library", rd_lib.c_str(), TCL_GLOBAL_ONLY)) {
    std::cerr << "[Warning] could not set ::tclreadline::library\n";
    return TCL_ERROR;
  }

  return TCL_OK;
}
}  // namespace in_bazel
