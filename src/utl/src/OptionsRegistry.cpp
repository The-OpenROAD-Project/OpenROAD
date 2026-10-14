// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "utl/OptionsRegistry.h"

#include <algorithm>
#include <string>
#include <vector>

namespace utl {

OptionsRegistry& OptionsRegistry::get()
{
  static OptionsRegistry registry;
  return registry;
}

void OptionsRegistry::add(const std::string& command, const SchemaBase& schema)
{
  for (auto& [name, existing] : schemas_) {
    if (name == command) {
      existing = &schema;
      return;
    }
  }
  schemas_.emplace_back(command, &schema);
}

const SchemaBase* OptionsRegistry::find(const std::string& command) const
{
  for (const auto& [name, schema] : schemas_) {
    if (name == command) {
      return schema;
    }
  }
  return nullptr;
}

std::vector<std::string> OptionsRegistry::commands() const
{
  std::vector<std::string> result;
  result.reserve(schemas_.size());
  for (const auto& [name, schema] : schemas_) {
    result.push_back(name);
  }
  std::sort(result.begin(), result.end());
  return result;
}

}  // namespace utl
