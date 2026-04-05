// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// This file to go away as soon as we use the tcl_library_init initialization
// in OpenSTA.

#include <unistd.h>  // readlink()

#include <climits>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h>
#endif

#include "rules_cc/cc/runfiles/runfiles.h"

namespace {
// Avoid adding any dependencies like boost.filesystem
// Returns path to running binary if possible, otherwise nullopt.
static std::optional<std::string> GetProgramLocation()
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
  return std::nullopt;
}

std::string GetTclLibraryBaseLocation()
{
  using rules_cc::cc::runfiles::Runfiles;
  std::string error;
  std::unique_ptr<Runfiles> runfiles(Runfiles::Create(
      *GetProgramLocation(), BAZEL_CURRENT_REPOSITORY, &error));
  if (!runfiles) {
    std::cerr << "[Warning] Failed to create bazel runfiles: " << error << "\n";
    return "";
  }

  std::error_code ec;
  for (const std::string loc : {"openroad", "opensta", "_main"}) {
    const std::string check_loc = loc + "/bazel/tcl_resources_dir";
    const std::string path = runfiles->Rlocation(check_loc);
    if (!path.empty() && std::filesystem::exists(path, ec)) {
      return path;
    }
  }
  return "";
}

class BazelInitializer
{
 public:
  BazelInitializer()
  {
    const std::string lib_mount_point = GetTclLibraryBaseLocation();
    if (!lib_mount_point.empty()) {
      const std::string tcl_path = lib_mount_point + "/library";
      setenv("TCL_LIBRARY", tcl_path.c_str(), true);
    }
  }
};

// Provide via global constructor.
static BazelInitializer bazel_initializer;
}  // namespace
