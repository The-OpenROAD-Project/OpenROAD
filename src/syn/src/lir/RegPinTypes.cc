// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/RegPinTypes.h"

#include <array>

namespace syn::lir {

namespace {

constexpr std::array<const char*, kNumRegPinTypes> kRegPinTypeNames = {
    "data",
    "clock",
    "en",
    "srst",
    "sset",
    "srst_val",
    "arst",
    "aset",
    "arst_val",
    "aload",
    "ad",
    "se",
    "si",
};

constexpr bool allNamed()
{
  for (const char* name : kRegPinTypeNames) {
    if (name == nullptr) {
      return false;
    }
  }
  return true;
}
static_assert(allNamed(), "kRegPinTypeNames is missing an entry");

}  // namespace

const char* regPinTypeName(RegPinType type)
{
  const unsigned i = static_cast<unsigned>(type);
  return i < kRegPinTypeNames.size() ? kRegPinTypeNames[i] : "?";
}

const char* regOutputName(RegOutput output)
{
  constexpr std::array<const char*, kNumRegOutputs> kNames = {"q", "qn", "so"};
  const unsigned i = static_cast<unsigned>(output);
  return i < kNames.size() ? kNames[i] : "?";
}

}  // namespace syn::lir
