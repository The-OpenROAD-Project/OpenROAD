// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/OptionsRegistry.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace utl {

OptionsRegistry& OptionsRegistry::get()
{
  static OptionsRegistry registry;
  return registry;
}

void OptionsRegistry::add(const std::string& command,
                          const SchemaBase& schema,
                          ArgAliases aliases)
{
  for (auto& [name, entry] : entries_) {
    if (name == command) {
      entry = {&schema, std::move(aliases)};
      return;
    }
  }
  entries_.emplace_back(command, OptionsEntry{&schema, std::move(aliases)});
}

const OptionsEntry* OptionsRegistry::find(const std::string& command) const
{
  for (const auto& [name, entry] : entries_) {
    if (name == command) {
      return &entry;
    }
  }
  return nullptr;
}

std::vector<std::string> OptionsRegistry::commands() const
{
  std::vector<std::string> result;
  result.reserve(entries_.size());
  for (const auto& [name, entry] : entries_) {
    result.push_back(name);
  }
  std::sort(result.begin(), result.end());
  return result;
}

}  // namespace utl
