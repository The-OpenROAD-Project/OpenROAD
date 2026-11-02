// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// GPU runtime helpers for the gpl GPU kernel series.
//
// Compiled only in the GPU build (--config=gpu). This TU has no device code of
// its own — it only calls getenv and the Kokkos lifecycle API — but it includes
// <Kokkos_Core.hpp>, which (when Kokkos was built with the CUDA/HIP backend)
// bakes KOKKOS_ENABLE_CUDA into its config and requires __CUDACC__. It is
// therefore compiled as CUDA in :gpl_kokkos; see src/gpl/BUILD.

#include "gpuRuntime.h"

#include <cctype>
#include <cstdlib>
#include <string>

namespace gpl {

namespace {

// Lower-case a copy of the string for case-insensitive comparison.
std::string toLower(const char* s)
{
  std::string out(s);
  for (char& c : out) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return out;
}

}  // namespace

bool gpuEnabled()
{
  // Magic-static: the environment is read exactly once per process.
  static const bool enabled = [] {
    const char* env = std::getenv("ENABLE_GPU");
    if (env == nullptr) {
      // GPU is the default backend when compiled in.
      return true;
    }
    const std::string value = toLower(env);
    if (value.empty() || value == "0" || value == "off" || value == "false"
        || value == "no") {
      return false;
    }
    return true;
  }();
  return enabled;
}

}  // namespace gpl
