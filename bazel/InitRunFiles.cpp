#include <unistd.h>

#include <climits>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "rules_cc/cc/runfiles/runfiles.h"

// Avoid adding any dependencies like boost.filesystem
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

// Global constructor class to initialize Bazel runfiles and environment
// variables
class BazelInitializer
{
 public:
  BazelInitializer()
  {
    using rules_cc::cc::runfiles::Runfiles;

    std::string error;
    std::unique_ptr<Runfiles> runfiles(Runfiles::Create(
        getProgramLocation().value(), BAZEL_CURRENT_REPOSITORY, &error));
    if (!runfiles) {
      std::cerr << "Error initializing Bazel runfiles: " << error << std::endl;
      std::exit(1);
    }

    // Set the TCL_LIBRARY environment variable
    std::string path = runfiles->Rlocation("tk_tcl/library/");
    if (!path.empty()) {
      setenv("TCL_LIBRARY", path.c_str(), 0);
    } else {
      std::cerr << "Error: Could not locate 'tk_tcl/library/' in runfiles."
                << std::endl;
      std::exit(1);
    }
  }
};

// Instantiate the global constructor
static BazelInitializer bazel_initializer;
