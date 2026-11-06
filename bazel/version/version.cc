// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "ord/Version.hh"

namespace ord {

namespace {

// bazel/stamp_version.py finds the marker in the linked binary and overwrites
// the version that follows it. Keep the layout in sync with that script.
struct VersionSlot
{
  char marker[32];
  char version[256];
};

[[gnu::used]] constexpr VersionSlot kVersionSlot
    = {"OPENROAD_VERSION_SLOT_MARKER", "bazel-nostamp"};

}  // namespace

const char* stampedVersion()
{
  // Hide the slot's address from the optimizer. Otherwise it could fold reads
  // of the placeholder text at compile or link time, and the stamped bytes
  // would never be read.
  const VersionSlot* slot = &kVersionSlot;
  asm("" : "+r"(slot));
  return slot->version;
}

}  // namespace ord
