// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "syn/lir/RegPinTypes.h"

#include <array>
#include <string_view>

namespace syn::lir {

namespace {

constexpr std::array<std::string_view, kNumRegPinTypes> kRegPinTypeNames = {
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
  for (const std::string_view name : kRegPinTypeNames) {
    if (name.empty()) {
      return false;
    }
  }
  return true;
}
static_assert(allNamed(), "kRegPinTypeNames is missing an entry");

}  // namespace

std::string_view regPinTypeName(RegPinType type)
{
  const int i = static_cast<int>(type);
  return i >= 0 && i < kNumRegPinTypes ? kRegPinTypeNames[i] : "?";
}

std::string_view regOutputName(RegOutput output)
{
  constexpr std::array<std::string_view, kNumRegOutputs> kNames
      = {"q", "qn", "so"};
  const int i = static_cast<int>(output);
  return i >= 0 && i < kNumRegOutputs ? kNames[i] : "?";
}

}  // namespace syn::lir
