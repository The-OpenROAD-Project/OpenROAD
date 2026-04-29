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

// In tcl 9, we can use the //zipfs:/ virtual file system (unless
// we specifically disabled with the --//bazel:use_zipfs=False flag)
#if TCL_MAJOR_VERSION >= 9 && !defined(USE_TCL_RUNFILE_INIT)
#define USE_ZIPFS_INIT 1
#else
#define USE_ZIPFS_INIT 0
#endif

#if USE_ZIPFS_INIT
#include "bazel/tcl_resources_zip_data.h"
#else

#include <limits.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif

#include <memory>

#include "rules_cc/cc/runfiles/runfiles.h"
#endif

namespace in_bazel {

#if !USE_ZIPFS_INIT
// Avoid adding any dependencies like boost.filesystem
// Returns path to running binary if possible.
static std::string GetProgramLocation()
{
#if defined(_WIN32)
  char result[MAX_PATH + 1] = {'\0'};
  auto path_len = GetModuleFileNameA(NULL, result, MAX_PATH);
#elif defined(__APPLE__)
  char result[MAXPATHLEN + 1] = {'\0'};
  uint32_t path_len = MAXPATHLEN;
  if (_NSGetExecutablePath(result, &path_len) != 0) {
    path_len = readlink(result, result, MAXPATHLEN);
  }
#else
  char result[PATH_MAX + 1] = {'\0'};
  ssize_t path_len = readlink("/proc/self/exe", result, PATH_MAX);
#endif
  if (path_len > 0) {
    return result;
  }
  return Tcl_GetNameOfExecutable();
}
#endif

static std::optional<std::string> TclLibraryMountPoint(Tcl_Interp* interp)
{
  // In tcl9, we can use //zipfs:/ otherwise we need to point to the
  // directory where the tcl library files are extracted.
#if USE_ZIPFS_INIT
  if (TclZipfs_MountBuffer(
          interp, kTclResourceZip, sizeof(kTclResourceZip), "/app", 0)
      != TCL_OK) {
    std::cerr << "[Warning] Failed to mount Tcl zipfs.\n";
    return std::nullopt;
  }
  return Tcl_GetStringResult(interp);
#else
  using rules_cc::cc::runfiles::Runfiles;

  auto find_tcl_resources
      = [](const Runfiles* runfiles) -> std::optional<std::string> {
    std::error_code ec;
    for (const std::string loc : {"openroad", "opensta", "_main"}) {
      const std::string check_loc = loc + "/bazel/tcl_resources_dir";
      const std::string path = runfiles->Rlocation(check_loc);
      if (!path.empty() && std::filesystem::exists(path, ec)) {
        return path;
      }
    }
    return std::nullopt;
  };

  // First try with the inherited RUNFILES_* env vars.  When OpenROAD
  // is the direct Bazel target (or invoked from an sh_test whose
  // runfiles tree also contains OpenROAD's data), those env vars
  // already point at the correct tree.
  std::string error;
  std::unique_ptr<Runfiles> runfiles(
      Runfiles::Create(GetProgramLocation(), BAZEL_CURRENT_REPOSITORY, &error));
  if (runfiles) {
    if (auto path = find_tcl_resources(runfiles.get())) {
      return path;
    }
  }

  // Fallback: when OpenROAD is invoked by a build system that
  // previously ran another Bazel binary (e.g. a Python wrapper), the
  // inherited RUNFILES_* variables point to the *other* binary's
  // runfiles tree and won't contain OpenROAD's tcl_resources_dir.
  // Unset them and retry so Runfiles::Create falls back to the
  // exe_path derived from /proc/self/exe.
  unsetenv("RUNFILES_DIR");
  unsetenv("RUNFILES_MANIFEST_FILE");
  unsetenv("RUNFILES_MANIFEST_ONLY");

  runfiles.reset(
      Runfiles::Create(GetProgramLocation(), BAZEL_CURRENT_REPOSITORY, &error));
  if (!runfiles) {
    std::cerr << "[Warning] Failed to create bazel runfiles: " << error << "\n";
    return std::nullopt;
  }

  if (auto path = find_tcl_resources(runfiles.get())) {
    return path;
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
