// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "bazel/tcl_library_init.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>

#include "tcl.h"

#if TCL_MAJOR_VERSION >= 9 && !defined(USE_TCL_RUNFILE_INIT)
#include "bazel/tcl_resources_zip_data.h"
#else
#include <memory>

#include "rules_cc/cc/runfiles/runfiles.h"
#endif

namespace in_bazel {
static std::optional<std::string> TclLibraryMountPoint(Tcl_Interp* interp)
{
  // In tcl9, we can use //zipfs:/ otherwise we need to point to the
  // directory where the tcl library files are extracted.
#if TCL_MAJOR_VERSION >= 9 && !defined(USE_TCL_RUNFILE_INIT)
  if (TclZipfs_MountBuffer(
          interp, kTclResourceZip, sizeof(kTclResourceZip), "/app", 0)
      != TCL_OK) {
    std::cerr << "[Warning] Failed to mount Tcl zipfs.\n";
    return std::nullopt;
  }
  return Tcl_GetStringResult(interp);
#else
  using rules_cc::cc::runfiles::Runfiles;
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(
      Tcl_GetNameOfExecutable(), BAZEL_CURRENT_REPOSITORY, &error));
  if (!runfiles) {
    std::cerr << "[Warning] Failed to create bazel runfiles: " << error << "\n";
    return std::nullopt;
  }

  std::error_code ec;
  for (const std::string loc : {"openroad", "opensta", "_main"}) {
    const std::string check_loc = loc + "/bazel/tcl_resources_dir";
    const std::string path = runfiles->Rlocation(check_loc);
    if (!path.empty() && std::filesystem::exists(path, ec)) {
      return path;
    }
  }
  return std::nullopt;
#endif
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
